/********************************************************************************
 * \file assembler.h
 * \author Patrick Torgeson (torgersonpatricks@gmail.com)
 * \brief
 * \version 0.1
 * \date 2021-12-26
 *
 * @copyright Copyright (c) 2021
 *
 ********************************************************************************/


#ifndef ES_ASSEMBLER_H
#define ES_ASSEMBLER_H


#include "instruction.h"
#include "vm.h"


#define ES_ASSEMBLER_TOKEN_SIZE 20


typedef struct es_assembler_state_t
{
    es_state* es;

    const char* source;
    const char* c;

    char token[ES_ASSEMBLER_TOKEN_SIZE];
    size_t t;

    char* labelbuf;
    char** labelptrs;
    size_t* labelposs;

    es_instruction* program;
    es_instruction* i;
    size_t ipos;

    int diagcode;
    int errcount;
    char diagmsg[100];

    size_t ssize;
    size_t tsize;

    size_t lsize;
    size_t lcapacity;

    size_t lbsize;
    size_t lbcapacity;

    size_t psize;
    size_t pcapacity;

    int line;

} es_assembler_state;


int es_assemble(es_state* es, const char* source, size_t size);


#endif