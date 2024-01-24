/*

Rutgers University - New Brunswick
CS 518 - Operating Systems Design
A2: WriteOnce File System
Authors: Himanshu Rathi (hr393), Shashwenth Muralidharan (sm2785)

Tested on ilab : 

*/


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
//#include "writeOnce_FS.h"


/******Global Variables Definition******/


#define FS_SIZE 4*1024*1024
#define BLOCK_SIZE 1024
#define TOTAL_INODES 60
#define MAX_FILE_NAME_SIZE 50
#define BLOCK_DATA_SIZE 1020
#define TOTAL_DATA_BLOCKS 4031
#define UNUSEDMAP_BLOCKS 4

//defining flags
#define WO_RDONLY 1
#define WO_WRONLY 2
#define WO_RDWR 3

//defining mode
//#define WO_CREAT 4
#define CREATE_MODE 4
//static char BUFFER[FS_SIZE];

int init = 0;

char *BUFFER;
char* disk_name;

extern int errno;


typedef struct superblock{
    char uid[10];
    int total_blocks;
    //void* datablocks_start;
    //int inodes_start;
    int total_inodes;
    int total_datablocks;
    //int unused_datablocks;
    short total_files;
    //void* unused_arr_start;
}superblock;

typedef struct inode{
    char inuse;
    char status;
    int total_data_blocks;
    //char *start_data_block;
    short start_data_block;
    char filename[MAX_FILE_NAME_SIZE];
    unsigned short fd;
    int file_size;
    int curr_offset;
    short permission;
}inode;


typedef struct datablock{
    //void* nextblock;
    short nextblock;
    char data[1020];
}datablock;


//superblock *sblock;
int wo_mount(char* filename, void* mem);
int wo_create(char* filename, int flags);
int wo_open(char* filename, int flags);
//int wo_open(char* filename, int flags, ...);
int wo_read( int fd, void* buffer, int bytes);
int wo_write( int fd, void* buffer, int bytes);
int wo_umount(void* mem);
int wo_close(int fd);
void printInodes(void* address);
void printUnusedValues();
void printDataBlocks();
void printSuperBlock(void* address);


/***************************************/

int min(int a, int b)
{
    return a>b?b:a;
}

char* charconcat(char* dest, char* src, size_t n)
{
    //printf("In CharConcat\n");
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    dest[n] = '\0';
    //printf("Exiting CharConcat\n");    
    return dest;
}



void superblockInit(void* address)
{   
    strcpy(((superblock*)address)->uid,"RUNBF22OST");
    ((superblock*)address)->total_blocks = FS_SIZE/BLOCK_SIZE;
    ((superblock*)address)->total_inodes = TOTAL_INODES;
    ((superblock*)address)->total_datablocks = ((superblock*)address)->total_blocks - ((superblock*)address)->total_inodes - UNUSEDMAP_BLOCKS - 1;
    //((superblock*)address)->unused_datablocks = ((superblock*)address)->total_datablocks;
    ((superblock*)address)->total_files = 0;
}

void inodesInit(void* address)
{
    for(int i=0;i<TOTAL_INODES;i++)
    {
        address = address + BLOCK_SIZE;
        //printf()     
        ((inode*)address)->inuse = 'N';
        ((inode*)address)->status = 'C';
        ((inode*)address)->start_data_block = -1;
        ((inode*)address)->total_data_blocks = 0;
        ((inode*)address)->file_size = 0;
        ((inode*)address)->fd = i+1;
        ((inode*)address)->curr_offset = 0;
        ((inode*)address)->permission = -1;     
    }
}


void unusedMapInit(void* address)
{
    for(int i=0;i<UNUSEDMAP_BLOCKS*BLOCK_SIZE;i++)
    {
        *(char*)(address) = '0';
        address = address + 1;
    }
}


void datablocksInit(void* address)
{
    for(int i=0;i<TOTAL_DATA_BLOCKS;i++)
    {
        ((datablock*)address)->nextblock = -1;    
        address  = address + BLOCK_SIZE;
    }
}

