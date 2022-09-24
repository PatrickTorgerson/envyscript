#include "value.h"
#include "string.h"


//*************************************************************************
bool es_cmp_values(es_value *l, es_value *r)
{
    if(l->tid != r->tid) return false;

    switch(l->tid)
    {
    case ES_NIL:
        return true;
    case ES_INT:
    case ES_FLOAT:
    case ES_BOOL:
        return l->u == r->u;
    case ES_STRING:
        return es_cmp_strings(AS_STRING(l), AS_STRING(r)) == 0;
    default:
        printf("\n\n  >  value comparison error!\n\n");
        exit(-1);
    }

    return false;
}


//*************************************************************************
void es_copy_value(es_value *dest, es_value *src)
{
    // TODO: improve for case where types are equal

    if(dest->tid >= ES_STRUCT)
        es_destroy_value(dest);

    if(src->tid < ES_STRUCT)
    {
        dest->tid = src->tid;
        dest->u   = src->u;
    }
    else
    {
        if(src->obj == NULL) return;

        dest->tid = src->tid;

        // take ownership
        if(src->obj->refcount == 0)
        {
            dest->obj = src->obj;
            dest->obj->refcount = 1;
            return;
        }

        // copy
        switch(src->tid)
        {
        case ES_STRING:
            dest->obj = ES_ALLOCATE_OBJ(es_string);
            es_construct_string(AS_STRING(dest), AS_STRING(src)->data, AS_STRING(src)->size);
            break;
        default:
            printf("\n\n  >  value copy error!\n\n");
            exit(-1);
        }
    }
}


//*************************************************************************
void es_destroy_value(es_value *v)
{
    if(v->tid < ES_STRUCT)
    {
        v->tid = ES_NIL;
        v->u   = 0;
    }
    else
    {
        if(v->obj == NULL) return;

        v->obj->refcount -= 1;
        if(v->obj->refcount > 0) return;

        switch(v->tid)
        {
        case ES_STRING:
            es_destroy_string(AS_STRING(v));
            break;
        default:
            printf("\n\n  >  value destroy error!\n\n");
            exit(-1);
        }

        free(v->obj);
        v->obj = NULL;
        v->tid = ES_NIL;
    }
}