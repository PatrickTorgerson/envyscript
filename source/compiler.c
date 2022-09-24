
#include "vm.h"
#include "lex.h"
#include "disassembly.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>


typedef struct { const char *c; size_t s; } str;

es_array(u32);
es_array(u64);
es_array(str);
es_array(es_instruction);


//*************************************************************************
typedef struct cstate_t
{
    es_state *es;

    es_instruction_arr program;
    u64_arr func_offsets;

    es_lexeme_arr lexemes;

    u32_arr indent_stack;

    es_lexeme *cl;  //  current lexeme
    es_lexeme *pl;  //  previous lexeme

    u32_arr operand_stack;
    u32 *top;

    u32 next_register;

    str_arr locals;

    int errcount;
    int panic;
} cstate;


//*************************************************************************
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,   // =
    PREC_OR,           // or ||
    PREC_AND,          // and &&
    PREC_BITWISE_OR,   // |
    PREC_BITWISE_XOR,  // ^
    PREC_BITWISE_AND,  // &
    PREC_EQUALITY,     // == !=
    PREC_COMPARISON,   // < > <= >=
    PREC_IN,           // in
    PREC_SHIFTS,       // << >>
    PREC_TERM,         // + -
    PREC_FACTOR,       // * / %
    PREC_UNARY,        // ! -
    PREC_CALL,         // . () []
    PREC_PRIMARY
} es_precedence;


static es_precedence precedence[] =
{
    [LEX_PLUS]             =  PREC_TERM,
    [LEX_MINUS]            =  PREC_TERM,
    [LEX_STAR]             =  PREC_FACTOR,
    [LEX_SLASH]            =  PREC_FACTOR,
    [LEX_PERCENT]          =  PREC_FACTOR,
    [LEX_AMPERSAND]        =  PREC_BITWISE_AND,
    [LEX_PIPE]             =  PREC_BITWISE_OR,
    [LEX_CARROT]           =  PREC_BITWISE_XOR,
    [LEX_TILDE]            =  PREC_UNARY,
    [LEX_AMPERSAND2]       =  PREC_AND,
    [LEX_PIPE2]            =  PREC_OR,
    [LEX_BANG]             =  PREC_UNARY,
    [LEX_EQUAL2]           =  PREC_EQUALITY,
    [LEX_BANG_EQUAL]       =  PREC_EQUALITY,
    [LEX_LESS]             =  PREC_COMPARISON,
    [LEX_LESS_EQUAL]       =  PREC_COMPARISON,
    [LEX_EQUAL]            =  PREC_ASSIGNMENT,
    [LEX_GREATER]          =  PREC_COMPARISON,
    [LEX_GREATER_EQUAL]    =  PREC_COMPARISON,
    [LEX_PLUS_EQUAL]       =  PREC_ASSIGNMENT,
    [LEX_MINUS_EQUAL]      =  PREC_ASSIGNMENT,
    [LEX_STAR_EQUAL]       =  PREC_ASSIGNMENT,
    [LEX_SLASH_EQUAL]      =  PREC_ASSIGNMENT,
    [LEX_PERCENT_EQUAL]    =  PREC_ASSIGNMENT,
    [LEX_AMPERSAND_EQUAL]  =  PREC_ASSIGNMENT,
    [LEX_PIPE_EQUAL]       =  PREC_ASSIGNMENT,
    [LEX_CARROT_EQUAL]     =  PREC_ASSIGNMENT,
    [LEX_AND]              =  PREC_AND,
    [LEX_OR]               =  PREC_OR,
    [LEX_IN]               =  PREC_IN,
    [LEX_IS]               =  PREC_IN,
};


//*************************************************************************
static void error(cstate* cs, const char *fmt, ...)
{
    if(cs->panic) return;
    printf("es error LN%i : ", cs->cl->line);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    cs->errcount += 1;
    cs->panic = 1;
}


//*************************************************************************
static void writeins(cstate* cs, u32 ins)
{
    es_arrpushv(es_instruction, cs->program, ins);

    char buffer[50];
    es_disassemble_ins(ins, buffer, 50);
    printf("%s\n", buffer);
}


//*************************************************************************
static void advance(cstate *cs)
{
    cs->pl = cs->cl;

    if(cs->cl->type == LEX_EOF)
    {
        error(cs, "trying to advance past EOF");
        return;
    }

    for(;;)
    {
        cs->cl += 1;
        if(cs->cl->type != LEX_INVALID) break;
        error(cs, "%.*s", cs->cl->size, cs->cl->ptr);
    }
}


