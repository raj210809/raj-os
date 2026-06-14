/*
 * vmm.c — 4-level x86-64 page tables allocated from PMM.
 */

#include "pmm.h"
#include "serial.h"
#include "vmm.h"

#include <stddef.h>
#include <stdint.h>

#define VMM_IDENTITY_BYTES  (128U * 1024U * 1024U)
#define PTE_ADDR_MASK       0x000FFFFFFFFFF000ULL

#define PML4_INDEX(v)  (((v) >> 39) & 0x1FFU)
#define PDPT_INDEX(v)  (((v) >> 30) & 0x1FFU)
#define PD_INDEX(v)    (((v) >> 21) & 0x1FFU)
#define PT_INDEX(v)    (((v) >> 12) & 0x1FFU)

static uint64_t kernel_pml4_phys;

static inline uint64_t read_cr3(void)
{
    uint64_t cr3;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

static inline void write_cr3(uint64_t cr3)
{
    __asm__ __volatile__("mov %0, %%cr3" : : "r"(cr3) : "memory");
}

static inline void invlpg(uint64_t virt)
{
    __asm__ __volatile__("invlpg (%0)" : : "r"(virt) : "memory");
}

static uint64_t *phys_to_virt(uint64_t phys)
{
    return (uint64_t *)(uintptr_t)phys;
}

static void __attribute__((noinline)) zero_page(void *page)
{
    uint8_t *bytes = (uint8_t *)page;

    for (size_t i = 0; i < PAGE_SIZE; i++) {
        bytes[i] = 0;
    }
}

static int is_canonical(uint64_t virt)
{
    uint64_t sign = (virt >> 47) & 1ULL;
    uint64_t high = virt >> 48;

    if (sign == 0ULL) {
        return high == 0ULL;
    }
    return high == 0xFFFFULL;
}

static uint64_t *get_or_create_table(uint64_t *parent, uint32_t index, uint64_t table_flags)
{
    uint64_t entry = parent[index];

    if ((entry & VMM_PRESENT) != 0ULL) {
        return phys_to_virt(entry & PTE_ADDR_MASK);
    }

    uint32_t phys = pmm_alloc_page();
    if (phys == 0U) {
        return NULL;
    }

    uint64_t *table = phys_to_virt(phys);
    zero_page(table);
    parent[index] = (uint64_t)phys | table_flags | VMM_PRESENT;
    return table;
}

static int identity_map_128mib(uint64_t *pml4)
{
    uint64_t *pdpt = get_or_create_table(pml4, 0, VMM_WRITE);
    if (pdpt == NULL) {
        return -1;
    }

    uint64_t *pd = get_or_create_table(pdpt, 0, VMM_WRITE);
    if (pd == NULL) {
        return -1;
    }

    for (uint32_t pd_i = 0; pd_i < 64U; pd_i++) {
        uint32_t pt_phys = pmm_alloc_page();
        if (pt_phys == 0U) {
            return -1;
        }

        uint64_t *pt = phys_to_virt(pt_phys);
        zero_page(pt);

        uint64_t base = (uint64_t)pd_i * (2U * 1024U * 1024U);
        for (uint32_t pt_i = 0; pt_i < 512U; pt_i++) {
            uint64_t phys = base + (uint64_t)pt_i * PAGE_SIZE;
            pt[pt_i] = phys | VMM_KERNEL_FLAGS;
        }

        pd[pd_i] = (uint64_t)pt_phys | VMM_WRITE | VMM_PRESENT;
    }

    return 0;
}

void vmm_init(void)
{
    uint32_t pml4_phys = pmm_alloc_page();
    if (pml4_phys == 0U) {
        serial_putln("[vmm] failed to allocate PML4");
        return;
    }

    uint64_t *pml4 = phys_to_virt(pml4_phys);
    zero_page(pml4);

    if (identity_map_128mib(pml4) != 0) {
        serial_putln("[vmm] identity map setup failed");
        return;
    }

    kernel_pml4_phys = pml4_phys;
    write_cr3(kernel_pml4_phys);

    serial_puts("[vmm] 128 MiB identity map (4 KiB pages), CR3=");
    serial_print_hex64(kernel_pml4_phys);
    serial_putln("");
}

uint64_t vmm_get_cr3(void)
{
    return read_cr3();
}

uint64_t vmm_virt_to_phys(uint64_t virt)
{
    if (!is_canonical(virt)) {
        return 0;
    }

    uint64_t cr3 = read_cr3();
    uint64_t *pml4 = phys_to_virt(cr3 & PTE_ADDR_MASK);

    uint64_t pml4e = pml4[PML4_INDEX(virt)];
    if ((pml4e & VMM_PRESENT) == 0ULL) {
        return 0;
    }

    uint64_t *pdpt = phys_to_virt(pml4e & PTE_ADDR_MASK);
    uint64_t pdpte = pdpt[PDPT_INDEX(virt)];
    if ((pdpte & VMM_PRESENT) == 0ULL) {
        return 0;
    }
    if ((pdpte & VMM_HUGE) != 0ULL) {
        return (pdpte & PTE_ADDR_MASK) + (virt & 0x3FFFFFFFULL);
    }

    uint64_t *pd = phys_to_virt(pdpte & PTE_ADDR_MASK);
    uint64_t pde = pd[PD_INDEX(virt)];
    if ((pde & VMM_PRESENT) == 0ULL) {
        return 0;
    }
    if ((pde & VMM_HUGE) != 0ULL) {
        return (pde & PTE_ADDR_MASK) + (virt & 0x1FFFFFULL);
    }

    uint64_t *pt = phys_to_virt(pde & PTE_ADDR_MASK);
    uint64_t pte = pt[PT_INDEX(virt)];
    if ((pte & VMM_PRESENT) == 0ULL) {
        return 0;
    }

    return (pte & PTE_ADDR_MASK) + (virt & 0xFFFULL);
}

int vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags)
{
    if (!is_canonical(virt)) {
        return -1;
    }
    if ((virt & (PAGE_SIZE - 1U)) != 0U || (phys & (PAGE_SIZE - 1U)) != 0U) {
        return -1;
    }
    if (phys == 0U) {
        return -1;
    }

    uint64_t *pml4 = phys_to_virt(read_cr3() & PTE_ADDR_MASK);
    uint64_t *pdpt = get_or_create_table(pml4, (uint32_t)PML4_INDEX(virt), VMM_WRITE);
    if (pdpt == NULL) {
        return -1;
    }

    uint64_t *pd = get_or_create_table(pdpt, (uint32_t)PDPT_INDEX(virt), VMM_WRITE);
    if (pd == NULL) {
        return -1;
    }

    uint64_t *pt = get_or_create_table(pd, (uint32_t)PD_INDEX(virt), VMM_WRITE);
    if (pt == NULL) {
        return -1;
    }

    pt[PT_INDEX(virt)] = (phys & PTE_ADDR_MASK) | (flags | VMM_PRESENT);
    invlpg(virt);
    return 0;
}

