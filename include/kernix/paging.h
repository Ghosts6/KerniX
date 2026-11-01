#ifndef KERNIX_PAGING_H
#define KERNIX_PAGING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define PAGE_SIZE 4096u

#define KERNEL_VIRT_BASE 0xC0000000u

// Bootstrap maps this much low memory (identity + higher half) with 4 MiB pages.
#define KERNEL_EARLY_MAP_MB 16u

#define PAGE_PRESENT 0x001u
#define PAGE_WRITE   0x002u
#define PAGE_USER    0x004u
// Available software PTE bit — marks a shared copy-on-write page.
#define PAGE_COW     0x200u

#define PTE_FRAME_MASK 0xFFFFF000u

// Classic flat layout for user processes.
#define USER_CODE_BASE 0x08048000u
#define USER_STACK_TOP 0xBFFFF000u

static inline uint32_t virt_to_phys(uint32_t vaddr) {
    return vaddr - KERNEL_VIRT_BASE;
}

static inline uint32_t phys_to_virt(uint32_t paddr) {
    return paddr + KERNEL_VIRT_BASE;
}

static inline uint32_t page_align_down(uint32_t addr) {
    return addr & ~(PAGE_SIZE - 1u);
}

static inline uint32_t page_align_up(uint32_t addr) {
    return (addr + PAGE_SIZE - 1u) & ~(PAGE_SIZE - 1u);
}

void paging_init(void);

// Physical address of the shared kernel page directory.
uint32_t vmm_kernel_pd_phys(void);

// New address space with kernel mappings copied; returns PD physical address, or 0.
uint32_t vmm_create_address_space(void);

// Map one page into the given page directory (phys). Does not switch CR3.
bool vmm_map_page(uint32_t pd_phys, uint32_t virt, uint32_t phys, uint32_t flags);

// Physical frame for virt in pd, or 0. Optionally returns raw PTE flags.
uint32_t vmm_get_mapping(uint32_t pd_phys, uint32_t virt, uint32_t *flags_out);

// Clone user mappings from src as read-only COW into a new address space.
uint32_t vmm_clone_cow(uint32_t src_pd_phys);

// Resolve a write fault on a COW page in the active address space. Returns true if handled.
bool vmm_handle_cow_fault(uint32_t pd_phys, uint32_t virt);

void vmm_switch(uint32_t pd_phys);

bool map_page(uint32_t virt, uint32_t phys, uint32_t flags);
void unmap_page(uint32_t virt);
uint32_t get_mapping(uint32_t virt);
void invlpg(uint32_t virt);

// Page-fault error code bits (Intel Vol. 3).
#define PF_PRESENT  (1u << 0)
#define PF_WRITE    (1u << 1)
#define PF_USER     (1u << 2)
#define PF_RESERVED (1u << 3)
#define PF_IFETCH   (1u << 4)

void page_fault_dump(uint32_t err_code, uint32_t cr2);

#endif
