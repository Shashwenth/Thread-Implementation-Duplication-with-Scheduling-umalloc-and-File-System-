/*
    Rutgers University, New Brunswick
    CS 518 -- Operating System Theory
    A1: Umalloc!
    Group members: Himanshu Rathi(hr393), Shashwenth Muralidharan(sm2785)
    ilab Machine Tested: rlab1.cs.rutgers.edu
*/

#include <stdio.h>
#include "umalloc.h"
#include <string.h>
#include <time.h>


void* umalloc(size_t size, char* file, int line)
{
    size_t new_size = MEM_SIZE - metadata_size;
    if(size>new_size)
    {
        //printf("Requested Memory Exceeds the available Memory\n");
        //If the requested size is greater than the total allocatable size return NULL with the below error
        printf("Error in %s at line %d: Requested Memory Exceeds the available Memory\n",file, line);
        return NULL;
    }

    if(size<1){
        //If the requested size is 0, return NULL with the below error
        printf("Error in %s at line %d: Size to be allocated should be greater than 0\n",file, line);
        return NULL;
    }

    //For the very first time when umalloc is called, update the metadata considering the complete array as one block and set init to 1
    if(init=='0')
    {
        //Initialize the metadata     
        ((block_info*)&mem[0])->block_size = new_size;
        ((block_info*)&mem[0])->block_allocated = 'N';
        
        init = '1';
    }
    
    //Find a freeNode with the needed size in the First-Fit Fashion
    size_t expected_size = size;
    char *temp;

    //to keep track of the the available size summed up across all the unallocated fragments of the memory/char array
    size_t availableFreeSize = 0;
    for(temp = &mem[0];temp<&mem[MEM_SIZE]; temp = temp + ((block_info*)temp)->block_size + metadata_size)
    {
       // printf("Address of Temp: %p\n", temp);
        //check if a particular block is allocated or not, if not, only then proceed further
            if(((block_info*)temp)->block_allocated=='N')
            {
                if(temp + expected_size + metadata_size <= &mem[MEM_SIZE])
                {
                    //if the available block has same size as the requested size, simple return the start of block and set its allocated flag to 'Y'
                    if(((block_info*)temp)->block_size==expected_size)
                    {
                            ((block_info*)temp)->block_allocated='Y';
                            //add metadata size as the metadata will be stored at the start of the block, after which the actual block address will start
                            return (void*)(temp+metadata_size);
                    }
                    else if(((block_info*)temp)->block_size>expected_size)
                    {
                        //printf("Free block Available\n");
                        break;
                    }
                    else
                    {
                        availableFreeSize += ((block_info*)temp)->block_size;
                    }
                }
            }
       
    }
    //printf("Last Address of MEM: %p\n",&mem[MEM_SIZE]);
    //printf("Address of temp: %p\n",temp);
    if(temp>=&mem[MEM_SIZE])
    {
        //Case when there is enough free memory, but there is no block large enough for the allocation
        if(expected_size<=availableFreeSize)
        {
            printf("Error in %s at line %d: No block available to allocate the requested Size\n",file, line);
            return NULL;    
        }
        //Case when there is no free memory
        else if(availableFreeSize==0)
        {
            printf("Error in %s at line %d: No Free memory available for allocation\n",file, line);
            return NULL;        
        }
        //Case when memory is not full but there is not enough free memory for the allocation
        else
        {
            printf("Error in %s at line %d: Requested Size is more than the available Free memory\n",file, line);
            return NULL;
        }
    }

    //Once the available block is found.. allocate the necessary size and assign the remaining size(if any) as a new free block
    
    //compute the size of the adjacent block  (size remaining after allocating the requested size)
    size_t adjblock_size = ((block_info*)temp)->block_size - expected_size - metadata_size;

    if(((block_info*)temp)->block_size - expected_size -1 >=metadata_size) //to avaoid the case where a fragment is created with a size<=sizeof(metadata)
    {
        char *adj_block = temp + expected_size + metadata_size;
        //Update the metadata of the original block
        ((block_info*)temp)->block_allocated = 'Y';
        ((block_info*)temp)->block_size = size;

        //Assign the correct metadata info for the adjacent block
        ((block_info*)adj_block)->block_allocated = 'N';
        ((block_info*)adj_block)->block_size = adjblock_size;
    }
    else
    {
        ((block_info*)temp)->block_allocated = 'Y';
        //((block_info*)temp)->block_size = size;
    }

    return (void*)(temp+metadata_size);
}


