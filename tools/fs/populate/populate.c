#include <fs/fat/fatdefs.h>
#include <def/status.h>
#include <def/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define BUILD_DIR "../../../build/"
#define CLUSTER_SIZE 4096
#define CLUSTER_START 3000
#define FEOF 0x0FFFFFF8

#define OPTION 2
#define PATH1  3
#define PATH2  4

#define _NEXT_CLUSTER(cur) \
    (fat->table[cur] & 0x0FFFFFFF)

#define _LBA(clus) \
    (fat->firstDataSector + ((clus - 2) * fat->headers.boot.secPerClus))

#define CHK_FEOF(cluster) \
    (cluster >= FEOF)

#define _SEC(lba) \
    (lba * fat->headers.boot.bytesPerSec)

#define _EXIT(status, f) \
        fclose(f);       \
        return status    \

static FILE *stream;

struct FileDescriptor
{
    struct FAT32DirectoryEntry entry;
    uint32_t cursor;
    uint32_t currentCluster;
};

static char p[PATH_MAX];
char *get(const char *pathname)
{
    memset(p, 0, sizeof(p));
    strcat(p, BUILD_DIR);
    strcat(p, pathname);
    return p;
}

static uint32_t _get_cluster_entry(struct FAT32DirectoryEntry *entry)
{
    return (entry->DIR_FstClusHI << 16) | entry->DIR_FstClusLO;
}

static int _find_free_cluster(struct FAT *fat)
{
    if (!fat || !fat->table)
    {
        return INVALID_ARG;
    }

    uint32_t cluster;
    if (fat->fsInfo.nextFreeCluster >= fat->headers.extended.rootClus && fat->fsInfo.nextFreeCluster < fat->totalClusters)
    {
        cluster = fat->fsInfo.nextFreeCluster;
        for (; cluster < fat->totalClusters; cluster++)
        {
            if ((fat->table[cluster] & 0x0FFFFFFF) == 0)
            {
                return cluster;
            }
        }
    }

    cluster = fat->headers.extended.rootClus;
    for (; cluster < fat->totalClusters; cluster++)
    {
        if ((fat->table[cluster] & 0x0FFFFFFF) == 0)
        {
            return cluster;
        }
    }

    return OUT_OF_BOUNDS;
}

uint32_t _reserve_next_cluster(struct FAT *fat)
{
    uint32_t cluster = _find_free_cluster(fat);
    if (cluster < 0)
    {
        return cluster;
    }

    if (fat->fsInfo.nextFreeCluster != 0xFFFFFFFF)
    {
        fat->fsInfo.nextFreeCluster = cluster + 1;
    }

    if (fat->fsInfo.freeClusterCount != 0xFFFFFFFF)
    {
        fat->fsInfo.freeClusterCount--;
    }

    return cluster;
}

static void _strupper(char *str)
{
    for (int i = 0; str[i]; i++)
    {
        if (str[i] >= 'a' && str[i] <= 'z')
        {
            str[i] -= 32;
        }
    }
}

static void _format_fat_name(const char *filename, char *out)
{
    memset(out, ' ', 11);

    if (!filename || strlen(filename) == 0)
    {
        out[0] = '\0';
        return;
    }

    if (strlen(filename) > 11)
    {
        // If the filename is longer than 11 characters, truncate it
        strncpy(out, filename, 8);

        // Copy extension (last 3 chars after dot, if present)
        const char *dot = strrchr(filename, '.');
        if (dot && *(dot + 1))
        {
            const char *ext = dot + 1;
            size_t extLen = strlen(ext);
            if (extLen > 3)
                ext += extLen - 3;
            strcpy(out + 8, ext);
        }
        else
        {
            memset(out + 8, ' ', 3);
        }

        out[6] = '~'; // Indicate that the name is truncated
        out[7] = '1'; // Add a number to differentiate
        out[11] = '\0';

        _strupper(out);

        return;
    }

    const char *dot = strrchr(filename, '.');
    uint8_t nameLenght = 0;
    uint8_t extLenght = 0;

    for (const char *p = filename; *p && p != dot && nameLenght < 8; p++)
    {
        out[nameLenght++] = (unsigned char)*p;
    }

    if (dot && *(dot + 1))
    {
        const char *ext = dot + 1;
        while (*ext && extLenght < 3)
        {
            out[8 + extLenght++] = (unsigned char)*ext;
            ext++;
        }
    }

    out[11] = '\0';

    _strupper(out);
}

