// Offline test with per-block flags: test_blocks in.wav ref.wav mix comp dist flags(c,f,d,b)
#include "../GuitarSuiteDSP.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
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
            if (bits != 32 || fmt != 3) { fclose(f); return false; }
            out.resize(sz / 4); fread(out.data(), 4, out.size(), f); sr = rate; ch = nch; fclose(f); return true;
        } else fseek(f, sz + (sz & 1), SEEK_CUR);
    }
    fclose(f); return false;
}
int main(int argc, char** argv) {
    std::vector<float> in, ref; int sr, ch, sr2, ch2;
    if (!readWavF32(argv[1], in, sr, ch) || !readWavF32(argv[2], ref, sr2, ch2)) { fprintf(stderr, "read fail\n"); return 2; }
    std::vector<float> x(in.size() / ch), r(ref.size() / ch2);
    for (size_t i = 0; i < x.size(); ++i) x[i] = in[i * ch];
    for (size_t i = 0; i < r.size(); ++i) r[i] = ref[i * ch2];
    gs::Params p; p.mix = atof(argv[3]); p.compAmount = atof(argv[4]); p.distAmount = atof(argv[5]);
    std::string fl = argv[6]; p.compOn = fl.find('c') != std::string::npos; p.filtOn = fl.find('f') != std::string::npos;
    p.distOn = fl.find('d') != std::string::npos; p.boxOn = fl.find('b') != std::string::npos;
    gs::GuitarSuite g; g.prepare(sr); g.setParams(p);
    std::vector<float> y(x.size()); for (size_t i = 0; i < x.size(); ++i) y[i] = g.process(x[i]);
    size_t n = std::min(x.size(), r.size()); double best = 1e9; int bestLag = 0;
    for (int lag = -12; lag <= 12; ++lag) { double e = 0, s = 0;
        for (size_t i = 20; i + 20 < n; ++i) { double d = r[i] - y[i + lag]; e += d * d; s += (double)r[i] * r[i]; }
        double db = 10 * log10(e / s); if (db < best) { best = db; bestLag = lag; } }
    float pk = 0; for (float v : y) pk = std::max(pk, std::fabs(v));
    printf("%-22s flags=%-4s mix=%3s comp=%3s dist=%3s : err %.1f dB (lag %d)  peak %.1f dBFS\n", argv[2] + strlen(argv[2]) - 22 > argv[2] ? strrchr(argv[2], '/') + 1 : argv[2], argv[6], argv[3], argv[4], argv[5], best, bestLag, 20 * log10(pk));
    return 0;
}
