//#pragma once
//#include "bitmap.h"
#include "disk_driver.h"
//#include "simplefs.h" 
#include <stdio.h>


//Testing bitmap and disk driver
void printDiskStatus(DiskDriver* disk);

int main (){
	DiskDriver disk;
	
	int block_number = 31; //TODO: DiskDriver_resume is dipendent by this value. FIX THAT.
	int res = DiskDriver_init(&disk, "test_fs.hex", block_number);
	if (res==-1) {
		printf("AHIME'! \n");
		return -1;
	}
	else printDiskStatus(&disk);
	
	/**Now testing getBlock, writeBlock, readBlock and freeBlock.	
	 * In the section below, comment the tests you don't want to run 
	 */
	
	//writeBlock test
	char* src = malloc(sizeof(char)*BLOCK_SIZE);
	int i,j;
	for(j=0;j<block_number;j++){
		for(i=0;i<BLOCK_SIZE;i++) src[i] = 'A' + j;
		if(DiskDriver_writeBlock(&disk, src, j)==-1){
			printf("ACCIPICCHIA!\n");
			exit(-1);
		}
	}
	printf("\nAfter writeBlock test:\n");
	printDiskStatus(&disk);
	
	//freeBlock test
	for(j=0;j<block_number;j+=2){
		if(DiskDriver_freeBlock(&disk, j)==-1){
			printf("OLLALLA'!\n");
			exit(-1);
		}
	}
	printf("\nAfter freeBlock test:\n");
	printDiskStatus(&disk);
	
	//readBlock test
	char* dest = (char*)malloc(BLOCK_SIZE+1);
	dest[BLOCK_SIZE] = '\0';
	for(j=0;j<block_number;j++){
		if(DiskDriver_readBlock(&disk, dest, j)==-2){
			printf("OLLALLA'!\n");
			exit(-1);
		}
		if(DiskDriver_readBlock(&disk, dest, j)==-1)
			printf("Empty Block\n");
		else printf("Reading block:\n%s\n",dest);
	}
	free(dest);
	printf("\nAfter readBlock test:\n");
	printDiskStatus(&disk);
	
	//getFreeBlock test
	printf("Free Block: %d\n", DiskDriver_getFreeBlock(&disk, 0));
	printf("Free Block: %d\n", DiskDriver_getFreeBlock(&disk, 1));
	printf("Free Block: %d\n", DiskDriver_getFreeBlock(&disk, 31));
	
	DiskDriver_close(&disk);
	
	return 0;
}

void printDiskStatus(DiskDriver* disk){
	printf("\n");
	printf("First Block Mapped = %ud\n", disk->first_mapped_block);
	printf("File Descriptor = %d\n", disk->fd);
	printf("Number of Free Blocks = %d\n", disk->free_blocks);
	printf("First Block Offset (concerning bitmap) = %d\n\n", disk->first_block_offset);
}
