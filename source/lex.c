#include "lex.h"
#include "value.h"

#include <ctype.h>
#include <stdarg.h>


//*************************************************************************
typedef struct lexstate_t
{
    const char *source;
    const char *c;
    size_t ssize;
    size_t lsize;
    es_lexeme_arr *lexemes;

    int line;
    int col;
    int indent;
} lexstate;


//*************************************************************************
static void push_lexeme(lexstate* ls, int type);


//*************************************************************************
static int isoneof(char c, const char *str)
{
    for(; *str != '\0'; ++str)
        if(c == *str) return 1;
    return 0;
}


//*************************************************************************
static int isopchar(char c)
{
    return isoneof(c, "+-*/%&|^~!<>");
}


//*************************************************************************
static int isopen(char c)
{
    return isoneof(c, "([{");
}


//*************************************************************************
static int isclose(char c)
{
    return isoneof(c, ")]}");
}


//*************************************************************************
static int isillegal(char c)
{
    return isoneof(c, "`@#$\\?");
}


//*************************************************************************
static int isdelimiter(char c)
{
    if(isspace(c))     return 1;
    if(isopchar(c))    return 1;
    if(isopen(c))      return 1;
    if(isclose(c))     return 1;
    if(isillegal(c))   return 1;
    if(c == ';')       return 1;
    if(c == ',')       return 1;
    if(c == '\'')      return 1;
    if(c == '\"')      return 1;
    if(c == '=')      return 1;
    if(c == '\0')      return 1;

    return 0;
}


//*************************************************************************
static char nextc(lexstate *ls)
{
    if(*ls->c == '\n')
    {
        ls->lsize = 1;
        push_lexeme(ls, LEX_NEWLINE);

        ls->line += 1;
        ls->indent = 0;

        ls->c += 1;
        for(; *ls->c == ' '; ls->c += 1)
            ls->indent += 1;

        ls->col = ls->indent + 1;

        return *(ls->c);
    }

    ls->col += 1;

    return *(ls->c++);
}


//*************************************************************************
static void push_lexeme(lexstate* ls, int type)
{
    // determine catagory

    int catagory;
    if(type >= LEXC_WHITE)          catagory = LEXC_WHITE;
    else if(type >= LEXC_STRUCTURE) catagory = LEXC_STRUCTURE;
    else if(type >= LEXC_LITERAL)   catagory = LEXC_LITERAL;
    else if(type >= LEXC_KEYWORD)   catagory = LEXC_KEYWORD;
    else if(type >= LEXC_OPERATOR)  catagory = LEXC_OPERATOR;
    else                            catagory = LEXC_INVALID;

    // push lexeme

    es_arrpush(es_lexeme, (*ls->lexemes));

    es_arrback((*ls->lexemes)).ptr  = ls->c;
    es_arrback((*ls->lexemes)).size = ls->lsize;
    es_arrback((*ls->lexemes)).type = type;
    es_arrback((*ls->lexemes)).catagory = catagory;
    es_arrback((*ls->lexemes)).line = ls->line;
    es_arrback((*ls->lexemes)).col = ls->col;
    es_arrback((*ls->lexemes)).indent = ls->indent;

    if(type == LEX_EOF) return;

    // jump over lexeme
    while(--(ls->lsize) > 0) nextc(ls);
}


//*************************************************************************
static void push_char(lexstate* ls, int type)
{
    ls->lsize = 1;
    push_lexeme(ls, type);
}


//*************************************************************************
static char peekc(lexstate *ls)
{
    return *(ls->c + 1);
}


//*************************************************************************
static void skipwhite(lexstate* ls)
{
    while(isspace(*ls->c)) nextc(ls);
}


//*************************************************************************
static void readword(lexstate* ls)
{
    skipwhite(ls);
    ls->lsize = 0ull;
    const char* c = ls->c;
    while(!isdelimiter(*c))
    {
        ++(ls->lsize);
        ++c;
    }
}


//*************************************************************************
static void readintliteral(lexstate* ls)
{
    skipwhite(ls);
    ls->lsize = 0ull;
    const char* c = ls->c;
    while(isdigit(*c))
    {
        ++(ls->lsize);
        ++c;
    }
}


//*************************************************************************
static void readuntil(lexstate *ls, char d)
{
    skipwhite(ls);
    ls->lsize = 0ull;
    const char* c = ls->c;
    while(*c != d && *c != '\0')
    {
        ++(ls->lsize);
        ++c;
    }

    if(*c == '\0')
    {
        ls->lsize = 0;
        push_lexeme(ls, LEX_INVALID);
    }
}

//*************************************************************************
static int compare(lexstate* ls, const char *s)
{
    if(ls->lsize == strlen(s) && strncmp(ls->c, s, ls->lsize) == 0)
        return 1;
    else return 0;
}


//*************************************************************************
static int isstroneof(lexstate* ls, const char * a[], size_t asize)
{
    for(size_t i = 0; i < asize; ++i)
    {
        if(compare(ls, a[i]))
            return (int) i + 1;
    }
    return 0;
}


