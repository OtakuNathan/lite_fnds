#ifndef LITE_FNDS_PAUSE_H
#define LITE_FNDS_PAUSE_H

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#elif defined(__aarch64__)
#else
#include <thread>
#endif

namespace lite_fnds {
    inline void yield() noexcept {
#if defined(__x86_64__) || defined(_M_X64)
        _mm_pause();
#elif defined(__aarch64__)
        __asm__ __volatile__("yield");
#else
        std::this_thread::yield();
#endif
    }
}

#endif
