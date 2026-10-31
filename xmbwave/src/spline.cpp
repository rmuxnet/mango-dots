#include "spline.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr double FlowSpeed = 0.18;
constexpr double Tension = 0.12;
constexpr double Damping = 0.0001;
constexpr double Length = 0.306001;
constexpr double Spacing = 407.658;
constexpr double TimeStep = 1.0;
constexpr double BandAmplitude = 0.2;
constexpr double BandSecondaryFreq = 7.0;
constexpr double BandSecondaryAmp = 0.025;
constexpr double TravelSpeed1 = 0.25;
constexpr double TravelAmp1 = 0.014;
constexpr double TravelSpeed2 = 0.15;
constexpr double TravelAmp2 = 0.008;
constexpr double Perturbation = 0.0998587;
constexpr double PerturbationScale = 0.07;
constexpr double PipelineBlend = 0.45;
constexpr double DescriptorStrength = 0.7;
constexpr int DescriptorSeed = 1337;
constexpr double DescriptorMotion = 0.65;
constexpr double KernelGain = 0.04;
constexpr double NormalizeGain = 0.08;
constexpr double KernelPhaseStep = 0.45;
constexpr double IndexJitter = 0.006;
constexpr double TemporalSmooth = 0.84;
constexpr double ReferenceFps = 60.0;

constexpr double NormA[4] = {0.39584, -0.0052389996, -0.58664495, 0.189007};
constexpr double NormB[4] = {-0.003751, -0.57536095, 0.161975, 0.417137};
constexpr int R37Words[4] = {0x00, 0x11, 0x22, 0x33};

double fract(double v)
{
    return v - std::floor(v);
}

double hash01(double x)
{
    return fract(std::sin(x) * 43758.5453123);
}

double clamp01(double v)
{
    return std::clamp(v, 0.0, 1.0);
}

double jsMod(double v, double len)
{
    double out = std::fmod(v, len);
    if (out < 0)
        out += len;
    return out;
}

}

XmbSpline::XmbSpline()
{
    const double len = DescBytes;
    for (int i = 0; i < DescBytes; ++i) {
        const double f = i / len;
        const double wave = std::sin((f * 97.0) * (0.3 + Length)) * 0.5
            + std::sin((f * 211.0) * (0.15 + Tension * 3.0)) * 0.3
            + std::sin((f * 17.0) * (0.2 + PerturbationScale * 2.0)) * 0.2;
        const double noise = hash01(i * 13.37 + DescriptorSeed * 0.01) * 2.0 - 1.0;
        const double v = clamp01((wave * DescriptorStrength + noise * 0.35 + 1.0) * 0.5);
        m_desc[i] = static_cast<uint8_t>(static_cast<int>(v * 255));
    }

    const double freq[Terms] = {6.2, 4.8, M_PI * 1.3, M_PI * 2.8, (4.0 + Length * 2.0) * Spacing * 0.01};
    for (int i = 0; i < ControlPoints; ++i) {
        const double x = i / double(ControlPoints - 1);
        for (int k = 0; k < Terms; ++k) {
            m_cpTrig[(i * Terms + k) * 2] = std::sin(x * freq[k]);
            m_cpTrig[(i * Terms + k) * 2 + 1] = std::cos(x * freq[k]);
        }
    }

    for (int c = 0; c < Cols; ++c)
        m_colBasis[c] = basis(c / double(Cols - 1));
}

XmbSpline::Basis XmbSpline::basis(double u)
{
    const int n = ControlPoints - 3;
    const double s = std::clamp(u * n, 0.0, n - 1e-6);
    const int seg = static_cast<int>(s);
    const double f = s - seg;
    const double f2 = f * f;
    const double f3 = f2 * f;
    return {seg, {float((1 - 3 * f + 3 * f2 - f3) / 6), float((4 - 6 * f2 + 3 * f3) / 6),
                  float((1 + 3 * f + 3 * f2 - 3 * f3) / 6), float(f3 / 6)}};
}

void XmbSpline::rowWeights(float v, int &y0, int &y1, float &t)
{
    const float y = v * Rows - 0.5f;
    const float fy = std::floor(y);
    t = y - fy;
    y0 = std::clamp(static_cast<int>(fy), 0, Rows - 1);
    y1 = std::clamp(static_cast<int>(fy) + 1, 0, Rows - 1);
}

const double *XmbSpline::tableEntry(int entry)
{
    double *out = &m_table[entry * 4];
    if (m_tableStamp[entry] == m_stamp)
        return out;
    m_tableStamp[entry] = m_stamp;

    double c[16];
    const int descBase = (entry * 0x130) % DescBytes;
    for (int blk = 0; blk < 4; ++blk) {
        for (int lane = 0; lane < 4; ++lane) {
            const int idx = (descBase + blk * 0x10 + lane * 4 + entry % 7) % DescBytes;
            const double centered = m_desc[idx] / 255.0 * 2.0 - 1.0;
            const double harmonic = std::sin(entry * 0.07 + blk * 0.91 + lane * 1.37 + m_phase * 0.23);
            c[blk * 4 + lane] = centered * 0.75 + harmonic * 0.25;
        }
    }

    const double b300[4] = {Damping, Length, Tension, Spacing * 0.001};
    for (int lane = 0; lane < 4; ++lane) {
        const double raw = c[lane] * b300[0] + c[4 + lane] * b300[1] + c[8 + lane] * b300[2] + c[12 + lane] * b300[3];
        const double denomAbs = std::max(std::abs(NormB[lane]), 0.05);
        const double denom = NormB[lane] < 0 ? -denomAbs : denomAbs;
        out[lane] = std::tanh(((raw - NormA[lane]) / denom) * NormalizeGain);
    }
    return out;
}