int wo_mount(char* filename, void* mem)
{
    disk_name = filename;
    int fd = open(filename, O_RDWR | O_CREAT , 0777);
    if(fd<0)
    {
        errno = 1;
        return -1;
    }
    struct stat file_stat;
    fstat(fd, &file_stat);
    BUFFER = mem;

    if(init==1)
    {
        //printf("File System is already Mounted\n");
        errno = 1;
        // printf("Error : %s\n", strerror(errno));
        return -1; 
    }

    //printf("Mount file size: %ld\n",file_stat.st_size);
    if(file_stat.st_size == 0) 
    {
        superblockInit(mem);
        inodesInit(mem);
        mem = mem + (TOTAL_INODES+1)*BLOCK_SIZE;
        unusedMapInit(mem);
        mem = mem + UNUSEDMAP_BLOCKS*BLOCK_SIZE;
        datablocksInit(mem);
        //init == 1;
    }
    else
    {
        //read the filesystem in Buffer
        //int read_fs = read(fd, mem, FS_SIZE);
        char* temp = malloc(sizeof(superblock));
        int read_sb = read(fd, temp, sizeof(superblock));
        //printf("Read File System size: %d\n", file_stat.st_size);
        if(file_stat.st_size!=FS_SIZE)
        {
            // printf("File is broken\n");
            close(fd);
            errno = 1;
            //printf("Error : %s\n", strerror(errno));
            return -1;
        }
        //printf("Signature: %s\n", ((superblock*)temp)->uid);
        if(strcmp(((superblock*)temp)->uid, "RUNBF22OST")!=0)
        {
            //printf("File is broken2\n");
            close(fd);
            errno = 1;
            //printf("Error : %s\n", strerror(errno));
            return -1;
        }
        lseek(fd, 0, SEEK_SET);
        int read_fs = read(fd, mem, FS_SIZE);
        close(fd);
        //init = 1;
    }

    
    init = 1;
    close(fd);
    //printf("Returning from open\n");
    return 0;
}


int CheckIfFileExists(char* open_file)
{
    void* arr = BUFFER + BLOCK_SIZE;
    char temp[MAX_FILE_NAME_SIZE];
    strncpy(temp,open_file, MAX_FILE_NAME_SIZE);
    //printf("Passed File name: %s\n", temp);
    for(int i=0;i<TOTAL_INODES;i++)
    {
        if(((inode*)arr)->inuse=='Y')
        {
            if(strcmp(((inode*)arr)->filename,temp)==0)
            {    
                if(((inode*)arr)->status=='C')
                    return 1;
                else 
                    return 0;
            }
        }
       
        arr = arr + BLOCK_SIZE;
    }
    return -1;
}


void* FindFreeInode(char* open_file, int mode)
{
    void* arr = BUFFER + BLOCK_SIZE;
    //printf("Start Address : %p\n", arr);
    //printf("Inode In Use: %c\n",((inode*)arr)->inuse);
    //printf("Inode File Name: %s\n",((inode*)arr)->filename);
    char temp[MAX_FILE_NAME_SIZE];
    strncpy(temp,open_file, MAX_FILE_NAME_SIZE);
    for(int i=0;i<TOTAL_INODES;i++)
    {
        if(mode==4)
        {
            if(((inode*)arr)->inuse=='N')
                return (void*)arr;
        }
        else
        {
            if(((inode*)arr)->inuse=='Y' && strcmp(((inode*)arr)->filename,temp)==0)
                return (void*)arr;
        }
        arr = arr + BLOCK_SIZE;
    }
    return NULL;
}


int FindFreeDataBlock()
{
    //void* add = ((superblock*)BUFFER)->unused_arr_start;
    void* add = BUFFER + BLOCK_SIZE*(TOTAL_INODES+1);
    for(int i=0;i<TOTAL_DATA_BLOCKS;i++)
    {
        if(*(char*)add == '0')
        {
            //*(char*)add = '1';
            return i;
        }
        add = add+1;
    }
    return -1;
}


