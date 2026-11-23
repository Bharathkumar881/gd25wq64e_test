#ifndef EEPROM_H
#define EEPROM_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <stdint.h>
#include <stddef.h>

/* Flash memory size: 8 MB */
#define FLASH_SIZE       0x800000U

/* Flash sector and page sizes */
#define SECTOR_SIZE      4096U
#define PAGE_SIZE        256U

/* Public API */

/**
 * @brief Read the JEDEC ID from the flash.
 * @param id Pointer to a 3-byte array to store the ID (e.g., C8 65 17).
 * @return 0 on success, negative errno on failure.
 */
int flash_read_jedec(uint8_t id[3]);

/**
 * @brief Erase a 4KB sector.
 * @param addr Start address of the sector (must be 4KB aligned).
 * @return 0 on success, negative errno on failure.
 */
int flash_sector_erase(uint32_t addr);

/**
 * @brief Write data to the flash (max 256 bytes per page).
 * @param addr Start address.
 * @param data Pointer to data buffer.
 * @param len Length of data (max 256).
 * @return 0 on success, negative errno on failure.
 */
int flash_page_program(uint32_t addr, const uint8_t *data, size_t len);

/**
 * @brief Read data from the flash (max 256 bytes).
 * @param addr Start address.
 * @param out Pointer to output buffer.
 * @param len Length of data to read.
 * @return 0 on success, negative errno on failure.
 */
int flash_read_data(uint32_t addr, uint8_t *out, size_t len);

/**
 * @brief Put the flash into Deep Power-Down (Sleep) mode.
 * @return 0 on success, negative errno on failure.
 */
int flash_enter_sleep(void);

/**
 * @brief Wake the flash from Deep Power-Down mode.
 * @return 0 on success, negative errno on failure.
 */
int flash_wake_up(void);

#endif /* EEPROM_H */