//*************************************************************************
static int consume(cstate *cs, es_lexeme_type t)
{
    if(cs->cl->type == t)
    {
        advance(cs);
        return 1;
    }
    else
    {
        error(cs, "unexpected symbol '%.*s'", cs->cl->size, cs->cl->ptr);
        advance(cs);
        return 0;
    }
}


//*************************************************************************
static es_lexeme *peek(cstate *cs)
{
    return cs->cl + 1;
}


//*************************************************************************
static void literal(cstate *cs);
static void grouping(cstate *cs);
static void binary(cstate *cs);
static void unary(cstate *cs);
static void statement(cstate *cs);
static void varaccess(cstate *cs);
static void funccall(cstate *cs);


//*************************************************************************
static void expression(cstate *cs, es_precedence p)
{
    if(cs->cl->catagory == LEXC_LITERAL)
        literal(cs);
    else if(cs->cl->catagory == LEXC_OPERATOR)
        unary(cs);
    else if(cs->cl->type == LEX_OPEN_PAREN)
        grouping(cs);
    else if(cs->cl->type == LEX_IDENTIFIER)
    {
        if(peek(cs)->type == LEX_OPEN_PAREN)
            funccall(cs);
        else
            varaccess(cs);
    }
    else
    {
        error(cs, "expected expression");
        advance(cs);
        return;
    }

    while(cs->cl->catagory == LEXC_OPERATOR && precedence[cs->cl->type] >= p)
        binary(cs);
}


//*************************************************************************
static void literal(cstate *cs)
{
    str s = {cs->cl->ptr,cs->cl->size};

    if(cs->cl->type == LEX_MINUS) advance(cs);

    int type = cs->cl->type;

    size_t k;
    switch(type)
    {
        case LEX_INTEGER :
        {
            k = es_addk_int(cs->es, strtoll(s.c, NULL, 10));
            break;
        }
        case LEX_STRING :
        {
            k = es_addk_string(cs->es, s.c, s.s);
            break;
        }
        default: error(cs, "expected literal");
    }

    es_arrpushv(u32, cs->operand_stack, (u32)ASK(k));

    if(peek(cs)->catagory != LEXC_OPERATOR && cs->pl->catagory != LEXC_OPERATOR)
    {
        // TODO: use OP_MOVI if possible
        writeins(cs, INS_OAY(OP_MOV,cs->next_register,ASK(k)));
        cs->next_register += 1;
    }

    advance(cs);
}


//*************************************************************************
static void unary(cstate *cs)
{
    if(cs->cl->type == LEX_MINUS && cs->cl[1].catagory == LEXC_LITERAL)
    {
        literal(cs);
    }
    else
    {
        int op = cs->cl->type;

        es_precedence p = precedence[op];

        if(p != PREC_UNARY)
        {
            error(cs, "expected unary operator");
            advance(cs);
            return;
        }

        advance(cs);
        expression(cs, p+1);

        u32 operand = es_arrpop(cs->operand_stack);

        u32 dest;
        if(!ISK(operand) && operand >> 1 >= cs->locals.size - 1)
            dest = operand >> 1;
        else
            dest = cs->next_register;

        cs->next_register = dest + 1;

        es_arrpushv(u32, cs->operand_stack, dest << 1);

        writeins(cs, INS_OAY(op,dest,operand));
    }
}


//*************************************************************************
static void grouping(cstate *cs)
{
    consume(cs, LEX_OPEN_PAREN);
    expression(cs, PREC_ASSIGNMENT);
    consume(cs, LEX_CLOSE_PAREN);
}


//*************************************************************************
static void binary(cstate *cs)
{
    int op = cs->cl->type;

    es_precedence p = precedence[op];

    u32 lh = es_arrpop(cs->operand_stack);

    advance(cs);
    expression(cs, p+1);

    u32 rh = es_arrpop(cs->operand_stack);

    if(op == LEX_GREATER || op == LEX_GREATER_EQUAL)
    {
        u32 t = lh;
        lh = rh;
        rh = t;
        op -= 2;
    }

    u32 dest;
    if(!ISK(lh) && (lh >> 1) > cs->locals.size - 1)
        dest = lh >> 1;
    else if(!ISK(rh) && (rh >> 1) > cs->locals.size - 1)
        dest = rh >> 1;
    else
        dest = cs->next_register;

    cs->next_register = dest + 1;

    es_arrpushv(u32, cs->operand_stack, dest << 1);

    writeins(cs, INS_OABC(op,dest,lh,rh));
}


