#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "raylib.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 320
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

void draw(Chip8Emulator *emulator) {
    BeginDrawing();
    ClearBackground(BLACK);
    DrawText("Congrats! You created your first window!", 100, 100, 20, WHITE);
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
    // Initialize memory, registers, timers, and display
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Chip-8 Emulator");
}

void runEmulator(Chip8Emulator *emulator) {
    SetTargetFPS(60);
    uint32_t lastTickTime = GetTime();

    while (!WindowShouldClose()) {
        if (GetTime() - lastTickTime >= CHIP8_TICK_RATE) {
            if (emulator->delayTimer > 0) {
                emulator->delayTimer--;
            }
            if (emulator->soundTimer > 0) {
                // Handle sound timer
            }
            lastTickTime = GetTime();
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
