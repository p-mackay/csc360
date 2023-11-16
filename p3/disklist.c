#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Super block 
struct __attribute__((__packed__)) superblock_t {
    uint8_t fs_id [8];
    uint16_t block_size;                //Block size
    uint32_t file_system_block_count;   //Block count
    uint32_t fat_start_block;           //FAT starts
    uint32_t fat_block_count;           //FAT blocks
    uint32_t root_dir_start_block;      //Root directory starts
    uint32_t root_dir_block_count;      //Root directory blocks
};

// Time and date entry
struct __attribute__((__packed__)) dir_entry_timedate_t { 
    uint16_t year;
    uint8_t month; 
    uint8_t day; 
    uint8_t hour; 
    uint8_t minute; 
    uint8_t second;
};

// Directory entry
struct __attribute__((__packed__)) dir_entry_t {
    uint8_t         status; 
    uint32_t        starting_block;  
    uint32_t        block_count; 
    uint32_t        size;
    struct dir_entry_timedate_t create_time; 
    struct dir_entry_timedate_t modify_time;
    uint8_t         filename[31];  
    uint8_t         unused[6];
};

uint16_t swap_endian_16(uint16_t value) {
    return (value >> 8) | (value << 8);
}

uint32_t swap_endian_32(uint32_t value) {
    return ((value >> 24) & 0xff) |
    ((value << 8) & 0xff0000) |
    ((value >> 8) & 0xff00) |
    ((value << 24) & 0xff000000);
}

void list_directory_contents(FILE *file, uint32_t start_block, uint32_t block_size, uint32_t dir_size) {
    uint32_t dir_pos = start_block * block_size;

    // Read directory entries
    struct dir_entry_t *entries = malloc(dir_size);
    if (!entries) {
        perror("Error allocating memory for directory entries");
        return;
    }

    fseek(file, dir_pos, SEEK_SET);
    fread(entries, dir_size, 1, file);

    // Iterate over each directory entry
    for (size_t i = 0; i < dir_size / sizeof(struct dir_entry_t); ++i) {
        struct dir_entry_t *entry = &entries[i];

        // Check if the entry is valid
        if (entry->status & 0x01) {
            char type = 'U'; // Default to 'Unknown' type
            if ((entry->status & 0x02) && !(entry->status & 0x04)) {
                type = 'F'; // It's a file
            } else if (entry->status & 0x04) {
                type = 'D'; // It's a directory
            }

            if (type == 'F' || type == 'D') {
                char datetime[20];
                sprintf(datetime, "%04u/%02u/%02u %02u:%02u:%02u",
                        swap_endian_16(entry->create_time.year),
                        entry->create_time.month,
                        entry->create_time.day,
                        entry->create_time.hour,
                        entry->create_time.minute,
                        entry->create_time.second);

                // Print the formatted entry
                printf("%c %10u %30s %s\n",
                       type,
                       swap_endian_32(entry->size),
                       entry->filename,
                       datetime);
            }
        }
    }

    free(entries);
}

int get_subdir_starting_block(FILE* file, const struct superblock_t* sb, const char* subdir_name, uint32_t parent_dir_start_block, uint32_t parent_dir_block_count, uint32_t* starting_block, uint32_t* block_count) {
    uint32_t parent_dir_pos = parent_dir_start_block * sb->block_size;
    uint32_t parent_dir_size = parent_dir_block_count * sb->block_size;

    // Allocate memory for parent directory entries
    struct dir_entry_t *entries = malloc(parent_dir_size);
    if (!entries) {
        perror("Error allocating memory for directory entries");
        return -1;
    }

    fseek(file, parent_dir_pos, SEEK_SET);
    fread(entries, parent_dir_size, 1, file);

    int found = 0;
    for (size_t i = 0; i < parent_dir_size / sizeof(struct dir_entry_t); ++i) {
        struct dir_entry_t *entry = &entries[i];

        // Check if the entry is valid and is a directory
        if ((entry->status & 0x01) && (entry->status & 0x04)) {
            char temp_filename[32]; // One extra byte for the null terminator
            strncpy(temp_filename, (const char *)entry->filename, 31);
            temp_filename[31] = '\0'; // Ensure null termination
            // Check if the name matches the subdirectory name
            if (strncmp(temp_filename, subdir_name, strlen(subdir_name)) == 0) {
                *starting_block = swap_endian_32(entry->starting_block);
                *block_count = swap_endian_32(entry->block_count);
                found = 1;
                break;  // Exit the loop once the subdirectory is found
            }
        }
    }

    free(entries); // Free the allocated memory

    return found ? 0 : -1; // Return 0 if found, -1 if not found
}


