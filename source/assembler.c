#include "assembler.h"
#include "disassembly.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>


//*************************************************************************
int readint(const char *str, int64_t *i)
{
    char* end = NULL;

    int64_t result = strtoll(str, &end, 10);

    // must consume entire string
    if(*end != '\0') return 0;

    *i = result;
    return 1;
}


//*************************************************************************
int readfloat(const char *str, long double *f)
{
    char* end = NULL;

    long double result = strtold(str, &end);

    // must consume entire string
    if(*end != '\0') return 0;

    *f = result;
    return 1;
}


//*************************************************************************
void asmerr(es_assembler_state* state, const char *fmt, ...)
{
    printf("assembler LN%i : ", state->line);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    state->errcount++;
    state->diagcode = 1;
    va_end(args);
    printf("\n");
}


//*************************************************************************
void tokentolower(es_assembler_state* state)
{
    int i = 0;
    while(i != state->t)
    {
        state->token[i] = tolower(state->token[i]);
        ++i;
    }
}


//*************************************************************************
void writeins(es_assembler_state* state, es_instruction ins)
{
    if(state->psize + 1 > state->pcapacity)
    {
        asmerr(state, "out of memory");
    }

    *(state->i) = ins;
    state->i++;
    state->ipos++;
    state->psize++;

    char buffer[50];

    es_disassemble_ins(ins, buffer, 50);
    printf(" -- %04i : %s\n", state->line, buffer);
}


//*************************************************************************
char nextchar(es_assembler_state* state)
{
    // count lines
    if(*(state->c) == '\n')
    { state->line += 1; }

    // return char, then increment
    return *(state->c++);
}


//*************************************************************************
void skipwhitespace(es_assembler_state* state)
{
    while(isspace(*state->c)) nextchar(state);
}


//*************************************************************************
void readchar(es_assembler_state* state)
{
    if(state->t >= state->tsize)
    {
        asmerr(state, "token overflow");
        nextchar(state);
        return;
    }

    state->token[state->t] = nextchar(state);
    ++(state->t);
}


//*************************************************************************
static void readword(es_assembler_state* state)
{
    skipwhitespace(state);

    // reset token
    state->t = 0;

    // read until next whitespace char
    while(!isspace(*state->c) && *state->c != '\0') readchar(state);
    state->token[state->t] = '\0';

    // assembly is case insensitive
    tokentolower(state);
}


//*************************************************************************
size_t labelpos(es_assembler_state* state)
{
    for(size_t i = 0; i < state->lsize; ++i)
    {
        if(strcmp(state->token, state->labelptrs[i]) == 0)
        {
            return state->labelposs[i];
        }
    }

    return (size_t)(-1);
}


//*************************************************************************
int32_t readarg(es_assembler_state* state, es_opargtype argt)
{
    int64_t k;
    long double f;
    readword(state);

    switch(argt)
    {

    case ARGT_R:
        if(state->token[0] == 'r')
        if(readint(state->token + 1, &k))
        return (int32_t)(k);
        break;

    case ARGT_OR:
        if(state->token[0] == 'r')
        {
            if(readint(state->token + 1, &k))
            return (int32_t)(k+1);
        }
        else if(state->token[0] == '0' && state->t > 0)
            return 0;
        break;

    case ARGT_K:
        if(state->token[0] == 'k')
        {
            if(readint(state->token + 1, &k))
            return (int32_t)(k);
        }
        else if(readint(state->token, &k))
        {
            return (int32_t) es_addk_int(state->es, k);
        }
        else if(readfloat(state->token, &f))
        {
            return (int32_t) es_addk_float(state->es, f);
        }
        else
        {
            // read labels as kst : function
            size_t jmploc = labelpos(state);
            if(jmploc != (size_t)(-1))
            {
                return (int32_t) es_addk_func(state->es, state->program + jmploc, state->token);
            }
        }
        break;

    case ARGT_RK:
        if(state->token[0] == 'r')
        {
            if(readint(state->token + 1, &k))
            return (int32_t)(k << 1);
        }
        else if(state->token[0] == 'k')
        {
            if(readint(state->token + 1, &k))
            return (int32_t)((k << 1) | 1);
        }
        else if(readint(state->token, &k))
        {
            return (int32_t)((es_addk_int(state->es, k) << 1) | 1);
        }
        else if(readfloat(state->token, &f))
        {
            return (int32_t)((es_addk_float(state->es, f) << 1) | 1);
        }
        else
        {
            // read labels as kst : function
            size_t jmploc = labelpos(state);
            if(jmploc != (size_t)(-1))
            {
                return (int32_t)((es_addk_func(state->es, state->program + jmploc, state->token) << 1) | 1);
            }
        }
        break;

    case ARGT_I:
        if(readint(state->token, &k))
        if(k >= 0)
        return (int32_t)(k);
        break;

    case ARGT_SI:
        if(readint(state->token, &k))
            return (int32_t)(k);
        else
        {
            // read labels as si : jump offset
            size_t jmploc = labelpos(state);
            if(jmploc != (size_t)(-1))
            {
                return (int32_t) (jmploc - (state->ipos+1));
            }
        }
        break;
    }

    asmerr(state, "invalid argument '%s'", state->token);
    return 0;
}


