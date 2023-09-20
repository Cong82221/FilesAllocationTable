#include "FAT.h"

/*******************************************************************************
* Prototypes
*******************************************************************************/

/**
 * Name: countNode
 * @brief Counts the number of nodes in the linked list.
 *
 * @param head The head of the linked list.
 *
 * @return The number of nodes in the linked list.
 */
uint8_t countNode(p_EntryList head);

/**
 * Name: converCharToInt
 * @brief Converts a character array representing a number to an integer.
 *
 * @param arr Pointer to the character array to be converted to an integer.
 *
 * @return The converted integer value.
 *         If the array contains non-numeric characters, the function returns -1.
 */
uint8_t converCharToInt(char *arr);

/**
 * Name: display
 * @brief Display the information of files and directories stored in the linked list of directory entries.
 *
 * @param head: The head of the linked list containing directory entries.
 */
void display(p_EntryList head);

/**
 * Name: APP
 * @brief Main application function to manage the file system and user interaction.
 *
 * @param filePath: The path to the disk file.
 * @param fp: A pointer to the opened file.
 */
void APP(const char * FileFath, FILE *fp);

/*******************************************************************************
* Code
*******************************************************************************/
int main()
{
    char *filePath = NULL;
    FILE *fp = NULL;
    filePath = "fat32.img";

    APP(filePath, fp);

    fclose(fp);
}

uint8_t countNode(p_EntryList head)
{
    p_EntryList current = NULL;
    uint8_t count = 0;

    current = head;

    /* Count until null pointer is encountered */
    while (current != NULL)
    {
        count++;
        current = current->next;  /* Move to the next node in the list */
    }

    return count;
}

uint8_t converCharToInt(char *arr)
{
    uint8_t index = 0;
    uint8_t inter = 0;
    uint8_t flag = 0;
    uint8_t value = 0;
    uint8_t size = 0;

    size = strlen(arr);   /* Determine the size of the character array */

    /* convert char array to int */
    for( index = 0 ; index < size; index++){
        if ((arr[index] >= '0') &&  (arr[index] <= '9'))
        {
            inter = arr[index] - '0';
            value = value * 10 + inter;

            printf("value %d: %d", index, value);
        }
        else {
            flag = 1;
        }
    }

    /* If non-numeric characters are found */
    if (flag)
    {
        value = -1;
    }

    return value;
}

void display(p_EntryList head)
{
    p_EntryList current = NULL;
    uint16_t count = 0;
    uint16_t year = 0;
    uint8_t month = 0;
    uint8_t day = 0;
    uint8_t hour = 0;
    uint8_t min = 0;
    uint8_t sec = 0;
    uint8_t index = 0;

    printf("\n-------------------------------------------------------------------\n");
    printf("No.  Name\t\tSize\t\tDate\t\tTime\n");

    current = head;

    /* Loop through all entries in the linked list and display the information */
    while(current != NULL)
    {
        printf(" %d   ", ++count);

        /* Print name of file in folder */
        for(index = 0; index < 8; index++)
        {
            printf("%c", current->entry.fileName[index]);
        }

        /* Print dot if it's not a directory */
        if(current->entry.attributes != ATTR_DIRECTORY)
        {
            printf (".");
            /* Print type of file */
            for(index = 8; index < SIZE_OF_NAME; index++)
            {
            printf("%c", current->entry.fileName[index]);
            }
        }
        else
        {
            printf("");
        }

        /* Print size of file */
        if(current->entry.attributes != ATTR_DIRECTORY)
        {
            printf ("\t%i\t\t", current->entry.fileSize);
        }
        else
        {
            printf("\t\t\t\t");
        }

        /* Print the last date and time recorded */
        year = (current->entry.writeDate >> SHIFT_9_BIT) + SET_YEAR;
        month = (current->entry.writeDate >> SHIFT_5_BIT) & MASK_MONTH;
        day = current->entry.writeDate & MASK_DAY;

        hour = current->entry.writeTime >> SHIFT_11_BIT;
        min  = (current->entry.writeTime >> SHIFT_5_BIT) & MASK_MINUTE;
        sec = current->entry.writeTime & MASK_SECOND;

        printf("%02d/%02d/%04d ",day, month, year);
        printf("\t%02d:%02d:%02d\n",hour, min, sec);

        current = current->next;
    }

    printf(" %d   Exit program!\n", (count + 1));

    printf("-------------------------------------------------------------------\n");
}

void APP(const char * filePath, FILE *fp)
{
    t_location local = {0};
    t_bootSector bootInfo = {0};
    p_EntryList head = NULL;
    p_EntryList temp = NULL;
    uint8_t* buff = NULL;
    uint8_t thisFatType = 0;
    char str[10];
    uint32_t startEntry = 0;
    uint32_t select = 0;
    uint8_t count = 0;
    uint8_t ans = 0;
    uint8_t flagExit = 0;
    uint32_t index = 0;

    bootInfo = initFileFAT(filePath, fp);
    local = localEachRegion();
    thisFatType = fatType();

    /* Based on the FAT type, read the directory entry information */
    switch(thisFatType)
    {
    case FAT_12:
    case FAT_16:
        startEntry = 0;
        loadDirEntry(&head, startEntry);
        break;
    case FAT_32:
        startEntry = bootInfo.rootClus;
        loadDirEntry(&head, startEntry);
        break;
    default:
        break;
    }

    while(flagExit == 0)
    {
        temp = head;
        ans = countNode(temp) + 1;
        do
        {
            display(head);
            printf("Enter your option (1 - %d): ", ans);
            gets(str);
            select = converCharToInt(str);
            printf("%d\n", converCharToInt(str));
            system("cls");

            /* Check if the user input is valid or if it's the exit option */
            if(select == ans)
            {
                system("cls");
                printf("\nGood bye!\n");
                flagExit = 1;
            }

        } while((select > ans) || (select < 1));

        while(temp != NULL)
        {
            if(count == select - 1)
            {
                /* Check if the selected entry is a directory or a file */
                if(temp->entry.attributes == ATTR_DIRECTORY)
                {
                    loadDirEntry(&temp, temp->entry.startCluster);
                    head = temp;
                }
                else
                {
                    if(temp->entry.fileSize > 0)
                    {
                        buff = (uint8_t *)malloc(((temp->entry.fileSize/bootInfo.bytsPerSec) + 1)
                                                 * bootInfo.bytsPerSec
                                                 * bootInfo.secPerClus);

                        if(buff == NULL)
                        {
                            printf("The disk is empty.\n");
                            flagExit = 1;
                        }
                        else
                        {
                            loadFile(buff, temp->entry.startCluster);

                            /* Display the file content on the screen */
                            for (index = 0; index < temp->entry.fileSize; index++)
                            {
                                printf("%c", buff[index]);
                            }

                            free(buff);
                        }
                    }
                }
            }
            count++;
            temp = temp->next;
        }
        count = 0;
    }
}
