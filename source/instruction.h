/********************************************************************************
 * \file instruction.h
 * \author Patrick Torgeson (torgersonpatricks@gmail.com)
 * \brief
 * \version 0.1
 * \date 2021-12-24
 *
 * @copyright Copyright (c) 2021
 *
 ********************************************************************************/


#ifndef ES_INSTRUCTION_H
#define ES_INSTRUCTION_H


#include <stdint.h>


typedef uint32_t es_instruction;


// -- instruction arg limits


#define OSIZE 6
#define ASIZE 8
#define BSIZE 9
#define CSIZE 9
#define XSIZE (ASIZE+BSIZE+CSIZE)
#define YSIZE (BSIZE+CSIZE)

#define XPOS 0
#define YPOS XPOS
#define CPOS XPOS
#define BPOS CSIZE
#define APOS (BPOS+BSIZE)
#define OPOS (APOS+ASIZE)

#define OSIGSIZE  2
#define INFSIZE   3

#define YINFPOS 0
#define XINFPOS (YINFPOS+INFSIZE)
#define CINFPOS (XINFPOS+INFSIZE)
#define BINFPOS (CINFPOS+INFSIZE)
#define AINFPOS (BINFPOS+INFSIZE)
#define OSIGPOS (AINFPOS+INFSIZE)


// -- helper macroos


// creates a mask with 'n' 1 bits at position 'p'
#define MASK1(n,p) ((~((~0ull)<<(n)))<<(p))
// creates a mask with 'n' 0 bits at position 'p'
#define MASK0(n,p) (~MASK1(n,p))

// reads 's' bits at pos 'p' from 'i'
#define GETARG(i,p,s) (((i)>>(p)) & MASK1(s,0))

// Sign extend first 'b' bits of 'x' into a 64 bit int
#define SIGNBIT(b) MASK1(1,(b-1))
#define SEXT(b,x) ((((int64_t)(x) & MASK1(b,0)) ^ SIGNBIT(b)) - SIGNBIT(b))

// read args from instruction
#define O(i) GETARG(i,OPOS,OSIZE)
#define A(i) GETARG(i,APOS,ASIZE)
#define B(i) GETARG(i,BPOS,BSIZE)
#define C(i) GETARG(i,CPOS,CSIZE)
#define X(i) GETARG(i,XPOS,XSIZE)
#define Y(i) GETARG(i,YPOS,YSIZE)

// read args and sign extend
#define AS(i) SEXT(ASIZE,A(i))
#define BS(i) SEXT(BSIZE,B(i))
#define CS(i) SEXT(CSIZE,C(i))
#define XS(i) SEXT(XSIZE,X(i))
#define YS(i) SEXT(YSIZE,Y(i))

// ARGT_RK is constant query
#define ISK(v) (v & 1)
#define AISK(i) (0)
#define BISK(i) ISK(B(i))
#define CISK(i) ISK(C(i))
#define XISK(i) ISK(X(i))
#define YISK(i) ISK(Y(i))

// ARGT_RK get value
#define ARK(i) -1
#define BRK(i) (B(i) >> 1)
#define CRK(i) (C(i) >> 1)
#define XRK(i) (X(i) >> 1)
#define YRK(i) (Y(i) >> 1)

// ARGT_RK disassembly prefix
#define ARKPRE(i) ("ARKPRE"#i)
#define BRKPRE(i) (BISK(i)?"k":"r")
#define CRKPRE(i) (CISK(i)?"k":"r")
#define XRKPRE(i) (XISK(i)?"k":"r")
#define YRKPRE(i) (YISK(i)?"k":"r")

// ARGT_OR get value
#define AOR(i)  (A(i)?(A(i)-1):0)
#define BOR(i)  (B(i)?(B(i)-1):0)
#define COR(i)  (C(i)?(C(i)-1):0)
#define XOR(i)  (X(i)?(X(i)-1):0)
#define YOR(i)  (Y(i)?(Y(i)-1):0)

// ARGT_OR disassembly prefix
#define AORPRE(i) (A(i)?"r":"")
#define BORPRE(i) (B(i)?"r":"")
#define CORPRE(i) (C(i)?"r":"")
#define XORPRE(i) (X(i)?"r":"")
#define YORPRE(i) (Y(i)?"r":"")

// Instruction synthisis
#define MAKEARG(x,p,s) (((x)&MASK1(s,0)) << (p))
#define MAKEO(x) MAKEARG(x,OPOS,OSIZE)
#define MAKEA(x) MAKEARG(x,APOS,ASIZE)
#define MAKEB(x) MAKEARG(x,BPOS,BSIZE)
#define MAKEC(x) MAKEARG(x,CPOS,CSIZE)
#define MAKEX(x) MAKEARG(x,XPOS,XSIZE)
#define MAKEY(x) MAKEARG(x,YPOS,YSIZE)
#define INS_OABC(o,a,b,c)  (MAKEO(o) | MAKEA(a) | MAKEB(b) | MAKEC(c))
#define INS_OAY(o,a,y)     (MAKEO(o) | MAKEA(a) | MAKEY(y))
#define INS_OX(o,x)        (MAKEO(o) | MAKEX(x))

