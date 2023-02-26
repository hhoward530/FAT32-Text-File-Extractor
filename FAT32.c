#pragma pack(1)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h> //this is here exclusively for formatting numbers with commas


/*
Written By: Hunter Howard
for Operating Systems Project 3

This program is designed to accept an image file representative of a FAT32 drive
and perform various functions on it. Those functions include printing the contents
of the root directory and extracting files from it. The files that are extracted from
the image are made in the directory containing the source file of the code.
*/

//
//
//Structs for the various structures within the FAT32 drive
//
//

struct DirectoryEntry
{
    unsigned char fileName[8];  // 0-7
    unsigned char extension[3]; // 8-10
    unsigned char attributes;   // 11
    unsigned char WindowsNT;    // 12
    unsigned char seconds;      // 13
    unsigned int createSec : 5;
    unsigned int createMin : 6;
    unsigned int createHour : 5;
    // unsigned char createTime[2]; //14-15
    unsigned int createDay : 5;
    unsigned int createMonth : 4;
    unsigned int createYear : 7;
    // unsigned char createDate[2]; //16-17
    unsigned char lastAcessed[2];      // 18-19
    unsigned char highClusterStart[2]; // 20-21
    unsigned char timeLastModified[2]; // 22-23
    unsigned char dateLastModified[2]; // 24-25
    unsigned char lowClusterStart[2];  // 26-27
    unsigned int fileSize;             // 28-31
};
struct LongDirectoryEntry
{
    unsigned char lastEntry;    // 0
    unsigned char name1[10];    // 1-10
    unsigned char attributes;   // 11
    unsigned char type;         // 12
    unsigned char checkSum;     // 13
    unsigned char name2[12];    // 14-25
    unsigned char FstClusLO[2]; // 26-27
    unsigned char name3[4];     // 28-31
};

struct PartitionTableEntry
{
    unsigned char bootFlag;
    unsigned char CHSBegin[3];
    unsigned char typeCode;
    unsigned char CHSEnd[3];
    unsigned int LBABegin;
    unsigned int LBAEnd;
};
struct MBRStruct
{
    unsigned char bootCode[446];
    struct PartitionTableEntry part1;
    struct PartitionTableEntry part2;
    struct PartitionTableEntry part3;
    struct PartitionTableEntry part4;
    unsigned short flag;
} MBR;
struct BPBStruct
{
    unsigned char BS_jmpBoot[3];    // Jump instruction to boot code
    unsigned char BS_OEMNane[8];    // 8-Character string (not null terminated)
    unsigned short BPB_BytsPerSec;  // Had BETTER be 512!
    unsigned char BPB_SecPerClus;   // How many sectors make up a cluster?
    unsigned short BPB_RsvdSecCnt;  // # of reserved sectors at the beginning (including the BPB)?
    unsigned char BPB_NumFATs;      // How many copies of the FAT are there? (had better be 2)
    unsigned short BPB_RootEntCnt;  // ZERO for FAT32
    unsigned short BPB_TotSec16;    // ZERO for FAT32
    unsigned char BPB_Media;        // SHOULD be 0xF8 for "fixed", but isn't critical for us
    unsigned short BPB_FATSz16;     // ZERO for FAT32
    unsigned short BPB_SecPerTrk;   // Don't care; we're using LBA; no CHS
    unsigned short BPB_NumHeads;    // Don't care; we're using LBA; no CHS
    unsigned int BPB_HiddSec;       // Don't care ?
    unsigned int BPB_TotSec32;      // Total Number of Sectors on the volume
    unsigned int BPB_FATSz32;       // How many sectors long is ONE Copy of the FAT?
    unsigned short BPB_Flags;       // Flags (see document)
    unsigned short BPB_FSVer;       // Version of the File System
    unsigned int BPB_RootClus;      // Cluster number where the root directory starts (should be 2)
    unsigned short BPB_FSInfo;      // What sector is the FSINFO struct located in? Usually 1
    unsigned short BPB_BkBootSec;   // REALLY should be 6 – (sector # of the boot record backup)
    unsigned char BPB_Reserved[12]; // Should be all zeroes -- reserved for future use
    unsigned char BS_DrvNum;        // Drive number for int 13 access (ignore)
    unsigned char BS_Reserved1;     // Reserved (should be 0)
    unsigned char BS_BootSig;       // Boot Signature (must be 0x29)
    unsigned char BS_VolID[4];      // Volume ID
    unsigned char BS_VolLab[11];    // Volume Label
    unsigned char BS_FilSysType[8]; // Must be "FAT32 "
    unsigned char unused[420];      // Not used
    unsigned char signature[2];     // MUST BE 0x55 AA
} BPB;

