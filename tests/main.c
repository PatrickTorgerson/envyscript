#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"
#include "assembler.h"
#include "disassembly.h"
#include "array.h"
#include "map.h"
#include "lex.h"


char* readfile(const char* filename)
{
    char* buffer = NULL;
    FILE* fp = fopen(filename, "r");
    if(!fp) return buffer;
    if(fseek(fp, 0l, SEEK_END) != 0) return buffer;
    long bufsize = ftell(fp);
    if (bufsize == -1) return buffer;
    buffer = malloc(sizeof(char) * (bufsize + 1));
    if(fseek(fp, 0l, SEEK_SET) != 0)
    {
        free(buffer);
        return NULL;
    }
    size_t newlen = fread(buffer, sizeof(char), bufsize, fp);
    if(ferror(fp) != 0)
    {
        free(buffer);
        return NULL;
    }
    buffer[newlen] = '\0';
    return buffer;
}


int main()
{
    es_state es;
    es_construct_state(&es);

    // printf("\n=========================================\n");
    // printf("=== Assembling source\n\n");
    // char* src = readfile("test.esasm");
    // if(!src) return 1;
    // int esasm = es_assemble(&es, src, strlen(src));
    // free(src);
    // if(esasm != 0) return 1;
    // printf("successfully assembled %llu instructions\n", es.codechunks.data[0].size);
    // printf("\n=========================================\n");
    // printf("=== Execution trace\n\n");
    // es_execute_bytecode(&es, es.codechunks.data[0].instructions, es.codechunks.data[0].size);
    // printf("\n\n=========================================\n");

    char* src = readfile("test.es");
    if(!src) return 1;


    printf("\n=========================================\n");
    printf("=== Compiling source\n\n");
    int errcount = es_compile(&es, src, strlen(src));
    free(src);
    if(errcount != 0) return 1;
    printf("successfully compiled %llu instructions\n", es.codechunks.data[0].size);
    printf("\n\n=========================================\n");
    printf("\n=========================================\n");
    printf("=== Execution trace\n\n");
    int rets = es_call(&es, "main");
    printf("\n=========================================\n");
    es_print_values(es.stack, rets);
    printf("\n\n=========================================\n");

    es_destruct_state(&es);

    return 0;
}