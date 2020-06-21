

/*** THIS FILE WAS DEPRECATED AND ITS FEATURES ARE INTEGRATED INTO disk_driver.h***/


#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct{
	unsigned int num_bitmap_cells; //bitmap len
	//char bitmap; //this is a CURSOR. Has to be read as "&" len = num_bitmap_cells
	//char padding; //a series of bytes to adjust bitmap size to a multiple of 4096 (page_size)
} BitMap;


// returns the index of the first cell having status "status"
// in the bitmap bmap, and starts looking from position start
int BitMap_get(BitMap* bitmap, int start, int status);

// sets the bit at index pos in bmap to status
int BitMap_set(BitMap* bitmap, int pos, int status);



//Functions implementation
/*
int BitMap_get(BitMap* bitmap, int start, int status){
	int i = start;
	char* cursor = &bitmap->bitmap;
	while ((int)cursor[i] != status){
		if(i<bitmap->num_bitmap_cells)
			i++;
		else return -1;
	}
	return i;
}
*/
int BitMap_get(BitMap* bitmap, int start, int status){
	int i = start;
	char* cursor = (char*)bitmap;
	cursor += sizeof(bitmap->num_bitmap_cells);
	while ((int)cursor[i] != status){
		if(i < bitmap->num_bitmap_cells)
			i++;
		else return -1;
	}
	return i;
}
/*
int BitMap_set(BitMap* bitmap, int pos, int status){
	char* cursor = &bitmap->bitmap;
	if(status!=0 && status!=1) return -1;
	if(pos < bitmap->num_bitmap_cells){ 
		printf("Bitmap_set DBG\n");
		cursor[pos] = status; // SEG FAULT!
		return 0;
	}
	else return -1;
}
*/
int BitMap_set(BitMap* bitmap, int pos, int status){
	char* cursor = (char*)bitmap;
	cursor += sizeof(int);

	if(status!=0 && status!=1) return -1;
	if(pos < bitmap->num_bitmap_cells){ 
		printf("Bitmap_set DBG\n");
		//cursor[pos] = status; // SEG FAULT! Writing protection enabled!
		memcpy(cursor+pos, &status, sizeof(char));
		return 0;
	}
	else return -1;
}
