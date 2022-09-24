/********************************************************************************
 * \file object.h
 * \author Patrick Torgeson (torgersonpatricks@gmail.com)
 * \brief
 * \version 0.1
 * \date 2022-01-14
 *
 * @copyright Copyright (c) 2022
 *
 ********************************************************************************/


#ifndef ES_OBJECT_H
#define ES_OBJECT_H


#include "common.h"


typedef struct es_object_t
{
    u64 refcount;
} es_object;

#define ES_ALLOCATE_OBJ(o) ((es_object*)malloc(sizeof(o)))


#endif