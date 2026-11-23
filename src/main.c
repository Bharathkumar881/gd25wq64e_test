#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>
#include <string.h>
#include "eeprom.h"
#define FLASH_NODE DT_NODELABEL(gd25wq64)
#define TEST_ADDRESS 0x000000U
//htis was chnaage yesh the itis uplading the hitgub spaces 
int main(void)
{
    int ret;
    uint8_t id[3] = {0};
    static const char write_msg[] = "Hello GD25WQ64E";
    uint8_t read_buf[sizeof(write_msg) - 1] = {0};

    printk("\n====================================\n");
    printk("    GD25WQ64E SPI FLASH TEST\n");
    printk("====================================\n");

    /* 1. SPI Ready & Power-on Delay */
    if (!spi_is_ready_dt(&(struct spi_dt_spec)SPI_DT_SPEC_GET(FLASH_NODE, SPI_WORD_SET(8), 0))) {
        printk("ERROR: SPI device not ready\n");
        return -1;
    }
    
    printk("Waiting for flash power-up...\n");
    k_msleep(50); 
    printk("SPI device ready\n");

    /* 2. Read JEDEC ID */
    printk("\nReading JEDEC ID...\n");
    ret = flash_read_jedec(id);
    if (ret != 0) {
        printk("JEDEC read failed: %d\n", ret);
        return ret;
    }
    printk("JEDEC ID: %02X %02X %02X\n", id[0], id[1], id[2]);

    if (id[0] != 0xC8 || id[1] != 0x65 || id[2] != 0x17) {
        printk("ERROR: Unexpected Flash ID\n");
        return -1;
    }
    printk("GD25WQ64E detected successfully\n");

    /* 3. Erase */
    printk("\nErasing sector...\n");
    ret = flash_sector_erase(TEST_ADDRESS);
    if (ret != 0) {
        printk("Sector erase failed: %d\n", ret);
        return ret;
    }
    printk("Sector erase successful\n");

    /* 4. Write */
    printk("\nWriting data...\n");
    printk("Message: %s\n", write_msg);
    ret = flash_page_program(TEST_ADDRESS, (const uint8_t *)write_msg, sizeof(write_msg) - 1);
    if (ret != 0) {
        printk("Page program failed: %d\n", ret);
        return ret;
    }
    printk("Data written successfully\n");

    /* 5. Sleep & Wake */
    printk("\nPutting flash into Deep Power-Down (Sleep)...\n");
    ret = flash_enter_sleep();
    if (ret != 0) return ret;

    k_msleep(1000); 
    printk("Flash is sleeping...\n");

    printk("\nWaking up flash...\n");
    ret = flash_wake_up();
    if (ret != 0) return ret;
    printk("Flash awake!\n");

    /* 6. Read & Verify */
    printk("\nReading data...\n");
    ret = flash_read_data(TEST_ADDRESS, read_buf, sizeof(read_buf));
    if (ret != 0) {
        printk("Flash read failed: %d\n", ret);
        return ret;
    }
    printk("Read data: %.*s\n", (int)sizeof(read_buf), read_buf);

    printk("\nVerifying data...\n");
    if (memcmp(write_msg, read_buf, sizeof(read_buf)) == 0) {
        printk("SUCCESS: DATA MATCHED\n");
    } else {
        printk("ERROR: DATA MISMATCH\n");
        return -1;
    }

    printk("\n====================================\n");
    printk("       FLASH TEST COMPLETED\n");
    printk("====================================\n");

    return 0;
}