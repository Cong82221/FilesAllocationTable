#include "FAT.h"
#include "HAL.h"

/*******************************************************************************
* Define
*******************************************************************************/
#define LITTLE_ENDIAN(X, Y)  (((uint32_t)(Y) << SHIFT_8_BIT) | (X))

/*******************************************************************************
* Variables
*******************************************************************************/
static t_bootSector s_bootInfo;
static t_location s_local;

/*******************************************************************************
* Prototypes
*******************************************************************************/

/**
 * Name: deleteList
 * @brief Free the memory used by a linked list.
 *
 * @param head Pointer to the head pointer of the linked list.
 *
 * @return void.
 */
static void deleteList(p_EntryList *head);

/**
 * Name: nextCluster
 * @brief Retrieves the value of the next cluster in the File Allocation Table (FAT) for a given cluster.
 *
 * @param cluster The cluster number for which to find the next cluster.
 *
 * @return The value of the next cluster in the FAT for the given cluster.
 */
static uint32_t nextCluster(uint32_t cluster);

/**
 * Name: createNewNodeAsDirEntry
 * @brief Creates a new node representing a directory entry and copies the information from the buffer.
 *
 * @param buff Pointer to a buffer containing the data of a directory entry to be copied.
 *
 * @return Pointer to the newly created node representing the directory entry.
 *         If memory allocation fails, NULL is returned.
 */
static p_EntryList createNewNodeAsDirEntry(const uint8_t *buff);

/**
 * Name: fatType
 * @brief Determine the FAT type based on the total number of clusters.
 *
 * @return The FAT type: FAT_12, FAT_16, or FAT_32.
 */
uint8_t fatType();

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

/*******************************************************************************
* Code
*******************************************************************************/
static void deleteList(p_EntryList *head)
{
    p_EntryList current = NULL;
    p_EntryList next = NULL;

    current = *head;

    /* Free the allocated memory until null pointer is encountered */
    while(current != NULL)
    {
        next = current->next;
        free(current);
        current = next; /* Move to the next node in the list */
    }

    *head = NULL;
}

t_bootSector initFileFAT(const char *filePath, FILE *fp)
{
    uint8_t *buff = NULL;
    uint32_t totalSec = 0;
    uint32_t fatSize = 0;
    uint8_t thisFatType = 0;

    fp = HAL_Init(filePath);

    /* Allocate some memory */
    buff = (uint8_t*)malloc(BYTE_PER_SECTOR * sizeof(uint8_t));

    if(buff == NULL)
    {
        printf("The disk is empty.\n");
    }
    /* Read the boot sector from the disk */
    else if(HAL_ReadSector(0, buff) != BYTE_PER_SECTOR)
    {
        printf("Read boot region error.\n");
    }
    else
    {
        /* Store the information of the boot sector */
        s_bootInfo.bytsPerSec = LITTLE_ENDIAN(buff[0x0B], buff[0x0C]); /* Bytes per sector */
        s_bootInfo.secPerClus = buff[0x0D];                            /* Sectors per cluster */
        s_bootInfo.rsvdSecCnt = LITTLE_ENDIAN(buff[0x0E], buff[0x0F]); /* Reserved sector count */
        s_bootInfo.numFATs = buff[0x10];                               /* Number of FATs */
        s_bootInfo.rootEntCnt = LITTLE_ENDIAN(buff[0x11], buff[0x12]); /* Root directory entry count */
        fatSize = LITTLE_ENDIAN(buff[0x16], buff[0x17]);               /* FAT size in sectors */
        totalSec = LITTLE_ENDIAN(buff[0x13], buff[0x14]);              /* Total number of sectors in the file system */

        /* If totalSec or fatSize is 0, read extended fields to get the actual values */
        if(totalSec == 0)
        {
            totalSec = (uint32_t)(buff[0x23] << SHIFT_24_BIT)
                        | (uint32_t)(buff[0x22] << SHIFT_16_BIT)
                        | (uint32_t)(buff[0x21] << SHIFT_8_BIT)
                        | buff[0x20];
        }

        if(fatSize == 0)
        {
            fatSize = (uint32_t)(buff[0x27] << SHIFT_24_BIT)
                    | (uint32_t)(buff[0x26] << SHIFT_16_BIT)
                    | (uint32_t)(buff[0x25] << SHIFT_8_BIT)
                    | buff[0x24];
        }

        s_bootInfo.FATsz = fatSize;
        s_bootInfo.totalSector = totalSec;

        /* Check the FAT type and set the root cluster for FAT_32 */
        thisFatType = fatType();
        if(thisFatType == FAT_32)
        {
            s_bootInfo.rootClus = (uint32_t)(buff[0x2F] << SHIFT_24_BIT)
                                | (uint32_t)(buff[0x2E] << SHIFT_16_BIT)
                                | (uint32_t)(buff[0x2D] << SHIFT_8_BIT)
                                | buff[0x2C];
        }

        free(buff);

        /* Update the number of byte in a sector */
        HAL_Update(s_bootInfo.bytsPerSec);
    }

    return s_bootInfo;
}

