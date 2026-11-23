#include "eeprom.h"
#include <zephyr/devicetree.h>
#include <string.h>
#include <errno.h>

#define FLASH_NODE DT_NODELABEL(gd25wq64)

static const struct spi_dt_spec flash_spi =
    SPI_DT_SPEC_GET(FLASH_NODE, SPI_WORD_SET(8), 0);

/* GD25WQ64E SPI COMMANDS */
#define CMD_WREN         0x06
#define CMD_RDSR1        0x05
#define CMD_READ         0x03
#define CMD_PP           0x02
#define CMD_SE           0x20
#define CMD_RDID         0x9F
#define CMD_DP           0xB9  /* Deep Power-Down */
#define CMD_RDP          0xAB  /* Release from Deep Power-Down */

#define SR1_WIP          (1U << 0)

/* --- Private Helper Functions --- */

static int flash_read_status(uint8_t *status)
{
    uint8_t tx_buf[2] = { CMD_RDSR1, 0x00 };
    uint8_t rx_buf[2] = {0};

    struct spi_buf txb = { .buf = tx_buf, .len = sizeof(tx_buf) };
    struct spi_buf rxb = { .buf = rx_buf, .len = sizeof(rx_buf) };
    struct spi_buf_set tx = { .buffers = &txb, .count = 1 };
    struct spi_buf_set rx = { .buffers = &rxb, .count = 1 };

    int ret = spi_transceive_dt(&flash_spi, &tx, &rx);
    if (ret != 0) return ret;

    *status = rx_buf[1];
    return 0;
}

static int flash_wait_ready(void)
{
    uint8_t status = 0;

    for (int i = 0; i < 10000; i++) {
        int ret = flash_read_status(&status);
        if (ret != 0) return ret;
        if ((status & SR1_WIP) == 0) return 0;
        k_msleep(1);
    }
    return -ETIMEDOUT;
}

static int flash_write_enable(void)
{
    uint8_t cmd = CMD_WREN;
    struct spi_buf txb = { .buf = &cmd, .len = 1 };
    struct spi_buf_set tx = { .buffers = &txb, .count = 1 };
    return spi_write_dt(&flash_spi, &tx);
}

/* --- Public API Implementation --- */

int flash_read_jedec(uint8_t id[3])
{
    uint8_t tx_buf[4] = { CMD_RDID, 0x00, 0x00, 0x00 };
    uint8_t rx_buf[4] = {0};

    struct spi_buf txb = { .buf = tx_buf, .len = sizeof(tx_buf) };
    struct spi_buf rxb = { .buf = rx_buf, .len = sizeof(rx_buf) };
    struct spi_buf_set tx = { .buffers = &txb, .count = 1 };
    struct spi_buf_set rx = { .buffers = &rxb, .count = 1 };

    int ret = spi_transceive_dt(&flash_spi, &tx, &rx);
    if (ret != 0) return ret;

    id[0] = rx_buf[1];
    id[1] = rx_buf[2];
    id[2] = rx_buf[3];

    return 0;
}

int flash_sector_erase(uint32_t addr)
{
    if (addr >= FLASH_SIZE || (addr % SECTOR_SIZE) != 0) {
        return -EINVAL;
    }

    uint8_t tx_buf[4] = {
        CMD_SE,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)addr
    };

    struct spi_buf txb = { .buf = tx_buf, .len = sizeof(tx_buf) };
    struct spi_buf_set tx = { .buffers = &txb, .count = 1 };

    int ret = flash_write_enable();
    if (ret != 0) return ret;

    ret = spi_write_dt(&flash_spi, &tx);
    if (ret != 0) return ret;

    return flash_wait_ready();
}

int flash_page_program(uint32_t addr, const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0 || len > PAGE_SIZE ||
        addr >= FLASH_SIZE || len > FLASH_SIZE - addr ||
        (addr % PAGE_SIZE) + len > PAGE_SIZE) {
        return -EINVAL;
    }

    uint8_t hdr[4] = {
        CMD_PP,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)addr
    };

    struct spi_buf txb[2] = {
        { .buf = hdr, .len = sizeof(hdr) },
        { .buf = (void *)data, .len = len }
    };

    struct spi_buf_set tx = { .buffers = txb, .count = 2 };

    int ret = flash_write_enable();
    if (ret != 0) return ret;

    ret = spi_write_dt(&flash_spi, &tx);
    if (ret != 0) return ret;

    return flash_wait_ready();
}

int flash_read_data(uint32_t addr, uint8_t *out, size_t len)
{
    if (out == NULL || len == 0 || addr >= FLASH_SIZE ||
        len > FLASH_SIZE - addr || len > PAGE_SIZE) {
        return -EINVAL;
    }

    uint8_t tx_buf[4 + PAGE_SIZE] = {0};
    uint8_t rx_buf[4 + PAGE_SIZE] = {0};

    tx_buf[0] = CMD_READ;
    tx_buf[1] = (uint8_t)(addr >> 16);
    tx_buf[2] = (uint8_t)(addr >> 8);
    tx_buf[3] = (uint8_t)addr;

    struct spi_buf txb = { .buf = tx_buf, .len = 4 + len };
    struct spi_buf rxb = { .buf = rx_buf, .len = 4 + len };
    struct spi_buf_set tx = { .buffers = &txb, .count = 1 };
    struct spi_buf_set rx = { .buffers = &rxb, .count = 1 };

    int ret = spi_transceive_dt(&flash_spi, &tx, &rx);
    if (ret != 0) return ret;

    memcpy(out, &rx_buf[4], len);
    return 0;
}

int flash_enter_sleep(void)
{
    uint8_t cmd = CMD_DP;
    struct spi_buf txb = { .buf = &cmd, .len = 1 };
    struct spi_buf_set tx = { .buffers = &txb, .count = 1 };

    int ret = spi_write_dt(&flash_spi, &tx);
    if (ret != 0) return ret;

    k_usleep(10);
    return 0;
}

int flash_wake_up(void)
{
    uint8_t cmd = CMD_RDP;
    struct spi_buf txb = { .buf = &cmd, .len = 1 };
    struct spi_buf_set tx = { .buffers = &txb, .count = 1 };

    int ret = spi_write_dt(&flash_spi, &tx);
    if (ret != 0) return ret;

    k_usleep(50);
    return 0;
}