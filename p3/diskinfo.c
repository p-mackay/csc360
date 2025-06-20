#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <netinet/in.h>

// Define a structure for the superblock
struct __attribute__((__packed__)) superblock_t {
    uint8_t fs_id [8];
    uint16_t block_size;                //Block size
    uint32_t file_system_block_count;   //Block count
    uint32_t fat_start_block;           //FAT starts
    uint32_t fat_block_count;           //FAT blocks
    uint32_t root_dir_start_block;      //Root directory starts
    uint32_t root_dir_block_count;      //Root directory blocks
};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file system image>\n", argv[0]);
        return 1;
    }
    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        perror("Error opening file");
        return 1;
    }
    //fseek(file, 8, SEEK_SET);

    // Read the superblock
    struct superblock_t sb;
    fread(&sb, sizeof(struct superblock_t), 1, file);

    sb.block_size = htons(sb.block_size);
    sb.file_system_block_count = htonl(sb.file_system_block_count);
    sb.fat_start_block = htonl(sb.fat_start_block);
    sb.fat_block_count = htonl(sb.fat_block_count);
    sb.root_dir_start_block = htonl(sb.root_dir_start_block);
    sb.root_dir_block_count = htonl(sb.root_dir_block_count);
    fclose(file);
    // Print the superblock information
    printf("Super block information\n");
    printf("Block size: %u\n", sb.block_size);
    printf("Block count: %u\n", sb.file_system_block_count);
    printf("FAT starts: %u\n", sb.fat_start_block);
    printf("FAT blocks: %u\n", sb.fat_block_count);
    printf("Root directory starts: %u\n", sb.root_dir_start_block);
    printf("Root directory blocks: %u\n", sb.root_dir_block_count);


   file = fopen(argv[1], "rb");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    // Calculate the size of the FAT section
    uint32_t fat_size = sb.fat_block_count * sb.block_size;

    // Allocate memory for FAT
    uint32_t *fat = malloc(fat_size);
    if (!fat) {
        perror("Error allocating memory for FAT");
        fclose(file);
        return 1;
    }

    // Read the FAT
    fseek(file, sb.fat_start_block * sb.block_size, SEEK_SET);
    fread(fat, fat_size, 1, file);
    fclose(file);

    // Count free, reserved, and allocated blocks
    uint32_t free_blocks = 0, reserved_blocks = 0, allocated_blocks = 0;
    for (uint32_t i = 0; i < fat_size / sizeof(uint32_t); i++) {
        if (htonl(fat[i]) == 0) { 
            free_blocks++;
        } else if (htonl(fat[i]) == 1) { 
            reserved_blocks++;
        } else {
            allocated_blocks++;
        }
    }

    // Display FAT information
    printf("\n");
    printf("FAT information\n");
    printf("Free blocks: %u\n", free_blocks);
    printf("Reserved blocks: %u\n", reserved_blocks);
    printf("Allocated blocks: %u\n", allocated_blocks);

    free(fat); // Free the allocated memory 


    return 0;
}