uint8_t fatType()
{
    uint8_t fatType = 0;
    uint32_t totalClusters = 0;

    /* The total number of clusters */
    totalClusters = s_bootInfo.totalSector/s_bootInfo.secPerClus;

    /* Check the total number of clusters to determine the FAT type */
    if (totalClusters < FAT12_CLUST_COUNT)
    {
        fatType = FAT_12;
    }
    else
    {
        if(totalClusters < FAT16_CLUST_COUNT)
        {
            fatType = FAT_16;
        }
        else
        {
            fatType = FAT_32;
        }

    }

    return fatType;
}

t_location localEachRegion(void)
{
    /* FAT Region */
    s_local.FATStartSector = s_bootInfo.rsvdSecCnt;
    s_local.sectorInFAT = s_bootInfo.FATsz * s_bootInfo.numFATs;
    /* Root Directory Region */
    s_local.rootDirStartSector = s_local.FATStartSector + s_local.sectorInFAT;
    s_local.sectorInRootDir = ((SIZE_ROOT_ENTRY * s_bootInfo.rootEntCnt) +
                               (s_bootInfo.bytsPerSec -1)) / s_bootInfo.bytsPerSec;
    /* Data Region */
    s_local.dataStartSector = s_local.rootDirStartSector + s_local.sectorInRootDir;

    return s_local;
}

static uint32_t nextCluster(uint32_t cluster)
{
    uint8_t *fatSector = NULL;
    uint16_t thisFATOffset = 0;
    uint32_t thisEntryVal = 0;
    uint8_t thisFatType = 0;
    thisFatType = fatType();

    /* Allocate some memory */
    fatSector = (uint8_t*)malloc(s_bootInfo.FATsz * s_bootInfo.bytsPerSec * sizeof(uint8_t));

    if(fatSector == NULL)
    {
        printf("The disk is empty.\n");
    }
    else if(HAL_ReadMultiSector(s_bootInfo.rsvdSecCnt, s_bootInfo.FATsz, fatSector) != s_bootInfo.FATsz * s_bootInfo.bytsPerSec)
    {
        printf("Read FAT Region error.\n");
        free(fatSector);
    }
    else
    {
        switch(thisFatType)
        {
        case FAT_12:
            /* Multiply the cluster by 1.5 to get the FAT position */
            thisFATOffset = cluster + (cluster >> 1);
            /* If the cluster that index is odd */
            if (cluster & 1)
            {
                thisEntryVal = (fatSector[thisFATOffset] >> SHIFT_4_BIT)
                            | ((uint32_t)fatSector[thisFATOffset + 1] << SHIFT_4_BIT);
            }
            /* If the cluster that index is even */
            else
            {
                thisEntryVal = (fatSector[thisFATOffset])
                            | ((uint32_t)(fatSector[thisFATOffset + 1] & 0x0F) << SHIFT_8_BIT);
            }

            break;
        case FAT_16:
            /* Multiply the cluster by 2 to get the FAT position */
            thisFATOffset = cluster * 2;

            thisEntryVal = (uint32_t)(fatSector[thisFATOffset]) | ((uint32_t)fatSector[thisFATOffset + 1] << SHIFT_8_BIT);

            break;
        case FAT_32:
            /* Multiply the cluster by 4 to get the FAT position */
            thisFATOffset = cluster * 4;

            thisEntryVal = (uint32_t)(fatSector[thisFATOffset])
                        | ((uint32_t)fatSector[thisFATOffset + 1] << SHIFT_8_BIT)
                        | ((uint32_t)fatSector[thisFATOffset + 2] << SHIFT_16_BIT)
                        | ((uint32_t)(fatSector[thisFATOffset + 3] & 0x0F) << SHIFT_24_BIT);

            break;
        default:
            break;
        }

        free(fatSector);
        
    }

    return thisEntryVal;
}

static p_EntryList createNewNodeAsDirEntry(const uint8_t *buff)
{
    uint16_t index = 0;
    p_EntryList newNode = NULL;

    /* Allocate some memory */
    newNode = (p_EntryList) malloc(sizeof(t_entryList));

    if(newNode == NULL)
    {
        printf("The disk is empty.\n");
    }
    else
    {
        /* Store the information of the current directory entry */
        memcpy(newNode->entry.fileName, buff, SIZE_OF_NAME);
        newNode->entry.attributes = buff[0x0B];
        newNode->entry.writeTime = LITTLE_ENDIAN(buff[0x16], buff[0x17]);
        newNode->entry.writeDate = LITTLE_ENDIAN(buff[0x18], buff[0x19]);
        newNode->entry.startCluster = LITTLE_ENDIAN(buff[0x1A], buff[0x1B]);
        newNode->entry.fileSize = (uint32_t)(buff[0x1F] << SHIFT_24_BIT)
                                | (uint32_t)(buff[0x1E] << SHIFT_16_BIT)
                                | (uint32_t)(buff[0x1D] << SHIFT_8_BIT)
                                | buff[0x1C];

        newNode->next = NULL;
    }

    return newNode;
}

