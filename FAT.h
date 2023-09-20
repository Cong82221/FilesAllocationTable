#ifndef _FAT_H_
#define _FAT_H_

/*******************************************************************************
* Include
*******************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
* Define
*******************************************************************************/
#define SIZE_OF_NAME         11U
#define FIRST_CLUSTER        2U
#define SIZE_ROOT_ENTRY      32U

#define FAT_12               12U
#define FAT_16               16U
#define FAT_32               32U

#define FAT12_CLUST_COUNT    4085U
#define FAT16_CLUST_COUNT    65525U
#define FAT32_CLUST_COUNT    4294967285U

#define SHIFT_24_BIT         24U
#define SHIFT_16_BIT         16U
#define SHIFT_11_BIT         11U
#define SHIFT_9_BIT          9U
#define SHIFT_8_BIT          8U
#define SHIFT_5_BIT          5U
#define SHIFT_4_BIT          4U
#define SHIFT_1_BIT          1U

#define SET_YEAR             1980U
#define MASK_DAY             0x1FU
#define MASK_MONTH           0x0FU
#define MASK_MINUTE          0x3FU
#define MASK_SECOND          0x1FU

#define ATTR_FILE            0x00U
#define ATTR_READ_ONLY       0x01U
#define ATTR_HIDDEN          0x02U
#define ATTR_SYSTEM          0x04U
#define ATTR_LONG_FILE_NAME  0X0FU
#define ATTR_VOLUME_ID       0X08U
#define ATTR_DIRECTORY       0X10U
#define ATTR_ARCHIVE         0X20U

#define INVALID_FILE_NAME    0x00U
#define DELETED_FILE_NAME    0xE5U
#define LAST_CRUSTER_12      0XFF8U
#define LAST_CRUSTER_16      0xFFF8U
#define LAST_CRUSTER_32      0xFFFFFF8U

typedef struct
{
    uint32_t    bytsPerSec;              /* Size of Sector. */
    uint16_t    secPerClus;              /* Size of Cluster. */
    uint16_t    rsvdSecCnt;              /* Number of Sector in Reserved Sector Region. */
    uint8_t     numFATs;                 /* Number of FAT table in FAT Region. */
    uint16_t    rootEntCnt;              /* Max directory entries in root directory. */
    uint16_t    FATsz;                   /* Number of sector in FAT table. */
    uint32_t    totalSector;             /* The total number of sector */

    /* Only FAT32 */
    uint16_t    rootClus;                /* The location of the first sector in the directory region */
} t_bootSector;

typedef struct
{
    uint8_t     fileName[SIZE_OF_NAME];  /* File Name */
    uint8_t     attributes;              /* Holds the attributes code */
    uint16_t    writeTime;               /* Time of last write */
    uint16_t    writeDate;               /* Date of last write */
    uint32_t    startCluster;            /* Pointer to the first cluster of the file */
    uint32_t    fileSize;                /* File size in bytes */
} t_direcroryEntry;

typedef struct entry
{
    t_direcroryEntry entry;
    struct entry *next;
} t_entryList;

typedef t_entryList* p_EntryList;

typedef struct
{
    uint16_t FATStartSector;
    uint16_t sectorInFAT;
    uint16_t rootDirStartSector;
    uint16_t sectorInRootDir;
    uint16_t dataStartSector;
} t_location;

/*******************************************************************************
* API
*******************************************************************************/

/**
 * Name: initFileFAT
 * @brief Initializes the file system and reads the boot sector information from a disk file.
 *
 * @param filePath: The path to the disk file.
 * @param fp: A pointer to the opened file.
 *
 * @return t_bootSector: The boot sector information read from the disk file.
 */
t_bootSector initFileFAT(const char *filePath, FILE *fp);

/**
 * Name: localEachRegion
 * @brief Calculate the starting sectors and the number of sectors by regions in the file system.
 *
 * @return A `t_location` structure containing information about regions.
 */
t_location localEachRegion(void);

/**
 * Name: readDirEntry
 * @brief Read directory entries from the specified start entry in the file system
 *        and create linked list nodes for valid entries.
 *
 * @param head: A pointer to the head of the linked list.
 * @param startEntry: The start entry to read directory entries from.
 */
void readDirEntry(p_EntryList *head, uint32_t startEntry);

/**
 * Name: loadDirEntry
 * @brief Load directory entries from the specified start cluster in the file system
 *        and update the linked list with the new entries.
 *
 * @param head: A pointer to the head of the linked list.
 * @param startCluster: The start cluster to read directory entries from.
 */
void loadDirEntry(p_EntryList *head, uint32_t startCluster);

/**
 * Name: loadFile
 * @brief Load the contents of a file starting from the specified start cluster in the file system
 *        and store the data in the provided buffer.
 *
 * @param buff: A pointer to the buffer where the file data will be stored.
 * @param startCluster: The start cluster of the file.
 */
void loadFile(uint8_t *buff, uint32_t startCluster);

/**
 * Name: fatType
 * @brief Determine the FAT type based on the total number of clusters.
 *
 * @return The FAT type: FAT_12, FAT_16, or FAT_32.
 */
uint8_t fatType();

#endif /* _FAT_H_ */

