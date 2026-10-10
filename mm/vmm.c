#include <kernix/paging.h>
#include <kernix/pmm.h>
#include <kernix/string.h>
#include <kernix/kprintf.h>
#include <kernix/kernel.h>
#include <kernix/cpu.h>

#include <stdint.h>

#define PDE_COUNT 1024u
#define PTE_COUNT 1024u
#define PDE_PRESENT 0x001u
#define PDE_WRITE   0x002u
#define PDE_USER    0x004u
#define PDE_4MB     0x080u

static uint32_t *kernel_pd; // virtual address of the active page directory

static uint32_t *pde_table(uint32_t pde) {
    uint32_t entry = kernel_pd[pde];
    if ((entry & PDE_PRESENT) == 0) {
        return 0;
    }
    if ((entry & PDE_4MB) != 0) {
        return 0;
    }
    return (uint32_t *)phys_to_virt(entry & PTE_FRAME_MASK);
}

static uint32_t *ensure_page_table(uint32_t pde_index, uint32_t flags) {
    uint32_t entry = kernel_pd[pde_index];
    if ((entry & PDE_PRESENT) != 0) {
        if ((entry & PDE_4MB) != 0) {
            panic("ensure_page_table: 4MiB PDE in use at %u", pde_index);
        }
        return (uint32_t *)phys_to_virt(entry & PTE_FRAME_MASK);
    }

    void *frame = pmm_alloc_frame();
    if (frame == 0) {
        return 0;
    }

    uint32_t phys = (uint32_t)(uintptr_t)frame;
    uint32_t *table = (uint32_t *)phys_to_virt(phys);
    memset(table, 0, PAGE_SIZE);

    uint32_t pde_flags = PDE_PRESENT | PDE_WRITE;
    if ((flags & PAGE_USER) != 0) {
        pde_flags |= PDE_USER;
    }
    kernel_pd[pde_index] = (phys & PTE_FRAME_MASK) | pde_flags;
    return table;
}

static void map_range_4k(uint32_t virt_start, uint32_t phys_start, uint32_t length,
                         uint32_t flags) {
    uint32_t virt = page_align_down(virt_start);
    uint32_t phys = page_align_down(phys_start);
    uint32_t end = page_align_up(virt_start + length);

    while (virt < end) {
        if (!map_page(virt, phys, flags)) {
            panic("paging_init: map_page failed at %x", virt);
        }
        virt += PAGE_SIZE;
        phys += PAGE_SIZE;
    }
}

void invlpg(uint32_t virt) {
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

bool map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pdi = (virt >> 22) & 0x3FFu;
    uint32_t pti = (virt >> 12) & 0x3FFu;

    uint32_t *table = ensure_page_table(pdi, flags);
    if (table == 0) {
        return false;
    }

    uint32_t pte_flags = PAGE_PRESENT;
    if ((flags & PAGE_WRITE) != 0) {
        pte_flags |= PAGE_WRITE;
    }
    if ((flags & PAGE_USER) != 0) {
        pte_flags |= PAGE_USER;
    }

    table[pti] = (phys & PTE_FRAME_MASK) | pte_flags;
    invlpg(virt);
    return true;
}

void unmap_page(uint32_t virt) {
    uint32_t pdi = (virt >> 22) & 0x3FFu;
    uint32_t pti = (virt >> 12) & 0x3FFu;
    uint32_t *table = pde_table(pdi);
    if (table == 0) {
        return;
    }
    table[pti] = 0;
    invlpg(virt);
}

uint32_t get_mapping(uint32_t virt) {
    uint32_t pdi = (virt >> 22) & 0x3FFu;
    uint32_t pti = (virt >> 12) & 0x3FFu;

    uint32_t pde = kernel_pd[pdi];
    if ((pde & PDE_PRESENT) == 0) {
        return 0;
    }
    if ((pde & PDE_4MB) != 0) {
        uint32_t base = pde & 0xFFC00000u;
        return base | (virt & 0x003FFFFFu);
    }

    uint32_t *table = (uint32_t *)phys_to_virt(pde & PTE_FRAME_MASK);
    uint32_t pte = table[pti];
    if ((pte & PAGE_PRESENT) == 0) {
        return 0;
    }
    return (pte & PTE_FRAME_MASK) | (virt & (PAGE_SIZE - 1u));
}

void page_fault_dump(uint32_t err_code, uint32_t cr2) {
    kprintf("  page fault cr2=%x err=0x%x (", cr2, err_code);
    kprintf("%s", (err_code & PF_PRESENT) ? "protection" : "not-present");
    kprintf(", %s", (err_code & PF_WRITE) ? "write" : "read");
    kprintf(", %s", (err_code & PF_USER) ? "user" : "supervisor");
    if (err_code & PF_RESERVED) {
        kprintf(", reserved");
    }
    if (err_code & PF_IFETCH) {
        kprintf(", ifetch");
    }
    kprintf(")\n");
}

void paging_init(void) {
    // Replace the bootstrap 4 MiB PSE directory with 4 KiB page tables.
    void *pd_frame = pmm_alloc_frame();
    if (pd_frame == 0) {
        panic("paging_init: no frame for page directory");
    }

    uint32_t pd_phys = (uint32_t)(uintptr_t)pd_frame;
    kernel_pd = (uint32_t *)phys_to_virt(pd_phys);
    memset(kernel_pd, 0, PAGE_SIZE);

    const uint32_t map_bytes = KERNEL_EARLY_MAP_MB * 1024u * 1024u;
    map_range_4k(0, 0, map_bytes, PAGE_PRESENT | PAGE_WRITE);
    map_range_4k(KERNEL_VIRT_BASE, 0, map_bytes, PAGE_PRESENT | PAGE_WRITE);

    write_cr3(pd_phys);

    // Drop PSE; all mappings are now 4 KiB.
    write_cr4(read_cr4() & ~0x00000010u);

    kprintf("paging: 4K tables active, mapped low %u MiB + higher half\n",
            KERNEL_EARLY_MAP_MB);
}
