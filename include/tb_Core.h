
#pragma once

#include <cassert>

// A thin wrapper on `assert` to avoid unused variable warnings in release builds
#ifndef NDEBUG
#define tb_assert(expr) assert(expr); (void)(expr)
#else
#define tb_assert(expr) (void)(expr)
#endif