static int _find_entry(struct FAT *fat, char *itemName, uint32_t dirCluster, struct FAT32DirectoryEntry *buff)
{
    struct FAT32DirectoryEntry buffer;
    char filename[12];
    _format_fat_name(itemName, filename);

    uint32_t lba = _LBA(dirCluster);

    fseek(stream, _SEC(lba), SEEK_SET);
    int counter = 0;
    int maxTries = 1000; // Entries
    while (maxTries--)
    {
        fread(&buffer, 1, sizeof(buffer), stream);
        counter++;

        if (buffer.DIR_Name[0] == 0x0)
        {
            return FILE_NOT_FOUND;
        }

        if (buffer.DIR_Name[0] == 0xE5)
        {
            continue;
        }

        if (strncmp((char *)buffer.DIR_Name, filename, 11) == 0)
        {
            memcpy(buff, &buffer, sizeof(struct FAT32DirectoryEntry));
            return (counter * sizeof(struct FAT32DirectoryEntry)) + lba;
        }
    }

    return FILE_NOT_FOUND;
}

static int _traverse_path(struct FAT *fat, const char *path, struct FAT32DirectoryEntry *buff)
{
    char pathCopy[PATH_MAX];
    memset(pathCopy, 0x0, sizeof(pathCopy));
    strncpy(pathCopy, path, PATH_MAX - 1);

    char *token = strtok(pathCopy, "/");
    if (!token)
    {
        return INVALID_ARG;
    }

    int lba = _find_entry(fat, token, fat->rootClus, buff);
    if (lba < 0)
    {
        return FILE_NOT_FOUND;
    }

    while ((token = strtok(NULL, "/")))
    {
        if (buff->DIR_Attr & ATTR_ARCHIVE)
        {
            return NOT_SUPPORTED; // Cannot traverse into a file
        }

        lba = _find_entry(fat, token, _get_cluster_entry(buff), buff);
        if (lba < 0)
        {
            return lba;
        }
    }

    return lba;
}

int _init_fat(struct FAT *fat)
{
    if (!fat)
    {
        return INVALID_ARG;
    }

    memset(fat, 0x0, sizeof(struct FAT));
    int status = SUCCESS;

    fseek(stream, 0, SEEK_SET);
    fread(&fat->headers, 1, sizeof(fat->headers), stream);

    fseek(stream, _SEC(fat->headers.extended.FSInfo), SEEK_SET);
    fread(&fat->fsInfo, 1, sizeof(fat->fsInfo), stream);

    uint32_t fatStartSector = fat->headers.boot.rsvdSecCnt;
    uint32_t fatSize = fat->headers.extended.FATSz32;
    uint32_t fatTotalClusters = fatSize * fat->headers.boot.bytesPerSec / 4;
    uint32_t fatBystes = fatSize * fat->headers.boot.bytesPerSec;

    if (fat->fsInfo.leadSignature != 0x41615252 ||
        fat->fsInfo.structSignature != 0x61417272 ||
        fat->fsInfo.trailSignature != 0xAA550000)
    {
        fprintf(stderr, "Invalid FSInfo Sig or not found!\n");
    }
    else
    {
        if (fat->fsInfo.freeClusterCount == 0xFFFFFFFF)
            fat->fsInfo.freeClusterCount = fatTotalClusters;
        if (fat->fsInfo.nextFreeCluster == 0xFFFFFFFF)
            fat->fsInfo.nextFreeCluster = CLUSTER_START;
    }

    uint32_t *fat_table = (uint32_t *)malloc(fatBystes);
    if (!fat_table)
    {
        status = NO_MEMORY;
        goto err;
    }

    fseek(stream, _SEC(fatStartSector), SEEK_SET);
    fread(fat_table, 1, fatBystes, stream);

    // FAT
    fat->firstDataSector = fatStartSector + (fat->headers.boot.numFATs * fatSize);
    fat->totalClusters = fatTotalClusters;
    fat->table = fat_table;

    // Root
    fat->rootClus = fat->headers.extended.rootClus;

err:
    return status;
}

