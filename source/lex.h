/********************************************************************************
 * \file lex.h
 * \author Patrick Torgeson (torgersonpatricks@gmail.com)
 * \brief
 * \version 0.1
 * \date 2022-01-05
 *
 * @copyright Copyright (c) 2022
 *
 ********************************************************************************/


#ifndef ES_LEX_H
#define ES_LEX_H


#include "array.h"
#include "instruction.h"


typedef enum es_lexeme_type
{
    // operators

    LEX_PLUS        = OP_ADD,
    LEX_MINUS       = OP_SUB, // unary or binary
    LEX_STAR        = OP_MUL,
    LEX_SLASH       = OP_DIV,
    LEX_PERCENT     = OP_MOD,
    LEX_AMPERSAND   = OP_BAND,
    LEX_PIPE        = OP_BOR,
    LEX_CARROT      = OP_BXOR,
    LEX_TILDE       = OP_BNOT, // unary
    LEX_AMPERSAND2  = OP_LAND,
    LEX_PIPE2       = OP_LOR,
    LEX_BANG        = OP_LNOT, // unary
    LEX_EQUAL2      = OP_EQ,
    LEX_BANG_EQUAL  = OP_NE,
    LEX_LESS        = OP_LT,
    LEX_LESS_EQUAL  = OP_LE,

    LEX_GREATER,
    LEX_GREATER_EQUAL,

    LEX_PLUS_EQUAL,
    LEX_MINUS_EQUAL,
    LEX_STAR_EQUAL,
    LEX_SLASH_EQUAL,
    LEX_PERCENT_EQUAL,
    LEX_AMPERSAND_EQUAL,
    LEX_PIPE_EQUAL,
    LEX_CARROT_EQUAL,

    // keywords

    LEX_AND,
    LEX_OR,
    LEX_IN,
    LEX_IS,

    LEX_FUNC,
    LEX_CONST,
    LEX_VAR,
    LEX_STRUCT,
    LEX_IF,
    LEX_ELSE,
    LEX_FOR,
    LEX_SWITCH,
    LEX_BREAK,
    LEX_CONTINUE,
    LEX_IMPORT,
    LEX_RETURN,
    LEX_BLOCK,

    // literals

    LEX_IDENTIFIER,
    LEX_STRING,
    LEX_INTEGER,
    LEX_FLOAT,
    LEX_BINARY,
    LEX_OCTAL,
    LEX_HEXADECIMAL,
    LEX_TRUE,
    LEX_FALSE,
    LEX_NIL,

    // structural

    LEX_OPEN_PAREN,
    LEX_CLOSE_PAREN,
    LEX_OPEN_SQUARE,
    LEX_CLOSE_SQUARE,
    LEX_OPEN_CURLY,
    LEX_CLOSE_CURLY,
    LEX_SEMICOLON,
    LEX_COMMA,
    LEX_EQUAL,

    // white space

    LEX_NEWLINE,
    LEX_EOF,

    // admin

    LEX_COUNT,
    LEX_INVALID
} es_lexeme_type;


typedef enum es_lexeme_catagory_t
{
    LEXC_OPERATOR  = LEX_PLUS,
    LEXC_KEYWORD   = LEX_AND,
    LEXC_LITERAL   = LEX_STRING,
    LEXC_STRUCTURE = LEX_OPEN_PAREN,
    LEXC_WHITE     = LEX_NEWLINE,
    LEXC_INVALID,
} es_lexeme_catagory;


typedef struct es_lexeme_t
{
    const char *ptr;
    size_t size;
    int type;
    int catagory;
    int line;
    int col;
    int indent;
} es_lexeme;


es_array(es_lexeme);


int es_lex(const char *source, size_t ssize, es_lexeme_arr *lexemes);
void es_print_lexeme(es_lexeme *lexeme);


#endif