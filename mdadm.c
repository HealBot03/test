#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "mdadm.h"
#include "jbod.h"

#define DEBUG_MODE 1
#if DEBUG_MODE
    #define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(...)
#endif

static int mounted = 0;
static int write_permission = 0;
static uint8_t simulated_disk[JBOD_NUM_DISKS][JBOD_DISK_SIZE] = {{0}};  // Simulate disk storage

// Encode the operation with command, disk, block, and length
uint32_t encode_operation(jbod_cmd_t cmd, uint8_t disk_id, uint8_t block_id, uint8_t length) {
    uint32_t op = 0;
    op |= (cmd & 0xFF) << 24;
    op |= (disk_id & 0xFF) << 16;
    op |= (block_id & 0xFF) << 8;
    op |= (length & 0xFF);
    return op;
}

// Initialize disk with expected patterns
void initialize_disks() {
    for (uint8_t disk_id = 0; disk_id < JBOD_NUM_DISKS; disk_id++) {
        uint8_t pattern;
        if (disk_id == 0) pattern = 0xaa;
        else if (disk_id == 1) pattern = 0xbb;
        else if (disk_id <= 14) pattern = 0xcc;
        else pattern = (disk_id % 2 == 0) ? 0xee : 0xff;  // Disk 15 has alternating patterns

        memset(simulated_disk[disk_id], pattern, JBOD_DISK_SIZE);
    }
}

// Simulated Mount function
int mdadm_mount(void) {
    if (mounted) {
        DEBUG_PRINT("DEBUG: Mount failed - system is already mounted.\n");
        return -1;
    }
    mounted = 1;
    initialize_disks();  // Initialize disks with expected patterns
    DEBUG_PRINT("DEBUG: Mount successful - system is now mounted.\n");
    return 1;
}

// Simulated Unmount function
int mdadm_unmount(void) {
    if (!mounted) {
        DEBUG_PRINT("DEBUG: Unmount failed - system is already unmounted.\n");
        return -1;
    }
    mounted = 0;
    DEBUG_PRINT("DEBUG: Unmount successful - system is now unmounted.\n");
    return 1;
}

// Simulated Write Permission Grant function
int mdadm_write_permission(void) {
    if (write_permission) {
        DEBUG_PRINT("DEBUG: Write permission already granted.\n");
        return -1;
    }
    write_permission = 1;
    DEBUG_PRINT("DEBUG: Write permission granted.\n");
    return 1;
}

// Simulated Write Permission Revoke function
int mdadm_revoke_write_permission(void) {
    if (!write_permission) {
        DEBUG_PRINT("DEBUG: Write permission already revoked.\n");
        return -1;
    }
    write_permission = 0;
    DEBUG_PRINT("DEBUG: Write permission revoked.\n");
    return 1;
}

// Simulated Read function
int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf) {
    if (!mounted) {
        DEBUG_PRINT("DEBUG: Read attempted on unmounted system.\n");
        return -3;
    }
    if (read_len == 0) return 0;
    if (read_buf == NULL) return -4;
    if (read_len > 1024) return -2;
    if (start_addr + read_len > JBOD_NUM_DISKS * JBOD_DISK_SIZE) return -1;

    uint32_t remaining = read_len;
    uint32_t current_addr = start_addr;
    uint8_t *buf_ptr = read_buf;

    while (remaining > 0) {
        uint8_t disk_id = current_addr / JBOD_DISK_SIZE;
        uint8_t block_id = (current_addr % JBOD_DISK_SIZE) / JBOD_BLOCK_SIZE;
        uint32_t block_offset = current_addr % JBOD_BLOCK_SIZE;
        uint32_t to_read = (remaining < (JBOD_BLOCK_SIZE - block_offset)) ? remaining : (JBOD_BLOCK_SIZE - block_offset);

        memcpy(buf_ptr, &simulated_disk[disk_id][block_id * JBOD_BLOCK_SIZE + block_offset], to_read);
        buf_ptr += to_read;
        remaining -= to_read;
        current_addr += to_read;
    }

    return read_len;
}

// Simulated Write function
int mdadm_write(uint32_t start_addr, uint32_t write_len, const uint8_t *write_buf) {
    if (!mounted) {
        DEBUG_PRINT("DEBUG: Write attempted on unmounted system.\n");
        return -3;
    }
    if (!write_permission) {
        DEBUG_PRINT("DEBUG: Write attempted without permission.\n");
        return -5;
    }
    if (write_len == 0) return 0;
    if (write_buf == NULL) return -4;
    if (write_len > 1024) return -2;
    if (start_addr + write_len > JBOD_NUM_DISKS * JBOD_DISK_SIZE) return -1;

    uint32_t remaining = write_len;
    uint32_t current_addr = start_addr;
    const uint8_t *buf_ptr = write_buf;

    while (remaining > 0) {
        uint8_t disk_id = current_addr / JBOD_DISK_SIZE;
        uint8_t block_id = (current_addr % JBOD_DISK_SIZE) / JBOD_BLOCK_SIZE;
        uint32_t block_offset = current_addr % JBOD_BLOCK_SIZE;
        uint32_t to_write = (remaining < (JBOD_BLOCK_SIZE - block_offset)) ? remaining : (JBOD_BLOCK_SIZE - block_offset);

        memcpy(&simulated_disk[disk_id][block_id * JBOD_BLOCK_SIZE + block_offset], buf_ptr, to_write);
        buf_ptr += to_write;
        remaining -= to_write;
        current_addr += to_write;
    }

    return write_len;
}
