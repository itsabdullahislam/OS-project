#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include<wait.h>
#include<sys/ipc.h>

#define N 10 //maximum lenth of row and columns. This must be increased for big matrices.

typedef struct {
  int matrix[N][N];
  int numberOfRows;
  int numberOfColumns;
}
MatrixType;

// this is a type that holds 3 matrices to be used and shared in the memory
typedef struct {
  MatrixType matrixA, matrixB, mResult;
}
DataTableType;

void loadDataAndCreateSharedMem();
DataTableType * getDataTableFromSharedMem();
void readDataFromFile(char * fileName, MatrixType * matrix);
void createSharedMem(DataTableType * dataTable);
DataTableType * getDataTableFromSharedMem();
void calcMatrixThreaded(DataTableType * dataTable);
void printMatrix(DataTableType * dataTable);
void printMatrixAB(DataTableType * dataTable);

int main(void) 
{
  	pid_t pid;
  	pid = fork(); // process for loading and sharing memory
  	if (pid == 0) 
	{
    	printf("Process 1: reading data and sharing memory. (id = %d)\n", getpid());
    	loadDataAndCreateSharedMem();
    	return 0;
  	} 
	else if(pid > 0) 
	{
    	wait(NULL);
    	pid_t pid2 = fork(); // process for reading shared memory and multiplication using threads
    	if (pid2 == 0) 
		{
    		printf("Process 2: reading shared memory and calculate multiplication of matrices using threads. (id = %d)\n", getpid());
      		DataTableType * data = getDataTableFromSharedMem();
      		calcMatrixThreaded(data);
    	} 
		else if (pid2 > 0) 
		{
    	  	wait(NULL);
    	  	pid_t pid3 = fork(); // process for printing result
   			if (pid3 == 0) 
			{
    		    printf("Process 3: print result. (id = %d)\n", (int) getpid());
    	    	DataTableType * data = getDataTableFromSharedMem();
    	    	printMatrix(data);
    	  	} 
			else if (pid3 > 0) 
			{
    	    	wait(NULL);
    	  	}
    	}
 	}
  	return 0;
}

/*
 * this method:
 * 1- loads data from files
 * 2- prints the loaded matrices
 * 3- calls a method to create shared memory
 */
void loadDataAndCreateSharedMem() 
{
  	DataTableType * dataTable = malloc(sizeof(DataTableType));
  	readDataFromFile("matrixA.txt", & dataTable -> matrixA); //pass a reference of matrix A
  	readDataFromFile("matrixB.txt", & dataTable -> matrixB); // pass a reference of matrix B
	printMatrixAB(dataTable);
  	createSharedMem(dataTable);
}

/*
 * this method:
 * 1- accepts a file name of a matrix and a matrix type to write data
 */
void readDataFromFile(char * fileName, MatrixType * matrix) 
{
  	int LINESZ = N * N;
  	char buff[LINESZ];
  	FILE * fin = fopen(fileName, "r");
	if (fin == NULL) 
	{
    	printf("File not found.\n");
    	return;
  	}
  	int list[N * N];
  	if (fin != NULL) 
	{
    int countAll = 0, countRows = 0, countColumns = 0;
    while (fgets(buff, LINESZ, fin)) 
	{
    	countRows++; // track the rows of input matirx
      	char * pch;
    	pch = strtok(buff, " "); // tokenize input files as " " is found
      	countColumns = 0;
      	while (pch != NULL) 
		{
        	countColumns++; // track the colunms of input matirx
        	int cellValue = atoi(pch); // converting string char of InputFile to int
        	list[countAll] = cellValue; // arranging all matirx value as a list
        	countAll++;
        	pch = strtok(NULL, " ");
      	}
    }
    // now modifying reference matrix
    matrix -> numberOfColumns = countColumns;
    matrix -> numberOfRows = countRows;
    int i, j;
    int n = 0;
    for (i = 0; i < countRows; i++) 
	{
		for (j = 0; j < countColumns; j++) 
		{
        	matrix -> matrix[i][j] = list[n]; //assigning values to reference matrix from list
        	n++;
      	}
    }
    fclose(fin);
  }

}

/*
 * this method:
 * 1- creates a shared memory and inserts the given data
 */

void createSharedMem(DataTableType * dataTable) 
{
	const char * name = "calcMatrixSharedMemName";
  	const int SIZE = sizeof(DataTableType);
  	int shm_fd;
  	DataTableType * ptr = NULL;
  	// create shared memory segment 
  	shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
  	// configuring size of the shared memory segment 
  	ftruncate(shm_fd, SIZE);
  	// now mapping shared memory segment in the address space of the process 
  	ptr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  	if (ptr == MAP_FAILED) 
	{
    	printf("Map failed\n");
    	//return -1;
  	}
  	// writting dataTable to shared memory:
  	* ptr = * dataTable;
}

