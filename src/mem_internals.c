/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <sys/mman.h>
#include <assert.h>
#include <stdint.h>
#include "mem.h"
#include "mem_internals.h"
unsigned long knuth_mmix_one_round(unsigned long in)
{
    return in * 6364136223846793005UL % 1442695040888963407UL;
}

void *mark_memarea_and_get_user_ptr(void *ptr, unsigned long size, MemKind k)
{
    unsigned long magic = knuth_mmix_one_round((unsigned long)ptr);
    switch (k){
        case SMALL_KIND:
            magic = magic & 0xFFFFFFFFFFFFFFFC;
            break;
        case MEDIUM_KIND:
            magic = (magic & 0xFFFFFFFFFFFFFFFC) | 0x00000001;
            break;
        case LARGE_KIND:
            magic = (magic & 0xFFFFFFFFFFFFFFFC) | 0x00000002;
            break;
    }
    unsigned long * ptr_bis = (unsigned long *)ptr;   
    *ptr_bis = size;
    ptr_bis++;
    *ptr_bis = magic;
    ptr_bis += (int)(size/8)-3;
    *ptr_bis = magic;
    ptr_bis++;
    *ptr_bis = size;
    ptr_bis -= (int)(size/8) -3;
    return (void *)ptr_bis;  
}

Alloc mark_check_and_get_alloc(void *ptr)
{
    /* ecrire votre code ici */
    Alloc a = {};
    unsigned long size;
    MemKind k;
    unsigned long magic;
    char * ptr_bis = (char *)ptr;
    ptr_bis -= 16;
    size = *ptr_bis;
    ptr_bis+= 8;
    magic = *ptr_bis;
    if ((magic & 0b11UL) ==0){
        k = SMALL_KIND;
    }
    else{
        if ((magic & 0b11UL) ==0b01UL){
            k = MEDIUM_KIND;
        }
        else{
            k = LARGE_KIND;
        }
    }
    ptr_bis += size -24;
    assert(*ptr_bis == magic);
    ptr_bis += 8;
    assert(*ptr_bis == size);
    ptr_bis -= size -8;
    a.ptr = (void *)ptr_bis;
    a.size = size;
    a.kind= k;
    return a;
}


unsigned long
mem_realloc_small() {
    assert(arena.chunkpool == 0);
    unsigned long size = (FIRST_ALLOC_SMALL << arena.small_next_exponant);
    arena.chunkpool = mmap(0,
			   size,
			   PROT_READ | PROT_WRITE | PROT_EXEC,
			   MAP_PRIVATE | MAP_ANONYMOUS,
			   -1,
			   0);
    if (arena.chunkpool == MAP_FAILED)
	handle_fatalError("small realloc");
    arena.small_next_exponant++;
    return size;
}

unsigned long
mem_realloc_medium() {
    uint32_t indice = FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant;
    assert(arena.TZL[indice] == 0);
    unsigned long size = (FIRST_ALLOC_MEDIUM << arena.medium_next_exponant);
    assert( size == (1 << indice));
    arena.TZL[indice] = mmap(0,
			     size*2, // twice the size to allign
			     PROT_READ | PROT_WRITE | PROT_EXEC,
			     MAP_PRIVATE | MAP_ANONYMOUS,
			     -1,
			     0);
    if (arena.TZL[indice] == MAP_FAILED)
	handle_fatalError("medium realloc");
    // align allocation to a multiple of the size
    // for buddy algo
    arena.TZL[indice] += (size - (((intptr_t)arena.TZL[indice]) % size));
    arena.medium_next_exponant++;
    return size; // lie on allocation size, but never free
}


// used for test in buddy algo
unsigned int
nb_TZL_entries() {
    int nb = 0;
    
    for(int i=0; i < TZL_SIZE; i++)
	if ( arena.TZL[i] )
	    nb ++;

    return nb;
}
