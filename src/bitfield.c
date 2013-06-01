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
 * \file bitfield.c
 * \brief Bitfield manipulation.
 * \author Sebastien Vincent
 * \date 2007-2013
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitfield.h"

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

struct bitfield* bitfield_new(uint32_t nb)
{
  struct bitfield* ret = NULL;
  uint32_t size = 0;

  size = sizeof(uint32_t) * nb;

  ret = malloc(sizeof(struct bitfield));
  if(!ret)
  {
    return NULL;
  }

  memset(ret, 0x00, sizeof(struct bitfield));
  ret->bits = malloc(size);

  if(!ret->bits)
  {
    free(ret);
    return NULL;
  }

  ret->len = nb;
  return ret;
}

void bitfield_free(struct bitfield** b)
{
  free((*b)->bits);
  free(*b);
  *b = NULL;
}

int bitfield_set_bit(struct bitfield* b, uint32_t bit, int value)
{
  uint32_t k = 0;
  uint32_t l = 0;
  uint32_t mask = 0;

  if(bit > (b->len * sizeof(uint32_t) * 8) - 1)
  {
    return -1;
  }

  k = bit / 8; /* byte in wich the bit we want to set is located */
  l = k / 4; /* which integer of the set of four */
  mask = 1 << (31 - (bit % 32)); /* set a bitmask */

  if(value)
  {
    b->bits[l] = b->bits[l] | mask; /* set to 1 */
  }
  else
  {
    b->bits[l] = b->bits[l] & (~mask); /* set to 0 */
  }

  return l;
}

int bitfield_get_bit(struct bitfield* b, uint32_t bit)
{
  uint32_t k = 0;
  uint32_t l = 0;
  uint32_t mask = 0;

  if(bit > (b->len * sizeof(uint32_t) * 8) - 1)
  {
    return -1;
  }

  k = bit / 8;
  l = k / 4;

  mask = 1 << (31 - (bit % 32));

  if(b->bits[l] & mask)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

#ifdef __cplusplus
}
#endif

