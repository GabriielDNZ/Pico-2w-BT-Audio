#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "btstack_link_key_db.h"
#include "btstack_link_key_db_memory.h"

#define FLASH_OFFSET   (PICO_FLASH_SIZE_BYTES - 4096)   // last 4 KiB page
#define MAX_KEYS       8

static btstack_link_key_db_memory_t mem_db;
static uint8_t page[4096];

static void flash_load(void){
    memcpy(page, (uint8_t const *)(XIP_BASE + FLASH_OFFSET), sizeof(page));
    btstack_link_key_db_memory_from_binary(&mem_db, page, sizeof(page));
}

static void flash_save_callback(void){
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_OFFSET, sizeof(page));
    btstack_link_key_db_memory_to_binary(&mem_db, page, sizeof(page));
    flash_range_program(FLASH_OFFSET, page, sizeof(page));
    restore_interrupts(ints);
}

const btstack_link_key_db_t * btstack_link_key_db_flash_instance(void){
    flash_load();
    mem_db.store_link_key_callback = flash_save_callback;
    mem_db.max_count = MAX_KEYS;
    return btstack_link_key_db_memory_instance(&mem_db);
}
