#ifndef _HAL_H_
#define _HAL_H_

/*******************************************************************************
* Include
*******************************************************************************/

#include <stdint.h>
#include <stdio.h>

/*******************************************************************************
* Define
*******************************************************************************/

#define BYTE_PER_SECTOR 512U

/*******************************************************************************
* API
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

#endif /* _FAT_H_ */

