#include <stdint.h>
#include <stddef.h>

void kernel_main() {
    uint16_t* vga = (uint16_t*) 0xB8000;
    
    // Clear screen
    for (int i = 0; i < 80 * 25; i++) {
        vga[i] = (uint16_t) ' ' | (uint16_t) 15 << 8;
    }

    // Print OK
    vga[0] = (uint16_t) 'O' | (uint16_t) 15 << 8;
    vga[1] = (uint16_t) 'K' | (uint16_t) 15 << 8;
    
    while(1) {}
}
