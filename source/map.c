#include "map.h"

#include <stdlib.h>
#include <stdio.h>


#define MAP_MAX_LOAD     0.75
#define MAP_TARGET_LOAD  0.5


//*************************************************************************
size_t hashvalue(es_value *v)
{
    // FNV-1a hash : http://www.isthe.com/chongo/tech/comp/fnv/
    const size_t FNV_prime    = 1099511628211ull;
    const size_t offset_basis = 14695981039346656037ull;

    size_t hash = offset_basis;

    uint8_t* end = ((uint8_t*)v) + sizeof(es_value);

    for(uint8_t* octet = (uint8_t*) v; octet != end; ++octet)
    {
        hash ^= *octet;
        hash *= FNV_prime;
    }

    if(v->tid == ES_STRING)
    {
        for(char *c = AS_STRING(v)->data; *c != '\0'; ++c)
        {
            hash ^= *c;
            hash *= FNV_prime;
        }
    }

    return hash;
}


//*************************************************************************
es_map_node *probe(es_map *map, es_value *key)
{
    size_t index = hashvalue(key) % map->capacity;
    size_t i = index;

    // first encountered tombstone
    es_map_node *tombstone = NULL;

    for(;;)
    {
        if(es_cmp_values(key, &(map->data[i].k)))
        {
            // found key
            return map->data + i;
        }
        else if(!tombstone && map->data[i].k.tid == ES_NIL && map->data[i].k.u == 1)
        {
            // first tombstone, save and continue
            tombstone = map->data + i;
        }
        else if(map->data[i].k.tid == ES_NIL && map->data[i].k.u == 0)
        {
            // empty node, return tombstone to recycle if possible
            return (tombstone)? tombstone : map->data + i;
        }

        i = (i+1) % map->capacity;
        if(i == index) break;
    }

    // map is 100% full

    if(tombstone) return tombstone;

    printf("\n  >  map overflow!  <\n\n");
    return NULL;
}


//*************************************************************************
int es_mapgrow(es_map *map, size_t newcap)
{
    const size_t max = (size_t)(~0ull);

    while(max % newcap != 0) ++newcap;

    if(newcap < map->capacity) return 0;

    // allocate new buffer
    es_map_node* ptr = malloc(newcap * sizeof(es_map_node));
    if(!ptr) return 0;

    // init nodes as empty
    for(size_t i = 0; i < newcap; ++i)
    {
        ptr[i].k.tid = ES_NIL;
        ptr[i].k.u   = 0;
    }

    // temp destination map
    es_map temp;
    temp.data = ptr;
    temp.capacity = newcap;
    temp.size = 0ull;
    temp.tombstones = 0ull;

    // copy over old buffer
    // we can't memcpy as the hashes change with capacity
    if(map->data)
    {
        for(size_t i = 0; i < map->capacity; ++i)
        {
            if(map->data[i].k.tid == ES_NIL) continue;

            es_map_node *node = probe(&temp, &(map->data[i].k));
            if(node && node->k.tid == ES_NIL)
            {
                node->k.tid = map->data[i].k.tid;
                node->k.u   = map->data[i].k.u;
                node->v.tid = map->data[i].v.tid;
                node->v.u   = map->data[i].v.u;
                ++(temp.size);
            }
            else
            {
                printf("\n  >  source map duplicate key?  <\n\n");
                exit(-1);
            }
        }
        free(map->data);
    }

    if(map->size != temp.size)
    {
        printf("\n  >  map realloc error, coudn't copy all nodes  <\n\n");
        exit(-1);
    }

    printf("growing map [%02.2f>%02.2f] : %04lli : %04lli\n",
        (double)(map->size) / (double)(map->capacity),
        (double)(temp.size) / (double)(temp.capacity),
        newcap,
        map->tombstones);

    map->data       = temp.data;
    map->capacity   = temp.capacity;
    map->size       = temp.size;
    map->tombstones = temp.tombstones;

    return 1;
}


//*************************************************************************
void es_destroy_map(es_map *map)
{
    if(map->data) free(map->data);
    map->data = NULL;
    map->capacity = 0ull;
    map->size = 0ull;
    map->tombstones = 0ull;
}


//*************************************************************************
void es_construct_map(es_map *map)
{
    map->data = NULL;
    es_destroy_map(map);
    es_mapgrow(map, 4ull);
}


//*************************************************************************
int reevalmem(es_map *map)
{
    double load_factor = (double)(map->size + map->tombstones) / (double)(map->capacity);

    if(load_factor >= MAP_MAX_LOAD)
    {
        return es_mapgrow(map, (size_t)(map->size / MAP_TARGET_LOAD));
    }
    return 0;
}


//*************************************************************************
es_value *es_mapget(es_map *map, es_value *key)
{
    if(map->size == 0) return NULL;
    es_map_node *node = probe(map, key);
    if(node->k.tid == ES_NIL)  return NULL;
    else                       return &(node->v);
}


//*************************************************************************
es_value *es_mapgetadd(es_map *map, es_value *key)
{
    reevalmem(map);
    es_map_node *node = probe(map, key);
    if(node->k.tid == ES_NIL)
    {
        if(node->k.u == 1) --(map->tombstones);
        es_copy_value(&node->k, key);
        node->v.tid = ES_NIL;
        node->v.u   = 0;
        ++(map->size);
        return &node->v;
    }
    else return &node->v;
    return NULL;
}


//*************************************************************************
es_value *es_mapset(es_map *map, es_value *key, es_value *value)
{
    es_value *v = es_mapgetadd(map, key);
    es_copy_value(v, value);
    return v;
}


//*************************************************************************
void es_maperase(es_map *map, es_value *key)
{
    if(map->size == 0) return;
    es_map_node *node = probe(map, key);
    if(node->k.tid == ES_NIL) return;
    es_destroy_value(&node->k);
    es_destroy_value(&node->v);
    node->k.tid = ES_NIL;
    node->k.u   = 1; // tombstone
    --(map->size);
    ++(map->tombstones);
}