/*
 * this method:
 * 1- connects the shared memory and returns a pointer to that data
 */

DataTableType * getDataTableFromSharedMem() 
{
	const char * name = "calcMatrixSharedMemName";
	const int SIZE = sizeof(DataTableType);
  	int shm_fd;
  	DataTableType * ptr = NULL;
  	int i;
  	/* open the shared memory segment */
  	shm_fd = shm_open(name, O_RDWR, 0666);
  	if (shm_fd == -1) 
	{
    	printf("shared memory failed\n");
    	exit(-1);
  	}
  	/* now map the shared memory segment in the address space of the process */
  	ptr = (DataTableType * ) mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  	if (ptr == MAP_FAILED) 
	{
  	 	printf("Map failed\n");
    	exit(-1);
  	}
  	return ptr;
}

// this is a struct used for each thread for calculating product of matrices
typedef struct {
  int i; // row 
  int j; // column 
  DataTableType * dataTable;
}
ThreadData;

void * oneThread(void * param);

/*
 * this method accepts a DataTableType pointer and creates threads to calculate product of matrices
 */
void calcMatrixThreaded(DataTableType * dataTable) 
{
	if (dataTable -> matrixA.numberOfRows != dataTable -> matrixB.numberOfColumns) 
	{
    	printf("Matrices with entered orders can't be multiplied with each other.\n");
    	exit(0);
  	} 
	else 
	{
	    int i, j, count = 0;
	    for (i = 0; i < dataTable -> matrixA.numberOfRows; i++) 
		{
	    	for (j = 0; j < dataTable -> matrixB.numberOfColumns; j++) 
			{
	        	ThreadData * data = malloc(sizeof(ThreadData * ));
	        	data -> i = i;
	        	data -> j = j;
	        	data -> dataTable = dataTable;
	        	pthread_t tid; //Thread ID
	        	pthread_attr_t attr; //Set of thread attributes	
	        	//Get the default attributes
	        	pthread_attr_init( & attr);
	        	//Create the thread
	        	pthread_create( & tid, & attr, oneThread, data);
	        	//Make sure the parent waits for all thread to complete
	        	pthread_join(tid, NULL);
	        	count++;
	      	}
	    }
	    printf("Total number of threads used for calculating multiplication: %d\n", count);
	}
}

/*
 * this methods:
 * 1- is a function that is called by one thread. it reads row and column and calculates their product
 * 2- to be used in the result matrix (which will be the product of the matrices)
 */

void * oneThread(void * param) 
{
	ThreadData * data = param; // the structure that holds our data
  	int n, sum = 0; //the counter and sum
  	for (n = 0; n < data -> dataTable -> matrixB.numberOfColumns; n++) 
	{
    	sum = sum + data -> dataTable -> matrixA.matrix[data -> i][n] * data -> dataTable -> matrixB.matrix[n][data -> j];
  	}
  	data -> dataTable -> mResult.matrix[data -> i][data -> j] = sum;
  	//Exit the thread
  	pthread_exit(0);
}

void printMatrix(DataTableType * dataTable) // For printing Matrices
{
	printf("Product of Matrices A and B:\n");
	int i, j;
	for (i = 0; i < dataTable -> matrixA.numberOfRows; i++) 
	{
    	for (j = 0; j < dataTable -> matrixB.numberOfColumns; j++)
		{
    		printf("%d\t", dataTable -> mResult.matrix[i][j]);
    	}
		printf("\n");
  	}
}

void printMatrixAB(DataTableType * dataTable) // For printing resulting matrices 
{
	printf("Matrices A and B:\n");
  	int i, j;
  	printf("Matrix A:\n");
  	for (i = 0; i < dataTable -> matrixA.numberOfRows; i++) 
	{
    	for (j = 0; j < dataTable -> matrixA.numberOfColumns; j++)
		{
      		printf("%d\t", dataTable -> matrixA.matrix[i][j]);
    	}	
		printf("\n");
  	}
	printf("\nMatrix B:\n");
  	for (i = 0; i < dataTable -> matrixB.numberOfRows; i++) 
	{
    	for (j = 0; j < dataTable -> matrixB.numberOfColumns; j++)
      	printf("%d\t", dataTable -> matrixB.matrix[i][j]);
    	printf("\n");
  	}
}
