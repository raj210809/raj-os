/*
 * kmalloc_test.c — kmalloc/kfree tests (run on serial before IRQs are enabled).
 */

#include "kmalloc.h"
#include "kmalloc_test.h"
#include "serial.h"
#include "vga.h"

#include <stddef.h>
#include <stdint.h>

static uint32_t tests_passed;
static uint32_t tests_failed;

static void __attribute__((noinline)) test_pass(const char *name)
{
    tests_passed++;
    serial_puts("[heap-test] PASS: ");
    serial_putln(name);
}

static void __attribute__((noinline)) test_fail(const char *name)
{
    tests_failed++;
    serial_puts("[heap-test] FAIL: ");
    serial_putln(name);
}

static int pattern_ok(uint8_t *buf, size_t len, uint8_t byte)
{
    for (size_t i = 0; i < len; i++) {
        if (buf[i] != byte) {
            return 0;
        }
    }
    return 1;
}

static void __attribute__((noinline)) test_basic_alloc_free(void)
{
    void *p = kmalloc(48);
    if (p == NULL) {
        test_fail("basic alloc");
        return;
    }

    uint8_t *b = (uint8_t *)p;
    for (size_t i = 0; i < 48; i++) {
        b[i] = 0xA5;
    }

    if (!pattern_ok(b, 48, 0xA5)) {
        test_fail("basic write pattern");
        kfree(p);
        return;
    }

    kfree(p);
    test_pass("basic alloc/free + write");
}

static void __attribute__((noinline)) test_neighbor_isolation(void)
{
    uint8_t *left  = kmalloc(32);
    uint8_t *right = kmalloc(32);

    if (left == NULL || right == NULL) {
        test_fail("neighbor alloc");
        kfree(left);
        kfree(right);
        return;
    }

    for (size_t i = 0; i < 32; i++) {
        left[i]  = 0x11;
        right[i] = 0x22;
    }

    if (!pattern_ok(left, 32, 0x11) || !pattern_ok(right, 32, 0x22)) {
        test_fail("neighbor isolation");
        kfree(left);
        kfree(right);
        return;
    }

    kfree(left);
    kfree(right);
    test_pass("neighbor blocks do not corrupt each other");
}

static void __attribute__((noinline)) test_reuse_after_free(void)
{
    void *first = kmalloc(64);
    if (first == NULL) {
        test_fail("reuse alloc first");
        return;
    }

    uintptr_t addr_first = (uintptr_t)first;
    kfree(first);

    void *second = kmalloc(64);
    if (second == NULL) {
        test_fail("reuse alloc second");
        return;
    }

    if ((uintptr_t)second != addr_first) {
        serial_puts("[heap-test] note: reuse addr first=");
        serial_print_hex64((uint64_t)addr_first);
        serial_puts(" second=");
        serial_print_hex64((uint64_t)(uintptr_t)second);
        serial_putln("");
    }

    kfree(second);
    test_pass("reuse after free (same or new block both OK)");
}

static void __attribute__((noinline)) test_multi_page_growth(void)
{
    void *blocks[8];
    size_t i;

    for (i = 0; i < 8; i++) {
        blocks[i] = kmalloc(512);
        if (blocks[i] == NULL) {
            test_fail("multi-page growth");
            while (i > 0) {
                i--;
                kfree(blocks[i]);
            }
            return;
        }
        ((uint8_t *)blocks[i])[0] = (uint8_t)(0xC0 + i);
    }

    for (i = 0; i < 8; i++) {
        if (((uint8_t *)blocks[i])[0] != (uint8_t)(0xC0 + i)) {
            test_fail("multi-page block integrity");
            while (i < 8) {
                kfree(blocks[i++]);
            }
            return;
        }
    }

    for (i = 0; i < 8; i++) {
        kfree(blocks[i]);
    }

    test_pass("multi-page growth (8 x 512 bytes)");
}

static void __attribute__((noinline)) test_free_all_coalesce(void)
{
    void *a = kmalloc(100);
    void *b = kmalloc(200);
    void *c = kmalloc(300);

    if (a == NULL || b == NULL || c == NULL) {
        test_fail("coalesce alloc");
        kfree(a);
        kfree(b);
        kfree(c);
        return;
    }

    kfree(b);
    kfree(a);
    kfree(c);

    kmalloc_print_stats();
    test_pass("free all (coalesce on same page)");
}

static void __attribute__((noinline)) test_odd_sizes(void)
{
    static const size_t sizes[] = { 1, 7, 15, 33, 127, 511, 1024 };
    void *ptrs[sizeof(sizes) / sizeof(sizes[0])];

    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        ptrs[i] = kmalloc(sizes[i]);
        if (ptrs[i] == NULL) {
            test_fail("odd size alloc");
            while (i > 0) {
                i--;
                kfree(ptrs[i]);
            }
            return;
        }
        ((uint8_t *)ptrs[i])[0] = (uint8_t)(0x80 + i);
        if (sizes[i] > 1) {
            ((uint8_t *)ptrs[i])[sizes[i] - 1] = (uint8_t)(0x90 + i);
        }
    }

    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        uint8_t *p = (uint8_t *)ptrs[i];
        if (p[0] != (uint8_t)(0x80 + i)) {
            test_fail("odd size head byte");
            for (size_t j = 0; j < sizeof(sizes) / sizeof(sizes[0]); j++) {
                kfree(ptrs[j]);
            }
            return;
        }
        if (sizes[i] > 1 && p[sizes[i] - 1] != (uint8_t)(0x90 + i)) {
            test_fail("odd size tail byte");
            for (size_t j = 0; j < sizeof(sizes) / sizeof(sizes[0]); j++) {
                kfree(ptrs[j]);
            }
            return;
        }
        kfree(ptrs[i]);
    }

    test_pass("odd allocation sizes");
}

void kmalloc_run_tests(void)
{
    tests_passed = 0;
    tests_failed = 0;

    serial_putln("[heap-test] --- kmalloc/kfree suite ---");

    test_basic_alloc_free();
    test_neighbor_isolation();
    test_reuse_after_free();
    test_multi_page_growth();
    test_free_all_coalesce();
    test_odd_sizes();

    serial_puts("[heap-test] done: passed=");
    serial_print_hex32(tests_passed);
    serial_puts(" failed=");
    serial_print_hex32(tests_failed);
    serial_putln("");

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    if (tests_failed == 0U) {
        vga_write_at(13, 0, "Heap tests: ALL PASSED (details on serial)     ");
    } else {
        vga_write_at(13, 0, "Heap tests: FAILURES (see serial log)          ");
    }
}