// set args
#define SETA(i,a) ((i) = (((i) & MASK0(ASIZE,APOS)) | MAKEA(a)))
#define SETB(i,b) ((i) = (((i) & MASK0(BSIZE,BPOS)) | MAKEB(a)))
#define SETC(i,c) ((i) = (((i) & MASK0(CSIZE,CPOS)) | MAKEC(a)))
#define SETX(i,x) ((i) = (((i) & MASK0(XSIZE,XPOS)) | MAKEX(a)))
#define SETY(i,y) ((i) = (((i) & MASK0(YSIZE,YPOS)) | MAKEY(a)))

#define ASK(k) (((k) << 1) | 1)

// Instruction reflection
#define GETOPSIG(inf) GETARG(inf,OSIGPOS,OSIGSIZE)
#define GETATYPE(inf) GETARG(inf,AINFPOS,INFSIZE)
#define GETBTYPE(inf) GETARG(inf,BINFPOS,INFSIZE)
#define GETCTYPE(inf) GETARG(inf,CINFPOS,INFSIZE)
#define GETXTYPE(inf) GETARG(inf,XINFPOS,INFSIZE)
#define GETYTYPE(inf) GETARG(inf,YINFPOS,INFSIZE)
#define MAKEOSIG(x)  MAKEARG(x,OSIGPOS,OSIGSIZE)
#define MAKEATYPE(x) MAKEARG(x,AINFPOS,INFSIZE)
#define MAKEBTYPE(x) MAKEARG(x,BINFPOS,INFSIZE)
#define MAKECTYPE(x) MAKEARG(x,CINFPOS,INFSIZE)
#define MAKEXTYPE(x) MAKEARG(x,XINFPOS,INFSIZE)
#define MAKEYTYPE(x) MAKEARG(x,YINFPOS,INFSIZE)

#define MAKEOPINFO(s,a,b,c,x,y) \
(MAKEOSIG(s) | MAKEATYPE(a) | MAKEBTYPE(b) | MAKECTYPE(c) | MAKEXTYPE(x) | MAKEYTYPE(y))


// Instruction format
//   32 bits
//   Fields:
//   - O : 6 bits           64 // opcode
//   - A : 8 bits          256
//   - B : 9 bits          512
//   - C : 9 bits          512
//   - X : 26 bits  67,108,864
//   - Y : 18 bits     262,144


typedef enum es_opsigniture
{
    SIG_ABC,    // instruction with 3 args 8+9+9
    SIG_AY,     // instruction with 2 args 8+18
    SIG_X,      // instruction with 1 arg  26

    SIG_COUNT,
    SIG_INVALID,
} es_opsigniture;


// argument types, used for assembly / disassembly
typedef enum es_opargtype_t
{
    ARGT_U,       // unused
    ARGT_R,       // register, value on stack
    ARGT_OR,      // optional register, 0 or r-1
    ARGT_K,       // constant
    ARGT_RK,      // register or constant
    ARGT_I,       // immidiate integer
    ARGT_SI,      // immidiate signed integer

    ARGT_COUNT,
    ARGT_INVALID,
} es_opargtype;


// op codes
typedef enum es_opcode
{
    OP_ADD,   // add  R(a) RK(b) RK(c)   ; a = b + c
    OP_SUB,   // sub  R(a) RK(b) RK(c)   ; a = b - c
    OP_MUL,   // mul  R(a) RK(b) RK(c)   ; a = b * c
    OP_DIV,   // div  R(a) RK(b) RK(c)   ; a = b / c
    OP_MOD,
    OP_BAND,
    OP_BOR,
    OP_BXOR,
    OP_BNOT,
    OP_LAND,
    OP_LOR,
    OP_LNOT,
    OP_EQ,    // eq   R(a) RK(b) RK(c)  ; a = b == c
    OP_NE,    // ne   R(a) RK(b) RK(c)  ; a = b != c
    OP_LT,    // lt   R(a) RK(b) RK(c)  ; a = b <  c
    OP_LE,    // le   R(a) RK(b) RK(c)  ; a = b <= c

    OP_MOV,   // mov  R(a) RK(y)         ; a = y
    OP_MOVI,  // movi R(a) SI(y)         ; a = y
    OP_JMP,   // jmp  I(a) SI(y)         ; ip += y * (result == a || a >= 2)
    OP_CALL,  // call R(a) I(y)          ; y(a+1,a+2,...)
    OP_RET,   // ret  I(X)               ; returns x values

    OP_NEG,

    // OP_TEST,

    // OP_READS,
    // OP_WRITES,

    // OP_READA,
    // OP_WRITEA,

    // OP_READM,
    // OP_WRITEM,

    OP_COUNT,
    OP_INVALID,
} es_opcode;


typedef unsigned es_opinfo;


es_opinfo es_get_opinfo(es_opcode opcode);
const char* es_get_opname(es_opcode opcode);
es_opcode es_get_opcode(const char* opname);


#define OPSIG(op) (GETOPSIG(es_get_opinfo(op)))
#define ATYPE(op) (GETATYPE(es_get_opinfo(op)))
#define BTYPE(op) (GETBTYPE(es_get_opinfo(op)))
#define CTYPE(op) (GETCTYPE(es_get_opinfo(op)))
#define XTYPE(op) (GETXTYPE(es_get_opinfo(op)))
#define YTYPE(op) (GETYTYPE(es_get_opinfo(op)))

#define ABCINF(a,b,c)  MAKEOPINFO(SIG_ABC,  a,b,c,0,0)
#define AYINF(a,y)     MAKEOPINFO(SIG_AY,   a,0,0,0,y)
#define XINF(x)        MAKEOPINFO(SIG_X,    0,0,0,x,0)


#endif