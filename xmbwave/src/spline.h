#pragma once

#include <array>
#include <cstdint>

class XmbSpline
{
public:
    static constexpr int Rows = 64;
    static constexpr int Cols = 100;

    XmbSpline();
    void compute(double t, double dt, double shift);
    static void rowWeights(float v, int &y0, int &y1, float &t);
    const float *d1(int row) const { return &m_d1[row * Cols]; }
    const float *d2(int row) const { return &m_d2[row * Cols]; }

private:
    static constexpr int Entries = 0x169;
    static constexpr int Iters = 8;
    static constexpr int DescBytes = 0x2200;
    static constexpr int KernelVecs = Iters * 8;
    static constexpr int ControlPoints = 28;
    static constexpr int Terms = 5;

    struct Basis {
        int seg;
        float w[4];
    };

    static Basis basis(double u);
    const double *tableEntry(int entry);
    void runKernel(double t, double dt);

    std::array<uint8_t, DescBytes> m_desc{};
    std::array<double, Entries * 4> m_table{};
    std::array<uint32_t, Entries> m_tableStamp{};
    uint32_t m_stamp = 0;
    double m_phase = 0;
    std::array<double, KernelVecs * 4> m_kernel{};
    std::array<double, KernelVecs * 4> m_prev{};
    std::array<double, ControlPoints * Terms * 2> m_cpTrig{};
    std::array<Basis, Cols> m_colBasis{};
    std::array<float, Rows * Cols> m_d1{};
    std::array<float, Rows * Cols> m_d2{};
};
