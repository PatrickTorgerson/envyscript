#include "disassembly.h"

#include <stdio.h>


//*************************************************************************
#define write(f,...)  {cw = sprintf_s(buffer,bsize,f,__VA_ARGS__);\
                      if(cw < 1) { return cw; }\
                      buffer += cw;\
                      bsize  -= cw;}\


//*************************************************************************
#define pad write("%.*s ", (3 - cw), "      ");
#define writearg(a) \
    switch(GET##a##TYPE(inf))\
    {\
    case ARGT_R:   write(" r%lli",  a##(i));                 pad break;\
    case ARGT_K:   write(" k%lli",  a##(i));                 pad break;\
    case ARGT_OR:  write(" %s%lli", a##ORPRE(i), a##OR(i));  pad break;\
    case ARGT_RK:  write(" %s%lli", a##RKPRE(i), a##RK(i));  pad break;\
    case ARGT_I:   write(" %lli",   a##(i));                 pad break;\
    case ARGT_SI:  write(" %lli",   a##S(i));                pad break;\
    }


//*************************************************************************
int es_disassemble_ins(es_instruction i, char* buffer, size_t bsize)
{
    int op = O(i);
    const char* name = es_get_opname(op);
    es_opinfo inf = es_get_opinfo(op);
    int cw = 0;
    size_t initial = bsize;

    write("%s",name);
    write("%.*s ", (6 - cw), "      ");

    writearg(A);
    writearg(B);
    writearg(C);
    writearg(X);
    writearg(Y);

    return (int)(initial - bsize);
}


