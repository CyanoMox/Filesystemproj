//#pragma once
#include "bitmap.h"
#include "disk_driver.h"
//#include "simplefs.h" 
#include <stdio.h>


//Testing bitmap and disk driver
void printDiskStatus(DiskDriver* disk);

int main (){
	DiskDriver disk;
	int res = DiskDriver_init(&disk, "test_fs", 3);
	if (res==-1) {
		printf("AHIME'! \n");
		return -1;
	}
	else printDiskStatus(&disk);
	
	//Now test getBlock, writeBlock and readBlock
	char* src = malloc(sizeof(char)*BLOCK_SIZE);
	int i;
	for(i=0;i<BLOCK_SIZE;i++) src[i] = 0xA1;
	DiskDriver_writeBlock(&disk, src, 0);
	
	return 0;
}

void printDiskStatus(DiskDriver* disk){
	printf("\n");
	printf("First Block Mapped = %ud\n", disk->first_mapped_block);
	printf("File Descriptor = %d\n", disk->fd);
	printf("Number of Free Blocks = %d\n", disk->free_blocks);
	printf("First Block Offset (concerning bitmap) = %d\n", disk->first_block_offset);
}
