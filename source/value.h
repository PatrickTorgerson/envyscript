/********************************************************************************
 * \file value.h
 * \author Patrick Torgeson (torgersonpatricks@gmail.com)
 * \brief
 * \version 0.1
 * \date 2021-12-24
 *
 * @copyright Copyright (c) 2021
 *
 ********************************************************************************/


#ifndef ES_VALUE_H
#define ES_VALUE_H


#include "common.h"
#include "string.h"
#include "array.h"


typedef u64 es_typeid;

struct es_value_arr_t;
struct es_map_t;


enum es_builtin
{
    ES_NIL,
    ES_INT,
    ES_FLOAT,
    ES_BOOL,
    ES_STRUCT,
    ES_FUNCPTR,
    ES_STRING,
    ES_ARRAY,
    ES_MAP,
    ES_BUILTIN_COUNT
};


typedef struct es_value_t
{
    es_typeid tid;
    union
    {
        u64    u;
        i64    i;
        f64    f;
        void  *p;

        es_object *obj;
    };
} es_value;

es_array(es_value);


bool es_cmp_values(es_value *l, es_value *r);
void es_copy_value(es_value *dest, es_value *src);
void es_destroy_value(es_value *v);

#endif