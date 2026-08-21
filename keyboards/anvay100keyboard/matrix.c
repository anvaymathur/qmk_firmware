#include "quantum.h"
#include "spi_master.h"

// Pull in the pin arrays we defined in config.h
static const pin_t row_pins[] = MATRIX_ROW_PINS;
static const pin_t col_pins[] = DIRECT_COL_PINS;

// =========================================================
// 1. THE INITIALIZATION CONSTRUCTOR (Runs once on boot)
// =========================================================
void matrix_init_custom(void) {
    // Phase 1: Boot the hardware SPI clock
    spi_init();

    // Phase 2: Configure Direct RP2040 Pins
    // Unselect all rows by setting them as inputs with pull-ups
    for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
        setPinInputHigh(row_pins[i]);
    }
    // Set direct columns as inputs with pull-ups
    for (uint8_t i = 0; i < 8; i++) {
        setPinInputHigh(col_pins[i]);
    }

    // Phase 3: Configure the MCP23S17 IO Expander
    spi_start(MCP23S17_CS_PIN, false, 0, 8); 
    spi_write(MCP23S17_ADDR_WRITE); // Opcode: Write
    spi_write(0x00);                // Start at IODIRA (Direction Register)
    spi_write(0xFF);                // Port A as Inputs (Cols 8-15)
    spi_write(0xFF);                // Auto-increments to Port B, set as Inputs (Cols 16-20)
    spi_stop();                     // Hang up

    spi_start(MCP23S17_CS_PIN, false, 0, 8);
    spi_write(MCP23S17_ADDR_WRITE); // Opcode: Write
    spi_write(0x0C);                // Start at GPPUA (Pull-up Register)
    spi_write(0xFF);                // Enable Port A pull-ups
    spi_write(0xFF);                // Auto-increments to GPPUB, enable Port B pull-ups
    spi_stop();                     // Hang up
}

// =========================================================
// 2. THE CONTINUOUS SCANNER (Runs endlessly in the background)
// =========================================================
bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool matrix_has_changed = false;

    // Loop through each of the 6 rows one at a time
    for (uint8_t current_row = 0; current_row < MATRIX_ROWS; current_row++) {
        
        // 1. SELECT THE ROW
        // To test a row, we turn it into an output and drive the voltage LOW (0V).
        setPinOutput(row_pins[current_row]);
        writePinLow(row_pins[current_row]);

        // 2. WAIT FOR ELECTRICAL STABILITY
        // Give the voltage a tiny fraction of a millisecond to stabilize across the board.
        wait_us(30);

        // This variable will hold the pressed/unpressed state of all 21 columns for this row.
        // It starts at 0 (all keys unpressed).
        matrix_row_t row_data = 0; 

        // 3. READ DIRECT PICO COLUMNS (Columns 0 to 7)
        for (uint8_t col = 0; col < 8; col++) {
            // Because of our pull-up resistors, unpressed keys read HIGH (1).
            // If a key is pressed, it connects to our LOW row, so it reads LOW (0).
            if (readPin(col_pins[col]) == 0) {
                // If pressed, flip the corresponding bit in our row_data variable to 1
                row_data |= (1UL << col);
            }
        }

        // 4. READ SPI EXPANDER COLUMNS (Columns 8 to 20)
        spi_start(MCP23S17_CS_PIN, false, 0, 8);
        spi_write(MCP23S17_ADDR_READ); // Opcode: "I want to Read"
        spi_write(0x12);               // Start at GPIOA (Port A Live Pin States)
        
        // Send a dummy byte (0x00) just to keep the clock ticking while the expander talks back
        uint8_t port_a_state = spi_read(); 
        // Send a second dummy byte. The expander auto-increments and sends back Port B.
        uint8_t port_b_state = spi_read(); 
        spi_stop();

        // 5. DECODE SPI DATA INTO OUR ROW VARIABLE

        // ---------------------------------------------------------
        // Check Port B (Columns 8 to 14)
        // GPB0 = Col 8, GPB1 = Col 9, ..., GPB6 = Col 14
        // ---------------------------------------------------------
        for (uint8_t pin = 0; pin <= 6; pin++) { // Only loop pins 0 through 6
            // Check if the specific bit for this pin is 0 (pressed)
            if ((port_b_state & (1 << pin)) == 0) { 
                // If GPB0 is pressed (pin=0), shift a 1 into Box 8. (8 + 0 = 8)
                // If GPB6 is pressed (pin=6), shift a 1 into Box 14. (8 + 6 = 14)
                row_data |= (1UL << (8 + pin)); 
            }
        }

        // ---------------------------------------------------------
        // Check Port A (Columns 15 to 20)
        // GPA7 = Col 15, GPA6 = Col 16, ..., GPA2 = Col 20
        // Notice this is wired in reverse, so the math must flip it!
        // ---------------------------------------------------------
        for (uint8_t pin = 2; pin <= 7; pin++) { // Only loop pins 2 through 7
            if ((port_a_state & (1 << pin)) == 0) {
                // We use a math trick to flip the order:
                // If pin=7 (GPA7), target column is 15 + (7 - 7) = Col 15
                // If pin=2 (GPA2), target column is 15 + (7 - 2) = Col 20
                uint8_t target_column = 15 + (7 - pin);
                
                row_data |= (1UL << target_column); 
            }
        }

        // 6. UNSELECT THE ROW
        // Turn the row back into a High-impedance input so it doesn't interfere with the next row's test.
        setPinInputHigh(row_pins[current_row]);

        // 7. REPORT TO QMK
        // Did anything on this specific row change since the last time we checked it?
        if (current_matrix[current_row] != row_data) {
            current_matrix[current_row] = row_data;
            matrix_has_changed = true; // Tell QMK to process a keystroke
        }
    }

    // Return true if any key anywhere on the board was pressed or released
    return matrix_has_changed;
}