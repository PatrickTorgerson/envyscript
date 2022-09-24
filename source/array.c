#include "array.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


#define PTRINC(p,i,s) ((void*)(((uint8_t*)(p)) + ((i)*(s))))
#define PTRDEC(p,i,s) ((void*)(((uint8_t*)(p)) - ((i)*(s))))


//*************************************************************************
void es_array_construct(void **data, size_t *size, size_t *capacity, size_t stride)
{
    *size     = 0ull;
    *capacity = 16ull;

    *data = malloc(*capacity * stride);
}


//*************************************************************************
void es_array_destruct(void **data, size_t *size, size_t *capacity)
{
    free(*data);

    *size     = 0ull;
    *capacity = 0ull;
}


//*************************************************************************
void *es_array_push(void **data, size_t *size, size_t *capacity, size_t stride)
{
    if(*size + 1 > *capacity)
    {
        size_t newcap = *capacity + 16ull;
        void* ptr = realloc(*data, newcap * stride);
        if(!ptr) return NULL;
        *data = ptr;
        *capacity = newcap;
        printf("arr realloc : %lli\n", newcap);
    }
    return PTRINC(*data, (*size)++, stride);
}


//*************************************************************************
void *es_array_pop(void **data, size_t *size, size_t *capacity, size_t stride)
{
    return PTRINC(*data, --(*size), stride);
}