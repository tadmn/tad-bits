#include "tb_Denormals.h"

#include <catch2/catch_test_macros.hpp>
#include <limits>

#if defined(__aarch64__)
    #include <cstdint>
#elif defined(__x86_64__) || defined(_M_X64)
    #include <immintrin.h>
#endif

using namespace tb;

TEST_CASE("FlushDenormalsToZero - construction", "[FlushDenormalsToZero]") {
    REQUIRE_NOTHROW(FlushDenormalsToZero{});
}

#if defined(__aarch64__)

namespace {
    uint64_t readFPCR() noexcept {
        uint64_t v;
        asm volatile("mrs %0, fpcr" : "=r"(v));
        return v;
    }
}

TEST_CASE("FlushDenormalsToZero - ARM64 CPU state", "[FlushDenormalsToZero]") {
    auto savedState = readFPCR();

    SECTION("FZ bit is set in FPCR during scope") {
        FlushDenormalsToZero ftz;
        REQUIRE((readFPCR() & (1ULL << 24)) != 0);
    }

    SECTION("FPCR is restored after scope") {
        { FlushDenormalsToZero ftz; }
        REQUIRE(readFPCR() == savedState);
    }

    SECTION("Nested objects restore state correctly") {
        FlushDenormalsToZero ftz1;
        auto stateWithFtz = readFPCR();
        { FlushDenormalsToZero ftz2; }
        REQUIRE(readFPCR() == stateWithFtz);
    }
}

TEST_CASE("FlushDenormalsToZero - ARM64 functional", "[FlushDenormalsToZero]") {
    // Ensure FZ is cleared so the baseline check is valid
    auto savedState = readFPCR();
    auto clearedState = savedState & ~(1ULL << 24);
    asm volatile("msr fpcr, %0" : : "r"(clearedState));

    SECTION("Denormals are flushed to zero") {
        volatile float tiny = std::numeric_limits<float>::min();
        volatile float half = 0.5f;
        REQUIRE(tiny * half != 0.0f); // denormal without FTZ

        FlushDenormalsToZero ftz;
        volatile float t = std::numeric_limits<float>::min();
        volatile float h = 0.5f;
        REQUIRE(t * h == 0.0f); // flushed with FTZ
    }

    asm volatile("msr fpcr, %0" : : "r"(savedState));
}

#elif defined(__x86_64__) || defined(_M_X64)

TEST_CASE("FlushDenormalsToZero - x86_64 CPU state", "[FlushDenormalsToZero]") {
    auto savedState = _mm_getcsr();

    SECTION("FTZ and DAZ bits are set in MXCSR during scope") {
        FlushDenormalsToZero ftz;
        REQUIRE((_mm_getcsr() & 0x8040u) == 0x8040u);
    }

    SECTION("MXCSR is restored after scope") {
        { FlushDenormalsToZero ftz; }
        REQUIRE(_mm_getcsr() == savedState);
    }

    SECTION("Nested objects restore state correctly") {
        FlushDenormalsToZero ftz1;
        auto stateWithFtz = _mm_getcsr();
        { FlushDenormalsToZero ftz2; }
        REQUIRE(_mm_getcsr() == stateWithFtz);
    }
}

TEST_CASE("FlushDenormalsToZero - x86_64 functional", "[FlushDenormalsToZero]") {
    // Ensure FTZ/DAZ are cleared so baseline checks are valid
    auto savedState = _mm_getcsr();
    _mm_setcsr(savedState & ~0x8040u);

    SECTION("Denormal outputs are flushed to zero (FTZ)") {
        volatile float tiny = std::numeric_limits<float>::min();
        volatile float half = 0.5f;
        REQUIRE(tiny * half != 0.0f); // produces a denormal without FTZ

        FlushDenormalsToZero ftz;
        volatile float t = std::numeric_limits<float>::min();
        volatile float h = 0.5f;
        REQUIRE(t * h == 0.0f); // flushed to zero
    }

    SECTION("Denormal inputs are treated as zero (DAZ)") {
        volatile float denormal = std::numeric_limits<float>::denorm_min();
        volatile float two = 2.0f;
        REQUIRE(denormal * two != 0.0f); // non-zero without DAZ

        FlushDenormalsToZero ftz;
        volatile float d = std::numeric_limits<float>::denorm_min();
        REQUIRE(d * two == 0.0f); // treated as zero
    }

    _mm_setcsr(savedState);
}

#endif // platform
