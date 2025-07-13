#ifndef _FAT32_INTERNAL_H
#define _FAT32_INTERNAL_H

#define CLUSTER_SIZE 4096
#define EOF 0x0FFFFFF8

#define ILEGAL_CHARS "+,;=[]"

#define CHK_EOF(cluster) \
	(cluster >= EOF)

#define _SEC(lba) \
    (lba * SECTOR_SIZE)

#define _CLUSTER(ClusHI, ClusLO) \
    ((ClusHI << 16) | ClusLO)

// Utils
uint8_t _fat32_fsinfo_sig_valid(struct FAT32FSInfo* fsInfo);
uint32_t _fat32_cluster_to_lba(struct FAT* fat, uint32_t cluster);
uint32_t _fat32_next_cluster(struct FAT* fat, uint32_t current);
uint32_t _fat32_reserve_next_cluster(struct FAT* fat);

// IO
int8_t _fat32_update(struct FAT* fat);

// Dir
int8_t _fat32_traverse_path(struct FAT *fat, const char *path, struct FAT32DirectoryEntry *buff);
int8_t _fat32_create_entry(struct FAT *fat, const char *pathname, int attr);
int8_t _fat32_save_entry(struct FAT* fat, struct FAT32DirectoryEntry* entry, uint32_t dirCluster);
int8_t _fat32_remove_entry(struct FAT *fat, const char *pathname);

// File
int64_t _fat32_get_entry_lba(struct FAT* fat, struct FAT32DirectoryEntry* entry, uint32_t dirCluster);
int8_t _fat32_truncate_entry(struct FAT* fat, struct FAT32DirectoryEntry* entry);
int8_t _fat32_free_chain(struct FAT* fat, uint32_t start);

// Name Generation
void _fat32_append_tilde(char *outname, int n);
void _fat32_generate_short_name(const char *name, char *out);

#endif