int update(struct FAT *fat)
{
    uint32_t fatStartSector = fat->headers.boot.rsvdSecCnt;
    uint32_t fatBystes = fat->headers.extended.FATSz32 * fat->headers.boot.bytesPerSec;

    fseek(stream, _SEC(fat->headers.extended.FSInfo), SEEK_SET);
    fwrite(&fat->fsInfo, 1, sizeof(fat->fsInfo), stream);

    fseek(stream, _SEC(fatStartSector), SEEK_SET);
    fwrite(fat->table, 1, fatBystes, stream);

    return SUCCESS;
}

int delete(struct FAT *fat, const char *pathname)
{
    const char *filename = strrchr(pathname, '/') + 1;
    if (strlen(filename) > 11)
    {
        return NOT_SUPPORTED;
    }

    struct FAT32DirectoryEntry entry;
    int lba = _traverse_path(fat, pathname, &entry);
    if (lba != SUCCESS)
    {
        return lba;
    }

    if (entry.DIR_Attr & ATTR_DIRECTORY)
    {
        return OP_ABORTED;
    }

    entry.DIR_Name[0] = 0xE5;
    uint32_t curCluster = _get_cluster_entry(&entry);
    while (!(CHK_FEOF(curCluster)))
    {
        uint32_t next = _NEXT_CLUSTER(curCluster);
        fat->table[curCluster] = 0;
        curCluster = next;
    }

    fseek(stream, _SEC(lba), SEEK_SET);
    fwrite(&entry, 1, sizeof(entry), stream);

    fat->fsInfo.nextFreeCluster = _get_cluster_entry(&entry);

    return update(fat);
}

int create(struct FAT *fat, const char *pathname, int attr)
{
    char pathCopy[PATH_MAX];
    memset(pathCopy, 0x0, sizeof(pathCopy));
    strncpy(pathCopy, pathname, PATH_MAX - 1);

    char *pathpart = strrchr(pathCopy, '/');
    char *filename = pathpart + 1;
    *pathpart = '\0';

    if (strlen(filename) > 11)
        return NOT_SUPPORTED;

    struct FAT32DirectoryEntry buffer;
    memset(&buffer, 0, sizeof(buffer));

    int dirCluster = -1;

    if (_traverse_path(fat, pathname, &buffer) > 0)
    {
        return FILE_EXISTS;
    }

    if (strlen(pathCopy) == 0)
    {
        dirCluster = fat->rootClus;
    }
    else
    {
        int status = _traverse_path(fat, pathCopy, &buffer);
        if (status < 0)
        {
            return status;
        }

        if (buffer.DIR_Attr & ATTR_ARCHIVE)
        {
            return INVALID_ARG;
        }

        dirCluster = _get_cluster_entry(&buffer);
    }

    uint32_t lba = _LBA(dirCluster);
    int counter = -1;

    fseek(stream, _SEC(lba), SEEK_SET);
    while (1)
    {
        fread(&buffer, 1, sizeof(buffer), stream);
        counter++;

        if (buffer.DIR_Name[0] != 0x0 && buffer.DIR_Name[0] != 0xE5)
        {
            continue;
        }

        memset(&buffer, 0x0, sizeof(buffer));
        uint32_t cluster = _reserve_next_cluster(fat);
        fat->table[cluster] = FEOF;

        char name[12];

        _format_fat_name(filename, name);

        memcpy(buffer.DIR_Name, name, 11);
        buffer.DIR_Attr = attr;
        buffer.DIR_FstClusHI = cluster >> 16;
        buffer.DIR_FstClusLO = cluster;

        fseek(stream, -sizeof(buffer), SEEK_CUR);
        fwrite(&buffer, 1, sizeof(buffer), stream);

        return update(fat);
    }

    return ERROR_IO;
}