int main(int argv, char* argc[])
{

    if(argv != 2)//make sure we have the correct number of arguments
    {
        printf("Incorrect number of parameters.\n");
        exit(1);
    }

while(1 == 1)//the loop that contains the code
{

    printf("\\>");
    char input[50];
    fgets(input, 50, stdin);


//
//If the user enters QUIT the program will exit
//
    if(strcmp(input, "QUIT\n") == 0)
    {
        exit(1);
    }

//
//If the user enters DIR, the contents of the root directory will print
//
if(strcmp(input, "DIR\n") == 0)
{
    FILE *fp = fopen(argc[1], "r"); //open the file and check it works
    if (!fp)
    {
        printf("Unable to open file.");
    }
    struct MBRStruct MBR;
    fseek(fp, 0, SEEK_SET); //start at spot 0 and read in the MBR
    fread(&MBR, 512, 1, fp);

    unsigned int FATLBAStart = MBR.part1.LBABegin;
    unsigned char sector[512];
    struct BPBStruct BPB;

    for (int i = 0; i < FATLBAStart; i++)//get to where the BPB starts
    {
        fread(&BPB, 512, 1, fp);
    }

    unsigned int rootDirStart = BPB.BPB_RsvdSecCnt + (BPB.BPB_NumFATs * BPB.BPB_FATSz32);//the starting location of the root directory
    unsigned int FATBegin = BPB.BPB_RsvdSecCnt;

    unsigned char wholeFAT[995840]; // total bytes = 512 bytes/sector * 1945 numbers of sectors in the FAT
    for (int i = 0; i < FATBegin; i++)
    {
        if (i == (FATBegin - 1)) // if we're at the start of the FAT,
        {
            fread(wholeFAT, 995840, 1, fp); // read the whole FAT into wholeFAT
            break;
        }
        fread(wholeFAT, 512, 1, fp); // read each sector until we get to the start of the fat
    }
    unsigned int byte;
    unsigned int *FAT = &wholeFAT;
    byte = FAT[2];              //find the root directory's second location
    unsigned int getAllDir = 2;//root directory always starts at 2
    unsigned int directoryIndex[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};//this will hold all of the root diercty cluster numbers
    int count = 0;

    while (getAllDir != 268435455)//while were not atb the end of the root directory
    {
        directoryIndex[count] = getAllDir;
        byte = FAT[getAllDir]; // continue to read in the clusters the root directory occupies
        getAllDir = byte;
        count++;
    }
   
    struct DirectoryEntry entry;            //read in each directoty entry
    struct LongDirectoryEntry longEntry;    //read them all into a long file name format just in case
    count = 0;
    unsigned int fileTotal = 0;         //total number of files in the directory
    unsigned long long totalBytes = 0;  //total number of bytes those files contains
    while (directoryIndex[count] != 0) //we will be reading each section of the root directory
    {

        fseek(fp, 0, SEEK_SET);
        if (directoryIndex[count] == 2) //if we're starting at the first cluster of the root
        {
            for (int i = 0; i < FATLBAStart + rootDirStart; i++) // this loop gets us to the start of the root directory
            {
                fread(sector, 512, 1, fp);
            }
            int longFile = 0;                   //that to determine if we have a LFN on deck
            unsigned char LongFileNames[60];    //this will contain the characters of the long file name
            int nextLongName = 0;               //increments our index in the long file name array
         
            for (int i = 0; i < 128; i++) //start reading in the directory entries
            {
                fread(&entry, 32, 1, fp); 
                if (entry.attributes == 0x0F && i > 2) //if we have just read a long file name entry
                {
                    longFile = 1;                       //mark that we have a long file entry on deck
                    memcpy(&longEntry, &entry, 32);     //read in the LFN, and put each character into the array that will conatin the long file name
                    LongFileNames[nextLongName] = longEntry.name3[3];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name3[2];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name3[1];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name3[0];
                    nextLongName++;

                    LongFileNames[nextLongName] = longEntry.name2[11];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[10];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[9];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[8];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[7];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[6];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[5];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[4];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[3];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[2];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[1];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[0];
                    nextLongName++;

                    LongFileNames[nextLongName] = longEntry.name1[9];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[8];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[7];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[6];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[5];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[4];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[3];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[2];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[1];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[0];
                    nextLongName++;

                    continue; //go back to see if there is more to the LFN
                }
                if ((entry.fileName[0] != 0x00) && ((entry.attributes & 0x02) == 0) && ((entry.attributes & 0x08) == 0))//if we have a regular directory entry
                {
                    if((entry.createHour < 12) && (entry.createHour > 0)) //print the date and time of creation
                    {
                        printf("%u/%u/%u  %02u:%02u AM", entry.createMonth, entry.createDay, (entry.createYear + 1980), entry.createHour, entry.createMin);
                    }
                    else if(entry.createHour == 0)
                    {
                        printf("%u/%u/%u  12:%02u AM", entry.createMonth, entry.createDay, (entry.createYear + 1980), entry.createMin);
                    }
                    else if(entry.createHour > 12)
                    {
                        int fixedHour = entry.createHour%12;
                        printf("%u/%u/%u  %02u:%02u PM", entry.createMonth, entry.createDay, (entry.createYear + 1980), fixedHour, entry.createMin);
                    }
                    setlocale(LC_NUMERIC, ""); //add commas to large numbers
                    printf("%'*u  ",13, entry.fileSize); //print the file size

                    unsigned char shortName[12]; //get the short name of the current file
                    memset(shortName, 0 , 12);
                    int shortNameSpot = 0;
                    for(int i = 0; i < 12; i++)
                    {
                        if(i < 8)
                        {
                             if(entry.fileName[i] != ' ')
                            {
                                shortName[shortNameSpot] = entry.fileName[i]; 
                                shortNameSpot++;
                            }                            
                            }
                        else if(i == 8)
                        {
                            shortName[shortNameSpot] = '.';
                            shortNameSpot++;
                        }
                        else if(i > 8)
                        {
                             shortName[shortNameSpot] = entry.extension[i-9];
                            shortNameSpot++;
                            }
                    }
                    int shortNameLen = 0;
                    for(int i = 0; i < 12; i ++)
                    {
                        if(shortName[i] != 0)
                            shortNameLen++;
                    }
                    int dif = 12 - shortNameLen;
                    for(int i = 0; i < 12; i++)
                    {
                        if(i < shortNameLen)
                        printf("%c", shortName[i]);
                        else
                        printf(" ");
                    }


                    printf("    ");
                    if (longFile == 1) //if we had a long file name on deck
                    {
                         for (int i = nextLongName; i >= 0; i--)//read backwards through the array, starting at the last character we entered
                        {
                            unsigned char partOfName = LongFileNames[i];
                            if(partOfName >= 32 && partOfName <= 127)   //print the characters of th elong file name
                                printf("%c", partOfName);
                        }
                        printf("%c", '\n');
                        longFile = 0;       //we no longer have a LFN on deck
                        nextLongName = 0;   //start back at the beginning of our long file name array
                        memset(&LongFileNames[0], 0, sizeof(LongFileNames));//clear out the array of long file names so there is no confusion for the next
                    }
                    else
                    printf("\n");
                    longFile = 0; //there was no long file
                }
                totalBytes += entry.fileSize;
                fileTotal++;
            }
        }
        else //we are not at the beginning of the root directory
        {
            for (int i = 0; i < FATLBAStart + rootDirStart + ((directoryIndex[count] - 2) * 8); i++) // this loop gets us to the next parts of the root directory
            {
                fread(sector, 512, 1, fp);
            }
            int longFile = 0;
            unsigned char *LongFileNames[60]; //LFN array
            int nextLongName = 0;             //LFN array index

            for (int i = 0; i < 128; i++) //start reading in entries
            {
                fread(&entry, 32, 1, fp);
                unsigned short hasLongEntry = 0;
                if (entry.attributes == 0x0F) //if we have a LFN
                {
                    longFile = 1;
                    memcpy(&longEntry, &entry, 32);                     //start reading it into our array
                    LongFileNames[nextLongName] = longEntry.name3[3];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name3[2];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name3[1];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name3[0];
                    nextLongName++;

                    LongFileNames[nextLongName] = longEntry.name2[11];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[10];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[9];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[8];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[7];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[6];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[5];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[4];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[3];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[2];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[1];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[0];
                    nextLongName++;

                    LongFileNames[nextLongName] = longEntry.name1[9];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[8];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[7];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[6];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[5];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[4];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[3];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[2];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[1];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[0];
                    nextLongName++;

                    continue;//see if there is more to the LFN
                }
                if ((entry.fileName[0] != 0x00) && ((entry.attributes & 0x02) == 0)) //if we see a normal directory entry
                {
                    if((entry.createHour < 12) && (entry.createHour > 0))//print the date and time
                    {
                        printf("%u/%u/%u  %02u:%02u AM", entry.createMonth, entry.createDay, (entry.createYear + 1980), entry.createHour, entry.createMin);
                    }
                    else if(entry.createHour == 0)
                    {
                        printf("%u/%u/%u  12:%02u AM", entry.createMonth, entry.createDay, (entry.createYear + 1980), entry.createHour, entry.createMin);
                    }
                    else if(entry.createHour > 12)
                    {
                        int fixedHour = entry.createHour%12;
                        printf("%u/%u/%u  %02u:%02u PM", entry.createMonth, entry.createDay, (entry.createYear + 1980), fixedHour, entry.createMin);
                    }
                    setlocale(LC_NUMERIC, "");//add commas to large numbers
                    printf("%'*u  ", 13, entry.fileSize); //print the short file name
                    
                    unsigned char shortName[12]; //get the short name of the current file
                    memset(shortName, 0 , 12);
                    int shortNameSpot = 0;
                    for(int i = 0; i < 12; i++)
                    {
                        if(i < 8)
                        {
                             if(entry.fileName[i] != ' ')
                            {
                                shortName[shortNameSpot] = entry.fileName[i]; 
                                shortNameSpot++;
                            }                            
                            }
                        else if(i == 8)
                        {
                            shortName[shortNameSpot] = '.';
                            shortNameSpot++;
                        }
                        else if(i > 8)
                        {
                             shortName[shortNameSpot] = entry.extension[i-9];
                            shortNameSpot++;
                            }
                    }
                    int shortNameLen = 0;
                    for(int i = 0; i < 12; i ++)
                    {
                        if(shortName[i] != 0)
                            shortNameLen++;
                    }
                    int dif = 12 - shortNameLen;
                    for(int i = 0; i < 12; i++)
                    {
                        if(i < shortNameLen)
                        printf("%c", shortName[i]);
                        else
                        printf(" ");
                    }


                    printf("    ");


                    if (longFile == 1) //if we have a long file name on deck
                    {
                        for (int i = nextLongName; i >= 0; i--) //print it
                        {
                            unsigned char partOfName = LongFileNames[i];
                            if(partOfName >= 32 && partOfName <= 127)
                                printf("%c", partOfName);
                        }
                        printf("%c", '\n');
                        longFile = 0;           //reset our values for next time
                        nextLongName = 0;
                        memset(&LongFileNames[0], 0, sizeof(LongFileNames));
                    }
                    else
                        printf("\n");
                    longFile = 0;
                }
                fileTotal++;
                totalBytes += entry.fileSize;
            }
        }
        count++;//go to the next cluster of the directory
    }
setlocale(LC_NUMERIC, "");//add commas to large numbers
printf("    Total File(s): %'u     %'llu bytes\n", fileTotal, totalBytes); //summary line
fseek(fp, 0, SEEK_SET); //go back to the start of file and close
fclose(fp);
}//end of DIR




//
//if the user enters to extract a file
//
 if(strstr(input, "EXTRACT") != NULL)//check that the command contains EXTRACT
    {
        char file[42];                      //this will hold the file name we're searching for
        memset(&file[0], 0, sizeof(file));
        for(int i = 0; i < 42; i++)         //start reading the file name that starts after 'EXTRACT '
        {
            if(input[i+8] >= 32 && input[i+8] <= 127)
                file[i] = input[i+8];
        }
        
        //start the extract here
 FILE *fp = fopen(argc[1], "r"); //open the file
    if (!fp)
    {
        printf("Unable to open file.");
    }
    struct MBRStruct MBR;
    fseek(fp, 0, SEEK_SET);
    fread(&MBR, 512, 1, fp);

    unsigned int FATLBAStart = MBR.part1.LBABegin;
    unsigned char sector[512];
    struct BPBStruct BPB;

    for (int i = 0; i < FATLBAStart; i++)
    {
        fread(&BPB, 512, 1, fp);
    }

    unsigned int rootDirStart = BPB.BPB_RsvdSecCnt + (BPB.BPB_NumFATs * BPB.BPB_FATSz32); 
    unsigned int FATBegin = BPB.BPB_RsvdSecCnt;

    unsigned char wholeFAT[995840]; // total bytes = 512 bytes/sector * 1945 numbers of sectors in the FAT
    for (int i = 0; i < FATBegin; i++)
    {
        if (i == (FATBegin - 1)) // if we're at the start of the FAT,
        {
            fread(wholeFAT, 995840, 1, fp); // read the whole FAT into wholeFAT
            break;
        }
        fread(wholeFAT, 512, 1, fp); // read each sector until we get to the start of the fat
    }
    unsigned int byte;
    unsigned int *FAT = &wholeFAT;  //we are finding the clusters of the root directory and storing them in an array
    byte = FAT[2];
    unsigned int getAllDir = 2;
    unsigned int directoryIndex[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int count = 0;

    while (getAllDir != 268435455)
    {
        directoryIndex[count] = getAllDir;
        byte = FAT[getAllDir]; // byte now points to nect directory
        getAllDir = byte;
        count++;
    }
   
    struct DirectoryEntry entry;
    struct LongDirectoryEntry longEntry;
    count = 0;
    int fileFound = 0;                  //flag to see if we've found the file we want
    while (directoryIndex[count] != 0) //while we still hav eroot directory left
    {

        fseek(fp, 0, SEEK_SET);
        if (directoryIndex[count] == 2)
        {
            for (int i = 0; i < FATLBAStart + rootDirStart; i++) // this loop gets us to the start of the root directory
            {
                fread(sector, 512, 1, fp);
            }
            int longFile = 0;
            unsigned char LongFileNames[60];                    //this will store our long file names
            memset(&LongFileNames[0], 0, sizeof(LongFileNames));
            int nextLongName = 0;
            // we start reading entries here
            for (int i = 0; i < 128; i++)
            {

                fread(&entry, 32, 1, fp);
                unsigned short hasLongEntry = 0;
                if (entry.attributes == 0x0F && i > 2) //if we have an LFN start reading it into an array
                {
                    longFile = 1;
                    memcpy(&longEntry, &entry, 32);
                    LongFileNames[nextLongName] = longEntry.name3[3];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name3[2];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name3[1];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name3[0];
                    nextLongName++;

                    LongFileNames[nextLongName] = longEntry.name2[11];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[10];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[9];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[8];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[7];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[6];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[5];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[4];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[3];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[2];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[1];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[0];
                    nextLongName++;

                    LongFileNames[nextLongName] = longEntry.name1[9];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[8];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[7];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[6];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[5];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[4];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[3];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[2];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[1];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[0];
                    nextLongName++;

                    continue;//see if there is more LFN
                }
                if ((entry.fileName[0] != 0x00) && ((entry.attributes & 0x02) == 0) && ((entry.attributes & 0x08) == 0)) //if we find a directory entry
                {

                    unsigned char shortName[12]; //get the short name of the current file
                    memset(shortName, 0 , 12);
                    int shortNameSpot = 0;
                    for(int i = 0; i < 12; i++)
                    {
                        if(i < 8)
                        {
                             if(entry.fileName[i] != ' ')
                            {
                                shortName[shortNameSpot] = entry.fileName[i]; 
                                shortNameSpot++;
                            }                            
                            }
                        else if(i == 8)
                        {
                            shortName[shortNameSpot] = '.';
                            shortNameSpot++;
                        }
                        else if(i > 8)
                        {
                             shortName[shortNameSpot] = entry.extension[i-9];
                            shortNameSpot++;
                            }
                    }
                    
                    if(strcmp(shortName, file) == 0) //did the user enter a short file name
                    {
                       
                        unsigned int *file;
                        unsigned char fileLocation[4];
        
                        memset(&fileLocation[0], 0, sizeof(fileLocation));
                        fileLocation[0] = entry.highClusterStart[0];
                        fileLocation[1] = entry.highClusterStart[1];
                        fileLocation[2] = entry.lowClusterStart[0];
                        fileLocation[3] = entry.lowClusterStart[1];
                        file = &fileLocation;
                        unsigned int location = file[0]/256/256;

                        fseek(fp, 0, SEEK_SET);
                        for (int i = 0; i < FATLBAStart + rootDirStart + (location*8); i++) //go to the stsrat of the file
                        {
                            fread(sector, 512, 1, fp);
                        }
                        
                        FILE* newFile;            //create our new file
                        newFile = fopen(shortName, "w");
                        if(strstr(shortName, ".TXT") != NULL) //if were reading a text file
                        {
                        for(int i = 0; i < 8; i++)
                        {
                            fread(sector, 512, 1, fp);
                            for(int j = 0; j < 512; j++)
                            {
                                if((sector[j] >= 32) && (sector[j] <= 127)) //only take the characters we want
                                {
                                    unsigned char letter = sector[j];
                                    fputc(letter, newFile);
                                }
                            }
                        }
                        }
                        else //otherwise, read all of the information in
                        {
                            for(int i = 0; i < 8; i++)
                            {
                                fread(sector, 512, 1, fp);
                                for(int j = 0; j < 512; j++)
                                {
                                        unsigned char letter = sector[j];
                                        fputc(&letter, newFile);
                                }
                            }
                        }
                        fclose(newFile);//close connection to our new file
                        fileFound = 1; //we have found the file we were looking for
                    }

                    if (longFile == 1)  //if we found a long file name
                    {
                         char checkLFN[60];
                         memset(&checkLFN[0], 0, sizeof(checkLFN)); //print the file name into a variable
                        int LFNIndex = 0;                           //this way it reads left to right and not right to left
                         for (int i = nextLongName; i >= 0; i--)
                        {
                            unsigned char partOfName = LongFileNames[i];
                            if(partOfName >= 32 && partOfName <= 127)
                            {
                                checkLFN[LFNIndex] = partOfName;
                                LFNIndex++;
                            }
                        }
                        //printf("%s\n", checkLFN);
                        if(strcmp(checkLFN, file) == 0) //if the user input matched our long file name
                        {
                            //retrieving file here
                        unsigned int *file;
                        unsigned char fileLocation[4];
        
                        memset(&fileLocation[0], 0, sizeof(fileLocation)); //get the cluster of the file
                        fileLocation[0] = entry.highClusterStart[0];
                        fileLocation[1] = entry.highClusterStart[1];
                        fileLocation[2] = entry.lowClusterStart[0];
                        fileLocation[3] = entry.lowClusterStart[1];
                        file = &fileLocation;
                        unsigned int location = file[0]/256/256;


                        fseek(fp, 0, SEEK_SET);
                        for (int i = 0; i < FATLBAStart + rootDirStart + (location*8); i++) //go to the stsrat of the file
                        {
                            fread(sector, 512, 1, fp);
                        }
                        
                        FILE* newFile;            //create our new file
                        newFile = fopen(checkLFN, "w");
                        if(strstr(checkLFN, ".txt") != NULL) //if were reading a text file
                        {
                        for(int i = 0; i < 8; i++)
                        {
                            fread(sector, 512, 1, fp);
                            for(int j = 0; j < 512; j++)
                            {
                                if((sector[j] >= 32) && (sector[j] <= 127)) //only take the characters we want
                                {
                                    unsigned char letter = sector[j];
                                    fputc(letter, newFile);
                                }
                            }
                        }
                        }
                        else //otherwise, read all of the information in
                        {
                            for(int i = 0; i < 8; i++)
                            {
                                fread(sector, 512, 1, fp);
                                for(int j = 0; j < 512; j++)
                                {
                                        unsigned char letter = sector[j];
                                        fputc(&letter, newFile);
                                }
                            }
                        }
                        fclose(newFile);//close connection to our new file
                        fileFound = 1; //we have found the file we were looking for
                        }
                        longFile = 0; //start back at the beginning for the next run
                        nextLongName = 0;
                        memset(&LongFileNames[0], 0, sizeof(LongFileNames));
                        memset(&checkLFN[0], 0, sizeof(checkLFN));
                    }
                    else//continue of we havent found the file yet
                    printf("");
                    longFile = 0;
                    memset(&LongFileNames[0], 0, sizeof(LongFileNames));
                }
            }
        }
        else //were at another cluster of the directory
        {
            for (int i = 0; i < FATLBAStart + rootDirStart + ((directoryIndex[count]- 2) * 8); i++) // this loop gets us to the start of the nect cluster
            {
                fread(sector, 512, 1, fp);
            }
            int longFile = 0;
            unsigned char *LongFileNames[60];       //store our long file names
            memset(&LongFileNames[0], 0, sizeof(LongFileNames));
            int nextLongName = 0;
            // we start reading entries here
            for (int i = 0; i < 128; i++)   //read in entries
            {
                fread(&entry, 32, 1, fp);
                if (entry.attributes == 0x0F)//read in the LFN if we have one
                {
                    longFile = 1;
                    memcpy(&longEntry, &entry, 32);
                    LongFileNames[nextLongName] = longEntry.name3[3];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name3[2];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name3[1];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name3[0];
                    nextLongName++;

                    LongFileNames[nextLongName] = longEntry.name2[11];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[10];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[9];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[8];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[7];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[6];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[5];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[4];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[3];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[2];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[1];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name2[0];
                    nextLongName++;

                    LongFileNames[nextLongName] = longEntry.name1[9];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[8];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[7];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[6];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[5];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[4];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[3];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[2];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[1];
                    nextLongName++;
                    LongFileNames[nextLongName] = longEntry.name1[0];
                    nextLongName++;

                    continue;//see if theres more lfn
                }
                if ((entry.fileName[0] != 0x00) && ((entry.attributes & 0x02) == 0))//we have a normal file
                {
                    unsigned char shortName[12]; //get the short file name of the current entry
                    memset(shortName, 0 , 12);
                    int shortNameSpot = 0;
                    for(int i = 0; i < 12; i++)
                    {
                        if(i < 8)
                        {
                             if(entry.fileName[i] != ' ')
                            {
                                shortName[shortNameSpot] = entry.fileName[i]; 
                                shortNameSpot++;
                            }                            
                            }
                        else if(i == 8)
                        {
                            shortName[shortNameSpot] = '.';
                            shortNameSpot++;
                        }
                        else if(i > 8)
                        {
                             shortName[shortNameSpot] = entry.extension[i-9];
                            shortNameSpot++;
                            }
                    }
                    
                    if(strcmp(shortName, file) == 0) //did the user enter a short file name
                    {
                       
                        unsigned int *file;
                        unsigned char fileLocation[4];
        
                        memset(&fileLocation[0], 0, sizeof(fileLocation));
                        fileLocation[0] = entry.highClusterStart[0];
                        fileLocation[1] = entry.highClusterStart[1];
                        fileLocation[2] = entry.lowClusterStart[0];
                        fileLocation[3] = entry.lowClusterStart[1];
                        file = &fileLocation;
                        unsigned int location = file[0]/256/256;

                        fseek(fp, 0, SEEK_SET);
                        for (int i = 0; i < FATLBAStart + rootDirStart + (location*8); i++) //go to the stsrat of the file
                        {
                            fread(sector, 512, 1, fp);
                        }
                        
                        FILE* newFile;            //create our new file
                        newFile = fopen(shortName, "w");
                        if(strstr(shortName, ".TXT") != NULL) //if were reading a text file
                        {
                        for(int i = 0; i < 8; i++)
                        {
                            fread(sector, 512, 1, fp);
                            for(int j = 0; j < 512; j++)
                            {
                                if((sector[j] >= 32) && (sector[j] <= 127)) //only take the characters we want
                                {
                                    unsigned char letter = sector[j];
                                    fputc(letter, newFile);
                                }
                            }
                        }
                        }
                        else //otherwise, read all of the information in
                        {
                            for(int i = 0; i < 8; i++)
                            {
                                fread(sector, 512, 1, fp);
                                for(int j = 0; j < 512; j++)
                                {
                                        unsigned char letter = sector[j];
                                        fputc(&letter, newFile);
                                }
                            }
                        }
                        fclose(newFile);//close connection to our new file
                        fileFound = 1; //we have found the file we were looking for
                    }

                    if (longFile == 1)  //if we found a long file name
                    {
                         char checkLFN[60];
                         memset(&checkLFN[0], 0, sizeof(checkLFN)); //print the file name into a variable
                        int LFNIndex = 0;                           //this way it reads left to right and not right to left
                         for (int i = nextLongName; i >= 0; i--)
                        {
                            unsigned char partOfName = LongFileNames[i];
                            if(partOfName >= 32 && partOfName <= 127)
                            {
                                checkLFN[LFNIndex] = partOfName;
                                LFNIndex++;
                            }
                        }
                        //printf("%s\n", checkLFN);
                        if(strcmp(checkLFN, file) == 0) //if the user input matched our long file name
                        {
                            //retrieving file here
                        unsigned int *file;
                        unsigned char fileLocation[4];
        
                        memset(&fileLocation[0], 0, sizeof(fileLocation)); //get the cluster of the file
                        fileLocation[0] = entry.highClusterStart[0];
                        fileLocation[1] = entry.highClusterStart[1];
                        fileLocation[2] = entry.lowClusterStart[0];
                        fileLocation[3] = entry.lowClusterStart[1];
                        file = &fileLocation;
                        unsigned int location = file[0]/256/256;


                        fseek(fp, 0, SEEK_SET);
                        for (int i = 0; i < FATLBAStart + rootDirStart + (location*8); i++) //go to the stsrat of the file
                        {
                            fread(sector, 512, 1, fp);
                        }
                        
                        FILE* newFile;            //create our new file
                        newFile = fopen(checkLFN, "w");
                        if(strstr(checkLFN, ".txt") != NULL) //if were reading a text file
                        {
                        for(int i = 0; i < 8; i++)
                        {
                            fread(sector, 512, 1, fp);
                            for(int j = 0; j < 512; j++)
                            {
                                if((sector[j] >= 32) && (sector[j] <= 127)) //only take the characters we want
                                {
                                    unsigned char letter = sector[j];
                                    fputc(letter, newFile);
                                }
                            }
                        }
                        }
                        else //otherwise, read all of the information in
                        {
                            for(int i = 0; i < 8; i++)
                            {
                                fread(sector, 512, 1, fp);
                                for(int j = 0; j < 512; j++)
                                {
                                        unsigned char letter = sector[j];
                                        fputc(&letter, newFile);
                                }
                            }
                        }
                        fclose(newFile);//close connection to our new file
                        fileFound = 1; //we have found the file we were looking for
                        }
                        longFile = 0; //start back at the beginning for the next run
                        nextLongName = 0;
                        memset(&LongFileNames[0], 0, sizeof(LongFileNames));
                        memset(&checkLFN[0], 0, sizeof(checkLFN));
                    }
                    else//continue of we havent found the file yet
                    printf("");
                    longFile = 0;
                    memset(&LongFileNames[0], 0, sizeof(LongFileNames));
                    
                }
            }
        }
        count++;//move to nect directory cluser
    }
if(fileFound == 0) //if we didnt find the file print this message
    printf("File not found\n");

memset(&file[0], 0, sizeof(file));
fseek(fp, 0, SEEK_SET);
fclose(fp);
   
    }

}
return 1;
}