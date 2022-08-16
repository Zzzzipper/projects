#pragma once

#include <utility>

template <class F>
class ScopeGuard {
public:
    explicit ScopeGuard(F&& f) : f_(std::move(f)) {}
    ~ScopeGuard() {
        try {
            f_();
        } catch (...) {
        }
    }

private:
    F f_;
};

// #define DEFER(f) ScopeGuard scope_##__LINE__{ [&] f }
#define CONCAT_(x,y) x##y
#define CONCAT(x,y) CONCAT_(x,y)
#define DEFER(f) ScopeGuard CONCAT(scope_, __LINE__){ [&] f }
