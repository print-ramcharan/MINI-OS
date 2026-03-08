#include "idt.h"
#include "vga.h"

idt_entry_t idt_entries[256];
idt_ptr_t idt_ptr;

isr_t interrupt_handlers[256];

extern void idt_flush(uint32_t);

// These ISRs will be defined in assembly
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

// IRQs (Hardware Interrupts) mapped securely past 31
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();
// System Call Int 0x80
extern void isr128();

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
  idt_entries[num].base_lo = base & 0xFFFF;
  idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
  idt_entries[num].sel = sel;
  idt_entries[num].always0 = 0;
  // We must uncomment the OR below when we get to using user-mode.
  // It sets the interrupt gate's privilege level to 3.
  idt_entries[num].flags = flags /* | 0x60 */;
}

// Remap PIC to avoid conflict with CPU exceptions
void pic_remap(int offset1, int offset2) {
// We will do inline assembly for port out since we don't have an io.h yet
#define outb(port, value)                                                      \
  asm volatile("outb %%al, %%dx" : : "d"(port), "a"(value))

  // ICW1: initialization
  outb(0x20, 0x11);
  outb(0xA0, 0x11);

  // ICW2: Vector offset
  outb(0x21, offset1);
  outb(0xA1, offset2);

  // ICW3: Cascade setup
  outb(0x21, 0x04);
  outb(0xA1, 0x02);

  // ICW4: Environment info
  outb(0x21, 0x01);
  outb(0xA1, 0x01);

  // Unmask all
  outb(0x21, 0x0);
  outb(0xA1, 0x0);
#undef outb
}

void idt_install() {
  idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
  idt_ptr.base = (uint32_t)&idt_entries;

  // Clear everything
  for (int i = 0; i < 256; i++) {
    idt_set_gate(i, 0, 0, 0);
    interrupt_handlers[i] = 0;
  }

  pic_remap(0x20, 0x28); // PIC1 mapped at 32, PIC2 mapped at 40

  // Set CPU exceptions (0-31)
  idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
  idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);
  idt_set_gate(2, (uint32_t)isr2, 0x08, 0x8E);
  idt_set_gate(3, (uint32_t)isr3, 0x08, 0x8E);
  idt_set_gate(4, (uint32_t)isr4, 0x08, 0x8E);
  idt_set_gate(5, (uint32_t)isr5, 0x08, 0x8E);
  idt_set_gate(6, (uint32_t)isr6, 0x08, 0x8E);
  idt_set_gate(7, (uint32_t)isr7, 0x08, 0x8E);
  idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E);
  idt_set_gate(9, (uint32_t)isr9, 0x08, 0x8E);
  idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
  idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
  idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
  idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
  idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
  idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
  idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
  idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
  idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
  idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
  idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
  idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
  idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
  idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
  idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
  idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
  idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
  idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
  idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
  idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
  idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
  idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);

  // Set IRQs (32-47)
  idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);
  idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);
  idt_set_gate(34, (uint32_t)irq2, 0x08, 0x8E);
  idt_set_gate(35, (uint32_t)irq3, 0x08, 0x8E);
  idt_set_gate(36, (uint32_t)irq4, 0x08, 0x8E);
  idt_set_gate(37, (uint32_t)irq5, 0x08, 0x8E);
  idt_set_gate(38, (uint32_t)irq6, 0x08, 0x8E);
  idt_set_gate(39, (uint32_t)irq7, 0x08, 0x8E);
  idt_set_gate(40, (uint32_t)irq8, 0x08, 0x8E);
  idt_set_gate(41, (uint32_t)irq9, 0x08, 0x8E);
  idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
  idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
  idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
  idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
  idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
  idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);

  // Set System Call Gate (128 = 0x80) -> we will enable usermode access later
  // via 0xEE flags
  idt_set_gate(128, (uint32_t)isr128, 0x08, 0x8E);

  idt_flush((uint32_t)&idt_ptr);

  // Let the CPU accept interrupts
  asm volatile("sti");
}

void register_interrupt_handler(uint8_t n, isr_t handler) {
  interrupt_handlers[n] = handler;
}

void isr_handler(registers_t regs) {
  if (interrupt_handlers[regs.int_no] != 0) {
    isr_t handler = interrupt_handlers[regs.int_no];
    handler(&regs);
  } else {
    terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
    print("Unhandled Interrupt: ");
    print_dec(regs.int_no);
    print("\n");
    for (;;)
      ; // System Halt
  }
}

void irq_handler(registers_t regs) {
  // Send an EOI (end of interrupt) signal to the PICs.
  // If this interrupt involved the slave.
  if (regs.int_no >= 40) {
    // Send reset signal to slave.
    asm volatile("outb %%al, %%dx" : : "d"(0xA0), "a"(0x20));
  }
  // Send reset signal to master.
  asm volatile("outb %%al, %%dx" : : "d"(0x20), "a"(0x20));

  if (interrupt_handlers[regs.int_no] != 0) {
    isr_t handler = interrupt_handlers[regs.int_no];
    handler(&regs);
  }
}
