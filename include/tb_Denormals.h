
#pragma once

#include <cstdint>

#if defined(__x86_64__) || defined(_M_X64)
    #include <immintrin.h>
#endif

namespace tb {

// RAII guard that enables flush-to-zero (FTZ) mode for the current thread's
// FPU, preventing slow denormal processing in DSP code. Restores previous
// state on destruction.
// 
// Supports macOS arm64, Linux x86_64, Windows x86_64.
class FlushDenormalsToZero {
public:
    FlushDenormalsToZero() noexcept {
#if defined(__aarch64__)
        uint64_t fpcr;
        asm volatile("mrs %0, fpcr" : "=r"(fpcr));
        mPreviousState = fpcr;
        asm volatile("msr fpcr, %0" : : "r"(fpcr | (1ULL << 24))); // FZ bit
#elif defined(__x86_64__) || defined(_M_X64)
        mPreviousState = _mm_getcsr();
        _mm_setcsr(mPreviousState | 0x8040u); // FTZ (bit 15) | DAZ (bit 6)
#endif
    }

    ~FlushDenormalsToZero() noexcept {
#if defined(__aarch64__)
        asm volatile("msr fpcr, %0" : : "r"(mPreviousState));
#elif defined(__x86_64__) || defined(_M_X64)
        _mm_setcsr(mPreviousState);
#endif
    }

    FlushDenormalsToZero(const FlushDenormalsToZero&) = delete;
    FlushDenormalsToZero& operator=(const FlushDenormalsToZero&) = delete;

private:
#if defined(__aarch64__)
    uint64_t mPreviousState {};
#elif defined(__x86_64__) || defined(_M_X64)
    unsigned int mPreviousState {};
#else
    #error "Unsupported arch"
#endif
};

} // namespace tb
