#include <stdio.h>
#include <stdlib.h>
#include "state.h"
#include "parser.h"
#include <termios.h>
#include <unistd.h>

void enable_raw_mode(){
    struct termios info;
    tcgetattr(STDIN_FILENO, &info);
    info.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &info);
}

void disable_raw_mode(){
    struct termios info;
    tcgetattr(STDIN_FILENO, &info);
    info.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &info);
}

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

    enable_raw_mode();

    uint16_t opcode = get_opcode(&state);


    // Keypad input only matters when the program asks for it.

    char key;
    while((key = getchar()) != 'L'){
        state_update_keypad(&state, key, true);
    }

    disable_raw_mode();
    return 0;
}