//*************************************************************************
void label(es_assembler_state* state)
{
    if(!state->labelbuf || !state->labelptrs || !state->labelposs)
    {
        asmerr(state, "null label data");
        return;
    }

    if(state->lsize > 0 && labelpos(state) != (size_t)(-1))
    {
        asmerr(state, "duplicate label '%s'", state->token);
        return;
    }

    // check lcapacity
    if(state->lsize + 1 > state->lcapacity)
    {
        size_t newcap = state->lcapacity * 2;

        char**  labelptrs_new = (char**)  realloc(state->labelptrs, newcap * sizeof(char*));
        size_t* labelposs_new = (size_t*) realloc(state->labelposs, newcap * sizeof(size_t));

        if(!labelptrs_new || !labelposs_new)
        {
            if(state->labelbuf)  free(state->labelbuf);
            if(state->labelptrs) free(state->labelptrs);
            if(state->labelposs) free(state->labelposs);

            state->labelbuf = NULL;
            state->labelptrs = NULL;
            state->labelposs = NULL;

            asmerr(state, "out of memory");
            return;
        }

        printf("realloc label ptrs: %lli\n", newcap);

        state->lcapacity = newcap;
        state->labelptrs = labelptrs_new;
        state->labelposs = labelposs_new;
    }

    // check lbcapacity
    if(state->lbsize + strlen(state->token) + 1 > state->lbcapacity)
    {
        size_t newcap = (state->lbsize + strlen(state->token) + 1) * 2;

        char* labelbuf_new = (char*) realloc(state->labelbuf, newcap * sizeof(char));

        if(!labelbuf_new)
        {

            if(state->labelbuf)  free(state->labelbuf);
            if(state->labelptrs) free(state->labelptrs);
            if(state->labelposs) free(state->labelposs);

            state->labelbuf = NULL;
            state->labelptrs = NULL;
            state->labelposs = NULL;

            asmerr(state, "out of memory");
            return;
        }

        printf("realloc label buffer : %lli\n", newcap);

        state->lbcapacity = newcap;
        state->labelbuf   = labelbuf_new;
    }

    // add label
    state->labelptrs[state->lsize] = state->labelbuf + state->lbsize;
    state->labelposs[state->lsize] = state->ipos;

    state->lbsize += snprintf(state->labelbuf + state->lbsize, state->lbcapacity - state->lbsize, state->token);

    // null terminate
    state->labelbuf[state->lbsize++] = '\0';

    ++(state->lsize);
}


//*************************************************************************
es_opcode getopcode(es_assembler_state* state)
{
    es_opcode opcode = es_get_opcode(state->token);

    if(opcode == OP_INVALID)
    {
        asmerr(state, "unrecognized instruction '%s'", state->token);
        while(*state->c != '\n') nextchar(state);
    }

    return opcode;
}


//*************************************************************************
void validateins(es_assembler_state* state)
{
    es_opcode opcode = getopcode(state);
    if(state->diagcode) return;

    es_opsigniture opsig = OPSIG(opcode);

    switch(opsig)
    {
    case SIG_ABC:
        readword(state);
    case SIG_AY:
        readword(state);
    case SIG_X:
        readword(state);
    }

    state->ipos++;
}