int wo_create(char* filename, int flags)
{
    if(!filename)
    {
        //print the error
        errno = 22;
        //printf("Error : %s\n", strerror(errno));
        return -1;
    }

    if(init == 0)
    {
        //printf("The Disk is not mounted Correctly\n");
        errno = 1;
        //printf("Error : %s\n", strerror(errno));
        return -1;
    }

    if(flags<1 || flags>3)
    {
        //printf("Incorrect Flag Passed!! File Can be Opened using one of WO_RDONLY, WO_WRONLY, WO_RDWR flags only\n");
        errno = 13;
        //printf("Error : %s\n", strerror(errno));
        return -1;
    }

    int fileexists = CheckIfFileExists(filename);
    //printf("File Exists: %d\n", fileexists);
    if(fileexists == 1 || fileexists == 0)
    {
        //printf("The file to be created already exists\n");
        errno = 17;
        //printf("Error : %s\n", strerror(errno));
        return -1;
    }
    //check if total no of files is within the limits;
    if(((superblock*)BUFFER)->total_files >= TOTAL_INODES)
    {
        //printf("Exceeding Max number of allocatable files\n");
        errno = 23;
        //printf("Error : %s\n", strerror(errno));
        return -1;
    }
    //else create the file
    void* freeInode = FindFreeInode(filename, CREATE_MODE);
    //printf("Free Inode address: %p\n",freeInode);
    strncpy(((inode*)freeInode)->filename, filename, 12);
    ((inode*)freeInode)->inuse = 'Y';
    ((inode*)freeInode)->status = 'O';
    ((inode*)freeInode)->permission = flags;
    int filedesc = ((inode*)freeInode)->fd;
    ((superblock*)BUFFER)->total_files++;
    return filedesc;

}


//int wo_open(char* filename, int flags, ...)
int wo_open(char* filename, int flags)
{
    if(!filename)
    {
        //print the error
        errno = 22;
        //printf("Error : %s\n", strerror(errno));
        return -1;
    }

    if(flags<1 || flags>3)
    {
        //printf("Incorrect Flag Passed!! File Can be Opened using one of WO_RDONLY, WO_WRONLY, WO_RDWR flags only\n");
        errno = 13;
        //printf("Error : %s\n", strerror(errno));
        return -1;
    }

    //Access the mode parameter
    // va_list args;
    // int mode = -1;
    // va_start(args, flags);
    // mode = va_arg(args,int);
    // va_end(args);
    //printf("Passed Mode: %d\n", mode);
    //Handle errors like File does not exist, incorect mode, incorrect flag
    int mode = 0;
    if(mode==4) //if opening in create mode ----- disabled right now
    {
        //printf("In Open\n");
        //if the file already exists return with error
        int fileexists = CheckIfFileExists(filename);
        //printf("File Exists: %d\n", fileexists);
        if(fileexists == 1 || fileexists == 0)
        {
            printf("The file to be created already exists\n");
            return -1;
        }
        //check if total no of files is within the limits;
        if(((superblock*)BUFFER)->total_files >= TOTAL_INODES)
        {
            printf("Exceeding Max number of allocatable files\n");
            return -1;
        }
        //else create the file
        void* freeInode = FindFreeInode(filename, mode);
        //printf("Free Inode address: %p\n",freeInode);
        strncpy(((inode*)freeInode)->filename, filename, 12);
        ((inode*)freeInode)->inuse = 'Y';
        ((inode*)freeInode)->status = 'O';
        ((inode*)freeInode)->permission = flags;
        int filedesc = ((inode*)freeInode)->fd;
        ((superblock*)BUFFER)->total_files++;
        return filedesc;
    }
    else
    { 
        //if opening 'NOT' in create mode
        //if the file does not exit ..return with error
        int fileexists = CheckIfFileExists(filename);
        //printf("File Exists: %d\n", fileexists);
        if(fileexists == -1)
        {
            //printf("File Attempted to be opened does not exist\n");
            errno = 2;
            //printf("Error : %s\n", strerror(errno));
            return -1;
        }
        else if(fileexists == 0)
        {
            //printf("File Attempted to be opened is already opened\n");
            errno = 1;
            //printf("Error : %s\n", strerror(errno));
            return -1;
        }
        //else open the existing file
        void* freeInode = FindFreeInode(filename, mode);
        //printf("Free Inode address: %p\n",freeInode);
        ((inode*)freeInode)->status = 'O';
        ((inode*)freeInode)->permission = flags;
        int filedesc = ((inode*)freeInode)->fd;
        return filedesc;
    }

    return -1;
}


