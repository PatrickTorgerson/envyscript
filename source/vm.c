#include "vm.h"

#include "disassembly.h"
#include "string.h"

#include <stdio.h>
#include <string.h>


//*************************************************************************
void es_construct_state(es_state *es)
{
    es->ssize = 128u;
    es->stack = (es_value*) malloc(es->ssize * sizeof(es_value));
    es->top = es->stack;

    es_construct_array(es_value, es->kst);
    es_construct_array(es_callframe, es->frames);
    es_construct_array(es_code, es->codechunks);

    es_construct_array(es_function, es->funcs);
}


//*************************************************************************
void es_destruct_state(es_state *es)
{
    free(es->stack);

    es->stack   =  (es_value*)      NULL;
    es->top     =  (es_value*)      NULL;

    es_destroy_array(es_callframe, es->frames);
    es_destroy_array(es_value, es->kst);

    for(size_t i = 0; i < es->codechunks.size; ++i)
    {
        free(es->codechunks.data[i].instructions);
    }
    for(size_t i = 0; i < es->funcs.size; ++i)
    {
        free(es->funcs.data[i].name);
    }

    es_destroy_array(es_code, es->codechunks);

    es_destroy_array(es_function, es->funcs);
}


//*************************************************************************
size_t addk(es_state *es, es_value *k)
{
    // search
    for(size_t i = 0; i < es->kst.size; ++i)
    {
        // TODO: expand for objects
        if(es->kst.data[i].tid == k->tid && es->kst.data[i].u == k->u)
        { return i; }
    }

    // add
    es_arrpush(es_value, es->kst);
    es_arrback(es->kst).u   = k->u;
    es_arrback(es->kst).tid = k->tid;
    return es->kst.size - 1;
}


//*************************************************************************
size_t es_addk_int(es_state *es, int64_t i)
{
    es_value k;
    k.i = i;
    k.tid = ES_INT;
    return addk(es,&k);
}


//*************************************************************************
size_t es_addk_float(es_state *es, long double f)
{
    es_value k;
    k.f = f;
    k.tid = ES_FLOAT;
    return addk(es,&k);
}


//*************************************************************************
size_t es_addk_string(es_state *es, const char *str, size_t strsize)
{
    es_value k;
    k.obj = ES_ALLOCATE_OBJ(es_string);
    k.tid = ES_STRING;
    es_construct_string((es_string*) k.obj, str, strsize);
    return addk(es,&k);
}


// //*************************************************************************
// size_t es_addk_func(es_state *es, es_instruction *ip, const char *fname)
// {
//     es_value k;
//     k.p = (void*) ip;
//     k.tid = ES_FUNC;
//     size_t ki = addk(es,&k);

//     if(es->fsize + strnlen_s(fname, 64ull) > es->fcapacity)
//     {
//         printf(" == ERROR : funcbuf overflow ==\n\n");
//         exit(-1);
//     }

//     es_arrpushv(cstr, es->funcnames, es->funcbuff + es->fsize);
//     es_arrpushv(size_t, es->funcs, ki);

//     strcpy_s(es->funcbuff + es->fsize, es->fcapacity - es->fsize, fname);
//     es->fsize += strnlen_s(fname, 64ull) + 1;
//     es->funcbuff[es->fsize-1] = '\0';

//     return ki;
// }


//*************************************************************************
#define write(f,...)  {cw = sprintf_s(buffer,bsize,f,__VA_ARGS__);\
                      if(cw < 1) { return cw; }\
                      buffer += cw;\
                      bsize  -= cw;}\


//*************************************************************************
int es_valuetostring(es_value *v, char *buffer, size_t bsize)
{
    int cw = 0;
    size_t initial = bsize;

    switch(v->tid)
    {
        case ES_INT:     write("%lli", v->i);                 break;
        case ES_FLOAT:   write("%f", v->f);                   break;
        case ES_BOOL:    write("%s", (v->u)?"true":"false");  break;
        case ES_NIL:     write("%s", "nil");                  break;
        case ES_STRING:  write("%s", AS_STRING(v)->data);     break;

        default: write("%s", "ERR");
    }

    return (int)(initial - bsize);
}