void readDirEntry(p_EntryList *head, uint32_t startEntry)
{
    p_EntryList newNode = NULL;
    p_EntryList temp = NULL;
    uint8_t *buff = NULL;
    uint32_t index = 0;
    uint32_t num = 0;
    uint8_t thisFatType = 0;

    thisFatType = fatType();
    switch(thisFatType)
    {
    case FAT_12:
    case FAT_16:
        if(startEntry == s_local.rootDirStartSector)
        {
            num = s_local.sectorInRootDir;
        }
        else
        {
            num = s_bootInfo.secPerClus;
        }
        break;
    case FAT_32:
        num = s_bootInfo.secPerClus;
        break;
    default:
        break;
    }

    /* Allocate some memory to store */
    buff = (uint8_t*)malloc(s_bootInfo.bytsPerSec * num * sizeof(uint8_t));

    if(buff == NULL)
    {
        printf("The disk is empty.\n");
    }
    /* Read the number of sector to the address of the entry and save to buff array */
    else if(HAL_ReadMultiSector(startEntry, num, buff) != s_bootInfo.bytsPerSec * num)
    {
        printf("Read Directory Entry error.\n");
        free(buff);
    }
    else
    {
        while(index < (num * s_bootInfo.bytsPerSec))
        {
            /* Define a node with the information of the directory entry  */
            newNode = createNewNodeAsDirEntry(&buff[index]);

            /* Create a new node if this is a file or a directory */
            if((newNode->entry.attributes == ATTR_DIRECTORY ||
                newNode->entry.attributes == ATTR_FILE ||
                newNode->entry.attributes == ATTR_READ_ONLY ||
                newNode->entry.attributes == ATTR_ARCHIVE) &&
                newNode->entry.fileName[0] != INVALID_FILE_NAME &&
                newNode->entry.fileName[0] != DELETED_FILE_NAME)
            {
                if(*head == NULL)
                {
                    *head = newNode;
                }
                else
                {
                    temp = *head;
                    while(temp->next != NULL)
                    {
                        temp = temp->next;
                    }

                    temp->next = newNode;
                }
            }
            /* Otherwise, free the node */
            else
            {
                free(newNode);
            }

            /* Point to next entry address */
            index += SIZE_ROOT_ENTRY;
        }

        free(buff);
    }
}

void loadDirEntry(p_EntryList *head, uint32_t startCluster)
{
    uint8_t *buff = NULL;
    uint32_t startEntry = 0;
    uint32_t temp = 0;
    uint8_t thisFatType = 0;
    uint32_t thisLastCluster = 0;

    temp = startCluster;
    thisFatType = fatType();

    /* Delete all link list */
    deleteList(head);

    /* Determine the value of the last cluster based on the FAT type */
    switch(thisFatType)
    {
    case FAT_12:
        thisLastCluster = LAST_CRUSTER_12;
        break;
    case FAT_16:
        thisLastCluster = LAST_CRUSTER_16;
        break;
    case FAT_32:
        if(temp == 0)
        {
            temp = s_bootInfo.rootClus;
        }
        thisLastCluster = LAST_CRUSTER_32;
        break;
    default:
        break;
    }

    while (temp < thisLastCluster)
    {
        if(temp == 0)
        {
            startEntry = s_local.rootDirStartSector;
            temp = thisLastCluster;
        }
        else
        {
            startEntry = ((temp - FIRST_CLUSTER)* s_bootInfo.secPerClus) + s_local.dataStartSector;
            temp = nextCluster(temp); /* next cluster */
        }

        /* Read content in directory */
        readDirEntry(head, startEntry);
    }
}

void loadFile(uint8_t *buff, uint32_t startCluster)
{
    uint16_t count = 0;
    uint16_t position = 0;
    uint32_t temp = 0;
    uint8_t thisFatType = 0;
    uint32_t thisLastCluster = 0;

    temp = startCluster;
    thisFatType = fatType();

    /* Determine the value of the last cluster based on the FAT type */
    switch(thisFatType)
    {
    case FAT_12:
        thisLastCluster = LAST_CRUSTER_12;
        break;
    case FAT_16:
        thisLastCluster = LAST_CRUSTER_16;
        break;
    case FAT_32:
        thisLastCluster = LAST_CRUSTER_32;
        break;
    default:
        break;
    }

    /* Read the file content from each cluster and store it in the buffer
       Stop when the end of the file (last cluster) is reached */
    while (temp < thisLastCluster)
    {
        position = ((temp - FIRST_CLUSTER) * s_bootInfo.secPerClus) + s_local.dataStartSector; /* Position of sector need to read */
        HAL_ReadMultiSector(position, s_bootInfo.secPerClus , buff + (s_bootInfo.bytsPerSec * s_bootInfo.secPerClus * count));
        count++;
        temp = nextCluster(temp); /* Next cluster */
    }
}

