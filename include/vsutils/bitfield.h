/*
 * Copyright (C) 2007-2013 Sebastien Vincent.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * \file bitfield.h
 * \brief Bitfield manipulation.
 * \author Sebastien Vincent
 * \date 2007-2013
 */

#ifndef VSUTILS_BITFIELD_H
#define VSUTILS_BITFIELD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

/**
 * \brief The structure represents a bitfield.
 */
struct bitfield
{
  uint32_t* bits; /**< The bitfield (array of "four bytes" integer) */
  uint32_t len; /**< Lenght of bits */
};

/**
 * \brief Create a new bitfield.
 * \param nb allocate nb "four byte" integer (i.e. nb * 4 * 8 bits).
 * \return pointer on the bitfield or NULL if failed.
 */
struct bitfield* bitfield_new(uint32_t nb);

/**
 * \brief Free the bitfield.
 * \param b pointer on the bitfield.
 */
void bitfield_free(struct bitfield** b);

/**
 * \brief Set a bit in the bitfield.
 * \param b pointer on a bitfield.
 * \param bit bit to set (begin to 0).
 * \param value value to set.
 * \return 0 if success, -1 otherwise.
 * \warning the first bit is number 0.
 */
int bitfield_set_bit(struct bitfield* b, uint32_t bit, int value);

/**
 * \brief Get the value for the specified bit.
 * \param b pointer on a bitfield.
 * \param bit bit you want to get the value.
 * \return value of the bit or -1 if failure.
 */
int bitfield_get_bit(struct bitfield* b, uint32_t bit);

#ifdef __cplusplus
}
#endif

#endif /* VSUTILS_BITFIELD_H */