//*************************************************************************
void es_printvalue(es_value *v)
{
    char buffer[200];
    es_valuetostring(v, buffer, 200);
    printf("%s", buffer);
}


//*************************************************************************
void es_print_values(es_value *vs, size_t size)
{
    printf("[");

    if(size == 0)   { printf("]"); return; }
    if(vs == NULL)  { printf("]"); return; }

    es_printvalue(vs);

    for(size_t i = 1; i < size; ++i)
    {
        printf(",");
        es_printvalue(vs + i);
    }

    printf("]");
}


//*************************************************************************
void es_print_stack(es_state *es)
{
    if(es_arrback(es->frames).base == es->top) { printf("[]"); return; }
    if(es_arrback(es->frames).base == NULL)    { printf("[]"); return; }

    es_print_values(es_arrback(es->frames).base, es->top - es_arrback(es->frames).base);
}


// [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[ Execution ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]


// helper macros
#define R(r)    (es_arrback(es->frames).base+(r))
#define K(k)    (es->kst.data+(k))
#define RA(i)   R(A(i))
#define RB(i)   R(B(i))
#define RC(i)   R(C(i))
#define RX(i)   R(X(i))
#define RY(i)   R(Y(i))
#define KA(i)   (K(A(i)))
#define KB(i)   (K(B(i)))
#define KC(i)   (K(C(i)))
#define KX(i)   (K(X(i)))
#define KY(i)   (K(Y(i)))
// #define RKB(i)  ((BISK(i)) ? (K(BRK(i))) : (R(BRK(i))))
// #define RKC(i)  ((CISK(i)) ? (K(CRK(i))) : (R(CRK(i))))
// #define RKX(i)  ((XISK(i)) ? (K(XRK(i))) : (R(XRK(i))))
// #define RKY(i)  ((YISK(i)) ? (K(YRK(i))) : (R(YRK(i))))
#define RKB(i)  (es->dispatch[BISK(i)] + BRK(i))
#define RKC(i)  (es->dispatch[CISK(i)] + CRK(i))
#define RKX(i)  (es->dispatch[XISK(i)] + XRK(i))
#define RKY(i)  (es->dispatch[YISK(i)] + YRK(i))
#define ORA(i)  (A(i) ? R(A(i)-1) : NULL)
#define ORB(i)  (A(i) ? R(A(i)-1) : NULL)
#define ORC(i)  (A(i) ? R(A(i)-1) : NULL)
#define ORX(i)  (A(i) ? R(A(i)-1) : NULL)
#define ORY(i)  (A(i) ? R(A(i)-1) : NULL)
#define BOOLALPHA(b) ((b)?"true":"false")


