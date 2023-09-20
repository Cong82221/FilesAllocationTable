#include "HAL.h"

/*******************************************************************************
* Variables
*******************************************************************************/
static FILE *fp = NULL;
static uint32_t sizeSector = BYTE_PER_SECTOR;

/*******************************************************************************
* Prototypes
*******************************************************************************/

/**
 * Name: HAL_Init
 * @brief Initializes the file system by opening a file in binary read mode.
 *
 * @param filePath: The path to the file to be opened.
 * @return FILE*: A pointer to the opened file. If failed, returns NULL.
 */
FILE* HAL_Init(const char * fileFath);

/**
 * Name: HAL_Update
 * @brief Update the number of bytes in a sector
 *
 * @param bytsPerSec the number of bytes in a sector
 */
void HAL_Update(uint32_t bytsPerSec);

/**
 * Name: HAL_ReadSector
 * @brief  Read sector position index
 *
 * @param index Position sector to read
 * @param buff Array contain this sector
 *
 * @return Byte read in this sector
 */
uint32_t HAL_ReadSector(uint32_t index, uint8_t *buff);

/**
 * Name: HAL_ReadMultiSector
 * @brief Read sector from index to num
 *
 * @param index Position sector to read
 * @param num Number sector read
 * @param buff Array contain this sector
 *
 * @return byteRead Byte read in 'num' sector
 */
uint32_t HAL_ReadMultiSector(uint32_t index, uint32_t num, uint8_t *buff);

/*******************************************************************************
* Code
*******************************************************************************/

FILE* HAL_Init(const char *fileFath)
{
    /* Open file to read */
    fp = fopen(fileFath, "rb");
    if (fp == NULL )
    {
        printf("Failed to open the file.\n");
    }

    return fp;
}

void HAL_Update(uint32_t bytsPerSec)
{
    sizeSector = bytsPerSec;
}

uint32_t HAL_ReadSector(uint32_t index, uint8_t *buff)
{
    uint64_t positionPointer = 0;
    uint32_t byteRead = 0;

    positionPointer = index * sizeSector;

    if(buff == NULL)
    {
        printf("The disk is empty!");
    }
    else if (fseek(fp, positionPointer, SEEK_SET) != 0x00)
    {
        printf("Error moving the file pointer.\n");
    }
    else
    {
        byteRead = fread(buff, sizeof(uint8_t), sizeSector, fp);
    }

    return byteRead;
}

uint32_t HAL_ReadMultiSector(uint32_t index, uint32_t num, uint8_t *buff)
{
    uint32_t positionPointer = 0;
    uint32_t byteRead = 0;

    positionPointer = index * sizeSector;

    if(buff == NULL)
    {
        printf("The disk is empty!");
    }
    else if(fseek(fp, positionPointer, SEEK_SET) != 0x00)
    {
        printf("Error moving the file pointer.\n");
    }
    else
    {
        byteRead = fread(buff, sizeof(uint8_t), sizeSector * num, fp);
    }

    return byteRead;
}
