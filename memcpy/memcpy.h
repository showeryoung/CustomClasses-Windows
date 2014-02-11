#ifndef MEMCPY_H
#define MEMCPY_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief MMX enhanced memory copy with address aligned
/// @note dst & src must be aligned to native integer
void* aligned_memcpy_mmx(void* dst, const void* src, size_t num);

/// @brief SSE2 enhanced memory copy with address aligned
/// @note dst & src must be aligned to native integer
void* aligned_memcpy_sse2(void* dst, const void* src, size_t num);

/// @brief SSE2 enhanced memory copy with address unaligned
/// @note dst & src doesn't need to be aligned to native integer
void* unaligned_memcpy_sse2(void* dst, const void* src, size_t num);

/// @brief AVX enhanced memory copy with address unaligned
/// @note dst & src doesn't need to be aligned to native integer
void* unaligned_memcpy_avx(void* dst, const void* src, size_t num);

#ifdef __cplusplus
}
#endif

#endif // MEMCPY_H