int read(struct FAT *fat, const char *buffer, uint32_t size, struct FileDescriptor *fd)
{
    return NOT_IMPLEMENTED;
}

int write(struct FAT *fat, const char *buffer, uint32_t size, struct FileDescriptor *fd)
{
    if (!fat || !buffer || size == 0 || !fd)
    {
        return INVALID_ARG;
    }

    if (fd->entry.DIR_Attr & ATTR_DIRECTORY)
    {
        return NOT_SUPPORTED; // Cannot write to a directory
    }

    uint32_t cursor = fd->cursor;
    uint32_t lba = _LBA(fd->currentCluster);

    uint32_t remaining = size;
    uint32_t totalWritten = 0;

    uint32_t clusterOffset = cursor % CLUSTER_SIZE;
    uint32_t bytesLeftInCluster = CLUSTER_SIZE - clusterOffset;

    fseek(stream, _SEC(lba) + clusterOffset, SEEK_SET);
    while(remaining > 0){
        uint32_t toWrite = (remaining < bytesLeftInCluster) ? remaining : bytesLeftInCluster;
        int writed = fwrite((uint8_t*)buffer + totalWritten, 1, toWrite, stream);

        cursor += writed;
        totalWritten += writed;
        remaining -= writed;
        bytesLeftInCluster -= writed;

        if(cursor % CLUSTER_SIZE == 0){
            uint32_t next = _NEXT_CLUSTER(fd->currentCluster);
            if(CHK_FEOF(next)){
                next = _reserve_next_cluster(fat);
                if(next < 0) return next;

                fat->table[fd->currentCluster] = next;
                fat->table[next] = FEOF;
            }

            fd->currentCluster = next;

            clusterOffset = cursor % CLUSTER_SIZE;
            bytesLeftInCluster = CLUSTER_SIZE - clusterOffset;

            lba = _LBA(next);
            fseek(stream, _SEC(lba) + clusterOffset, SEEK_SET);
        }
    }

    return update(fat);
}

int open(struct FAT *fat, const char *pathname, struct FileDescriptor *fd)
{
    int status = _traverse_path(fat, pathname, &fd->entry);
    if (status < 0)
    {
        return status;
    }

    fd->cursor = 0;
    fd->currentCluster = _get_cluster_entry(&fd->entry);

    return SUCCESS;
}

int _ext_fat_copy(struct FAT *fat, const char *extPath, const char *fatPath)
{
    int status = SUCCESS;
    struct FileDescriptor fd;
    memset(&fd, 0x0, sizeof(fd));

    FILE *f = fopen(extPath, "rb");
    if (!f)
    {
        perror("_ext_fat_copy: Error opening file\n");
        return ERROR;
    }

    if ((status = open(fat, fatPath, &fd)) != SUCCESS)
    {
        fprintf(stderr, "Error opening: %s\n", fatPath);
        _EXIT(status, f);
    }

    char buffer[4096];
    while (fread(buffer, 1, sizeof(buffer), f) > 0)
    {
        if ((status = write(fat, buffer, sizeof(buffer), &fd)) != SUCCESS)
        {
            fprintf(stderr, "Error Writing on: %s\n", fatPath);
            _EXIT(status, f);
        }
    }

    _EXIT(SUCCESS, f);
}

void _create_in_list(struct FAT* fat, int attr, const char** paths){
    for (int i = 0; paths[i]; i++)
    {
        printf("%s: %d\n", paths[i], create(fat, paths[i], attr));
    }
    
}

