#include "string.h"


//*************************************************************************
void es_construct_string(es_string *str, const char *init, size_t size)
{
    str->capacity = size + 1;
    str->size = size;
    str->data = (char*) malloc(str->capacity);
    memcpy(str->data, init, size);
    str->data[size] = '\0';
    str->obj.refcount = 1;
}


//*************************************************************************
void es_destroy_string(es_string *str)
{
    free(str->data);
    str->obj.refcount = 0;
    str->capacity = 0;
    str->size = 0;
    str->data = NULL;
}


//*************************************************************************
void es_copy_string(es_string *dest, es_string *src)
{
    char* ptr = realloc(dest->data, src->capacity);

    if(ptr)
    {
        dest->capacity = src->capacity;
        dest->size = src->size;
        dest->data = ptr;
    }

    memcpy_s(dest->data, dest->capacity, src->data, src->size);
}


//*************************************************************************
int es_cmp_strings(es_string *l, es_string *r)
{
    if(l->size == r->size)
        return strncmp(l->data, r->data, l->size);
    else
    {
        int cmp = strncmp(l->data, r->data, l->size);
        if(cmp == 0)
            return (l->size > r->size)*2 - 1;
        else return cmp;
    }
}