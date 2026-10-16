#include <kernix/gdt.h>
#include <kernix/string.h>

#include <stdint.h>

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// Soft x86 TSS (Intel Vol. 3); only ss0/esp0 are used here.
struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

static struct gdt_entry gdt[6];
static struct gdt_ptr gp;
static struct tss_entry tss;

static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (uint16_t)(base & 0xFFFF);
    gdt[num].base_middle = (uint8_t)((base >> 16) & 0xFF);
    gdt[num].base_high = (uint8_t)((base >> 24) & 0xFF);
    gdt[num].limit_low = (uint16_t)(limit & 0xFFFF);
    gdt[num].granularity = (uint8_t)((limit >> 16) & 0x0F);
    gdt[num].granularity |= (uint8_t)(gran & 0xF0);
    gdt[num].access = access;
}

static void gdt_flush(uint32_t gdt_ptr_addr) {
    __asm__ volatile(
        "lgdt (%0)\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "ljmp $0x08, $1f\n"
        "1:\n"
        :
        : "r"(gdt_ptr_addr)
        : "ax", "memory");
}

static void tss_flush(void) {
    __asm__ volatile("ltr %0" : : "r"((uint16_t)GDT_TSS) : "memory");
}

void gdt_set_kernel_stack(uint32_t esp0) {
    tss.esp0 = esp0;
    tss.ss0 = GDT_KERNEL_DATA;
}

void gdt_init(void) {
    gp.limit = (uint16_t)(sizeof(gdt) - 1);
    gp.base = (uint32_t)&gdt;

    memset(&tss, 0, sizeof(tss));
    tss.ss0 = GDT_KERNEL_DATA;
    tss.iomap_base = sizeof(tss);

    gdt_set_gate(0, 0, 0, 0, 0);                // null
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // kernel code
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // kernel data
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // user code
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // user data

    uint32_t tss_base = (uint32_t)(uintptr_t)&tss;
    uint32_t tss_limit = sizeof(tss) - 1u;
    gdt_set_gate(5, tss_base, tss_limit, 0x89, 0x00);

    gdt_flush((uint32_t)&gp);
    tss_flush();
}
