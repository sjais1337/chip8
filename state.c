#include "state.h"
#include <string.h>
#include <stdio.h>

void init_chip(state_ *state){
    state->sp = 0;
    state->st = 0;
    state->dt = 0;
    state->pc = 0x200;

    memset(state->V, 0, sizeof(state->V));
    memset(state->stack, 0, sizeof(state->stack));
    memset(state->keypad, 0, sizeof(state->keypad));
    memset(state->display, 0, sizeof(state->display));
    memset(state->memory, 0, sizeof(state->memory));

    uint8_t font_data[80] =  {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80, // F
    };

    for(int i = 0; i < 80; i++){
        state->memory[i] = font_data[i];
    }
}

uint8_t map_key_to_chip8(char key){
    uint8_t mapped;

    // 1 2 3 4   ->  1 2 3 C
    // Q W E R   ->  4 5 6 D
    // A S D F   ->  7 8 9 E
    // Z X C V   ->  A 0 B F

    switch (key){
        case '1': mapped = 0x1; break;
        case '2': mapped = 0x2; break;
        case '3': mapped = 0x3; break;
        case '4': mapped = 0xC; break;
        
        case 'Q': mapped = 0x4; break;
        case 'W': mapped = 0x5; break;
        case 'E': mapped = 0x6; break;
        case 'R': mapped = 0xD; break;

        case 'A': mapped = 0x7; break;
        case 'S': mapped = 0x8; break;
        case 'D': mapped = 0x9; break;
        case 'F': mapped = 0xE; break;

        case 'Z': mapped = 0xA; break;
        case 'X': mapped = 0x0; break;
        case 'C': mapped = 0xB; break;
        case 'V': mapped = 0xF; break;
        default: mapped = 255; break;
    }

    return mapped;
}

void print_keypad_state(bool* keypad){
    for(int i = 0; i < 16; i++ ) {
        if(keypad[i]) printf("%i", i);
    }
    printf("\n");
}

void state_update_keypad(state_ *state, char key, bool pressed){
    uint8_t mapped = map_key_to_chip8(key);

    if(mapped != 255) { 
        state->keypad[mapped] = pressed;
        // printf("%i Pressed!\n", mapped);
        print_keypad_state(state->keypad);
    }

}