//*************************************************************************
static int local_lookup(cstate *cs, const char* c, size_t s)
{
    // returns 0 if no such local exists, l+1
    for(size_t l = 0; l < cs->locals.size; ++l)
    {
        if(es_arrat(cs->locals, l).s == s && strncmp(es_arrat(cs->locals, l).c, c, s) == 0)
        {
            return (int) l + 1;
        }
    }

    return 0;
}


//*************************************************************************
static void varaccess(cstate *cs)
{
    int l = local_lookup(cs, cs->cl->ptr, cs->cl->size);

    if(l)
    {
        es_arrpushv(u32, cs->operand_stack, (l-1) << 1);

        if(peek(cs)->catagory != LEXC_OPERATOR && cs->pl->catagory != LEXC_OPERATOR)
        {
            // TODO: use OP_MOVI if possible
            writeins(cs, INS_OAY(OP_MOV,cs->next_register, (l-1) << 1));
            cs->next_register += 1;
        }

        advance(cs);
    }
    else
    {
        error(cs, "variable '%.*s' not defined", cs->cl->size, cs->cl->ptr);
        es_arrpushv(u32, cs->operand_stack, 1); // 1 == k0
        advance(cs);
    }
}


//*************************************************************************
static void block(cstate *cs)
{
    if(cs->cl->type == LEX_SEMICOLON)
    {
        advance(cs);
        return;
    }

    if(cs->cl->type == LEX_NEWLINE)
    {
        while(cs->cl->type == LEX_NEWLINE) advance(cs);
        if(cs->cl->indent <= es_arrback(cs->indent_stack))
            error(cs, "block's indent must be greater than enclosing block");
        es_arrpushv(u32, cs->indent_stack, cs->cl->indent);
    }
    else
    {
        es_arrpushv(u32, cs->indent_stack, ~0); // single line block
    }

    size_t l = cs->locals.size;

    while(true)
    {
        if(cs->cl->type == LEX_NEWLINE)
        {
            if(es_arrback(cs->indent_stack) != ~0)
            {
                advance(cs);
                continue;
            }
            else break;
        }

        if(cs->cl->type == LEX_EOF) break;

        if(es_arrback(cs->indent_stack) != ~0)
            if(cs->cl->indent != es_arrback(cs->indent_stack))
                break;

        statement(cs);
    }

    if(cs->cl->indent > es_arrback(cs->indent_stack))
        error(cs, "unexpected indent");

    cs->locals.size = l;
    es_arrpop(cs->indent_stack);
}


//*************************************************************************
static void assignment(cstate *cs)
{
    es_lexeme *ids = cs->cl;

    while(cs->cl->type != LEX_EQUAL) advance(cs);

    consume(cs, LEX_EQUAL);

    int first = 1;
    int r1 = -1;

    for(;; ++ids)
    {
        int r = local_lookup(cs, ids->ptr, ids->size);

        if(!r) error(cs, "variable '%.*s' not defined", ids->size, ids->ptr);

        if(r1 >= 0) // single expression
        {
            writeins(cs, INS_OAY(OP_MOV, r - 1, (r1 - 1) << 1));
        }
        else
        {
            expression(cs, PREC_OR);
            SETA(es_arrback(cs->program), r - 1);
        }

        ids += 1;

        if((r1 >= 0 && ids->type == LEX_COMMA) || (first && ids->type == LEX_COMMA && cs->cl->type != LEX_COMMA))
        {
            first = 0;
            r1 = r;
            continue;
        }
        else if(r1 < 0 && cs->cl->type == LEX_COMMA && ids->type == LEX_COMMA)
        {
            first = 0;
            advance(cs);
            continue;
        }

        break;
    }


    if(ids->type == LEX_EQUAL && cs->cl->type == LEX_COMMA)
        error(cs, "assignment has too many expressions");
    if(ids->type == LEX_COMMA)
        error(cs, "assignment has too few expressions");
    if(ids->type != LEX_EQUAL)
        error(cs, "syntax error durring assignment");
}