void _auto_populate(struct FAT* fat){
    const char* dirs[] = { "/boot", "/home", "/bin",  NULL };
    const char* files[] = { "/boot/init.bin", "/boot/kernel.bin", NULL };

    _create_in_list(fat, (ATTR_DIRECTORY | ATTR_SYSTEM | ATTR_READ_ONLY), dirs);
    _create_in_list(fat, (ATTR_ARCHIVE | ATTR_SYSTEM | ATTR_READ_ONLY), files);

    char *binInit = "bin/init.bin";
    char *binKernel = "bin/kernel.bin";

    // Copy/Update content(bin/init.bin -> /boot/init.bin; bin/kernel.bin -> /boot/kernel.bin)
    printf("%s -> %s: %d\n", binInit, files[0], _ext_fat_copy(fat, get(binInit), files[0]));
    printf("%s -> %s: %d\n", binKernel, files[1], _ext_fat_copy(fat, get(binKernel), files[1]));
}

void help() {
    printf("Usage: <img> <options> <path> <path>\n");
    printf("Options:\n");
    printf("\t-h: display this message\n");
    printf("\t-cf: create file <path1>\n");
    printf("\t-cd: create directory <path1>\n");
    printf("\t-de: delete entry <path1>\n");
    printf("\t-cp: copy file <path1> (local) to <path2> (FAT)\n");
}

int main(int argc, char** argv)
{
    if(argc == 2 && strcmp(argv[1], "-h") == 0){
        help();
        return 0;
    }

    if(argc <= 2){
        printf("Usage: %s <img> <options> <path> <path>\n", argv[0]);
        printf("Try: %s -h\n", argv[0]);
        return 1;
    }

    struct FAT fat;
    stream = fopen(argv[1], "rb+");
    if (!stream)
    {
        perror("Error opening the img!\n");
        return 1;
    }

    int status = _init_fat(&fat);
    if (status != SUCCESS)
    {
        fprintf(stderr, "Error initalizing FAT: %d\n", status);
        _EXIT(1, stream);
    }

    if(strcmp(argv[OPTION], "-g") == 0){
        _auto_populate(&fat);
    }
    else if(strcmp(argv[OPTION], "-cf") == 0){
        if(!argv[PATH1] || strlen(argv[PATH1]) == 0){
            printf("path: <path1> invalid!\n");
            _EXIT(1, stream);
        }

        int status = create(&fat, argv[PATH1], ATTR_ARCHIVE);
        printf("Create File: %s!\n", status < 0 ? "failed" : "sucess");
    }
    else if(strcmp(argv[OPTION], "-cd") == 0){
        if(!argv[PATH1] || strlen(argv[PATH1]) == 0){
            printf("path: <path1> invalid!\n");
            _EXIT(1, stream);
        }

        int status = create(&fat, argv[PATH1], ATTR_DIRECTORY);
        printf("Create Dir: %s!\n", status < 0 ? "failed" : "sucess");
    }
    else if(strcmp(argv[OPTION], "-de") == 0){
        if(!argv[PATH1] || strlen(argv[PATH1]) == 0){
            printf("path: <path1> invalid!\n");
            _EXIT(1, stream);
        }

        int status = delete(&fat, argv[PATH1]);
        printf("Delete: %s!\n", status < 0 ? "failed" : "sucess");
    }
    else if(strcmp(argv[OPTION], "-cp") == 0){
        if(!argv[PATH1] || strlen(argv[PATH1]) == 0){
            printf("path: <path1> invalid!\n");
            _EXIT(1, stream);
        }

        if(!argv[PATH2] || strlen(argv[PATH2]) == 0){
            printf("path: <path2> invalid!\n");
            _EXIT(1, stream);
        }

        int status = _ext_fat_copy(&fat, argv[PATH1], argv[PATH2]);
        printf("Copy: %s!\n", status < 0 ? "failed" : "success");
        
        _EXIT(status, stream);
    }

    _EXIT(0, stream);
}
