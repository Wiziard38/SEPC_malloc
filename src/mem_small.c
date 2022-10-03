/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <assert.h>
#include "mem.h"
#include "mem_internals.h"

void *emalloc_small(unsigned long size)
{
    if (arena.chunkpool == 0) {
        unsigned long size = mem_realloc_small();
        void ** ptr_current = arena.chunkpool;

        for (int i = 0; i < size; i += CHUNKSIZE) {
            *ptr_current = ptr_current + CHUNKSIZE/8;
            ptr_current = *ptr_current;
        }
    }

    void ** ptr_head = arena.chunkpool;
    void ** ptr_next = *ptr_head;
    arena.chunkpool = ptr_next;

    return mark_memarea_and_get_user_ptr(ptr_head, CHUNKSIZE, SMALL_KIND);
}

void efree_small(Alloc a) {
    void * ptr_head = arena.chunkpool;
    void ** ptr_new = a.ptr;
    *ptr_new = ptr_head;
    arena.chunkpool = ptr_new;
}
