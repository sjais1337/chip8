#ifndef PARSER_H
#define PARSER_H

#include "state.h"

uint16_t get_opcode(state_ *state);
void parse_opcode(uint16_t opcode, state_ *state);

#endif