//#pragma once
//#include "bitmap.h"
#include "disk_driver.h"
//#include "simplefs.h" 
#include <stdio.h>


//Testing bitmap and disk driver
void printDiskStatus(DiskDriver disk);

void writeBlock_test(DiskDriver disk, int block_mumber);
void freeBlock_test(DiskDriver disk, int block_number);
void readBlock_test(DiskDriver disk, int block_number);
void getFreeBlock_test(DiskDriver disk);
void resume_test(DiskDriver disk, int block_number);


int main (){
	DiskDriver disk;
	
	int block_number = 76458;
	int res = DiskDriver_init(&disk, "test_fs.hex", block_number);
	if (res==-1) {
		printf("AHIME'! \n");
		return -1;
	}
	else printDiskStatus(disk);
	
	/**Now testing getBlock, writeBlock, readBlock and freeBlock.	
	 * In the section below, comment the tests you don't want to run 
	 */
	
	//writeBlock test
	writeBlock_test(disk, block_number);
	
	//freeBlock test
	freeBlock_test(disk, block_number);
	
	//readBlock test
	readBlock_test(disk, block_number);
	
	//getFreeBlock test
	getFreeBlock_test(disk);
	
	//And now resume it!
	resume_test(disk, block_number);
	
	
	//BYE!
	DiskDriver_close(&disk);	
	return 0;
}

void printDiskStatus(DiskDriver disk){
	printf("\n");
	printf("First Block Mapped = %ud\n", disk.first_mapped_block);
	printf("File Descriptor = %d\n", disk.fd);
	printf("Number of Free Blocks = %d\n", disk.free_blocks);
	printf("First Block Offset \
				(concerning bitmap) = %d\n\n", disk.first_block_offset);
}


void writeBlock_test(DiskDriver disk, int block_number){
	char* src = malloc(sizeof(char)*BLOCK_SIZE);
	int i,j;
	for(j=0;j<block_number;j++){
		for(i=0;i<BLOCK_SIZE;i++) src[i] = 'A' + j;
		if(DiskDriver_writeBlock(&disk, src, j) == -1){
			printf("WriteBlock Error!!\n");
			exit(-1);
		}
	}
	printf("\nAfter writeBlock test:\n");
	printDiskStatus(disk);
	
	free(src);
}


void freeBlock_test(DiskDriver disk, int block_number){
	int i;
	for(i=0;i<block_number;i+=2){
		if(DiskDriver_freeBlock(&disk, i) != 0){
			printf("FreeBlock Error!!\n");
			exit(-1);
		}
	}
	printf("\nAfter freeBlock test:\n");
	printDiskStatus(disk);
}


void readBlock_test(DiskDriver disk, int block_number){
	int i;
	char* dest = (char*)malloc(BLOCK_SIZE+1);
	dest[BLOCK_SIZE] = '\0';
	for(i=0;i<block_number;i++){
		if(DiskDriver_readBlock(&disk, dest, i) == -2){
			printf("ReadBlock Error!!\n");
			exit(-1);
		}
		if(DiskDriver_readBlock(&disk, dest, i) == -1)
			printf("Empty Block\n");
		else printf("Reading block:\n%s\n",dest);
	}
	free(dest);
	printf("\nAfter readBlock test:\n");
	printDiskStatus(disk);
}


void getFreeBlock_test(DiskDriver disk){
	printf("Free Block: %d\n", DiskDriver_getFreeBlock(&disk, 0));
	printf("Free Block: %d\n", DiskDriver_getFreeBlock(&disk, 1));
	printf("Free Block: %d\n", DiskDriver_getFreeBlock(&disk, 31));
}


void resume_test(DiskDriver disk, int block_number){
	printf("\n\nTesting disk resumation\n\n");
	
	if(DiskDriver_resume(&disk, "test_fs.hex") == -1){
		printf("Resume Error!!");
	}
	
	char* src = malloc(sizeof(char)*BLOCK_SIZE);
	int i,j;
	for(j=0;j<block_number;j++){
		for(i=0;i<BLOCK_SIZE;i++) src[i] = 'B' + j;
		if(DiskDriver_writeBlock(&disk, src, j) == -1){
			printf("Allocation error in Resume test!\n");
			exit(-1);
		}
	}
	printf("\nAfter writeBlock test:\n");
	printDiskStatus(disk);
	
	
	if(DiskDriver_freeBlock(&disk, 1) == -1){ //Free block n. 1
		printf("FreeBlock Error!!\n");
		exit(-1);
	}
	
	printf("Freeing one last block\n");
	printDiskStatus(disk);
	
	free(src);
}