//*************************************************************************
void es_execute_bytecode(es_state *es, es_instruction *program, size_t size)
{
    es_instruction *ip = program;
    es_instruction *end = program + size;

    es_instruction i;

    es_value* eos = es->stack + es->ssize;

    es->dispatch[0] = es_arrback(es->frames).base;
    es->dispatch[1] = es->kst.data;

    char buffer[128];
    int tracecol = 35;

    while(ip != end)
    {

        i = *ip;
        ++ip;

        int written = es_disassemble_ins(i, buffer, tracecol);
        printf("%.*s", written, buffer);
        printf("%.*s ;  ", tracecol - written, "                              ");

        switch(O(i))
        {

        //------------------------------
        case OP_ADD:
        {
            es_value* a =  RA(i);

            if(a == es->top)
            { ++(es->top); }

            es_value* b = RKB(i);
            es_value* c = RKC(i);

            if(b->tid == ES_INT && c->tid == ES_INT)
            {
                printf("%lli + %lli = ", b->i, c->i);
                a->i = b->i + c->i;
                a->tid = b->tid;
                printf("%lli", a->i);
                break;
            }
            else if(b->tid == ES_FLOAT && c->tid == ES_FLOAT)
            {
                printf("%f + %f = ", b->f, c->f);
                a->f = b->f + c->f;
                a->tid = b->tid;
                printf("%f", a->f);
                break;
            }

            printf("runtime error add mistype");
            return;
        }

        //------------------------------
        case OP_SUB:
        {
            es_value* a =  RA(i);

            if(a == es->top)
            { ++(es->top); }

            es_value* b = RKB(i);
            es_value* c = RKC(i);

            if(b->tid == ES_INT && c->tid == ES_INT)
            {
                printf("%lli - %lli = ", b->i, c->i);
                a->i = b->i - c->i;
                a->tid = b->tid;
                printf("%lli", a->i);
                break;
            }
            else if(b->tid == ES_FLOAT && c->tid == ES_FLOAT)
            {
                printf("%f - %f = ", b->f, c->f);
                a->f = b->f - c->f;
                a->tid = b->tid;
                printf("%f", a->f);
                break;
            }

            printf("runtime error sub mistype");
            return;
        }

        //------------------------------
        case OP_MUL:
        {
            es_value* a =  RA(i);

            if(a == es->top)
            { ++(es->top); }

            es_value* b = RKB(i);
            es_value* c = RKC(i);

            if(b->tid == ES_INT && c->tid == ES_INT)
            {
                printf("%lli * %lli = ", b->i, c->i);
                a->i = b->i * c->i;
                a->tid = b->tid;
                printf("%lli", a->i);
                break;
            }
            else if(b->tid == ES_FLOAT && c->tid == ES_FLOAT)
            {
                printf("%f * %f = ", b->f, c->f);
                a->f = b->f * c->f;
                a->tid = b->tid;
                printf("%f", a->f);
                break;
            }

            printf("runtime error mul mistype");
            return;
        }

        //------------------------------
        case OP_DIV:
        {
            es_value* a =  RA(i);

            if(a == es->top)
            { ++(es->top); }

            es_value* b = RKB(i);
            es_value* c = RKC(i);

            if(b->tid == ES_INT && c->tid == ES_INT)
            {
                printf("%lli / %lli = ", b->i, c->i);
                a->i = b->i / c->i;
                a->tid = b->tid;
                printf("%lli", a->i);
                break;
            }
            else if(b->tid == ES_FLOAT && c->tid == ES_FLOAT)
            {
                printf("%f / %f = ", b->f, c->f);
                a->f = b->f / c->f;
                a->tid = b->tid;
                printf("%f", a->f);
                break;
            }

            printf("runtime error div mistype");
            return;
        }

        //------------------------------
        case OP_EQ:
        {
            es_value* a = RA(i);

            if(a == es->top)
            { ++(es->top); }

            es_value* b = RKB(i);
            es_value* c = RKC(i);

            if(b->tid == ES_INT && c->tid == ES_INT)
            {
                printf("%lli == %lli : ", b->i, c->i);
                a->i = b->i == c->i;
                a->tid = ES_BOOL;
            }
            else if(b->tid == ES_FLOAT && c->tid == ES_FLOAT)
            {
                printf("%f == %f : ", b->f, c->f);
                a->i = b->f == c->f;
                a->tid = ES_BOOL;
            }

            printf(BOOLALPHA(a->i));

            break;
        }

        //------------------------------
        case OP_LT:
        {
            es_value* a = RA(i);

            if(a == es->top)
            { ++(es->top); }

            es_value* b = RKB(i);
            es_value* c = RKC(i);

            if(b->tid == ES_INT && c->tid == ES_INT)
            {
                printf("%lli < %lli : ", b->i, c->i);
                a->i = b->i < c->i;
                a->tid = ES_BOOL;
            }
            else if(b->tid == ES_FLOAT && c->tid == ES_FLOAT)
            {
                printf("%f < %f : ", b->f, c->f);
                a->i = b->f < c->f;
                a->tid = ES_BOOL;
            }

            printf(BOOLALPHA(a->i));

            break;
        }

        //------------------------------
        case OP_LE:
        {
            es_value* a = RA(i);

            if(a == es->top)
            { ++(es->top); }

            es_value* b = RKB(i);
            es_value* c = RKC(i);

            if(b->tid == ES_INT && c->tid == ES_INT)
            {
                printf("%lli <= %lli : ", b->i, c->i);
                a->i = b->i <= c->i;
                a->tid = ES_BOOL;
            }
            else if(b->tid == ES_FLOAT && c->tid == ES_FLOAT)
            {
                printf("%f <= %f : ", b->f, c->f);
                a->i = b->f <= c->f;
                a->tid = ES_BOOL;
            }

            printf(BOOLALPHA(a->i));

            break;
        }

        //------------------------------
        case OP_NE:
        {
            es_value* a = RA(i);

            if(a == es->top)
            { ++(es->top); }

            es_value* b = RKB(i);
            es_value* c = RKC(i);

            if(b->tid == ES_INT && c->tid == ES_INT)
            {
                printf("%lli != %lli : ", b->i, c->i);
                a->i = b->i != c->i;
                a->tid = ES_BOOL;
            }
            else if(b->tid == ES_FLOAT && c->tid == ES_FLOAT)
            {
                printf("%f != %f : ", b->f, c->f);
                a->i = b->f != c->f;
                a->tid = ES_BOOL;
            }

            printf(BOOLALPHA(es->testresult));

            break;
        }

        //------------------------------
        case OP_JMP:
        {
            uint64_t a = A(i);
            int64_t  y = YS(i);

            y *= (es->top-1)->i == a || a >= 2;
            ip += y;

            printf("ip += %lli", y);

            break;
        }

        //------------------------------
        case OP_MOV:
        {
            es_value* a = RA(i);

            if(a == es->top)
            { es->top->tid = ES_NIL; ++(es->top); }

            es_value* b = RKY(i);

            es_copy_value(a,b);

            es_print_stack(es);

            break;
        }

        //------------------------------
        case OP_MOVI:
        {
            es_value* a = RA(i);

            if(a == es->top)
            { ++(es->top); }

            uint64_t b = YS(i);

            a->i   = b;
            a->tid = ES_INT;

            es_print_stack(es);

            break;
        }

        //------------------------------
        case OP_CALL:
        {
            es_value* x = RA(i);
            u64 fn = Y(i);

            es_arrpush(es_callframe, es->frames);

            if(es->frames.size > es->frames.capacity)
            {
                printf("\nSTACK OVERFLOW! frames\n");
                return;
            }

            es_arrback(es->frames).func = es->funcs.data + fn;
            es_arrback(es->frames).base = x;
            es_arrback(es->frames).retaddr = ip;

            es->dispatch[0] = es_arrback(es->frames).base;

            ip = es_arrback(es->frames).func->ip;

            printf("func %s", es_arrback(es->frames).func->name);
            es_print_stack(es);

            break;
        }

        //------------------------------
        case OP_RET:
        {
           u64 x = X(i);

           if(x != es_arrback(es->frames).func->returns)
           {
               printf("\n  >  runtime error : return mismatch  <\n");
               return;
           }

            ip      = es_arrback(es->frames).retaddr;
            es->top = es_arrback(es->frames).base + x;

            es_print_stack(es);
            printf(" <- %s", es_arrback(es->frames).func->name);

            es_arrpop(es->frames);

            es->dispatch[0] = es_arrback(es->frames).base;

            // return from main
            if(ip == NULL)
            {
                es_arrclear(es->frames);
                return;
            }

            break;
        }

        //------------------------------
        default: return;

        }

        printf("\n");
    }
}


//*************************************************************************
int es_call(es_state *es, const char* function)
{
    es_function *f = NULL;

    for(size_t i = 0; i < es->funcs.size; ++i)
    {
        if(strcmp(es->funcs.data[i].name, function) == 0)
        {
            f = es->funcs.data + i;
            break;
        }
    }

    if(!f) return -1;

    // TODO: check params

    printf("func %s() : \n\n", function);

    es_arrclear(es->frames);
    es_arrpush(es_callframe, es->frames);
    es_arrback(es->frames).base = es->stack;
    es_arrback(es->frames).func = f;
    es_arrback(es->frames).retaddr = NULL;

    es_execute_bytecode(es, f->ip, f->size);

    return f->returns;
}