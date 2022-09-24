/********************************************************************************
 * \file array.h
 * \author Patrick Torgeson (torgersonpatricks@gmail.com)
 * \brief
 * \version 0.1
 * \date 2022-01-03
 *
 * @copyright Copyright (c) 2022
 *
 ********************************************************************************/


#ifndef ES_ARRAY_H
#define ES_ARRAY_H


// array template
#define es_array(T)           \
    typedef struct T##_arr_t  \
    {                         \
        size_t size;          \
        size_t capacity;      \
        T *data;              \
    } T##_arr


#define es_construct_array(T, a) (es_array_construct(&(a).data, &(a).size, &(a).capacity, sizeof(T)))
#define es_destroy_array(T, a) (es_array_destruct(&(a).data, &(a).size, &(a).capacity))
#define es_arrpushv(T,a,v) ((*(T*)es_array_push(&(a).data, &(a).size, &(a).capacity, sizeof(T))) = (v))
#define es_arrpush(T,a) (es_array_push(&(a).data, &(a).size, &(a).capacity, sizeof(T)))
#define es_arrpop(a) ((a).data[--((a).size)])
#define es_arrpopn(a,n) ((a).size -= n)
#define es_arrback(a) ((a).data[(a).size-1])
#define es_arrnext(a) ((a).data[(a).size])
#define es_arrfront(a) ((a).data[0])
#define es_arrclear(a) ((a).size = 0ull)
#define es_arrat(a,n) ((a).data[n])
#define es_arratr(a,n) ((a).data[(a).size-n])


// generic interface
void  es_array_construct(void **data, size_t *size, size_t *capacity, size_t stride);
void  es_array_destruct(void **data, size_t *size, size_t *capacity);
void *es_array_push(void **data, size_t *size, size_t *capacity, size_t stride);


#endif