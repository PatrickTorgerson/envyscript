/********************************************************************************
 * \file hashtable.h
 * \author Patrick Torgeson (torgersonpatricks@gmail.com)
 * \brief
 * \version 0.1
 * \date 2022-01-04
 *
 * @copyright Copyright (c) 2022
 *
 ********************************************************************************/


#ifndef ES_HASHTABLE_H
#define ES_HASHTABLE_H


#include "common.h"
#include "value.h"


typedef struct { es_value k; es_value v; } es_map_node;


typedef struct es_map_t
{
    es_map_node* data;
    size_t size;
    size_t capacity;
    size_t tombstones;
} es_map;


void es_construct_map(es_map *map);
void es_destroy_map(es_map *map);

es_value *es_mapget(es_map *map, es_value *key);
es_value *es_mapgetadd(es_map *map, es_value *key);
es_value *es_mapset(es_map *map, es_value *key, es_value *value);
void es_maperase(es_map *map, es_value *key);

#endif