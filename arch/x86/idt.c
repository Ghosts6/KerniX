#include <kernix/idt.h>
#include <kernix/gdt.h>
#include <kernix/isr.h>

#include <stdint.h>
#include <stddef.h>

struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr idtp;

void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t flags) {
    idt[num].base_low = (uint16_t)(handler & 0xFFFF);
    idt[num].base_high = (uint16_t)((handler >> 16) & 0xFFFF);
    idt[num].selector = selector;
    idt[num].zero = 0;
    idt[num].flags = flags;
}

static void idt_load(uint32_t idt_ptr_addr) {
    __asm__ volatile("lidt (%0)" : : "r"(idt_ptr_addr) : "memory");
}

void idt_init(void) {
    for (size_t i = 0; i < 256; i++) {
        idt_set_gate((uint8_t)i, 0, 0, 0);
    }

    idtp.limit = (uint16_t)(sizeof(idt) - 1);
    idtp.base = (uint32_t)&idt;

    // Present, ring 0, 32-bit interrupt gate.
    const uint8_t gate = 0x8E;

    idt_set_gate(0, (uint32_t)isr0, GDT_KERNEL_CODE, gate);
    idt_set_gate(1, (uint32_t)isr1, GDT_KERNEL_CODE, gate);
    idt_set_gate(2, (uint32_t)isr2, GDT_KERNEL_CODE, gate);
    idt_set_gate(3, (uint32_t)isr3, GDT_KERNEL_CODE, gate);
    idt_set_gate(4, (uint32_t)isr4, GDT_KERNEL_CODE, gate);
    idt_set_gate(5, (uint32_t)isr5, GDT_KERNEL_CODE, gate);
    idt_set_gate(6, (uint32_t)isr6, GDT_KERNEL_CODE, gate);
    idt_set_gate(7, (uint32_t)isr7, GDT_KERNEL_CODE, gate);
    idt_set_gate(8, (uint32_t)isr8, GDT_KERNEL_CODE, gate);
    idt_set_gate(9, (uint32_t)isr9, GDT_KERNEL_CODE, gate);
    idt_set_gate(10, (uint32_t)isr10, GDT_KERNEL_CODE, gate);
    idt_set_gate(11, (uint32_t)isr11, GDT_KERNEL_CODE, gate);
    idt_set_gate(12, (uint32_t)isr12, GDT_KERNEL_CODE, gate);
    idt_set_gate(13, (uint32_t)isr13, GDT_KERNEL_CODE, gate);
    idt_set_gate(14, (uint32_t)isr14, GDT_KERNEL_CODE, gate);
    idt_set_gate(15, (uint32_t)isr15, GDT_KERNEL_CODE, gate);
    idt_set_gate(16, (uint32_t)isr16, GDT_KERNEL_CODE, gate);
    idt_set_gate(17, (uint32_t)isr17, GDT_KERNEL_CODE, gate);
    idt_set_gate(18, (uint32_t)isr18, GDT_KERNEL_CODE, gate);
    idt_set_gate(19, (uint32_t)isr19, GDT_KERNEL_CODE, gate);
    idt_set_gate(20, (uint32_t)isr20, GDT_KERNEL_CODE, gate);
    idt_set_gate(21, (uint32_t)isr21, GDT_KERNEL_CODE, gate);
    idt_set_gate(22, (uint32_t)isr22, GDT_KERNEL_CODE, gate);
    idt_set_gate(23, (uint32_t)isr23, GDT_KERNEL_CODE, gate);
    idt_set_gate(24, (uint32_t)isr24, GDT_KERNEL_CODE, gate);
    idt_set_gate(25, (uint32_t)isr25, GDT_KERNEL_CODE, gate);
    idt_set_gate(26, (uint32_t)isr26, GDT_KERNEL_CODE, gate);
    idt_set_gate(27, (uint32_t)isr27, GDT_KERNEL_CODE, gate);
    idt_set_gate(28, (uint32_t)isr28, GDT_KERNEL_CODE, gate);
    idt_set_gate(29, (uint32_t)isr29, GDT_KERNEL_CODE, gate);
    idt_set_gate(30, (uint32_t)isr30, GDT_KERNEL_CODE, gate);
    idt_set_gate(31, (uint32_t)isr31, GDT_KERNEL_CODE, gate);

    idt_set_gate(32, (uint32_t)irq0, GDT_KERNEL_CODE, gate);
    idt_set_gate(33, (uint32_t)irq1, GDT_KERNEL_CODE, gate);
    idt_set_gate(34, (uint32_t)irq2, GDT_KERNEL_CODE, gate);
    idt_set_gate(35, (uint32_t)irq3, GDT_KERNEL_CODE, gate);
    idt_set_gate(36, (uint32_t)irq4, GDT_KERNEL_CODE, gate);
    idt_set_gate(37, (uint32_t)irq5, GDT_KERNEL_CODE, gate);
    idt_set_gate(38, (uint32_t)irq6, GDT_KERNEL_CODE, gate);
    idt_set_gate(39, (uint32_t)irq7, GDT_KERNEL_CODE, gate);
    idt_set_gate(40, (uint32_t)irq8, GDT_KERNEL_CODE, gate);
    idt_set_gate(41, (uint32_t)irq9, GDT_KERNEL_CODE, gate);
    idt_set_gate(42, (uint32_t)irq10, GDT_KERNEL_CODE, gate);
    idt_set_gate(43, (uint32_t)irq11, GDT_KERNEL_CODE, gate);
    idt_set_gate(44, (uint32_t)irq12, GDT_KERNEL_CODE, gate);
    idt_set_gate(45, (uint32_t)irq13, GDT_KERNEL_CODE, gate);
    idt_set_gate(46, (uint32_t)irq14, GDT_KERNEL_CODE, gate);
    idt_set_gate(47, (uint32_t)irq15, GDT_KERNEL_CODE, gate);

    idt_load((uint32_t)&idtp);
}
