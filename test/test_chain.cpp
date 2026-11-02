// Offline test: run GuitarSuite DSP on a 32-bit float WAV and compare with a reference WAV.
#include "../GuitarSuiteDSP.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <string>

static bool readWavF32(const char* path, std::vector<float>& out, int& sr, int& ch) {
    FILE* f = fopen(path, "rb"); if (!f) return false;
    char id[4]; uint32_t sz; fread(id, 1, 4, f); fread(&sz, 4, 1, f); fread(id, 1, 4, f);
    uint16_t fmt = 0, nch = 0, bits = 0; uint32_t rate = 0;
    while (fread(id, 1, 4, f) == 4) {
        fread(&sz, 4, 1, f);
        if (!memcmp(id, "fmt ", 4)) {
            std::vector<char> b(sz); fread(b.data(), 1, sz, f);
            fmt = *(uint16_t*)&b[0]; nch = *(uint16_t*)&b[2]; rate = *(uint32_t*)&b[4]; bits = *(uint16_t*)&b[14];
            if (fmt == 0xFFFE && sz >= 26) fmt = *(uint16_t*)&b[24];
        } else if (!memcmp(id, "data", 4)) {
            if (bits != 32 || fmt != 3) { fprintf(stderr, "need 32-bit float wav (fmt %d bits %d)\n", fmt, bits); fclose(f); return false; }
            out.resize(sz / 4); fread(out.data(), 4, out.size(), f); sr = rate; ch = nch; fclose(f); return true;
        } else fseek(f, sz + (sz & 1), SEEK_CUR);
    }
    fclose(f); return false;
}
static void writeWavF32(const char* path, const std::vector<float>& x, int sr) {
    FILE* f = fopen(path, "wb"); uint32_t dsz = x.size() * 4, rsz = 36 + dsz; uint16_t u16; uint32_t u32;
    fwrite("RIFF", 1, 4, f); fwrite(&rsz, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); u32 = 16; fwrite(&u32, 4, 1, f);
    u16 = 3; fwrite(&u16, 2, 1, f); u16 = 1; fwrite(&u16, 2, 1, f); u32 = sr; fwrite(&u32, 4, 1, f);
    u32 = sr * 4; fwrite(&u32, 4, 1, f); u16 = 4; fwrite(&u16, 2, 1, f); u16 = 32; fwrite(&u16, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&dsz, 4, 1, f); fwrite(x.data(), 4, x.size(), f); fclose(f);
}

int main(int argc, char** argv) {
    if (argc < 4) { fprintf(stderr, "usage: test_chain in.wav ref.wav out.wav [mix] [comp] [dist]\n"); return 1; }
    std::vector<float> in, ref; int sr, ch, sr2, ch2;
    if (!readWavF32(argv[1], in, sr, ch) || !readWavF32(argv[2], ref, sr2, ch2)) return 2;
    std::vector<float> x(in.size() / ch), r(ref.size() / ch2);
    for (size_t i = 0; i < x.size(); ++i) x[i] = in[i * ch];
    for (size_t i = 0; i < r.size(); ++i) r[i] = ref[i * ch2];

    gs::Params p; p.mix = argc > 4 ? atof(argv[4]) : 100.f; p.compAmount = argc > 5 ? atof(argv[5]) : 100.f; p.distAmount = argc > 6 ? atof(argv[6]) : 70.f;
    gs::GuitarSuite g; g.prepare(sr); g.setParams(p);
    std::vector<float> y(x.size());
    for (size_t i = 0; i < x.size(); ++i) y[i] = g.process(x[i]);
    writeWavF32(argv[3], y, sr);

    size_t n = std::min(x.size(), r.size());
    double best = 1e9; int bestLag = 0;
    for (int lag = -3; lag <= 3; ++lag) {
        double e = 0, s = 0;
        for (size_t i = 10; i + 10 < n; ++i) { double d = r[i] - y[i + lag]; e += d * d; s += (double)r[i] * r[i]; }
        double db = 10 * log10(e / s); if (db < best) { best = db; bestLag = lag; }
    }
    printf("error vs reference: %.2f dB (lag %d)\n", best, bestLag);
    return 0;
}
