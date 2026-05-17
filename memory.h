#pragma once
#include "../core/ktypes.h"

/*
 * Memory Manager — Physical Page Frame Allocator
 *
 * ARCHITECTURE:
 *   The x86 paging system divides physical memory into fixed-size "frames" (pages).
 *   This manager tracks which frames are FREE or USED using a simple bitmap.
 *
 *   PAGE_SIZE = 4096 bytes (4 KB) — the standard x86 page size.
 *
 * LAYOUT (our simulated RAM pool):
 *   We carve out a static region in the kernel's BSS segment and treat it as
 *   our "physical RAM pool". This is a common technique in early OS development
 *   before a full physical memory map (e.g., from GRUB's multiboot info) is available.
 *
 *   RAM_SIZE  = 1 MB  (enough to demonstrate the allocator clearly)
 *   NUM_PAGES = 256   (1MB / 4KB = 256 pages)
 *
 * PCB INTEGRATION:
 *   Each process gets one or more pages for its data.
 *   The Process Control Block (PCB) stores which pages belong to it.
 *
 * ALGORITHM:
 *   First-Fit allocation — scans the bitmap from page 0, returns the first free page.
 *   Simple, fast, and easy to understand for educational purposes.
 */
namespace MemoryManager {

    constexpr uint32_t PAGE_SIZE  = 4096;          // 4 KB per page
    constexpr uint32_t RAM_SIZE   = 1024 * 1024;   // 1 MB simulated RAM pool
    constexpr uint32_t NUM_PAGES  = RAM_SIZE / PAGE_SIZE; // 256 pages

    /*
     * init
     * Marks all pages as FREE and sets up the memory pool.
     * Must be called before any alloc/free.
     */
    void init();

    /*
     * alloc_page
     * Finds the first free page (First-Fit), marks it USED, returns its page number.
     * Returns 0xFFFFFFFF if no pages are available (OOM).
     */
    uint32_t alloc_page();

    /*
     * free_page
     * Marks a page as FREE, making it available for future allocations.
     */
    void free_page(uint32_t page_num);

    /*
     * get_free_count
     * Returns the number of currently free pages.
     */
    uint32_t get_free_count();

    /*
     * get_used_count
     * Returns the number of currently allocated pages.
     */
    uint32_t get_used_count();

    /*
     * is_page_free
     * Returns true if the given page number is currently free.
     */
    bool is_page_free(uint32_t page_num);

    /*
     * get_page_address
     * Converts a page number to its base physical address in our RAM pool.
     */
    uint32_t get_page_address(uint32_t page_num);

}
