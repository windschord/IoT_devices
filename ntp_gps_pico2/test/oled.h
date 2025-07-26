// oled.h mock for native testing
#include "arduino_mock.h"

class OLED {
public:
    // Constants
    enum Width { W_128 = 128 };
    enum Height { H_64 = 64 };
    enum Controller { CTRL_SH1106 };
    enum Color { WHITE = 1, BLACK = 0 };
    enum Style { SOLID, HOLLOW };
    
    // Constructor
    OLED(uint8_t sda, uint8_t scl, uint8_t reset, Width w, Height h, Controller ctrl, uint8_t addr) {}
    
    // Basic functions
    void begin() {}
    void clear() {}
    void display() {}
    void useOffset(bool) {}
    
    // Drawing functions
    void draw_string(int x, int y, const char* text) {}
    void draw_line(int x1, int y1, int x2, int y2, Color color) {}
    void draw_rectangle(int x1, int y1, int x2, int y2, Style style, Color color) {}
    void draw_pixel(int x, int y, Color color) {}
    
    // Mock specific functions for testing
    void setDisplayPower(bool on) {}  // Mock function for power control
};