#include "fat32.h"
#include "printk.h"
#include "virtio.h"
#include "string.h"
#include "mbr.h"
#include "mm.h"
#include "printk.h"

struct fat32_bpb fat32_header;
struct fat32_volume fat32_volume;

uint8_t fat32_buf[VIRTIO_BLK_SECTOR_SIZE];
uint8_t fat32_table_buf[VIRTIO_BLK_SECTOR_SIZE];

uint64_t cluster_to_sector(uint64_t cluster) {
    return (cluster - 2) * fat32_volume.sec_per_cluster + fat32_volume.first_data_sec;
}

uint32_t next_cluster(uint64_t cluster) {
    uint64_t fat_offset = cluster * 4;
    uint64_t fat_sector = fat32_volume.first_fat_sec + fat_offset / VIRTIO_BLK_SECTOR_SIZE;
    virtio_blk_read_sector(fat_sector, fat32_table_buf);
    int index_in_sector = fat_offset % (VIRTIO_BLK_SECTOR_SIZE / sizeof(uint32_t));
    return *(uint32_t*)(fat32_table_buf + index_in_sector);
}

void fat32_init(uint64_t lba, uint64_t size) {
    virtio_blk_read_sector(lba, (void*)&fat32_header);
    fat32_volume.first_fat_sec = lba + fat32_header.rsvd_sec_cnt;
    fat32_volume.sec_per_cluster = fat32_header.sec_per_clus;
    fat32_volume.first_data_sec = fat32_volume.first_fat_sec + fat32_header.num_fats * fat32_header.fat_sz32;
    fat32_volume.fat_sz = fat32_header.fat_sz32;
}

int is_fat32(uint64_t lba) {
    virtio_blk_read_sector(lba, (void*)&fat32_header);
    if (fat32_header.boot_sector_signature != 0xaa55) {
        return 0;
    }
    return 1;
}

int next_slash(const char* path) {  // util function to be used in fat32_open_file
    int i = 0;
    while (path[i] != '\0' && path[i] != '/') {
        i++;
    }
    if (path[i] == '\0') {
        return -1;
    }
    return i;
}

void to_upper_case(char *str) {     // util function to be used in fat32_open_file
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] -= 32;
        }
    }
}

struct fat32_file fat32_open_file(const char *path) {
    struct fat32_file file;
    /* todo: open the file according to path */
    
    file.cluster = 0;
    file.dir.cluster = 0;
    file.dir.index = 0;

    const char *filename = path + 7;

    char upper_filename[MAX_PATH_LENGTH];
    strcpy(upper_filename, filename);
    to_upper_case(upper_filename);

    uint64_t root_cluster = fat32_header.root_clus;
    uint64_t current_cluster = root_cluster;

    while (current_cluster < 0x0FFFFFF8) {
        uint64_t sector = cluster_to_sector(current_cluster);
        
        for (int sec = 0; sec < fat32_volume.sec_per_cluster; sec++) {
            virtio_blk_read_sector(sector + sec, fat32_buf);
            
            struct fat32_dir_entry *entries = (struct fat32_dir_entry *)fat32_buf;
            int entries_per_sector = VIRTIO_BLK_SECTOR_SIZE / sizeof(struct fat32_dir_entry);
            
            for (int i = 0; i < entries_per_sector; i++) {
                if (entries[i].name[0] == 0) {
                    // End of directory
                    return file;
                }
                if (entries[i].name[0] == 0xE5) {
                    // Deleted entry, skip
                    continue;
                }
                if (entries[i].attr & 0x08) {
                    // Volume label, skip
                    continue;
                }
                if (entries[i].attr & 0x10) {
                    // Directory, skip
                    continue;
                }
                
                char entry_name[12];
                memcpy(entry_name, entries[i].name, 11);
                entry_name[11] = '\0';
                
                char formatted_name[13];
                int j = 0;
                for (int k = 0; k < 8 && entries[i].name[k] != ' '; k++) {
                    formatted_name[j++] = entries[i].name[k];
                }
                if (entries[i].name[8] != ' ') {
                    formatted_name[j++] = '.';
                    for (int k = 8; k < 11 && entries[i].name[k] != ' '; k++) {
                        formatted_name[j++] = entries[i].name[k];
                    }
                }
                formatted_name[j] = '\0';
                
                if (strcmp(formatted_name, upper_filename) == 0) {
                    file.cluster = ((uint32_t)entries[i].starthi << 16) | entries[i].startlow;
                    file.dir.cluster = current_cluster;
                    file.dir.index = i;
                    return file;
                }
            }
        }
        
        current_cluster = next_cluster(current_cluster);
    }
    return file;
}

int64_t fat32_lseek(struct file* file, int64_t offset, uint64_t whence) {
    if (whence == SEEK_SET) {
        file->cfo = offset;
    } else if (whence == SEEK_CUR) {
        file->cfo += offset;
    } else if (whence == SEEK_END) {
        struct fat32_file *fat32_file = &file->fat32_file;
        uint64_t dir_sector = cluster_to_sector(fat32_file->dir.cluster);
        virtio_blk_read_sector(dir_sector, fat32_buf);
        struct fat32_dir_entry *dir_entries = (struct fat32_dir_entry *)fat32_buf;
        uint32_t file_size = dir_entries[fat32_file->dir.index].size;
        file->cfo = file_size + offset;
    } else {
        printk("fat32_lseek: whence not implemented\n");
        while (1);
    }
    return file->cfo;
}

