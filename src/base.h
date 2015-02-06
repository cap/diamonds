#pragma once


#define cap_first(a, b) ((a)? (a) : (b))
#define cap_first_3(a, b, c) cap_first((a), cap_first((b), (c)))
#define cap_min(a, b) ((a) < (b)? (a) : (b))
#define cap_min_3(a, b, c) cap_min((a), cap_min((b), (c)))
#define cap_max(a, b) ((a) > (b)? (a) : (b))
#define cap_max_3(a, b, c) cap_max((a), cap_max((b), (c)))
#define cap_sign(x) ((x) < 0? -1 : ((x) > 0? 1 : 0))

#define cap_quote_def(x) #x
#define cap_str_def(x) cap_quote_def(x)

#define cap_array_size(a) (sizeof(a) / sizeof((a)[0]))
#define cap_zero_array(a, size) memset((a), 0, size * sizeof((a)[0]));

#define cap_kilobytes(a) ((a) * 1024)
#define cap_megabytes(a) (cap_kilobytes(a) * 1024)
#define cap_gigabytes(a) (cap_megabytes(a) * 1024)

#define cap_log_error(msg, ...) fprintf(stderr, msg, __VA_ARGS__)
#ifdef CAP_DEBUG
#define cap_log_debug(msg, ...) fprintf(stderr, msg, __VA_ARGS__)
#else
#define cap_log_debug(msg, ...)
#endif

#include <stdint.h>
typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef i32 b32;
typedef float f32;
typedef double f64;


#include "vector.h"
typedef Vector<i32, 2> v2i;
typedef Vector<i32, 3> v3i;
typedef Vector<i32, 4> v4i;
typedef Vector<f32, 2> v2f;
typedef Vector<f32, 3> v3f;
typedef Vector<f32, 4> v4f;
 // colors
typedef Vector<u8, 4> c4u8;
typedef Vector<f32, 4> c4f;
