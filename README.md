# MINI OS (Minimal Educational x86 Kernel)

MINI OS is a custom-built, bare-metal 32-bit operating system kernel developed for educational purposes and systems-level architecture demonstration. It dives deep into CPU modes, hardware interaction, and low-level software abstractions.

### Core Features

1. **Multiboot Bootloader**: Uses GRUB / QEMU to boot directly into a protected mode 32-bit environment.
2. **Memory Management Subsystems**:
    - **PMM (Physical Memory Manager)**: Bitmap-based page frame allocator parsing Multiboot memory maps.
    - **VMM (Virtual Memory / Paging)**: Sets up `CR3` and page directories to translate memory dynamically.
    - **Kernel Heap Allocator**: Custom `kmalloc()` and `kfree()` free-list implementation.
3. **Interrupt Handling (IDT & PIC)**: Traps CPU Exceptions safely and remaps the Programmable Interrupt Controller (8259) to capture hardware pulses.
4. **Hardware Drivers**:
    - **VGA Text Mode**: Direct memory mapped writing to `0xB8000` for colorized terminal output.
    - **PIT Timer**: Triggers `IRQ0` at precise 50Hz intervals to track system uptime.
    - **Keyboard**: Listens on PS/2 port `0x60` to interpret scan codes into ASCII events.
5. **Round-Robin Process Scheduler**: An assembly-driven preemptive context switcher using `timer.c` interrupts to weave execution between dynamically allocated threads.
6. **System Calls**: A dispatcher triggered via `int 0x80` simulating a bridge for later user-mode implementations.

### Tech Stack
- **C (gnu99)** — Core kernel logic and data structures.
- **NASM** — Assembly routines for context switching, GDT flushing, interrupts, and boot.
- **i686-elf cross-compiler** — To produce freestanding binaries without standard library dependencies.
- **Make** — Build automation.
- **QEMU** — Hardware emulation and testing.

## Build and Run Instructions

### Prerequisites (macOS/Linux)
You will need `i686-elf-gcc`, `nasm`, and `qemu`.

**macOS (via Homebrew)**
```bash
brew install i686-elf-gcc nasm qemu
```

### Execution
Running locally is fully supported by the included `Makefile`. Simply run:
```bash
# Compile everything and run in the QEMU emulator
make run
```

*Note: The script outputs `build/minios.bin` and feeds it directly into QEMU via the `-kernel` command skipping grub-mkrescue iso wrappers.*
