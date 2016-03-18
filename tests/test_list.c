/*
 * Copyright (C) 2013 Sebastien Vincent.
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
 * \file sample_list.c
 * \brief Tests for lists.
 * \author Sebastien Vincent
 * \date 2013
 */

#include <stdio.h>
#include <stdlib.h>

#include "list.h"

/**
 * \struct element
 * \brief Simple element structure.
 */
struct element
{
  int id; /**< \brief Identifier. */
  struct list_head list; /**< \brief For list management. */
};

/**
 * \brief Entry point of the program.
 * \param argc number of arguments.
 * \param argv array of arguments.
 * \return EXIT_SUCCESS or EXIT_FAILURE.
 */
int main(int argc, char** argv)
{
	struct list_head list;
	struct list_head* tmp = NULL;
	struct list_head* pos = NULL;
	struct element* el = (struct element*)malloc(sizeof(struct element));
	struct element* el2 = (struct element*)malloc(sizeof(struct element));
	struct element* el3 = (struct element*)malloc(sizeof(struct element));

	/* unused variables */
	(void)argc;
	(void)argv;

	if(!el || !el2 || !el3)
	{
		free(el);
		free(el2);
		free(el3);
		return EXIT_SUCCESS;
	}

	el->id = 1;
	el2->id = 2;
	el3->id = 3;

	/* initialize main list */
	list_head_init(&list);

	/* adds the elements */
	list_head_add(&list, &el->list);
	list_head_add(&list, &el2->list);
	list_head_add_tail(&list, &el3->list);

	fprintf(stdout, "Size: %u\n", list_head_size(&list));
	/* print out all elements */
	list_head_iterate_safe(&list, pos, tmp)
	{
		struct element* e = list_head_get(pos, struct element, list);
		fprintf(stdout, "Id: %d\n", e->id);
	}
	
	/* remove one element */
	list_head_remove(&list, &el->list);
	/* remove from the list does not mean free it so do not forget to free the
	 * element (if it was dynamically allocated) 
	 */
	free(el);

	fprintf(stdout, "Size: %u\n", list_head_size(&list));

	/* iterate over the list and free all elements */
	list_head_iterate_safe(&list, pos, tmp)
	{
		struct element* e = list_head_get(pos, struct element, list);
		fprintf(stdout, "Id: %d\n", e->id);
		list_head_remove(&list, &e->list);
		free(e);
	}

	return EXIT_SUCCESS;
}

