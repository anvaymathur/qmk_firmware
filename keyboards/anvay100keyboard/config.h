#pragma once

/* 1. Matrix Dimensions */
#define MATRIX_ROWS 6
#define MATRIX_COLS 21
// #define DIODE_DIRECTION COL2ROW

/* 2. RP2040 Hardware SPI Routing (SPI0 Instance) */
#define SPI_DRIVER   SPID0
#define SPI_SCK_PIN  GP18
#define SPI_MOSI_PIN GP19
#define SPI_MISO_PIN GP16
#define MCP23S17_CS_PIN GP17

// "matrix_pins": {
//         "cols": ["C2", "C2", "C2", "C2", "C2", "C2", "C2", "C2", "C2", "C2", "C2", "C2", "C2", "C2", "C2", "C2", "C2", "C2", "C2", "C2", "C2"],
//         "rows": ["D1", "D1", "D1", "D1", "D1", "D1"]
//     },

/* 3. MCP23S17 Hardware Opcode (A0=GND, A1=GND, A2=GND) */
#define MCP23S17_ADDR_WRITE 0x40
#define MCP23S17_ADDR_READ  0x41

/* 4. Direct Microcontroller Pins */
#define MATRIX_ROW_PINS { GP5, GP4, GP3, GP2, GP1, GP0 }
#define DIRECT_COL_PINS { GP22, GP21, GP20, GP6, GP7, GP8, GP9, GP10 }

/* 5. Rotary Encoder Pins */
// #define ENCODERS_PAD_A { GP27 }
// #define ENCODERS_PAD_B { GP28 }
#define ENCODER_RESOLUTION 4