void XmbSpline::runKernel(double t, double dt)
{
    ++m_stamp;
    m_phase = t * FlowSpeed * TimeStep * DescriptorMotion;

    auto sampleTable = [this](double idx, double *out) {
        const int i0 = static_cast<int>(std::floor(idx)) % Entries;
        const int i1 = (i0 + 1) % Entries;
        const double f = idx - std::floor(idx);
        const double *a = tableEntry(i0);
        const double *b = tableEntry(i1);
        for (int c = 0; c < 4; ++c)
            out[c] = a[c] * (1 - f) + b[c] * f;
    };

    for (int iter = 0; iter < Iters; ++iter) {
        const double phase = t * KernelPhaseStep + iter * 0.37;
        double v[4][4];
        for (int lane = 0; lane < 4; ++lane) {
            const int word = (R37Words[lane] + iter * 0x13) & 0xff;
            const int baseIdx = (19 * ((word >> 4) & 0xff) + (word & 0xf)) % Entries;
            const double offset = std::sin(phase + lane * 0.77) * IndexJitter * Entries;
            sampleTable(jsMod(baseIdx + offset, Entries), v[lane]);
        }

        const double mixA = 0.5 + 0.5 * std::sin(phase * 0.7);
        const double mixB = 0.5 + 0.5 * std::cos(phase * 0.9);
        for (int c = 0; c < 4; ++c) {
            const double stores[8] = {
                v[0][c] * (1 - mixA) + v[1][c] * mixA,
                v[1][c] * (1 - mixB) + v[2][c] * mixB,
                v[2][c] * (1 - mixA) + v[3][c] * mixA,
                v[3][c] * (1 - mixB) + v[0][c] * mixB,
                v[1][c] - v[0][c],
                v[2][c] - v[1][c],
                v[3][c] - v[2][c],
                v[0][c] - v[3][c],
            };
            for (int s = 0; s < 8; ++s)
                m_kernel[(iter * 8 + s) * 4 + c] = stores[s];
        }
    }

    const double keep = std::pow(TemporalSmooth, dt * ReferenceFps);
    for (size_t i = 0; i < m_kernel.size(); ++i) {
        const double smoothed = m_prev[i] * keep + m_kernel[i] * (1 - keep);
        m_kernel[i] = smoothed;
        m_prev[i] = smoothed;
    }
}

void XmbSpline::compute(double t, double dt, double shift)
{
    runKernel(t, dt);

    std::array<Basis, Cols> shifted;
    std::array<float, Cols> shiftedU;
    for (int c = 0; c < Cols; ++c) {
        const double u = c / double(Cols - 1) - shift;
        shiftedU[c] = float(u - std::floor(u));
        shifted[c] = basis(shiftedU[c]);
    }
    const Basis first = basis(0.0);
    const Basis last = basis(1.0);

    const double flow = t * FlowSpeed * TimeStep;
    double cp[ControlPoints];
    for (int row = 0; row < Rows; ++row) {
        const double z = (row / double(Rows - 1)) * 2 - 1;
        const double phases[Terms] = {
            flow * 0.25 + z * 1.7,
            z * BandSecondaryFreq + flow * 0.09,
            z * 0.8 - flow * TravelSpeed1,
            -z * 1.2 + flow * TravelSpeed2,
            (z * 4.0 - flow * 0.6) * (Spacing * 0.01),
        };
        double ps[Terms];
        double pc[Terms];
        for (int k = 0; k < Terms; ++k) {
            ps[k] = std::sin(phases[k]);
            pc[k] = std::cos(phases[k]);
        }

        for (int i = 0; i < ControlPoints; ++i) {
            const double *tr = &m_cpTrig[i * Terms * 2];
            const double band = ps[0] * tr[1] + pc[0] * tr[0];
            const double band2 = pc[1] * tr[3] - ps[1] * tr[2];
            const double travel1 = tr[4] * pc[2] + tr[5] * ps[2];
            const double travel2 = tr[6] * pc[3] + tr[7] * ps[3];
            const double perturb = tr[8] * pc[4] + tr[9] * ps[4];

            const double kvf = row * 0.93 + i * 0.61 + flow * 0.35;
            const int kv0 = ((static_cast<int>(std::floor(kvf)) % KernelVecs) + KernelVecs) % KernelVecs;
            const int kv1 = (kv0 + 1) % KernelVecs;
            const double kt = kvf - std::floor(kvf);
            const double *k0 = &m_kernel[kv0 * 4];
            const double *k1 = &m_kernel[kv1 * 4];
            const double kernel = (k0[0] * 0.45 + k0[1] * 0.25 + k0[2] * 0.2 + k0[3] * 0.1) * (1 - kt)
                + (k1[0] * 0.45 + k1[1] * 0.25 + k1[2] * 0.2 + k1[3] * 0.1) * kt;

            const double core = kernel * KernelGain + band * BandAmplitude + band2 * BandSecondaryAmp;
            const double legacy = travel1 * TravelAmp1 * Tension + travel2 * TravelAmp2
                + Perturbation * PerturbationScale * perturb;
            cp[i] = core * PipelineBlend + legacy * (1.0 - PipelineBlend);
        }

        auto eval = [&cp](const Basis &b) {
            return float(b.w[0] * cp[b.seg] + b.w[1] * cp[b.seg + 1] + b.w[2] * cp[b.seg + 2] + b.w[3] * cp[b.seg + 3]);
        };
        const float seam = eval(first) - eval(last);
        float *o1 = &m_d1[row * Cols];
        float *o2 = &m_d2[row * Cols];
        for (int c = 0; c < Cols; ++c) {
            o1[c] = eval(m_colBasis[c]);
            o2[c] = eval(shifted[c]) + seam * shiftedU[c];
        }
    }
}
