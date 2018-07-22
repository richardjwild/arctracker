#ifndef ARCTRACKER_BITS_H
#define ARCTRACKER_BITS_H

#define MASK_AND_SHIFT_RIGHT(value, mask, shift) \
(((unsigned int) (value) >> (unsigned int) (shift)) & (unsigned int) (mask))

#define MASK_5_SHIFT_RIGHT(value, shift) \
(__uint8_t) MASK_AND_SHIFT_RIGHT(value, 0x1f, shift)

#define MASK_6_SHIFT_RIGHT(value, shift) \
(__uint8_t) MASK_AND_SHIFT_RIGHT(value, 0x3f, shift)

#define MASK_8_SHIFT_RIGHT(value, shift) \
(__uint8_t) MASK_AND_SHIFT_RIGHT(value, 0xff, shift)

#define HIGH_NYBBLE(value) \
(__uint8_t) MASK_AND_SHIFT_RIGHT(value, 0xf, 4)

#define LOW_NYBBLE(value) \
(__uint8_t) MASK_AND_SHIFT_RIGHT(value, 0xf, 0)

#endif //ARCTRACKER_BITS_H