int wo_write( int fd, void* buffer, int bytes)
{
    //Check if fd is valid
    if(fd<1 || fd>TOTAL_INODES)
    {
        //printf("Incorrect FD! Does not exist\n");
        errno = 9;
        //printf("Error : %s\n", strerror(errno));
        return -1;
    }
    //If fd is valid, check if the file is Open, else return with error
    void* inode_add = BUFFER + BLOCK_SIZE*fd;
    if(((inode*)inode_add)->permission == 1)
    {
        //printf("File to be written has Read Only permission\n");
        errno = 13;
        //printf("Error : %s\n", strerror(errno));
        return -1;
    }

    if(((inode*)inode_add)->inuse=='N')
    {
        //printf("File nt found\n");
        errno = 2;
        //printf("Error : %s\n", strerror(errno));
        return -1;
    }

    if(((inode*)inode_add)->status=='C')
    {
        //printf("File to be written is Already Closed\n");
        errno = 2;
        //printf("Error : %s\n", strerror(errno));
        return -1;
    }
    /*
    If Open, check if start block is NULL, 
    if NULL.. find the freedatablock, update the start_data_block in inode,
    First 8 bytes of the datablock will be pointer to next data block of the file
    Rest 1016B will hold that data in that block 
    */

    // void* fdblock = BUFFER + BLOCK_SIZE*(TOTAL_INODES+1)+UNUSEDMAP_BLOCKS*BLOCK_SIZE+freeIdx*BLOCK_SIZE;
    if(((inode*)inode_add)->start_data_block == -1)
    {
        int written_bytes = 0;
        int freeIdx = FindFreeDataBlock();
        //printf("Free Data block Index: %d\n", freeIdx);
        if(freeIdx==-1){
            //printf("No Free Data block available\n");
            return written_bytes;
        }

        void* fdblock = BUFFER + BLOCK_SIZE*(TOTAL_INODES+1)+UNUSEDMAP_BLOCKS*BLOCK_SIZE+freeIdx*BLOCK_SIZE;
        //Updating the starting data block in inode
        ((inode*)inode_add)->start_data_block = freeIdx;
        //int req_blocks = ceil((double)bytes/(double)BLOCK_DATA_SIZE);
        
        int req_blocks = (bytes/BLOCK_DATA_SIZE) + ((bytes%BLOCK_DATA_SIZE)==0);
        //printf("Required Blocks: %d\n", req_blocks);

        //char temp[bytes];
        //strcpy(temp, buffer);
        //((datablock*)fdblock)->data = temp;

        strncpy(((datablock*)fdblock)->data,(char*)buffer, min(bytes, BLOCK_DATA_SIZE));  
        written_bytes += min(bytes, BLOCK_DATA_SIZE);
        //printf("Copied Data: %s\n",((datablock*)fdblock)->data);
        ((inode*)inode_add)->file_size += min(bytes, BLOCK_DATA_SIZE);
        ((inode*)inode_add)->total_data_blocks++;
        *(char*)(BUFFER+BLOCK_SIZE*(TOTAL_INODES+1)+freeIdx) = '1';
        req_blocks--;
        // datablock* prev = fdblock;
        int skipcnt = 1;
        int rem_bytes = bytes-BLOCK_DATA_SIZE;
        while(req_blocks>0 && rem_bytes>0)
        {
          int nextFreeIdx = FindFreeDataBlock();         
          if(nextFreeIdx==-1){
            //printf("No Free Data block available\n");
            return written_bytes;
          }
          //printf("Next Free Index: %d\n", nextFreeIdx);
          ((datablock*)fdblock)->nextblock = nextFreeIdx;
          fdblock = BUFFER + BLOCK_SIZE*(TOTAL_INODES+1)+UNUSEDMAP_BLOCKS*BLOCK_SIZE+nextFreeIdx*BLOCK_SIZE;
          strncpy(((datablock*)fdblock)->data,(char*)buffer + BLOCK_DATA_SIZE*skipcnt , min(BLOCK_DATA_SIZE,rem_bytes));
          written_bytes +=  min(BLOCK_DATA_SIZE,rem_bytes);
          skipcnt++;      
          req_blocks--;
          ((inode*)inode_add)->file_size += min(BLOCK_DATA_SIZE,rem_bytes);
          ((inode*)inode_add)->total_data_blocks++;
          *(char*)(BUFFER+BLOCK_SIZE*(TOTAL_INODES+1)+nextFreeIdx) = '1';
          rem_bytes -= min(BLOCK_DATA_SIZE,rem_bytes); 
        }
        return written_bytes;
    }
    else
    {
        //Traverse the datablocks till the next Index is -1
        void* dblock = BUFFER + BLOCK_SIZE*(TOTAL_INODES+1) + UNUSEDMAP_BLOCKS*BLOCK_SIZE + ((inode*)inode_add)->start_data_block*BLOCK_SIZE;
        while(((datablock*)dblock)->nextblock !=-1)
        {
            dblock = BUFFER + BLOCK_SIZE*(TOTAL_INODES+1) + UNUSEDMAP_BLOCKS*BLOCK_SIZE + ((datablock*)dblock)->nextblock*BLOCK_SIZE;
        } 
        int curr_data_blocks = ((inode*)inode_add)->total_data_blocks;
        int curr_file_size = ((inode*)inode_add)->file_size;
        // if(curr_data_blocks*BLOCK_DATA_SIZE == curr_file_size)
        // {
        //     //All the used datablocks are fully occupied
        // }
        // else
        // {
            //Last datablock is NOT fully occupied
            //printf("In the else part~~~~~~~\n");
            char temp[bytes];
            strcpy(temp, buffer);
            //int last_block_size = curr_data_blocks*BLOCK_DATA_SIZE - curr_file_size;
            int last_block_size = curr_file_size - (curr_data_blocks-1)*BLOCK_DATA_SIZE;
            int last_block_remaining = BLOCK_DATA_SIZE - last_block_size;
            int written_bytes = min(last_block_remaining, bytes);
            //printf("Last Block Size: %d\n", last_block_size);
            //printf("Last Block Remaining Size: %d\n", last_block_remaining);
            //printf("Temp Data: %s\n",temp);
            //printf("Data Block Before Writing :%s\n",((datablock*)dblock)->data);
            strncat(((datablock*)dblock)->data, temp, min(last_block_remaining,bytes));
            //printf("Data Block After Writing :%s\n",((datablock*)dblock)->data);
            int rem_bytes = bytes - min(last_block_remaining,bytes);
            
            int skipcnt = 0;
            //int req_blocks = ceil((double)rem_bytes/(double)BLOCK_DATA_SIZE);
            int req_blocks = (rem_bytes/BLOCK_DATA_SIZE) + ((rem_bytes%BLOCK_DATA_SIZE)!=0);
            //printf("Required Blocks: %d\n", req_blocks);
            //temp = temp + last_block_remaining;
            ((inode*)inode_add)->file_size += min(BLOCK_DATA_SIZE,min(last_block_remaining,bytes));

            char flag = 'Y';
            while(req_blocks>0 && rem_bytes>0)
            {
                //printf("Remaining Bytes: %d\n", rem_bytes);
                int nextFreeIdx = FindFreeDataBlock();         
                if(nextFreeIdx==-1){
                    //printf("No Free Data block available\n");
                    return written_bytes;
                }
                //printf("Next Free Index: %d\n", nextFreeIdx);
                ((datablock*)dblock)->nextblock = nextFreeIdx;
                dblock = BUFFER + BLOCK_SIZE*(TOTAL_INODES+1)+UNUSEDMAP_BLOCKS*BLOCK_SIZE+nextFreeIdx*BLOCK_SIZE;
                // if(flag=='Y'){
                //   strncpy(((datablock*)dblock)->data,(char*)temp + last_block_remaining, min(BLOCK_DATA_SIZE,rem_bytes));
                //   flag='N';  
                // }
                // else
                strncpy(((datablock*)dblock)->data,(char*)temp + last_block_remaining + BLOCK_DATA_SIZE*skipcnt , min(BLOCK_DATA_SIZE,rem_bytes));  
                written_bytes += min(BLOCK_DATA_SIZE,rem_bytes);
                skipcnt++;      
                req_blocks--;
                ((inode*)inode_add)->file_size += min(BLOCK_DATA_SIZE,rem_bytes);
                ((inode*)inode_add)->total_data_blocks++;
                *(char*)(BUFFER+BLOCK_SIZE*(TOTAL_INODES+1)+nextFreeIdx) = '1';
                rem_bytes -= min(BLOCK_DATA_SIZE,rem_bytes); 
            }
            return written_bytes;
        //}
    }  
    return -1;
}



