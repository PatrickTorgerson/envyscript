/********************************************************************************
 * \file disassembly.h
 * \author Patrick Torgeson (torgersonpatricks@gmail.com)
 * \brief
 * \version 0.1
 * \date 2021-12-24
 *
 * @copyright Copyright (c) 2021
 *
 ********************************************************************************/


#ifndef ES_DISASSEMBLY_H
#define ES_DISASSEMBLY_H


#include "instruction.h"
#include "value.h"


int es_disassemble_ins(es_instruction i, char* buffer, size_t bsize);


#endif