//*************************************************************************
int keyword(lexstate* ls)
{
    const char* keywords[] =
    {
        "and",
        "or",
        "in",
        "is",
        "func",
        "const",
        "var",
        "struct",
        "if",
        "else",
        "for",
        "switch",
        "break",
        "continue",
        "import",
        "return",
        "block",
        // and more!
    };

    const size_t asize = sizeof keywords / sizeof *keywords;
    int keyword = isstroneof(ls, keywords, asize);

    return keyword;
}


//*************************************************************************
int isoperator(lexstate* ls)
{
    const char* keywords[] =
    {
        "+",
        "-",
        "*",
        "/",
        "%",
        "&",
        "|",
        "^",
        "~",
        "&&",
        "||",
        "!",
        "==",
        "!=",
        "<",
        "<=",
        "=",
        ">",
        ">=",
        "+=",
        "-=",
        "*=",
        "/=",
        "%=",
        "&=",
        "|=",
        "^=",
    };

    const size_t asize = sizeof keywords / sizeof *keywords;
    return isstroneof(ls, keywords, asize);
}


//*************************************************************************
int readoperator(lexstate* ls)
{
    skipwhite(ls);
    ls->lsize = 0ull;
    const char* c = ls->c;
    while(isopchar(*c))
    {
        ++(ls->lsize);
        ++c;
    }

    // reduce until valid
    int optype = isoperator(ls);
    while(!optype)
    {
        --(ls->lsize);
        optype = isoperator(ls);
    }

    return LEXC_OPERATOR + (optype-1);
}


//*************************************************************************
int es_lex(const char *source, size_t ssize, es_lexeme_arr *lexemes)
{
    lexstate ls;

    ls.source = source;
    ls.c      = source;
    ls.ssize  = ssize;

    ls.lexemes = lexemes;

    ls.line = 1;
    ls.col  = 1;

    ls.indent = 0;

    while(*ls.c == ' ') ls.indent += 1;

    const char* end = source + ssize;

    for(; ls.c < end && *ls.c != '\0'; nextc(&ls))
    {
        // whitespace
        if(isspace(*ls.c)) continue;

        // comments
        else if(*ls.c == '/' && peekc(&ls) == '/')
        {
            while(*ls.c != '\n') nextc(&ls);
            continue;
        }

        else if(*ls.c == ';')
            push_char(&ls, LEX_SEMICOLON);
        else if(*ls.c == '(')
            push_char(&ls, LEX_OPEN_PAREN);
        else if(*ls.c == ')')
            push_char(&ls, LEX_CLOSE_PAREN);
        else if(*ls.c == '[')
            push_char(&ls, LEX_OPEN_SQUARE);
        else if(*ls.c == ']')
            push_char(&ls, LEX_CLOSE_SQUARE);
        else if(*ls.c == '{')
            push_char(&ls, LEX_OPEN_CURLY);
        else if(*ls.c == '}')
            push_char(&ls, LEX_CLOSE_CURLY);
        else if(*ls.c == ',')
            push_char(&ls, LEX_COMMA);

        // keyword , identifier , boolean , nill
        else if(isalpha(*ls.c))
        {
            readword(&ls);

            // boolean
            if(compare(&ls, "true"))
                push_lexeme(&ls, LEX_TRUE);
            else if(compare(&ls, "false"))
                push_lexeme(&ls, LEX_FALSE);

            // nil
            else if(compare(&ls, "nil"))
                push_lexeme(&ls, LEX_NIL);

            // keyword or identifier
            else
            {
                int type = keyword(&ls);
                if(type)
                    push_lexeme(&ls, LEXC_KEYWORD + (type-1));
                else
                    push_lexeme(&ls, LEX_IDENTIFIER);
            }
        }

        // assignment
        else if(*ls.c == '=' || (isopchar(*ls.c) && peekc(&ls) == '='))
        {
            // TODO: handle compound assignment
            push_char(&ls, LEX_EQUAL);
        }

        // operators
        else if(isopchar(*ls.c))
        {
            int type = readoperator(&ls);
            push_lexeme(&ls, type);
        }

        // numeric literal
        else if(isdigit(*ls.c))
        {
            readintliteral(&ls);
            push_lexeme(&ls, LEX_INTEGER);
        }

        // string literals
        else if(*ls.c == '\'' || *ls.c == '\"')
        {
            char open = *ls.c;
            ls.c += 1;
            readuntil(&ls, open);

            push_lexeme(&ls, LEX_STRING);

            // jump over closing quote
            ls.c += 1;
        }
    }

    ls.lsize = 0;
    push_lexeme(&ls, LEX_EOF);

    return -360;
}


//*************************************************************************
void es_print_lexeme(es_lexeme *lexeme)
{
    printf("%04i [%04i,%04i]: %.*s\n", lexeme->type, lexeme->line, lexeme->col, (int) lexeme->size, lexeme->ptr);
}