void ufree(void* ptr, char* file, int line)
{
    //Handling some exceptions
    if(ptr==NULL)
    {
        printf("Error in %s at line %d: Attempted to free a NULL address\n",file, line);
        return;
    }
    //printf("Address of pointer to be freed: %p\n", ptr);
    //check if the ptr passed is in the correct address space. If Not simply return 
    if((char*)ptr < mem + metadata_size)
    {
        printf("Error in %s at line %d: Attempted to free an out of bound memory address\n",file, line);
        return;
    }

    if((char*)ptr >= &mem[MEM_SIZE])
    {
        printf("Error in %s at line %d: Attempted to free an out of bound memory address\n",file, line);
        return;
    }

    if(((block_info*)((char*)ptr - metadata_size))->block_allocated != 'Y' && ((block_info*)((char*)ptr - metadata_size))->block_allocated != 'N')
    {
        printf("Error in %s at line %d: Attempted to free an incorrect memory location\n", file, line);
        return;
    }

    //computing the address of metadata of the passed pointer/allocated address
    char* ptr_metadata = (char*)ptr - metadata_size;
    char *temp;
    //printf("Metadata address of pointer to be freed: %p\n", ptr_metadata);
    for(temp = &mem[0];temp<&mem[MEM_SIZE]; temp = temp + ((block_info*)temp)->block_size + metadata_size)
    {
        if(ptr_metadata == temp) //=> address to be freed is allocated by umalloc
        {    
            //If the metadata of the address passed is already free()ed, return the error accordingly
            if(((block_info*)temp)->block_allocated=='N')
            {
                printf("Error in %s at line %d: Address Passed is already Free()ed\n", file, line);
                return;
            }

            //If the address is not already Free()ed, update the allocated flag to 'N'
            ((block_info*)temp)->block_allocated = 'N';

            //size_t delta_size = 0;
            
            //Once the allotted memory is freed, check the entire address space and coalesce all the adjacent Free blocks into one block
            char *metadata_itr;
            for(metadata_itr = &mem[0];metadata_itr!=NULL && metadata_itr<&mem[MEM_SIZE];)
            {
                if(((block_info*)metadata_itr)->block_allocated=='N')
                {
                    char* adj_block = metadata_itr + metadata_size + ((block_info*)metadata_itr)->block_size;
                    //If the adjacent block is out of scope, return as there is nothing to be coalesced
                    if(adj_block>=&mem[MEM_SIZE] || adj_block==NULL)
                    {
                        return;
                    }
                    if(((block_info*)adj_block)->block_allocated=='N')
                    {
                        //add the size of the adjacent block along with its metadata in the previous blocks block_size
                        ((block_info*)metadata_itr)->block_size += ((block_info*)adj_block)->block_size + metadata_size;
                    }
                    else
                    {
                        //If the adjacent block is not free.. simply move the metadata iterator to the next block
                        metadata_itr = metadata_itr + metadata_size + ((block_info*)metadata_itr)->block_size;    
                    }    
                }
                else
                {
                    metadata_itr = metadata_itr + metadata_size + ((block_info*)metadata_itr)->block_size;
                }
            }


            return;
        }
    }   
    printf("Error in %s at line %d: Address Passed is not from the allocatable region\n", file, line);
}

void prettyPrint()
{
    printf("*********BLOCKS START***********\n");
    char *itr = &mem[0];
    int cnt=0;

    for(;((block_info*)itr)->block_allocated=='Y' ||((block_info*)itr)->block_allocated=='N';
    itr = itr + metadata_size + ((block_info*)itr)->block_size)
    {
        printf("\nStarting address of Metadata: %p\n", itr);
        printf("Block Allocated ? (Y/N): %c\n", ((block_info*)itr)->block_allocated);
        printf("Available/Allocated Block Size: %d\n", ((block_info*)itr)->block_size);
        printf("Starting address of Block(after Metadata): %p\n", itr + metadata_size);
    }
    printf("**************END***************\n");
}