int wo_read(int fd, void* buffer, int bytes)
{
    //printf("In read function\n");

    if(fd<1 || fd>TOTAL_INODES)
    {
        //printf("Entered File descriptor does not exist\n");
        errno = 9;
        //printf("Error : %s\n", strerror(errno));
        return -1;
    }
    
    void* inode_add = BUFFER + BLOCK_SIZE*fd;

    if(((inode*)inode_add)->permission == 2)
    {
        //printf("File to be read has Write Only permission\n");
        errno = 13;
        //printf("Error : %s\n", strerror(errno));
        return -1;
    }
    //printf("In USe: %c\n",((inode*)inode_add)->inuse);
    if(((inode*)inode_add)->inuse=='N')
    {
        //printf("File not found\n");
        errno = 2;
        //printf("Error : %s\n", strerror(errno));
        return -1;
    }
    //printf("status: %c\n",((inode*)inode_add)->status);
    if(((inode*)inode_add)->status=='C')
    {
        //printf("File attempted to read is closed\n");
        errno = 2;
        //printf("Error : %s\n", strerror(errno));
        return -1;
    }
    //printf("file_size: %d\n",((inode*)inode_add)->file_size);
    if(((inode*)inode_add)->file_size == 0)
    {
        //printf("File attempted to read is empty\n");
        return 0;
    }

    /// Handling current offset??--- Check if file is already read? if not conitnue from the last offset?

    int curr_data_blocks = ((inode*)inode_add)->total_data_blocks;
    int curr_file_size = ((inode*)inode_add)->file_size;
    int curr_file_offset = ((inode*)inode_add)->curr_offset;
    int db_start_idx = ((inode*)inode_add)->start_data_block;

    if(curr_file_offset >= curr_file_size)
    {
        //printf("The file to be read is already fully read\n");
        return 0;
    }

    //char temp[curr_file_size];

    void* dbblock = BUFFER + (TOTAL_INODES+1)*BLOCK_SIZE + UNUSEDMAP_BLOCKS*BLOCK_SIZE + db_start_idx*BLOCK_SIZE;
    //int rem_bytes = min(curr_file_size, bytes);
    int rem_bytes = min(curr_file_size - curr_file_offset, bytes);
    int read_size = rem_bytes;
    int already_read_blocks = curr_file_offset/BLOCK_DATA_SIZE;
    int last_read_block_size = curr_file_offset - (already_read_blocks)*BLOCK_DATA_SIZE;    
    //printf("Last read Block size: %d\n", last_read_block_size);
    int rem_offset = curr_file_offset;
    while(already_read_blocks-- > 0)
    {
        //rem_bytes -= BLOCK_DATA_SIZE;
        rem_offset -= BLOCK_DATA_SIZE;
        int nextIdx = ((datablock*)dbblock)->nextblock;
        dbblock = BUFFER + (TOTAL_INODES+1)*BLOCK_SIZE + UNUSEDMAP_BLOCKS*BLOCK_SIZE + nextIdx*BLOCK_SIZE;
    }

    // void *temp = ((datablock*)dbblock)->data + rem_offset;
    void* temp = dbblock;
    //rem_bytes -= rem_offset;
    char flag = '1';
    while(rem_bytes>0)
    {
        //strncat((char*)buffer, ((datablock*)dbblock)->data, min(rem_bytes, BLOCK_DATA_SIZE));
        //printf("Current Data: %s\n",((datablock*)temp)->data);
        //printf("Remaining Bytes: %d\n", rem_bytes);
        //printf("Remaining Offset: %d\n", rem_offset);
        if(flag=='1')
        {
            charconcat((char*)buffer, ((datablock*)temp)->data + rem_offset, min(rem_bytes, BLOCK_DATA_SIZE - last_read_block_size));
            //printf("Data copied in Buffer: %s\n",(char*)buffer);
            buffer = buffer + min(rem_bytes, BLOCK_DATA_SIZE - last_read_block_size);
            ((inode*)inode_add)->curr_offset += min(rem_bytes, BLOCK_DATA_SIZE - last_read_block_size);
            //printf("Inode Curr Offset: %d\n", ((inode*)inode_add)->curr_offset);
            rem_bytes = rem_bytes - min(rem_bytes, BLOCK_DATA_SIZE - last_read_block_size);
            int nextIdx = ((datablock*)temp)->nextblock;
            //printf("Next Data block: %d\n", ((datablock*)temp)->nextblock);
            temp = BUFFER + (TOTAL_INODES+1)*BLOCK_SIZE + UNUSEDMAP_BLOCKS*BLOCK_SIZE + nextIdx*BLOCK_SIZE;
            //printf("Next Data block: %d\n", ((datablock*)temp)->nextblock);
            flag = '0';
        }
        else
        {
            //strncat((char*)buffer, ((datablock*)dbblock)->data, min(rem_bytes, BLOCK_DATA_SIZE));
            charconcat((char*)buffer, ((datablock*)temp)->data, min(rem_bytes, BLOCK_DATA_SIZE));
            //printf("Data copied in Buffer: %s\n",(char*)buffer);
            ((inode*)inode_add)->curr_offset += min(rem_bytes, BLOCK_DATA_SIZE);
            buffer = buffer + min(rem_bytes, BLOCK_DATA_SIZE);
            rem_bytes = rem_bytes - min(rem_bytes, BLOCK_DATA_SIZE);
            int nextIdx = ((datablock*)temp)->nextblock;
            //printf("Next Data block: %d\n", ((datablock*)temp)->nextblock);
            temp = BUFFER + (TOTAL_INODES+1)*BLOCK_SIZE + UNUSEDMAP_BLOCKS*BLOCK_SIZE + nextIdx*BLOCK_SIZE;
        }
    }

    //((inode*)inode_add)->curr_offset += min(curr_file_size, bytes);

    return read_size;
}




