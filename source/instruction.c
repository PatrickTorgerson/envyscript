#include "instruction.h"


#include <string.h>


//*************************************************************************
const char* es_opnames[] =
{
    "add",
    "sub",
    "mul",
    "div",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "eq",
    "ne",
    "lt",
    "le",
    "mov",
    "movi",
    "jmp",
    "call",
    "ret",
    "",
};


//*************************************************************************
es_opinfo es_opinfos[] =
{
    /* add   */ ABCINF(ARGT_R, ARGT_RK, ARGT_RK),
    /* sub   */ ABCINF(ARGT_R, ARGT_RK, ARGT_RK),
    /* mul   */ ABCINF(ARGT_R, ARGT_RK, ARGT_RK),
    /* div   */ ABCINF(ARGT_R, ARGT_RK, ARGT_RK),
    0,0,0,0,0,0,0,0,
    /* eq    */ ABCINF(ARGT_R, ARGT_RK, ARGT_RK),
    /* ne    */ ABCINF(ARGT_R, ARGT_RK, ARGT_RK),
    /* lt    */ ABCINF(ARGT_R, ARGT_RK, ARGT_RK),
    /* le    */ ABCINF(ARGT_R, ARGT_RK, ARGT_RK),
    /* mov   */ AYINF(ARGT_R, ARGT_RK),
    /* movi  */ AYINF(ARGT_R, ARGT_SI),
    /* jmp   */ AYINF(ARGT_I, ARGT_SI),
    /* call  */ AYINF(ARGT_R, ARGT_I),
    /* ret   */ XINF(ARGT_I),
    0,
};


//*************************************************************************
const char* es_get_opname(es_opcode opcode)
{
    return es_opnames[opcode];
}


//*************************************************************************
es_opinfo es_get_opinfo(es_opcode opcode)
{
    return es_opinfos[opcode];
}


//*************************************************************************
es_opcode es_get_opcode(const char* opname)
{
    for(size_t i = 0; i < OP_COUNT; ++i)
    {
        if(strcmp(opname, es_opnames[i]) == 0)
        {
            return (es_opcode) i;
        }
    }

    return OP_INVALID;
}