/*
 * vmm_test.c — Verify VMM identity map and dynamic map/unmap.
 */

#include "pmm.h"
#include "serial.h"
#include "vga.h"
#include "vmm.h"
#include "vmm_test.h"

#include <stdint.h>

#define VMM_TEST_VIRT  0x0000000040000000ULL

static uint32_t tests_passed;
static uint32_t tests_failed;

static void __attribute__((noinline)) test_pass(const char *name)
{
    tests_passed++;
    serial_puts("[vmm-test] PASS: ");
    serial_putln(name);
}

static void __attribute__((noinline)) test_fail(const char *name)
{
    tests_failed++;
    serial_puts("[vmm-test] FAIL: ");
    serial_putln(name);
}

static void __attribute__((noinline)) test_identity_lookup(void)
{
    uint64_t phys = vmm_virt_to_phys(0x100000ULL);
    if (phys != 0x100000ULL) {
        test_fail("identity lookup @ 1 MiB");
        return;
    }

    phys = vmm_virt_to_phys(0xB8000ULL);
    if (phys != 0xB8000ULL) {
        test_fail("identity lookup @ VGA");
        return;
    }

    test_pass("identity map lookups (1 MiB, VGA)");
}

static void __attribute__((noinline)) test_dynamic_map_unmap(void)
{
    uint32_t frame = pmm_alloc_page();
    if (frame == 0U) {
        test_fail("dynamic map: pmm alloc");
        return;
    }

    volatile uint32_t *direct = (volatile uint32_t *)(uintptr_t)frame;
    *direct = 0xDEADBEEFU;

    if (vmm_map_page(VMM_TEST_VIRT, frame, VMM_KERNEL_FLAGS) != 0) {
        test_fail("dynamic map: vmm_map_page");
        pmm_free_page(frame);
        return;
    }

    uint64_t resolved = vmm_virt_to_phys(VMM_TEST_VIRT);
    if (resolved != frame) {
        test_fail("dynamic map: virt_to_phys");
        vmm_unmap_page(VMM_TEST_VIRT);
        pmm_free_page(frame);
        return;
    }

    volatile uint32_t *mapped = (volatile uint32_t *)(uintptr_t)VMM_TEST_VIRT;
    *mapped = 0xCAFEBABEU;
    if (*direct != 0xCAFEBABEU) {
        test_fail("dynamic map: write via virt");
        vmm_unmap_page(VMM_TEST_VIRT);
        pmm_free_page(frame);
        return;
    }

    if (vmm_unmap_page(VMM_TEST_VIRT) != 0) {
        test_fail("dynamic unmap");
        pmm_free_page(frame);
        return;
    }

    if (vmm_virt_to_phys(VMM_TEST_VIRT) != 0ULL) {
        test_fail("dynamic unmap: still mapped");
        pmm_free_page(frame);
        return;
    }

    pmm_free_page(frame);
    test_pass("dynamic map/unmap @ 0x40000000");
}

void vmm_run_tests(void)
{
    tests_passed = 0;
    tests_failed = 0;

    serial_putln("[vmm-test] --- VMM suite ---");

    test_identity_lookup();
    test_dynamic_map_unmap();

    serial_puts("[vmm-test] done: passed=");
    serial_print_hex32(tests_passed);
    serial_puts(" failed=");
    serial_print_hex32(tests_failed);
    serial_putln("");

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    if (tests_failed == 0U) {
        vga_write_at(14, 0, "VMM tests: ALL PASSED (details on serial)       ");
    } else {
        vga_write_at(14, 0, "VMM tests: FAILURES (see serial log)            ");
    }
}
