/********************************************************************************
 * \file string.h
 * \author Patrick Torgeson (torgersonpatricks@gmail.com)
 * \brief
 * \version 0.1
 * \date 2022-01-05
 *
 * @copyright Copyright (c) 2022
 *
 ********************************************************************************/


#ifndef ES_STRING_H
#define ES_STRING_H


#include "common.h"
#include "object.h"


typedef struct es_string_t
{
    es_object obj;

    char* data;
    size_t size;
    size_t capacity;
} es_string;


#define AS_STRING(o) ((es_string*)(o->obj))


void es_construct_string(es_string *str, const char *init, size_t size);
void es_destroy_string(es_string *str);
void es_copy_string(es_string *dest, es_string *src);

int es_cmp_strings(es_string *l, es_string *r);

// void es_strappend(es_string *str, const es_string *other);
// es_string *es_strconcat(const es_string *l, const es_string *r);


#endif