#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

#define Panic(Message, ...)                                                                                         \
    {                                                                                                               \
        char msg[128];                                                                                              \
        std::snprintf(msg, sizeof(msg), Message, ##__VA_ARGS__);                                                    \
        char buf[256];                                                                                              \
        std::snprintf(buf, sizeof(buf), "Panic: %s in function %s (%s:%u)", msg, __FUNCTION__, __FILE__, __LINE__); \
        std::fputs(buf, stderr);                                                                                    \
        std::fflush(stderr);                                                                                        \
        std::abort();                                                                                               \
    }                                                                                                               \
    while (0)

#define Assert(Expr)                                                                                             \
    if (!(Expr)) {                                                                                               \
        char buf[256];                                                                                           \
        std::snprintf(buf, sizeof(buf), "%s in function %s (%s:%u)", "Assert failed: '" #Expr "'", __FUNCTION__, \
                      __FILE__, __LINE__);                                                                       \
        std::fputs(buf, stderr);                                                                                 \
        std::fflush(stderr);                                                                                     \
        std::abort();                                                                                            \
    }                                                                                                            \
    while (0)

