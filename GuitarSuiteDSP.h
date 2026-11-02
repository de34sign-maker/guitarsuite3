// GuitarSuiteDSP.h — black-box clone of Adobe Audition 3.0 "Guitar Suite"
// Chain: Compressor -> Filter (Retro BP) -> Distortion (Garage Fuzz) -> Box Modeler -> Mix
// Measured at 44.1 kHz (2 Nov 2026). Pure header, no dependencies.
#pragma once
#include <cmath>
#include <vector>
#include <algorithm>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "GuitarSuiteData.h"

namespace gs {

struct Params {
    bool  compOn = true;   float compAmount = 100.f;   // 0..100
    bool  filtOn = true;                               // fixed: Retro BP 2042 Hz / res 34 (measured)
    bool  distOn = true;   float distAmount = 70.f;    // 0..100
    bool  boxOn  = true;                               // fixed: Model 9
    float mix    = 30.f;                               // 0..100 (%)
};

// ---------- small helpers ----------
inline float dbToLin(float db) { return std::pow(10.f, db / 20.f); }
inline float linToDb(float v)  { return 20.f * std::log10(std::max(v, 1e-9f)); }

// FIR with ring buffer (direct form). Taps measured, lengths 111 / 256 — cheap enough.
class FIR {
public:
    void set(const float* taps, size_t n) { h.assign(taps, taps + n); buf.assign(n, 0.f); pos = 0; }
    void reset() { std::fill(buf.begin(), buf.end(), 0.f); pos = 0; }
    inline float process(float x) {
        const size_t n = h.size();
        buf[pos] = x;
        float acc = 0.f; size_t i = pos;
        for (size_t k = 0; k < n; ++k) { acc += h[k] * buf[i]; i = (i == 0) ? n - 1 : i - 1; }
        if (++pos >= n) pos = 0;
        return acc;
    }
private:
    std::vector<float> h, buf; size_t pos = 0;
};


// Resample a measured impulse response (kDataSampleRate) to the host sample rate with a windowed sinc,
// preserving the frequency response in Hz. Done once in prepare(); IRs are short.
inline std::vector<float> resampleIR(const float* h, size_t n, double srOld, double srNew)
{
    if (std::fabs(srOld - srNew) < 1.0) return std::vector<float>(h, h + n);
    const double ratio = srOld / srNew;        // old samples per new sample
    const int half = 64;
    const size_t nNew = (size_t)std::ceil((double)n / ratio) + (size_t)half;
    std::vector<float> out(nNew, 0.f);
    for (size_t i = 0; i < nNew; ++i) {
        const double t = (double)i * ratio;
        const long kc = (long)std::floor(t);
        double acc = 0.0;
        for (long k = std::max(0L, kc - half); k <= std::min((long)n - 1, kc + half); ++k) {
            const double x = t - (double)k;
            const double w = 0.5 * (1.0 + std::cos(M_PI * x / half));
            const double sinc = (std::fabs(x) < 1e-9) ? 1.0 : std::sin(M_PI * x) / (M_PI * x);
            acc += (double)h[k] * sinc * w;
        }
        out[i] = (float)(acc * ratio);
    }
    return out;
}

// Compressor: instant-attack peak follower with short release, hard-knee gain computer in dB,
// then one-pole smoothing of the *linear* gain (attack when gain falls, release when it rises).
class Compressor {
public:
    void prepare(double sr) {
        fs = sr;
        envRel  = (float)std::exp(-1.0 / (5.40e-3 * sr));
        gainAtt = (float)std::exp(-1.0 / (11.07e-3 * sr));
        gainRel = (float)std::exp(-1.0 / (65.2e-3 * sr));
        reset();
    }
    void reset() { env = 0.f; gain = 1.f; }
    void setAmount(float a) {
        a = std::clamp(a, 0.f, 100.f);
        // Measured at 44.1 kHz on 1 kHz steps, amounts 25/45/50/55/75/100 (residual < 0.03 dB):
        //   threshold  = -0.29874*a + 0.13 dBFS   ratio = 1.9291 + 0.03032*a   makeup = a/20 dB
        // Audition itself drops makeup to 0 dB at exactly a == 50 (a bug) — not reproduced.
        thrDb  = -0.29874f * a + 0.1304f;
        ratio  = 1.9291f + 0.03032f * a;
        makeup = a / 20.f;
        mkLin  = dbToLin(makeup);
        slope  = 1.f - 1.f / ratio;
    }
    inline float process(float x) {
        float a = std::fabs(x);
        env = (a > env) ? a : envRel * env + (1.f - envRel) * a;   // instant attack
        float over = linToDb(env) - thrDb;
        float grDb = over > 0.f ? over * slope : 0.f;
        float target = mkLin * dbToLin(-grDb);
        float c = (target < gain) ? gainAtt : gainRel;
        gain = c * gain + (1.f - c) * target;
        return x * gain;
    }
private:
    double fs = 44100; float envRel = 0, gainAtt = 0, gainRel = 0;
    float env = 0, gain = 1; float thrDb = -29.74f, ratio = 4.96f, makeup = 5.f, mkLin = 1.f, slope = 0.8f;
};

// Garage Fuzz: y = 0.31623 * tanh(k(amount) * x). log k interpolated through measured points.
class Fuzz {
public:
    void setAmount(float a) {
        a = std::clamp(a, 0.f, 100.f);
        // piecewise-linear in log k over measured points (10/30/50/70/90/100); linear extrapolation below 10
        constexpr int N = sizeof(kFuzzAmount) / sizeof(float);
        float lk;
        if (a <= kFuzzAmount[0]) {
            float s = (kFuzzLogK[1] - kFuzzLogK[0]) / (kFuzzAmount[1] - kFuzzAmount[0]);
            lk = kFuzzLogK[0] + (a - kFuzzAmount[0]) * s;
        } else {
            int i = 0; while (i < N - 2 && a > kFuzzAmount[i + 1]) ++i;
            float t = (a - kFuzzAmount[i]) / (kFuzzAmount[i + 1] - kFuzzAmount[i]);
            lk = kFuzzLogK[i] + t * (kFuzzLogK[i + 1] - kFuzzLogK[i]);
        }
        k = std::exp(lk);
    }
    inline float process(float x) const { return 0.31623f * std::tanh(k * x); }
private:
    float k = 39.287f;
};

// Whole module, mono. Stereo = two instances (Audition processed L/R independently; input was identical).
class GuitarSuite {
public:
    void prepare(double sr) {
        comp.prepare(sr);
        auto f = resampleIR(kFilterFIR_2042_res34, sizeof(kFilterFIR_2042_res34) / sizeof(float), kDataSampleRate, sr);
        auto b = resampleIR(kBoxIR_model9, sizeof(kBoxIR_model9) / sizeof(float), kDataSampleRate, sr);
        filt.set(f.data(), f.size());
        box.set(b.data(), b.size());
        setParams(p);
        reset();
    }
    void reset() { comp.reset(); filt.reset(); box.reset(); dryZ = 0.f; }
    // Audition trims 1 sample of latency from the wet path; we instead delay dry by 1 sample so the
    // dry/wet relation is identical. Report 1 sample of latency to the host.
    static constexpr int latencySamples = 1;
    void setParams(const Params& np) {
        p = np; comp.setAmount(p.compAmount); fuzz.setAmount(p.distAmount);
        wet = std::clamp(p.mix, 0.f, 100.f) / 100.f; dry = 1.f - wet;
    }
    inline float process(float x) {
        float y = x;
        if (p.compOn) y = comp.process(y);
        if (p.filtOn) y = filt.process(y);
        if (p.distOn) y = fuzz.process(y);
        if (p.boxOn)  y = box.process(y);
        lastWet = wet * y;
        float out = dry * dryZ + lastWet; dryZ = x;
        return out;
    }
    // The processed part of the last output sample (wet path x Mix) — for the UI scope only.
    float lastWet = 0.f;
private:
    Params p; Compressor comp; FIR filt; Fuzz fuzz; FIR box; float wet = 0.3f, dry = 0.7f, dryZ = 0.f;
};

} // namespace gs
