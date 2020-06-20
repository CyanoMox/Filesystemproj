#pragma once
#include <stdint.h>

typedef struct{
	unsigned int num_bitmap_cells; //bitmap len
	char* bitmap; //len = num_bitmap_cells
	char* padding; //a series of bytes to adjust bitmap size to a multiple of 4096 (page_size)
} BitMap;


// returns the index of the first cell having status "status"
// in the bitmap bmap, and starts looking from position start
int BitMap_get(BitMap* bitmap, int start, int status);

// sets the bit at index pos in bmap to status
int BitMap_set(BitMap* bitmap, int pos, int status);



//Functions implementation

int BitMap_get(BitMap* bitmap, int start, int status){
	int i = start;
	while ((int)bitmap->bitmap[i] != status){
		if(i<bitmap->num_bitmap_cells)
			i++;
		else return -1;
	}
	return i;
}


int BitMap_set(BitMap* bitmap, int pos, int status){
	
	if(status!=0 || status!=1) return -1;
	if(pos<bitmap->num_bitmap_cells){ 
		bitmap->bitmap[pos] = status; 
		return 0;
	}
	else return -1;
}
