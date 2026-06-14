/*
 * vmm.h — Virtual Memory Manager (x86-64 4-level paging on top of PMM).
 *
 * Boot uses 2 MiB huge-page identity map.  vmm_init() rebuilds the low 128 MiB
 * with 4 KiB pages (PMM-allocated tables) and switches CR3.  vmm_map_page() can
 * then map arbitrary canonical virtual addresses on demand.
 */

#ifndef VMM_H
#define VMM_H

#include <stdint.h>

/* Page-table entry attribute bits (Intel SDM Vol.3A §4.5). */
#define VMM_PRESENT   (1ULL << 0)
#define VMM_WRITE     (1ULL << 1)
#define VMM_USER      (1ULL << 2)
#define VMM_PWT       (1ULL << 3)
#define VMM_PCD       (1ULL << 4)
#define VMM_ACCESSED  (1ULL << 5)
#define VMM_DIRTY     (1ULL << 6)
#define VMM_HUGE      (1ULL << 7)

/* Default kernel mapping: present + writable. */
#define VMM_KERNEL_FLAGS  (VMM_PRESENT | VMM_WRITE)

/* Rebuild 128 MiB identity map with 4 KiB pages and load CR3. */
void vmm_init(void);

/*
 * Map one 4 KiB page.  virt and phys must be page-aligned.
 * Creates missing PML4/PDPT/PD/PT levels from PMM as needed.
 * Returns 0 on success, -1 on failure.
 */
int vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags);

/* Unmap one 4 KiB page and flush the TLB entry.  Returns 0 or -1. */
int vmm_unmap_page(uint64_t virt);

/* Resolve virtual → physical; returns 0 if not mapped. */
uint64_t vmm_virt_to_phys(uint64_t virt);

/* Active PML4 physical address (CR3). */
uint64_t vmm_get_cr3(void);

#endif /* VMM_H */
