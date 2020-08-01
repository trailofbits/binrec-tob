#include <stdio.h> 
#include <stdlib.h> 

// This function is used in qsort to decide the relative order 
// of elements at addresses p and q. 
int comparator(const void *p, const void *q) 
{ 
	// Get the values at given addresses 
	int l = *(const int *)p; 
	int r = *(const int *)q; 

	// both odd, put the greater of two first. 
	if ((l&1) && (r&1)) 
		return (r-l); 

	// both even, put the smaller of two first 
	if ( !(l&1) && !(r&1) ) 
		return (l-r); 

	// l is even, put r first 
	if (!(l&1)) 
		return 1; 

	// l is odd, put l first 
	return -1; 
} 

// A utility function to print an array 
void printArr(int arr[], int n) 
{ 
	int i; 
	for (i = 0; i < n; ++i) 
		printf("%d ", arr[i]); 
} 

// Driver program to test above function 
int main() 
{ 
	int arr[] = {1, 6, 5, 2, 3, 9, 4, 7, 8}; 

	int size = sizeof(arr) / sizeof(arr[0]); 
	qsort((void*)arr, size, sizeof(arr[0]), comparator); 

	printf("Output array is\n"); 
	printArr(arr, size); 

	return 0; 
} 