int wo_umount(void* mem)
{
    if(init==0)
    {
        errno = 1;
        return -1;
    }

    int fd = open(disk_name, O_RDWR | O_TRUNC, 0644);
    
    void* temp = mem;
    for(int i=0;i<TOTAL_INODES;i++)
    {
        temp = temp + BLOCK_SIZE;    
        if(((inode*)temp)->status=='O')
        {
          ((inode*)temp)->status = 'C';
          ((inode*)temp)->curr_offset = 0;     
        }
    }
    int write_bytes = write(fd, mem, FS_SIZE);
    //printf("Total Bytes written: %d\n", write_bytes);
    init = 0;
    //free(mem);
    //free(BUFFER);
    close(fd);
    return 0;
}


int wo_close(int fd)
{
    if(fd<1 || fd>TOTAL_INODES)
    {
        //printf("Incorrect FD! Does not exist\n");
        errno = 9;
        return -1;
    }

    void* add = BUFFER + BLOCK_SIZE*fd;
    if(((inode*)add)->status=='C')
    {
        //printf("File is Already Closed\n");
        errno = 1;
        return -1;
    }
    ((inode*)add)->status='C';
    ((inode*)add)->curr_offset=0;
    return 0;
}



void printSuperBlock(void* address)
{
    printf("Superblock start: %p\n",address); 
    printf("Total Blocks: %d\n",((superblock*)address)->total_blocks);
    printf("Total Data Blocks: %d\n",((superblock*)address)->total_datablocks); 
    printf("Total Inode Blocks: %d\n",((superblock*)address)->total_inodes);
    printf("Total Files: %d\n",((superblock*)address)->total_files); 
    //printf("Data Blocks Start: %p\n",((superblock*)address)->datablocks_start); 
    //printf("Unused Array Start: %p\n",((superblock*)address)->unused_arr_start);  
}

