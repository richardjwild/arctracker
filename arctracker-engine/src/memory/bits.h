#ifndef ARCTRACKER_BITS_H
#define ARCTRACKER_BITS_H

#include <stddef.h>
#include <stdint.h>

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

#endif //ARCTRACKER_BITS_H
