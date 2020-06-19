#pragma once
#include <stdint.h>

typedef struct{
	unsigned int num_bitmap_cells; //bitmap len
	char[num_bitmap_cells] bitmap;
} BitMap;


// returns the index of the first cell having status "status"
// in the bitmap bmap, and starts looking from position start
int BitMap_get(char* bitmap, int start, int status);

// sets the bit at index pos in bmap to status
int BitMap_set(char* bitmap, int pos, int status);



//Functions implementation

int BitMap_get(char* bitmap, int start, int status){
	int i=start;
	while (bitmap[i]!=status){ //TODO: fix type casts
		if(i<num_bitmap_cells)
			i++;
		else return -1;
	}
	return i;
}


int BitMap_set(char* bitmap, int pos, int status){
	
	if(status!=0 || status!=1) return -1;
	if(pos<num_bitmap_cells){ //checks for assignation
		bitmap[pos]=status; //TODO: fix type casts
		return 0;
	}
	else return -1;
}
