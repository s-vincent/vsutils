/*
 * Copyright (C) 2007-2008 Sebastien Vincent.
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
 * \file test_bitfield.c
 * \brief Test for the bitfield.
 * \author Sebastien Vincent
 */

#include <stdio.h>
#include <stdlib.h>

#include "bitfield.h"

/**
 * \brief Entry point of the program.
 * \param argc number of argument
 * \param argv array of arguments
 * \return EXIT_SUCCESS
 */
int main(int argc, char** argv)
{
  struct bitfield* b = NULL;
  uint8_t i = 0;

  printf("%s %d\n", argv[0], argc);
  b = bitfield_new(1);

  for (i = 1 ; i < 32 ; i++)
  {
    if(i < 7 || i > 15)
    {
      bitfield_set_bit(b, i, 1);
    }
  }

  for (i = 0 ; i < 32 ; i++)
  {
    printf("%d", bitfield_get_bit(b, i));
  }
  printf("\n");
  bitfield_free(&b);
  return EXIT_SUCCESS;
}

