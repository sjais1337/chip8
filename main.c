#include <stdio.h>
#include <stdlib.h>
#include "state.h"
#include "parser.h"

int main(int argc, char*argv[]){
    if(argc != 2){
        printf("Usage: %s <chip8_rom> \n", argv[0]);
        return 0;
    }

    state_ state;
    init_chip(&state);
    
    char* filename = argv[1];
    FILE *fp;
    fp = fopen(filename, "rb");

    if(fp == NULL){
        printf("Error opening file!");
        return 1;
    }

    size_t size = fread(&state.memory[0x200], 1, MAX_ROM_SIZE, fp);
    fclose(fp);

    uint16_t opcode = get_opcode(&state);

    printf("First opcode: %04X\n", opcode);

    // Taking as a gaurantee that binary will have even number of bytes
    return 0;
}