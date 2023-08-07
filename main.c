#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "raylib.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 320
#define PIXEL_SIZE 10
#define CHIP8_MEMORY_SIZE 4096
#define CHIP8_START_ADDRESS 0x200
#define CHIP8_TICK_RATE 1000 / 60

typedef struct {
    uint8_t memory[CHIP8_MEMORY_SIZE];
    uint8_t registers[16];
    uint16_t indexRegister;
    uint16_t programCounter;
    uint8_t delayTimer;
    uint8_t soundTimer;
    uint16_t stack[16];
    uint8_t stackPointer;
    uint8_t keys[16];
    bool display[64 * 32];
} Chip8Emulator;

uint8_t font[] = {
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
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void clearDisplay(bool *display) {
    for (int i = 0; i < 64 * 32; i++) {
        display[i] = false;
    }
}

void draw(Chip8Emulator *emulator) {
    BeginDrawing();
    ClearBackground(BLACK);
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            if (emulator->display[y * 64 + x]) {
                DrawRectangle(x * PIXEL_SIZE, y * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE, WHITE);
            }
        }
    }
    EndDrawing();
}

void loadROM(Chip8Emulator *emulator, const char *filePath) {
    FILE *romFile = fopen(filePath, "rb");
    if (romFile == NULL) {
        printf("Error opening ROM file.\n");
        exit(EXIT_FAILURE);
    }
    fseek(romFile, 0, SEEK_END);
    long romSize = ftell(romFile);
    rewind(romFile);
    if (romSize > CHIP8_MEMORY_SIZE - CHIP8_START_ADDRESS) {
        printf("ROM file is too large to fit in memory.\n");
        exit(EXIT_FAILURE);
    }
    fread(emulator->memory + CHIP8_START_ADDRESS, romSize, 1, romFile);
    fclose(romFile);
}

void initializeEmulator(Chip8Emulator *emulator) {
    emulator->programCounter = CHIP8_START_ADDRESS;
    emulator->stackPointer = 0;
    for (int i = 0; i < 80; i++) {
        emulator->memory[i + 0x50] = font[i];
    }
    for (int i = 0; i < 16; i++) {
        emulator->registers[i] = 0;
    }
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Chip-8 Emulator");
}

void emulateCycle(Chip8Emulator *emulator) {
    uint16_t instr = emulator->memory[emulator->programCounter + 1] | emulator->memory[emulator->programCounter] << 8;
    emulator->programCounter += 2;
    
    if ((instr & 0xf000) >> 12 == 0x7) {
        emulator->registers[(instr & 0x0f00) >> 8] += (instr & 0x00ff);
        return;
    }
    if ((instr & 0xf000) >> 12 == 0xd) {
        int rx = (instr & 0x0f00) >> 8;
        int ry = (instr & 0x00f0) >> 4;
        int n = instr & 0x000f;
        int x = emulator->registers[rx] % 64;
        int y = emulator->registers[ry] % 32;
        emulator->registers[0xf] = 0;
        for (int i = 0; i < n; i++) {
            uint8_t data = emulator->memory[emulator->indexRegister + i];
            x = emulator->registers[rx] % 64;
            for (int j = 7; j >= 0; j--) {
                int v = (data & (1 << j)) >> j;
                if (v && emulator->display[y * 64 + x]) {
                    emulator->display[y * 64 + x] = false;
                    emulator->registers[15] = 1;
                } else if (v) {
                    emulator->display[y * 64 + x] = true;
                }
                x++;
            }
            y++;
        }
        return;
    }
    if ((instr & 0xf000) >> 12 == 1) {
        emulator->programCounter = instr & 0x0fff;
        return;
    }
    if ((instr & 0xf000) >> 12 == 6) {
        emulator->registers[(instr & 0x0f00) >> 8] = instr & 0x00ff;
        return;
    }
    if ((instr & 0xf000) >> 12 == 0xa) {
        emulator->indexRegister = instr & 0x0fff;
        return;
    }
    switch (instr) {
        case 0x00e0:
            clearDisplay(emulator->display);
            break;
        default:
            printf("Instruction 0x%x not implemented\n", instr);
            assert(0);
    }
}

void runEmulator(Chip8Emulator *emulator) {
    SetTargetFPS(60);
    double lastTickTime = GetTime();
    while (!WindowShouldClose()) {
        emulateCycle(emulator);
        if (emulator->delayTimer > 0) {
            emulator->delayTimer--;
        }
        if (emulator->soundTimer > 0) {
            // TODO: play sound 
        }
        draw(emulator);
    }
    CloseWindow();
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <rom_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    Chip8Emulator emulator;
    initializeEmulator(&emulator);
    loadROM(&emulator, argv[1]);
    runEmulator(&emulator);

    return EXIT_SUCCESS;
}