void printInodes(void* address)
{
    printf("\n*****INODES******\n");
    for(int i=0;i<TOTAL_INODES;i++)
    {
        address = address + BLOCK_SIZE;
        printf("File Decriptor: %d\n",((inode*)(address))->fd);
        printf("File InUse: %c\n",((inode*)(address))->inuse);  
        printf("File Opened/Closed: %c\n",((inode*)(address))->status);
        printf("File Name: %s\n",((inode*)(address))->filename);
        printf("Total Data Blocks: %d\n",((inode*)(address))->total_data_blocks);
        printf("File Size: %d\n",((inode*)(address))->file_size);    
        printf("File Current Offset: %d\n",((inode*)(address))->curr_offset);
        printf("File Start Data block Index: %d\n",((inode*)(address))->start_data_block);      
    }
    printf("\n*****INODES END******\n");
}


void printDataBlocks()
{
    //void* address = ((superblock*)BUFFER)->datablocks_start;
    void* address = BUFFER + BLOCK_SIZE*(TOTAL_INODES+1) + UNUSEDMAP_BLOCKS*BLOCK_SIZE;
    for(int i=0;i<TOTAL_DATA_BLOCKS;i++)
    {
       // if(i<15){
            printf("Index of Next Block: %d\n", ((datablock*)address)->nextblock);
            printf("Data: %s\n", ((datablock*)address)->data);
            
        //}
        address = address + BLOCK_SIZE;
    }
}

void printUnusedValues()
{
    //void* address = ((superblock*)BUFFER)->unused_arr_start;
    void* address = BUFFER + BLOCK_SIZE*(TOTAL_INODES+1);
    for(int i=0;i<TOTAL_DATA_BLOCKS;i++)
    {
        //if(i>=4039)
         printf("Used/Unused: %c\n",*(char*)(address));
        address = address+1;      
    } 
}