//*************************************************************************
static int localvar(cstate *cs, bool gen)
{
    consume(cs, LEX_VAR);

    es_lexeme *ids = cs->cl;

    int newvars = 0;

    for(;;++ids)
    {
        if(cs->panic || local_lookup(cs, ids->ptr, ids->size))
        {
            error(cs, "variable '%.*s' redeclaration", ids->size, ids->ptr);
        }
        else
        {
            es_arrpush(str, cs->locals);
            es_arrback(cs->locals).c = ids->ptr;
            es_arrback(cs->locals).s = ids->size;
            newvars++;
        }

        ids += 1;

        if(ids->type == LEX_COMMA)
            continue;
        else break;
    }

    if(gen && ids->type == LEX_EQUAL)
        assignment(cs);
    else
    {
        while(cs->cl != ids) advance(cs);
        if(!gen) return newvars;
        for(;newvars > 0 ; --newvars)
            writeins(cs, INS_OAY(OP_MOVI, cs->locals.size - newvars, 0));
    }

    return newvars;
}


//*************************************************************************
static void funccall(cstate *cs)
{
    es_lexeme *fname = cs->cl;

    consume(cs, LEX_IDENTIFIER);
    consume(cs, LEX_OPEN_PAREN);

    u32 r = cs->next_register;

    // push params
    if(cs->cl->type != LEX_CLOSE_PAREN)
        while(true)
        {
            expression(cs, PREC_OR);
            if(cs->cl->type == LEX_COMMA)
            {
                advance(cs);
                continue;
            }
            else break;
        }

    consume(cs, LEX_CLOSE_PAREN);

    u64 f = -1;

    for(u64 i = 0; i < cs->es->funcs.size; ++i)
    {
        if(fname->size == strlen(cs->es->funcs.data[i].name) && strncmp(fname->ptr, cs->es->funcs.data[i].name, fname->size) == 0)
        {
            f = i;
            break;
        }
    }

    if(f == -1)
        error(cs, "function does not exist, '%.*s'", fname->size, fname->ptr);

    writeins(cs, INS_OAY(OP_CALL, r, f));
    es_arrpushv(u32, cs->operand_stack, r << 1);
    cs->next_register = r + 1;
}


//*************************************************************************
static void return_stmt(cstate *cs)
{
    consume(cs, LEX_RETURN);

    u32 r = 0;

    if(cs->cl->type == LEX_SEMICOLON || cs->cl->type == LEX_NEWLINE)
    {
        advance(cs);
    }
    else for(;;)
    {
        expression(cs, PREC_OR);
        SETA(es_arrback(cs->program), r);
        ++r;

        if(cs->cl->type == LEX_COMMA)
        {
            advance(cs);
            continue;
        }
        else break;
    }

    writeins(cs, INS_OX(OP_RET, r));
    es_arrback(cs->es->funcs).returns = r;
}


//*************************************************************************
static void if_stmt(cstate *cs)
{
    consume(cs, LEX_IF);

    expression(cs, PREC_OR);

    //if(!cs->boolean)
        //error(cs, "if condition must evaluate to bool");

    size_t jmp = cs->program.size;
    writeins(cs,0);

    cs->panic = 0;
    es_arrclear(cs->operand_stack);
    cs->next_register = (u32) cs->locals.size;

    block(cs);

    // else if 's

    cs->program.data[jmp] = INS_OAY(OP_JMP, false, cs->program.size - jmp - 1);
}


//*************************************************************************
static void for_stmt(cstate *cs)
{
    consume(cs, LEX_FOR);

    // beg_block(cs);

    // init

    expression(cs, PREC_OR);

    size_t jmp = cs->program.size;
    writeins(cs,0);

    // statements(cs);

    // inc

    // end_block(cs);
}


//*************************************************************************
static void statement(cstate *cs)
{
    cs->panic = 0;
    es_arrclear(cs->operand_stack);
    cs->next_register = (u32) cs->locals.size;

    switch(cs->cl->type)
    {
    case LEX_BLOCK:
        advance(cs);
        block(cs);
        return;

    case LEX_VAR:
        localvar(cs, true);
        break;

    case LEX_IDENTIFIER:
        // assignment or function call
        if(peek(cs)->type == LEX_OPEN_PAREN)
            funccall(cs);
        else
            assignment(cs);
        break;

    case LEX_RETURN:
        return_stmt(cs);
        break;

    case LEX_IF:
        if_stmt(cs);
        break;

    case LEX_FOR:
        for_stmt(cs);
        break;

    // empty statement
    case LEX_SEMICOLON: break;

    default:
        error(cs, "expected statement");
        advance(cs);
        return;
    }

    if(cs->cl->type == LEX_SEMICOLON) advance(cs);
}