int vmm_unmap_page(uint64_t virt)
{
    if (!is_canonical(virt) || (virt & (PAGE_SIZE - 1U)) != 0U) {
        return -1;
    }

    uint64_t cr3 = read_cr3();
    uint64_t *pml4 = phys_to_virt(cr3 & PTE_ADDR_MASK);

    uint64_t pml4e = pml4[PML4_INDEX(virt)];
    if ((pml4e & VMM_PRESENT) == 0ULL) {
        return -1;
    }

    uint64_t *pdpt = phys_to_virt(pml4e & PTE_ADDR_MASK);
    uint64_t pdpte = pdpt[PDPT_INDEX(virt)];
    if ((pdpte & VMM_PRESENT) == 0ULL || (pdpte & VMM_HUGE) != 0ULL) {
        return -1;
    }

    uint64_t *pd = phys_to_virt(pdpte & PTE_ADDR_MASK);
    uint64_t pde = pd[PD_INDEX(virt)];
    if ((pde & VMM_PRESENT) == 0ULL || (pde & VMM_HUGE) != 0ULL) {
        return -1;
    }

    uint64_t *pt = phys_to_virt(pde & PTE_ADDR_MASK);
    if ((pt[PT_INDEX(virt)] & VMM_PRESENT) == 0ULL) {
        return -1;
    }

    pt[PT_INDEX(virt)] = 0;
    invlpg(virt);
    return 0;
}
