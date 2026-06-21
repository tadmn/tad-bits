
#pragma once

#include <cassert>
#include <stdexcept>
#include <string>

// A thin wrapper on `assert` to avoid unused variable warnings in release builds
#ifndef NDEBUG
#define tb_assert(expr) assert(expr); (void)(expr)
#else
#define tb_assert(expr) (void)(expr)
#endif

namespace tb {
class Error : public std::runtime_error
{
public:
    explicit Error(const std::string& message)
        : std::runtime_error(message)
        , mMessage(message) {
    }

    const std::string& getMessage() const { return mMessage; }

private:
    std::string mMessage;
};

struct Result {
    std::string msg;
    explicit operator bool() const { return msg.empty(); }
};

template<typename T>
class ScopedSetter {
  public:
    ScopedSetter(T& original, T newValue) : mTarget(original), mOriginal(std::move(original)) {
        mTarget = std::move(newValue);
    }

    ~ScopedSetter() { mTarget = std::move(mOriginal); }

    ScopedSetter(const ScopedSetter&) = delete;
    ScopedSetter& operator=(const ScopedSetter&) = delete;
    ScopedSetter(ScopedSetter&&) = delete;  // see note below

  private:
    T& mTarget;
    T mOriginal;
};

}

#define tb_throw(_MSG) throw tb::Error(_MSG)
#define tb_throwIf(_COND) if (!(_COND)); else tb_throw("Failed:" #_COND)
#define tb_throwMsgIf(_COND, _MSG) if (!(_COND)); else tb_throw(_MSG)