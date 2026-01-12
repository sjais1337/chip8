#ifndef STATE_H
#define STATE_H

#include <stdint.h>

#define HEIGHT 32
#define WIDTH 64
#define MAX_ROM_SIZE 4096 - 512

typedef struct{
    uint8_t V[16];        // 16 registers
    uint16_t I;           // index register
    uint16_t pc;          // program counter
    uint8_t sp;           // stack pointer

    uint8_t memory[4096];
    uint16_t stack[16];

    uint8_t dt;           // delay timer
    uint8_t st;           // sound timer

    uint8_t display[HEIGHT * WIDTH];
    uint8_t keypad[16];
} state_;

void init_chip(state_ *state);

#endif 