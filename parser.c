#include "parser.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>

uint16_t get_opcode(state_ *state){
    uint16_t high_byte = state->memory[state->pc];
    uint16_t low_byte = state->memory[state->pc+1];
    return ((high_byte << 8) | low_byte);
}

void inst_jump(state_ *state, uint16_t nnn){
    state->pc = nnn;
}

void inst_jump_v0(state_ *state, uint16_t nnn){
    state->pc = nnn + state->V[0x0];
}

void inst_call(state_ *state, uint16_t nnn){
    state->stack[state->sp] = state->pc; // TOOD: probably increase pc by 2?
    state->sp++;
    state->pc = nnn;
}

void inst_ret(state_ *state){
    state->sp--;
    state->pc = state->stack[state->sp];
}

void inst_skip_byte_eq(state_ *state, uint8_t x, uint8_t kk){
    if(state->V[x] == kk){
        state->pc += 2;
    }
}

void inst_skip_byte_neq(state_ *state, uint8_t x, uint8_t kk){
    if(state->V[x] != kk){
        state->pc += 2;
    }
}

void inst_skipt_eq(state_ *state, uint8_t x, uint8_t y){
    if(state->V[x] == state->V[y]){
        state->pc += 2;
    }
}

void inst_skip_neq(state_ *state, uint8_t x, uint8_t y){
    if(state->V[x] != state->V[y]){
        state->pc += 2;
    }
}

void inst_load(state_ *state, uint8_t x, uint8_t kk){
    state->V[x] = kk;
}

void inst_add(state_ *state, uint8_t x, uint8_t kk){
    state->V[x] += kk;
}

void inst_load_xy(state_ *state, uint8_t x, uint8_t y){
    state->V[x] = state->V[y];
}

void inst_or_x_y(state_ *state, uint8_t x, uint8_t y){
    state->V[x] |= state->V[y];
}

void inst_and_x_y(state_ *state, uint8_t x, uint8_t y){
    state->V[x] &= state->V[y];
}

void inst_xor_x_y(state_ *state, uint8_t x, uint8_t y){
    state->V[x] ^= state->V[y];
}

void inst_add_x_y(state_ *state, uint8_t x, uint8_t y){
    uint16_t result = state->V[x] + state->V[y];
    state->V[0xF] = result > 0xFF;
    state->V[x] = result & 0x00FF;
}

void inst_sub_x_y(state_ *state, uint8_t x, uint8_t y){
    state->V[0xF] = (state->V[x] < state->V[y]) ? 0 : 1;
    state->V[x] = state->V[x] - state->V[y]; 
}

void inst_subn_x_y(state_ *state, uint8_t x, uint8_t y){
    state->V[0xF] = (state->V[y] < state->V[x]) ? 0 : 1;
    state->V[x] = state->V[y] - state->V[x]; 
}

void inst_shr_x(state_ *state, uint8_t x){
    state->V[0xF] = state->V[x] & 0b1;
    state->V[x] >>= 1;
}

void inst_shl_x(state_ *state, uint8_t x){
    state->V[0xF] = (state->V[x] & 0x80) >> 7;
    state->V[x] <<= 1;
}

void inst_load_addr(state_ *state, uint16_t nnn){
    state->I = nnn;
}

void inst_gen_rand_and(state_ *state, uint8_t x, uint8_t kk){
    srand(time(NULL));
    uint8_t rand_num = (rand() % 255) + 1;
    state->V[x] = rand_num & kk;
}

void inst_draw(state_ *state, uint8_t x, uint8_t y, uint8_t n){
    uint8_t start_x = state->V[x] % 64;
    uint8_t start_y = state->V[y] % 32;
    state->V[0xF] = 0;

    for(int row = 0; row < n; row++){
        int curr_y = start_y + row;
        if(curr_y >= 32) curr_y %= 32; 

        uint8_t byte = state->memory[state->I+row];

        for(int col = 0; col < 8; col++){
            uint8_t pixel = (byte >> (7 - col)) & 1;
            if(pixel){
                int curr_x = start_x + col;
                if(curr_x >= 64) curr_x %= 64; 

                int index = curr_y*64 + curr_x;
                if(state->display[index] == 1){
                    state->V[0xF]=1;
                }
                state->display[index] ^= 1;
            }
        }
    }
}

void inst_skip_pressed(state_ *state, uint8_t x){
    if(state->keypad[state->V[x]]){
        state->pc += 2;
    }
}

void inst_skip_npressed(state_ *state, uint8_t x){
    if(!state->keypad[state->V[x]]){
        state->pc += 2;
    }
}

void inst_clear(state_ *state){
    memset(state->display, 0, sizeof(state->display));
}


void parse_opcode(uint16_t opcode, state_* state){
    uint16_t nnn = opcode & 0x0FFF;
    uint8_t n = opcode & 0x000F;
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;
    uint8_t kk = (opcode & 0x00FF);

    switch(opcode & 0xF000){
        case 0x0000: 
            switch (opcode & 0x00FF){
                case 0xE0: inst_clear(state); break;
                case 0xEE: inst_ret(state); break;
            }
            break;
        case 0x1000: inst_jump(state, nnn); break;
        case 0x2000: inst_call(state, nnn); break;
        case 0x3000: inst_skip_byte_eq(state, x, kk); break;
        case 0x4000: inst_skip_byte_neq(state, x, kk); break;
        case 0x5000: inst_skipt_eq(state, x, y); break;
        case 0x6000: inst_load(state, x, kk); break;
        case 0x7000: inst_add(state, x, kk); break;
        case 0x8000: 
            switch(n){
                case 0x0: inst_load_xy(state, x, y); break;
                case 0x1: inst_or_x_y(state, x, y); break;
                case 0x2: inst_and_x_y(state, x, y); break;
                case 0x3: inst_xor_x_y(state, x, y); break;
                case 0x4: inst_add_x_y(state, x, y); break;
                case 0x5: inst_sub_x_y(state, x, y); break;
                case 0x6: inst_shr_x(state, x); break;
                case 0x7: inst_subn_x_y(state, x, y); break;
                case 0xE: inst_shl_x(state, x); break;
            }
            break;
        case 0x9000: inst_skip_neq(state, x, y); break;
        case 0xA000: inst_load_addr(state, nnn); break;
        case 0xB000: inst_jump_v0(state, nnn); break;
        case 0xC000: inst_gen_rand_and(state, x, kk); break;
        case 0xD000: inst_draw(state, x, y, n); break;
        case 0xE000: 
            switch(opcode & 0x00FF){
                case 0x9E: inst_skip_pressed(state, x); break;
                case 0xA1: inst_skip_npressed(state, x); break;
            }
            break;
        default: break; 
    }
}