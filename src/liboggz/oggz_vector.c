/*
   Copyright (C) 2003 Commonwealth Scientific and Industrial Research
   Organisation (CSIRO) Australia

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of CSIRO Australia nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "oggz_private.h"

/*
 * An optionally sorted vector
 *
 * if you set a comparison function (oggz_vector_set_cmp()), the vector
 * will be sorted and new elements will be inserted in sorted order.
 *
 * if you don't set a comparison function, new elements will be appended
 * at the tail
 *
 * to unset the comparison function, call oggz_vector_set_cmp(NULL,NULL)
 */

OggzVector *
oggz_vector_init (OggzVector * vector, size_t sizeof_element)
{
  vector->max_elements = 0;
  vector->nr_elements = 0;
  vector->sizeof_element = sizeof_element;
  vector->data = NULL;
  vector->compare = NULL;

  return vector;
}

void
oggz_vector_clear (OggzVector * vector)
{
  oggz_free (vector->data);
  vector->data = NULL;
  vector->nr_elements = 0;
  vector->max_elements = 0;
}

void *
oggz_vector_find (OggzVector * vector, OggzFindFunc func, long serialno)
{
  void * data;
  int i;

  for (i = 0; i < vector->nr_elements; i++) {
    data = vector->data[i];
    if (func (data, serialno))
      return data;
  }

  return NULL;
}

int
oggz_vector_foreach (OggzVector * vector, OggzFunc func)
{
  int i;

  for (i = 0; i < vector->nr_elements; i++) {
    func (vector->data[i]);
  }

  return 0;
}

static void
_array_swap (void *v[], int i, int j)
{
  void * t;

  t = v[i];
  v[i] = v[j];
  v[j] = t;
}

/**
 * Helper function for oggz_vector_element_add(). Sorts the vector by
 * insertion sort, assuming the tail element has just been added and the
 * rest of the vector is sorted.
 * \param vector An OggzVector
 * \pre The vector has just had a new element added to its tail
 * \pre All elements other than the tail element are already sorted.
 */
static void
oggz_vector_tail_insertion_sort (OggzVector * vector)
{
  int i;

  if (vector->compare == NULL) return;

  for (i = vector->nr_elements-1; i > 0; i--) {
    if (vector->compare (vector->data[i-1], vector->data[i],
			 vector->compare_user_data) > 0) {
      _array_swap (vector->data, i, i-1);
    } else {
      break;
    }
  }

  return;
}

void *
oggz_vector_add_element (OggzVector * vector, void * data)
{
  void * new_elements;
  int new_max_elements;

  vector->nr_elements++;

  if (vector->nr_elements > vector->max_elements) {
    if (vector->max_elements == 0) {
      new_max_elements = 1;
    } else {
      new_max_elements = vector->max_elements * 2;
    }

    new_elements =
      realloc (vector->data, (size_t)new_max_elements * sizeof (void *));

    if (new_elements == NULL) {
      vector->nr_elements--;
      return NULL;
    }

    vector->max_elements = new_max_elements;
    vector->data = new_elements;
  }

  vector->data[vector->nr_elements-1] = data;

  oggz_vector_tail_insertion_sort (vector);

  return data;
}

static void
oggz_vector_qsort (OggzVector * vector, int left, int right)
{
  int i, last;
  void ** v = vector->data;

  if (left >= right) return;

  _array_swap (v, left, (left + right)/2);
  last = left;
  for (i = left+1; i <= right; i++) {
    if (vector->compare (v[i], v[left], vector->compare_user_data) < 0)
      _array_swap (v, ++last, i);
  }
  _array_swap (v, left, last);
  oggz_vector_qsort (vector, left, last-1);
  oggz_vector_qsort (vector, last+1, right);
}

int
oggz_vector_set_cmp (OggzVector * vector, OggzCmpFunc compare,
		     void * user_data)
{
  vector->compare = compare;
  vector->compare_user_data = user_data;

  if (compare) {
    oggz_vector_qsort (vector, 0, vector->nr_elements-1);
  }

  return 0;
}

void *
oggz_vector_pop (OggzVector * vector)
{
  void * data;
  void * new_elements;
  int new_max_elements;

  if (!vector || vector->data == NULL) return NULL;

  data = vector->data[0];

  vector->nr_elements--;

  if (vector->nr_elements == 0) {
    oggz_vector_clear (vector);
  } else {
#if 0
    memmove (vector->data, &vector->data[1],
	     vector->nr_elements * sizeof (void *));
#else
    {
      int i;
      for (i = 0; i < vector->nr_elements; i++) {
	vector->data[i] = vector->data[i+1];
      }
    }
#endif
    if (vector->nr_elements < vector->max_elements/2) {
      new_max_elements = vector->max_elements/2;

      new_elements =
	realloc (vector->data, (size_t)new_max_elements * sizeof (void *));

      if (new_elements != NULL) {
	vector->max_elements = new_max_elements;
	vector->data = new_elements;
      }
    }

  }

  return data;

}
