/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <stdint.h>
#include <assert.h>
#include "mem.h"
#include "mem_internals.h"

unsigned int puiss2(unsigned long size) {
    unsigned int p=0;
    size = size -1; // allocation start in 0
    while(size) {  // get the largest bit
	p++;
	size >>= 1;
    }
    if (size > (1 << p))
	p++;
    return p;
}


void * emalloc_medium(unsigned long size)
{
    assert(size < LARGEALLOC);
    assert(size > SMALLALLOC);
    unsigned int i = puiss2(size);
    if (arena.TZL[i] != NULL){
        void ** ptr_head = arena.TZL[i];
        void ** ptr_next = * ptr_head;
        arena.TZL[i] = ptr_next;
        return mark_memarea_and_get_user_ptr(ptr_head, 1<<(i), MEDIUM_KIND);
    }
    else {
        char found = 0;
        int j = i;
        while ((found == 0) && (j < FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant)) {
            if (arena.TZL[j] != NULL) {
                found = 1;
            }
            j++;
        }
        if (found == 0) {
            mem_realloc_medium();
        }
        void ** ptr_current = arena.TZL[j];
        while (j > i) {
            arena.TZL[j-1] = (void **)((unsigned long)ptr_current ^ (1<<(j-1)));
            j--;
        }   
        return mark_memarea_and_get_user_ptr(ptr_current, 1<<(i), MEDIUM_KIND);
    }
}



void efree_medium(Alloc a) {
    /* ecrire votre code ici */
    void ** ptr_current = a.ptr;
    void ** buddy = (void **)((unsigned long)ptr_current ^ (1<<(a.size)));
    int i = puiss2(a.size);
    void ** ptr_tzl_before = arena.TZL[i];
    void ** ptr_tzl= NULL;
    while (*ptr_tzl_before != buddy  && ptr_tzl_before != NULL) {
        ptr_tzl_before = *ptr_tzl_before;
    }
    if (ptr_tzl_before == NULL) {
        *ptr_current = arena.TZL[i];
        arena.TZL[i] = ptr_current;
    } else {
        ptr_tzl = *ptr_tzl_before;
        *ptr_tzl_before = *ptr_tzl;
        if ((unsigned long)ptr_current > (unsigned long)buddy) {
            a.ptr = buddy;
        }
        a.size *= 2;
        efree_medium(a);
    }
}


