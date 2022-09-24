/********************************************************************************
 * \file vm.h
 * \author Patrick Torgeson (torgersonpatricks@gmail.com)
 * \brief
 * \version 0.1
 * \date 2021-12-05
 *
 * \copyright Copyright (c) 2021
 *
 ********************************************************************************/

#ifndef ES_VM_H
#define ES_VM_H


#include <stdlib.h>

#include "common.h"
#include "value.h"
#include "instruction.h"
#include "array.h"
#include "map.h"


typedef struct es_function_t
{
    char *name;
    i32 params;
    i32 returns;
    es_instruction *ip;
    size_t size;
} es_function;


typedef struct es_callframe_t
{
    es_function *func;
    es_value *base;
    es_instruction *retaddr;
} es_callframe;


typedef struct es_code_t
{
    es_instruction *instructions;
    size_t size;
} es_code;


typedef char* cstr;

es_array(es_callframe);
es_array(es_code);
es_array(cstr);
es_array(size_t);
es_array(es_function);


typedef struct es_state_t
{
    es_value *stack;
    es_value *top;

    es_value *dispatch[2];

    es_value_arr kst;
    es_callframe_arr frames;
    es_code_arr codechunks;

    es_function_arr funcs;

    size_t ssize;
    uint8_t testresult;

} es_state;


void es_construct_state(es_state *es);
void es_destruct_state(es_state *es);

size_t es_addk_int(es_state *es, int64_t i);
size_t es_addk_float(es_state *es, long double f);
size_t es_addk_string(es_state *es, const char *str, size_t strsize);
// size_t es_addk_func(es_state *es, es_instruction *ip, const char *fname);

void es_execute_bytecode(es_state *es, es_instruction *program, size_t size);
int es_call(es_state *es, const char* function);

void es_print_values(es_value *vs, size_t size);
void es_print_stack(es_state *es);

int es_compile(es_state *es, const char *source, size_t ssize);


#endif
