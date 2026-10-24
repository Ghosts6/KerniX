#include <kernix/proc.h>
#include <kernix/multiboot.h>
#include <kernix/paging.h>
#include <kernix/pmm.h>
#include <kernix/sched.h>
#include <kernix/string.h>
#include <kernix/kprintf.h>

#include <stdbool.h>
#include <stdint.h>

static bool map_image(uint32_t pd_phys, const void *image, size_t size) {
    const uint8_t *src = (const uint8_t *)image;
    uint32_t virt = USER_CODE_BASE;
    size_t remaining = size;

    while (remaining > 0) {
        void *frame = pmm_alloc_frame();
        if (frame == 0) {
            return false;
        }
        uint32_t phys = (uint32_t)(uintptr_t)frame;
        uint8_t *dst = (uint8_t *)(uintptr_t)phys_to_virt(phys);
        memset(dst, 0, PAGE_SIZE);

        size_t chunk = remaining < PAGE_SIZE ? remaining : PAGE_SIZE;
        memcpy(dst, src, chunk);

        if (!vmm_map_page(pd_phys, virt, phys, PAGE_PRESENT | PAGE_WRITE | PAGE_USER)) {
            return false;
        }

        src += chunk;
        remaining -= chunk;
        virt += PAGE_SIZE;
    }
    return true;
}

static bool map_user_stack(uint32_t pd_phys) {
    void *frame = pmm_alloc_frame();
    if (frame == 0) {
        return false;
    }
    uint32_t phys = (uint32_t)(uintptr_t)frame;
    memset((void *)(uintptr_t)phys_to_virt(phys), 0, PAGE_SIZE);
    uint32_t stack_page = USER_STACK_TOP - PAGE_SIZE;
    return vmm_map_page(pd_phys, stack_page, phys, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
}

task_t *user_spawn(const char *name, const void *image, size_t size, uint32_t entry_offset) {
    if (image == 0 || size == 0) {
        return 0;
    }

    uint32_t pd = vmm_create_address_space();
    if (pd == 0) {
        return 0;
    }
    if (!map_image(pd, image, size) || !map_user_stack(pd)) {
        return 0;
    }

    uint32_t entry = USER_CODE_BASE + entry_offset;
    task_t *t = task_create_user(name, pd, entry, USER_STACK_TOP);
    if (t == 0) {
        return 0;
    }
    kprintf("user: spawned %s pid=%u entry=%x size=%u\n", name, t->id, entry, (uint32_t)size);
    return t;
}

task_t *user_spawn_module(uint32_t mbi_addr, const char *name) {
    uint32_t count = 0;
    const multiboot_module_t *mods = multiboot_modules(mbi_addr, &count);
    if (mods == 0 || count == 0) {
        return 0;
    }

    uint32_t start = mods[0].mod_start;
    uint32_t end = mods[0].mod_end;
    if (end <= start) {
        return 0;
    }

    // Modules live in identity-mapped low memory after paging_init.
    const void *image = (const void *)(uintptr_t)start;
    return user_spawn(name != 0 ? name : "module", image, (size_t)(end - start), 0);
}