int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
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
    // =======================================================================
    struct superblock_t sb;
    fread(&sb, sizeof(struct superblock_t), 1, file);

    sb.block_size = swap_endian_16(sb.block_size);
    sb.file_system_block_count = swap_endian_32(sb.file_system_block_count);
    sb.fat_start_block = swap_endian_32(sb.fat_start_block);
    sb.fat_block_count = swap_endian_32(sb.fat_block_count);
    sb.root_dir_start_block = swap_endian_32(sb.root_dir_start_block);
    sb.root_dir_block_count = swap_endian_32(sb.root_dir_block_count);
    // Print the superblock information
    printf("Super block information\n");
    printf("Block size: %u\n", sb.block_size);
    printf("Block count: %u\n", sb.file_system_block_count);
    printf("FAT starts: %u\n", sb.fat_start_block);
    printf("FAT blocks: %u\n", sb.fat_block_count);
    printf("Root directory starts: %u\n", sb.root_dir_start_block);
    printf("Root directory blocks: %u\n", sb.root_dir_block_count);

    // Calculate the size of the FAT section
    uint32_t fat_size = sb.fat_block_count * sb.block_size;

    // Allocate memory for FAT
    uint32_t *fat = malloc(fat_size);
    if (!fat) {
        perror("Error allocating memory for FAT");
        return 1;
    }

    // Read the FAT
    fseek(file, sb.fat_start_block * sb.block_size, SEEK_SET);
    fread(fat, fat_size, 1, file);

    // Count free, reserved, and allocated blocks
    uint32_t free_blocks = 0, reserved_blocks = 0, allocated_blocks = 0;
    for (uint32_t i = 0; i < fat_size / sizeof(uint32_t); i++) {
        if (swap_endian_32(fat[i]) == 0) { // Assuming '0' indicates a free block
            free_blocks++;
        } else if (swap_endian_32(fat[i]) == 1) { // Assuming '1' indicates a reserved block
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
    // =======================================================================

    printf("\n");

    // Read entries of root and subdirectories
    // =======================================================================

    if (argc == 2){
        list_directory_contents(file, sb.root_dir_start_block, sb.block_size, sb.root_dir_block_count * sb.block_size);
    } else if (argc == 3){
        char *subdir_name = argv[2];
        
        if (subdir_name[0] == '/') {
            subdir_name++;  // Increment pointer to skip the leading slash
        }

        uint32_t subdir_start_block;
        uint32_t subdir_block_count; // You need to know the block count for subdir1

        if (get_subdir_starting_block(file, &sb, subdir_name, sb.root_dir_start_block, sb.root_dir_block_count, &subdir_start_block, &subdir_block_count) == 0) {
            list_directory_contents(file, subdir_start_block, sb.block_size, subdir_block_count * sb.block_size);
        } else {
            printf("Subdirectory '%s' not found\n", argv[2]);
        }

    }



    /*
    // List the contents of the root directory
    list_directory_contents(file, sb.root_dir_start_block, sb.block_size, sb.root_dir_block_count * sb.block_size);
    */

    // =======================================================================




    fclose(file);
    return 0;
}