//*************************************************************************
static void funcdecl(cstate *cs)
{
    consume(cs, LEX_FUNC);

    es_lexeme *fname = cs->cl;

    consume(cs, LEX_IDENTIFIER);

    es_arrclear(cs->locals);

    //int params = funcparamlist(cs);
    consume(cs, LEX_OPEN_PAREN);

    int params = 0;
    if(cs->cl->type != LEX_CLOSE_PAREN)
        do
            params += localvar(cs, false);
        while(cs->cl->type == LEX_COMMA);

    consume(cs, LEX_CLOSE_PAREN);

    //es_add_func(cs->es, fname->ptr, fname->size, 0, cs->program.data + cs->program.size);
    es_arrpushv(u64, cs->func_offsets, cs->program.size);

    es_arrpush(es_function, cs->es->funcs);
    es_arrback(cs->es->funcs).ip       = NULL;
    es_arrback(cs->es->funcs).name     = malloc(fname->size + 1);
    es_arrback(cs->es->funcs).params   = params;
    es_arrback(cs->es->funcs).returns  = 0;
    es_arrback(cs->es->funcs).size     = cs->program.size;

    memcpy(es_arrback(cs->es->funcs).name, fname->ptr, fname->size);
    es_arrback(cs->es->funcs).name[fname->size] = '\0';

    block(cs);

    writeins(cs, INS_OX(OP_RET, 0));

    es_arrback(cs->es->funcs).size = cs->program.size - es_arrback(cs->es->funcs).size;
}


//*************************************************************************
static void declaration(cstate *cs)
{
    cs->panic = 0;

    switch(cs->cl->type)
    {
    case LEX_FUNC:
        funcdecl(cs);
        return;

    case LEX_VAR:
        error(cs, "globals not yet supported :(");
        break;

    case LEX_CONST:
        error(cs, "consts not yet supported :(");
        break;

    case LEX_STRUCT:
        error(cs, "structs not yet supported :(");
        break;

    default:
        error(cs, "expected declaration");
        advance(cs);
        return;
    }

    advance(cs);
}


//*************************************************************************
int es_compile(es_state *es, const char *source, size_t ssize)
{
    cstate cs;

    es_construct_array(es_instruction, cs.program);
    es_construct_array(es_lexeme, cs.lexemes);
    es_construct_array(u32, cs.operand_stack);
    es_construct_array(u32, cs.indent_stack);
    es_construct_array(u64, cs.func_offsets);
    es_construct_array(str, cs.locals);

    size_t funcstart = es->funcs.size;

    cs.es = es;
    cs.next_register = 0;

    cs.errcount = 0;
    cs.panic = 0;

    es_lex(source, ssize, &cs.lexemes);

    es_arrpushv(u32, cs.indent_stack, cs.lexemes.data->indent);

    for(cs.cl = cs.lexemes.data; cs.cl->type != LEX_EOF;)
    {
        if(cs.cl->type == LEX_NEWLINE)
        {
            advance(&cs);
            continue;
        }

        declaration(&cs);
    }

    es_destroy_array(es_lexeme, cs.lexemes);
    es_destroy_array(u32, cs.operand_stack);
    es_destroy_array(u32, cs.indent_stack);
    es_destroy_array(str, cs.locals);

    if(cs.errcount > 0)
    {
        printf("Compilation failed with %i errors\n", cs.errcount);
        es_destroy_array(es_instruction, cs.program);
        es_destroy_array(u64, cs.func_offsets);
        return cs.errcount;
    }

    // shrink to fit
    cs.program.data = realloc(cs.program.data, cs.program.size * sizeof(es_instruction));

    for(size_t i = funcstart; i < es->funcs.size; ++i)
    {
        es->funcs.data[i].ip = cs.program.data + cs.func_offsets.data[i - funcstart];
    }

    es_destroy_array(u64, cs.func_offsets);

    es_arrpush(es_code, cs.es->codechunks);
    es_arrback(cs.es->codechunks).instructions = cs.program.data;
    es_arrback(cs.es->codechunks).size = cs.program.size;

    return cs.errcount;
}