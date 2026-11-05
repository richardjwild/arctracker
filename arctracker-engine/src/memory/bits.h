#ifndef ARCTRACKER_BITS_H
#define ARCTRACKER_BITS_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

_Static_assert(sizeof(float) == sizeof(uint32_t), "32-bit float required");

static inline unsigned int mask_and_shift_right(const unsigned int value, const unsigned int mask, const unsigned int shift)
{
    return value >> shift & mask;
}

static inline unsigned int mask_5_shift_right(const unsigned int value, const unsigned int shift)
{
    return mask_and_shift_right(value, 0x1f, shift);
}

static inline unsigned int mask_6_shift_right(const unsigned int value, const unsigned int shift)
{
    return mask_and_shift_right(value, 0x3f, shift);
}

static inline unsigned int mask_8_shift_right(const unsigned int value, const unsigned int shift)
{
    return mask_and_shift_right(value, 0xff, shift);
}

static inline unsigned int high_nybble(const unsigned int value)
{
    return mask_and_shift_right(value, 0xf, 4);
}

static inline unsigned int low_nybble(const unsigned int value)
{
    return value & 0xf;
}

static inline size_t align_to_word(const size_t length)
{
    return length % 4 ? length + (4 - length % 4) : length;
}

static inline uint32_t round_up_to_power_of_two(uint32_t n)
{
    if (n <= 1) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

static inline uint8_t read_u8(const uint8_t *addr)
{
    return *addr;
}

static inline int8_t read_s8(const uint8_t *addr)
{
    return (int8_t) *addr;
}

static inline uint16_t read_u16_le(const uint8_t *addr)
{
    return addr[0] | addr[1] << 8;
}

static inline uint32_t read_u32_le(const uint8_t *addr)
{
    return addr[0] | addr[1] << 8 | addr[2] << 16 | addr[3] << 24;
}

static inline float read_f32_le(const uint8_t *addr)
{
    const uint32_t bits = read_u32_le(addr);
    float value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

#endif //ARCTRACKER_BITS_H