//*************************************************************************
void instruction(es_assembler_state* state)
{
    es_opcode opcode = getopcode(state);
    if(state->diagcode) return;

    es_opsigniture opsig = OPSIG(opcode);

    switch(opsig)
    {

    case SIG_ABC:
    {
        int32_t a = readarg(state, ATYPE(opcode));
        int32_t b = readarg(state, BTYPE(opcode));
        int32_t c = readarg(state, CTYPE(opcode));
        if(state->diagcode) return;
        writeins(state, INS_OABC(opcode,a,b,c));
        return;
    }

    case SIG_AY:
    {
        int32_t a = readarg(state, ATYPE(opcode));
        int32_t y = readarg(state, YTYPE(opcode));
        if(state->diagcode) return;
        writeins(state, INS_OAY(opcode,a,y));
        return;
    }

    case SIG_X:
    {
        int32_t x = readarg(state, XTYPE(opcode));
        if(state->diagcode) return;
        writeins(state, INS_OX(opcode,x));
    }
    }
}


//*************************************************************************
int es_assemble(es_state* es, const char* source, size_t size)
{
    // -- sanity checks

    if(!es || !source || size == 0)
    {
        return 1;
    }

    // -- state setup

    es_assembler_state state;

    state.es = es;

    state.source = source;
    state.c      = source;
    const char* end = source + size;

    state.line = 1;

    state.token[0] = '\0';
    state.t = 0;

    state.lcapacity   = 8u;
    state.lsize       = 0u;
    state.lbcapacity  = 64u;
    state.lbsize      = 0u;

    state.labelbuf   =  (char*)    malloc(state.lbcapacity * sizeof(char));
    state.labelptrs  =  (char**)   malloc(state.lcapacity * sizeof(char*));
    state.labelposs  =  (size_t*)  malloc(state.lcapacity * sizeof(size_t));

    if(!state.labelbuf || !state.labelptrs || !state.labelposs)
    {
        if(state.labelbuf)  free(state.labelbuf);
        if(state.labelptrs) free(state.labelptrs);
        if(state.labelposs) free(state.labelposs);
        asmerr(&state, "out of memory");
        return 1;
    }

    state.diagcode = 0;
    state.errcount = 0;
    state.diagmsg[0] = '\0';

    state.ssize = size;
    state.tsize = ES_ASSEMBLER_TOKEN_SIZE;

    state.psize = 0u;
    state.ipos  = 0u;

    // -- read labels, count instructions

    for(; state.c < end; nextchar(&state))
    {
        if(isspace(*state.c)) continue;
        if(*state.c == ';')
        {
            while(*state.c != '\n') nextchar(&state);
            continue;
        }
        if(isalpha(*state.c))
        {
            readword(&state);

            if(state.t > 0 && state.token[state.t-1] == ':')
            {
                state.token[state.t-1] = '\0';
                label(&state);
            }
            else validateins(&state);
            continue;
        }
        asmerr(&state, "parsing error");
    }

    if(state.errcount > 0)
    {
        free(state.labelbuf);
        free(state.labelptrs);
        free(state.labelposs);
        printf("Assembly failed with %i errors\n", state.errcount);
        return state.errcount;
    }

    state.pcapacity = state.ipos;
    state.program = (es_instruction*) malloc(state.pcapacity * sizeof(es_instruction));

    if(!state.program)
    {
        asmerr(&state, "out of memory");
        free(state.labelbuf);
        free(state.labelptrs);
        free(state.labelposs);
        return state.errcount;
    }

    state.i = state.program;
    state.ipos = 0;

    state.c = state.source;
    state.t = 0;
    state.line = 1;

    // -- assembly loop

    for(; state.c < end; nextchar(&state))
    {
        if(isspace(*state.c)) continue;

        if(*state.c == ';')
        {
            while(*state.c != '\n') nextchar(&state);
            continue;
        }

        if(isalpha(*state.c))
        {
            state.diagcode = 0;

            readword(&state);

            if(state.t > 0 && state.token[state.t-1] != ':')
            {
                instruction(&state);
            }
            continue;
        }

        asmerr(&state, "parsing error");
    }

    // -- cleanup & return

    free(state.labelbuf);
    free(state.labelptrs);
    free(state.labelposs);

    if(state.errcount > 0)
    {
        printf("Assembly failed with %i errors\n", state.errcount);
        free(state.program);
        return state.errcount;
    }

    // shrink to fit
    state.program = realloc(state.program, state.psize * sizeof(es_instruction));

    es_arrpush(es_code, state.es->codechunks);
    es_arrback(state.es->codechunks).instructions = state.program;
    es_arrback(state.es->codechunks).size = state.psize;

    return 0;
}


//*************************************************************************