uint64_t fat32_table_sector_of_cluster(uint32_t cluster) {
    return fat32_volume.first_fat_sec + cluster / (VIRTIO_BLK_SECTOR_SIZE / sizeof(uint32_t));
}

int64_t fat32_read(struct file* file, void* buf, uint64_t len) {
    /* todo: read content to buf, and return read length */
    struct fat32_file *fat32_file = &file->fat32_file;
    uint64_t bytes_read = 0;
    uint64_t remaining = len;
    uint32_t current_cluster = fat32_file->cluster;
    uint64_t file_offset = file->cfo;
    uint64_t cluster_size = fat32_volume.sec_per_cluster * VIRTIO_BLK_SECTOR_SIZE;

    uint64_t dir_sector = cluster_to_sector(fat32_file->dir.cluster);
    virtio_blk_read_sector(dir_sector, fat32_buf);
    struct fat32_dir_entry *dir_entries = (struct fat32_dir_entry *)fat32_buf;
    uint32_t file_size = dir_entries[fat32_file->dir.index].size;
    
    if (file_offset >= file_size) {
        return 0;
    }
    if (file_offset + len > file_size) {
        remaining = file_size - file_offset;
        len = remaining;
    }

    uint64_t clusters_to_skip = file_offset / cluster_size;
    for (uint64_t i = 0; i < clusters_to_skip && current_cluster < 0x0FFFFFF8; i++) {
        current_cluster = next_cluster(current_cluster);
    }

    uint64_t offset_in_cluster = file_offset % cluster_size;

    while (remaining > 0 && current_cluster < 0x0FFFFFF8) {
        uint64_t sector = cluster_to_sector(current_cluster);
        uint64_t sector_offset = offset_in_cluster / VIRTIO_BLK_SECTOR_SIZE;
        uint64_t byte_offset_in_sector = offset_in_cluster % VIRTIO_BLK_SECTOR_SIZE;
        
        for (uint64_t sec = sector_offset; sec < fat32_volume.sec_per_cluster && remaining > 0; sec++) {
            virtio_blk_read_sector(sector + sec, fat32_buf);
            
            uint64_t bytes_to_copy = VIRTIO_BLK_SECTOR_SIZE - byte_offset_in_sector;
            if (bytes_to_copy > remaining) {
                bytes_to_copy = remaining;
            }
            
            memcpy((uint8_t*)buf + bytes_read, fat32_buf + byte_offset_in_sector, bytes_to_copy);
            bytes_read += bytes_to_copy;
            remaining -= bytes_to_copy;
            byte_offset_in_sector = 0;
        }
        
        offset_in_cluster = 0;
        current_cluster = next_cluster(current_cluster);
    }

    file->cfo += bytes_read;
    return bytes_read;
}

int64_t fat32_write(struct file* file, const void* buf, uint64_t len) {
    /* todo: fat32_write */
    struct fat32_file *fat32_file = &file->fat32_file;
    uint64_t bytes_written = 0;
    uint64_t remaining = len;
    uint32_t current_cluster = fat32_file->cluster;
    uint64_t file_offset = file->cfo;
    uint64_t cluster_size = fat32_volume.sec_per_cluster * VIRTIO_BLK_SECTOR_SIZE;

    uint64_t dir_sector = cluster_to_sector(fat32_file->dir.cluster);
    virtio_blk_read_sector(dir_sector, fat32_buf);
    struct fat32_dir_entry *dir_entries = (struct fat32_dir_entry *)fat32_buf;
    uint32_t file_size = dir_entries[fat32_file->dir.index].size;
    
    if (file_offset >= file_size) {
        return 0;
    }
    if (file_offset + len > file_size) {
        remaining = file_size - file_offset;
        len = remaining;
    }

    uint64_t clusters_to_skip = file_offset / cluster_size;
    for (uint64_t i = 0; i < clusters_to_skip && current_cluster < 0x0FFFFFF8; i++) {
        current_cluster = next_cluster(current_cluster);
    }

    uint64_t offset_in_cluster = file_offset % cluster_size;

    while (remaining > 0 && current_cluster < 0x0FFFFFF8) {
        uint64_t sector = cluster_to_sector(current_cluster);
        uint64_t sector_offset = offset_in_cluster / VIRTIO_BLK_SECTOR_SIZE;
        uint64_t byte_offset_in_sector = offset_in_cluster % VIRTIO_BLK_SECTOR_SIZE;
        
        for (uint64_t sec = sector_offset; sec < fat32_volume.sec_per_cluster && remaining > 0; sec++) {
            virtio_blk_read_sector(sector + sec, fat32_buf);
            
            uint64_t bytes_to_copy = VIRTIO_BLK_SECTOR_SIZE - byte_offset_in_sector;
            if (bytes_to_copy > remaining) {
                bytes_to_copy = remaining;
            }

            memcpy(fat32_buf + byte_offset_in_sector, (uint8_t*)buf + bytes_written, bytes_to_copy);
            virtio_blk_write_sector(sector + sec, fat32_buf);
            bytes_written += bytes_to_copy;
            remaining -= bytes_to_copy;
            byte_offset_in_sector = 0;
        }
        
        offset_in_cluster = 0;
        current_cluster = next_cluster(current_cluster);
    }

    file->cfo += bytes_written;
    return bytes_written;
}