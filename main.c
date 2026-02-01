#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "state.h"
#include "parser.h"
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define CLEAR_SCREEN    "\033[2J"
#define CURSOR_HOME     "\033[H"
#define CURSOR_HIDE     "\033[?25l"
#define CURSOR_SHOW     "\033[?25h"
#define RESET           "\033[0m"
#define FG_GREEN        "\033[32m"

#define KEY_HOLD_DURATION   60      // Frames to hold a key
#define CPU_DELAY        300    // ~666 Hz CPU speed
#define DISPLAY_DELAY    16000   // ~60 fps while waiting
#define TIMER_TICK_CYCLES   9       // Decrement timers every N cycles

void enable_raw_mode(void);
void disable_raw_mode(void);
int read_key(void);
void press_key(state_ *state, char key);
void update_keypad_hold(state_ *state);
void process_input(state_ *state);
int wait_for_key(state_ *state);
void draw_to_terminal(state_ *state);
void update_timers(state_ *state);
void cpu_cycle(state_ *state);

static struct termios termios_saved;
static char frame_buffer[16384];
static int key_hold_frames[16] = {0};
static int running = 1;

void enable_raw_mode(void) {
    // Save the terminal state from earlier.
    tcgetattr(STDIN_FILENO, &termios_saved);
    
    // Make a copy
    struct termios raw = termios_saved;

    // Canoninal and Echo 
    raw.c_lflag &= ~(ICANON | ECHO);

    // Set minimum characters and minimum time to 0
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    int flags = fcntl(STDIN_FILENO, F_GETFL);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    printf(CURSOR_HIDE);
    fflush(stdout);
}

void disable_raw_mode(void) {
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

void press_key(state_ *state, char key) {
    uint8_t mapped = map_key_to_chip8(key);
    if (mapped != 255) {
        key_hold_frames[mapped] = KEY_HOLD_DURATION;
        state->keypad[mapped] = true;
    }
}

void update_keypad_hold(state_ *state) {
    for (int i = 0; i < 16; i++) {
        state->keypad[i] = (key_hold_frames[i] > 0);
        if (key_hold_frames[i] > 0) {
            key_hold_frames[i]--;
        }
    }
}

void process_input(state_ *state) {
    int c;
    while ((c = read_key()) >= 0) {
        if (c == 27) {
            running = 0;
            return;
        }
        press_key(state, (char)c);
    }
}

int wait_for_key(state_ *state) {
    while (running) {
        int c = read_key();
        if (c >= 0) {
            if (c == 27) {
                running = 0;
                return -1;
            }
            uint8_t mapped = map_key_to_chip8((char)c);
            if (mapped != 255) return mapped;
        }
        draw_to_terminal(state);
        usleep(DISPLAY_DELAY);
    }
    return -1;
}

void draw_to_terminal(state_ *state) {
    char *buf = frame_buffer;
    
    buf += sprintf(buf, CURSOR_HOME);
    
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            buf += sprintf(buf, state->display[y * 64 + x] ? "██" : "  ");
        }
        buf += sprintf(buf, "\n");
    }

    for (int i = 0; i < 16; i++) {
        buf += sprintf(buf, state->keypad[i] ? FG_GREEN "%X" RESET : "-", i);
    }

    fputs(frame_buffer, stdout);
    fflush(stdout);
}

void update_timers(state_ *state) {
    static int counter = 0;
    if (++counter >= TIMER_TICK_CYCLES) {
        counter = 0;
        if (state->dt > 0) state->dt--;
        if (state->st > 0) state->st--;
    }
}

void cpu_cycle(state_ *state) {
    uint16_t opcode = get_opcode(state);
    state->pc += 2;

    if ((opcode & 0xF0FF) == 0xF00A) {
        uint8_t x = (opcode & 0x0F00) >> 8;
        int key = wait_for_key(state);
        if (key >= 0) state->V[x] = key;
    } else {
        parse_opcode(opcode, state);
    }
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <rom>\n", argv[0]);
        return 1;
    }

    // Initialize
    state_ state;
    init_chip(&state);
    srand(time(NULL));

    // Load ROM
    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("Error opening file");
        return 1;
    }
    fread(&state.memory[0x200], 1, MAX_ROM_SIZE, fp);
    fclose(fp);

    // Setup terminal
    enable_raw_mode();
    printf(CLEAR_SCREEN);

    // Main loop
    while (running) {
        process_input(&state);
        if (!running) break;

        cpu_cycle(&state);                  // Get Opcode, Execute, Update Coubnter by 2
        update_keypad_hold(&state);         // 
        update_timers(&state);
        draw_to_terminal(&state);

        usleep(CPU_DELAY);
    }

    // Cleanup
    disable_raw_mode();
    printf(CLEAR_SCREEN CURSOR_HOME "Emulator exited.\n");
    return 0;
}