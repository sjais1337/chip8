#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "state.h"
#include "parser.h"
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

static struct termios termios_saved;

#define CLEAR_SCREEN    "\033[2J"
#define CURSOR_HOME     "\033[H"
#define CURSOR_HIDE     "\033[?25l"
#define CURSOR_SHOW     "\033[?25h"
#define BOLD            "\033[1m"
#define RESET           "\033[0m"
#define FG_GREEN        "\033[32m"
#define FG_CYAN         "\033[36m"
#define FG_YELLOW       "\033[33m"

void enable_raw_mode(){
    tcgetattr(STDIN_FILENO, &termios_saved);
    struct termios info = termios_saved;

    info.c_lflag &= ~(ICANON | ECHO);

    info.c_cc[VMIN] = 0;
    info.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &info);

    int flags = fcntl(STDIN_FILENO, F_GETFL);

    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    printf(CURSOR_HIDE);
    fflush(stdout);
}

void disable_raw_mode(){
    int flags = fcntl(STDIN_FILENO, F_GETFL);
    fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios_saved);

    printf(CURSOR_SHOW);
    fflush(stdout);
}

int read_key(){
    char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if(n==1) return (unsigned char)c;
    return -1;
}

static char frame_buffer[16384];
static int key_hold_frames[16] = {0};

#define KEY_HOLD_DURATION 120 

void update_keypad_hold(state_ *state) {
    for(int i = 0; i < 16; i++){
        if(key_hold_frames[i]>0){
            key_hold_frames[i]--;
            state->keypad[i]=true;
        }else{
            state->keypad[i]=false;
        }
    }
}

void press_key(state_ *state, char key){
    uint8_t mapped = map_key_to_chip8(key);
    if(mapped != 255){
        key_hold_frames[mapped] = KEY_HOLD_DURATION;
        state->keypad[mapped] = true;
    }
}

void draw_to_terminal(state_ *state) {
    char* buf = frame_buffer;
    
    buf += sprintf(buf, CURSOR_HOME);
    
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            if (state->display[y * 64 + x]) {
                buf += sprintf(buf, "██");  
            } else {
                buf += sprintf(buf, "  ");  
            }
        }
        buf += sprintf(buf,"\n" RESET);
    }

    for (int i = 0; i < 16; i++) {
        if (state->keypad[i]) {
            buf += sprintf(buf, FG_GREEN "%X" RESET, i);
        } else {
            buf += sprintf(buf, "-");
        }
    }

    fputs(frame_buffer, stdout);
    fflush(stdout);
}

int main(int argc, char*argv[]){
    if(argc != 2){
        printf("Usage: %s <chip8_rom> \n", argv[0]);
        return 0;
    }

    state_ state;
    init_chip(&state);
    srand(time(NULL));  
   
    char* filename = argv[1];
    FILE *fp;
    fp = fopen(filename, "rb");

    if(fp == NULL){
        printf("Error opening file!");
        return 1;
    }

    fread(&state.memory[0x200], 1, MAX_ROM_SIZE, fp);
    fclose(fp);

    enable_raw_mode();
    printf(CLEAR_SCREEN);

    int timer_counter = 0;
    const int TIMER_DECREMENT_CYCLES = 9; 

    // Main emulation loop
    int running = 1;
    while(running) {

        int c;
        while ((c = read_key()) >= 0) {
            if (c == 27) { // ESC key to quit
                running = 0;
                break;
            } else {
                press_key(&state, (char)c);
            }
        }
        
        if (!running) break;

        uint16_t opcode = get_opcode(&state);
        
        state.pc += 2;

        if ((opcode & 0xF0FF) == 0xF00A) {
            uint8_t x = (opcode & 0x0F00) >> 8;

            int key_pressed = -1;
            while (key_pressed == -1 && running) {
                int ch = read_key();
                if (ch >= 0) {
                    if (ch == 27) { // ESC
                        running = 0;
                        break;
                    }
                    uint8_t mapped = map_key_to_chip8((char)ch);
                    if (mapped != 255) {
                        key_pressed = mapped;
                    }
                }

                draw_to_terminal(&state);
                usleep(16000); 
            }
            if (running) {
                state.V[x] = key_pressed;
            }
        } else {
            parse_opcode(opcode, &state);
        }

        update_keypad_hold(&state);

        // Update timers at 60Hz
        timer_counter++;
        if (timer_counter >= TIMER_DECREMENT_CYCLES) {
            timer_counter = 0;
            if (state.dt > 0) state.dt--;
            if (state.st > 0) {
                state.st--;
            }
        }

        draw_to_terminal(&state);
        usleep(1500); 
    }

    disable_raw_mode();
    printf(CLEAR_SCREEN CURSOR_HOME); // Clear screen on exit
    printf("Emulator exited.\n");
    return 0;
}