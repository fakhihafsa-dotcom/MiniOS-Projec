#include "memory.h"

namespace MemoryManager {

    /*
     * page_bitmap
     * Each byte represents one page: 0 = FREE, 1 = USED.
     * Using a byte-per-page (instead of bit-per-page) trades memory for simplicity.
     * In a production OS, you would use a proper bit-array to save space.
     */
    static uint8_t page_bitmap[NUM_PAGES];

    /*
     * ram_pool
     * The actual simulated physical memory.
     * __attribute__((aligned(PAGE_SIZE))) ensures the pool starts on a 4KB boundary,
     * which is required by the x86 paging hardware.
     */
    static __attribute__((aligned(PAGE_SIZE)))
    uint8_t ram_pool[RAM_SIZE];

    // Running count of free pages (cached for O(1) queries)
    static uint32_t free_count;

    void init() {
        // Mark every page as FREE
        for (uint32_t i = 0; i < NUM_PAGES; i++) {
            page_bitmap[i] = 0; // 0 = FREE
        }
        free_count = NUM_PAGES;
    }

    uint32_t alloc_page() {
        // First-Fit: scan from the start, return first free page
        for (uint32_t i = 0; i < NUM_PAGES; i++) {
            if (page_bitmap[i] == 0) {      // Found a free page
                page_bitmap[i] = 1;         // Mark as USED
                free_count--;
                return i;                   // Return page number
            }
        }
        return 0xFFFFFFFF; // Out of Memory sentinel
    }

    void free_page(uint32_t page_num) {
        if (page_num >= NUM_PAGES) return;  // Bounds check
        if (page_bitmap[page_num] == 0) return; // Already free, do nothing

        page_bitmap[page_num] = 0; // Mark as FREE
        free_count++;

        // Zero out the page data for security (prevent data leaks to next process)
        uint8_t* page_start = ram_pool + (page_num * PAGE_SIZE);
        for (uint32_t i = 0; i < PAGE_SIZE; i++) {
            page_start[i] = 0;
        }
    }

    uint32_t get_free_count() {
        return free_count;
    }

    uint32_t get_used_count() {
        return NUM_PAGES - free_count;
    }

    bool is_page_free(uint32_t page_num) {
        if (page_num >= NUM_PAGES) return false;
        return page_bitmap[page_num] == 0;
    }

    uint32_t get_page_address(uint32_t page_num) {
        return (uint32_t)(ram_pool + page_num * PAGE_SIZE);
    }

}
