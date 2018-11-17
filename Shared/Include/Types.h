#pragma once

#include <stdint.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;


static_assert(sizeof(s8) == 1, "s8 has invalid length!");
static_assert(sizeof(u8) == 1, "u8 has invalid length!");
static_assert(sizeof(s16) == 2, "s16 has invalid length!");
static_assert(sizeof(u16) == 2, "u16 has invalid length!");
static_assert(sizeof(s32) == 4, "s32 has invalid length!");
static_assert(sizeof(u32) == 4, "u32 has invalid length!");
static_assert(sizeof(f32) == 4, "f32 has invalid length!");
static_assert(sizeof(s64) == 8, "s64 has invalid length!");
static_assert(sizeof(u64) == 8, "u64 has invalid length!");
static_assert(sizeof(f64) == 8, "f64 has invalid length!");
