#include "parser.h"

uint16_t get_opcode(state_ *state){
    uint16_t high_byte = state->memory[state->pc];
    uint16_t low_byte = state->memory[state->pc+1];
    return ((high_byte << 8) | low_byte);
}

void parse_opcode(uint16_t opcode){
    uint16_t nnn = opcode & 0x0FFF;
    uint8_t n = opcode & 0x000F;
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;
    uint8_t kk = (opcode & 0x00FF);
}