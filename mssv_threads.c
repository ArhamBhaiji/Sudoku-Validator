/*
*F.Shabbir - 19201960
*The program implements a muti-threaded sudoku solution vaildator
*It has three methods that check for the validity of the row, column and 3x3 sub-grids
*A method is used to read the solution from a text file and load it into an array called 'buffer'
*Access control is implemented via mutex variables
*All invalid results are stored a file called 'log.txt' located in the same directory
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

//counter to check if all are valid
int counter = 0;
//holds valid regions - (named buffer2 in the assignment instructions)
int valid[11] = {0,0,0,0,0,0,0,0,0,0,0};
//holds the solution
static int buffer[9][9];
//mutex variables for access control
pthread_mutex_t bufferm;
pthread_mutex_t counterm;
pthread_mutex_t writem;
//file pointer for the log file
FILE *fp;
//defines sturct for thread parameters
typedef struct {
	int row;
	int column;
	int maxDelay;		
} parameters;

//validates rows
void *isRowValid (void *param)
{
	//gets the parameters passed to the thread
	parameters *params = (parameters*)param;
	int row = params->row;
	int col = params->column;
	int maxD = params->maxDelay;
	

	//to check if numbers 1-9 only appears once in the row
	int validityArray[9] = {0};

	//iterates through the entire row
	for (int i = 0; i < 9; i++) 
	{
		int num = buffer[row][i];	
		if (num < 1 || num > 9  || validityArray[num - 1] == 1)
		{
			pthread_mutex_lock(&writem); //locks write mutex

			//add to the log file
			fp = fopen( "log.txt" , "a" );
			if (fp == NULL)
			{
			    printf("Error opening log file!");
			    exit(1);
			}	

			fprintf(fp, "Thread ID-%u: row %d is invalid\n", (unsigned int)pthread_self(), row+1);
			fclose(fp);

			pthread_mutex_unlock(&writem); //unlocks write mutex

			pthread_exit(NULL); //thread exits due to invalid row

		} else {
			validityArray[num - 1] = 1; //changes the value in the validity array		
		}
	}

	//if the thread reaches this point, the row is valid
	pthread_mutex_lock(&bufferm);
	valid[row - 1] = 1; //sets value in the valid array
	pthread_mutex_unlock(&bufferm);
	
	pthread_mutex_lock(&counterm);
	counter++; //increases counter
	pthread_mutex_unlock(&counterm);

	printf("Validation results from thread-ID %u: row %d is valid\n", (unsigned int)pthread_self(), row+1);

	sleep(maxD);
	pthread_exit(NULL);
}

//validates columns
void *isColumnValid (void *param) {

	//gets the parameters passed to the thread
	parameters *params = (parameters*)param;
	int maxD = params->maxDelay;
	int colCount = 0;
	int invCols[9]; //holds the invalid columns
	int invCC; //invalid col count

	for (int i = 0; i < 9; i++) 
	{
		//to check if numbers 1-9 only appear once in the column
		int validityArray[9] = {0};
		for (int j = 0; j < 9; j++) 
		{
			int num = buffer[j][i];
			if (num < 1 || num > 9 || validityArray[num - 1] == 1) 
			{
				invCols[invCC++] = i; //stores all the invalid col numbers
				break;
			} 
			else 
			{
				validityArray[num - 1] = 1; //changes the value in the validity array	
			}

			//if it gets to this point the column in valid
			if (j == 8)
			{
				colCount++; //counts columns to display number of valid columns
				pthread_mutex_lock(&bufferm);
				valid[9] = valid[9]+1;
				pthread_mutex_unlock(&bufferm);
	
				pthread_mutex_lock(&counterm);
				counter++;
				pthread_mutex_unlock(&counterm);
			}
		}
	}

	if (invCC > 0)
	{
		pthread_mutex_lock(&writem); //locks write mutex
		//add to the log file
		fp = fopen( "log.txt" , "a" );
		if (fp == NULL)
		{
		    printf("Error opening log file!");
		    exit(1);
		}	
	
		fprintf(fp, "Thread ID-%u: column(s) ", (unsigned int)pthread_self());
		for (int x = 0; x < invCC; x++)
		{
			if (x == invCC - 1)
				fprintf(fp, "%d", invCols[x]+1);
			else
				fprintf(fp, "%d, ", invCols[x]+1);
		}
		fprintf(fp, " is/are invalid\n");

		fclose(fp);

		pthread_mutex_unlock(&writem); //unlocks write mutex
	}
	
	printf("Validation results from thread ID-%u: %d of 9 columns are valid\n", (unsigned int)pthread_self(), colCount); 
	sleep(maxD);
	pthread_exit(NULL);
}

//validates sub-grids
void *is3x3Valid (void *param) {

	//gets the parameters passed to the thread
	parameters *params = (parameters*)param;
	int maxD = params->maxDelay;

	int invalid =  1;
	int row = 0;
	int subSCount = 0;
	int invSubsR[9]; //holds the invalid subsections row
	int invSubsC[9]; //holds the invalid subsections col
	int invSC; //invalid subsection count

	while (row < 9) 
	{
		//resets the col value so it can iterate through all the subsections
		int col = 0;
		while (col < 9) 
		{
			//to check if numbers 1-9 only appear once in the subsection
			int validityArray[9] = {0};
			//iterates through the subsection
			for (int i = row; i < row + 3; i++) {
				for (int j = col; j < col + 3; j++) {
					int num = buffer[i][j];
					if (num < 1 || num > 9 || validityArray[num - 1] == 1) 
					{
						invSubsR[invSC] = row + 1; //stores all the invalid subsection row numbers
						invSubsC[invSC] = col + 1; //stores all the invalid subsection col numbers
						invSC++;
						invalid = 0; //sets the invalid value to 0 if the num is invalid
						break;
					} 
					else 
					{
						validityArray[num - 1] = 1; //changes the value in the validity array
					}
				}

				//breaks out of loop is invalid
				if (invalid == 0)
					break;
			}

			//makes sure subsection is valid
			if (invalid == 1)
			{
				//sets locks for shared variables
				pthread_mutex_lock(&bufferm);
				valid[10] = valid[10] + 1;
				pthread_mutex_unlock(&bufferm);
	
				pthread_mutex_lock(&counterm);
				counter++;
				pthread_mutex_unlock(&counterm);

				subSCount++;
			}
			//checks if the subsection is invalid and resets the value
			else if (invalid == 0) 
			{
				invalid = 1;
			}
			//moves to next subsection in the column
			col = col + 3;
		}	
		//moves to the next row of subsections
		row = row + 3;
	}

	if (invSC > 0)
	{
		pthread_mutex_lock(&writem); //locks write mutex
		//add to the log file
		fp = fopen( "log.txt" , "a" );
		if (fp == NULL)
		{
		    printf("Error opening log file!");
		    exit(1);
		}
	
		fprintf(fp, "Thread ID-%u: sub-grid(s) ", (unsigned int)pthread_self());
		for (int x = 0; x < invSC; x++)
		{
			if (x == invSC - 1)
				fprintf(fp, "[%d..%d, %d..%d]", invSubsR[x], invSubsR[x]+2, invSubsC[x], invSubsC[x]+2);
			else
				fprintf(fp, "[%d..%d, %d..%d], ", invSubsR[x], invSubsR[x]+2, invSubsC[x], invSubsC[x]+2);
		}
		fprintf(fp, " is/are invalid\n");

		fclose(fp);

		pthread_mutex_unlock(&writem); //unlocks write mutex
	}

	printf("Validation results from thread ID-%u: %d of 9 subs-grids are valid\n", (unsigned int)pthread_self(), subSCount); 
	sleep(maxD);
	pthread_exit(NULL); //thread exits
}

//sets the solution into the buffer array
void setSolution (const char *path) 
{
	FILE *f;
	if (f = fopen(path, "r"))  //opens file for reading if it exists
	{
		for(int i = 0; i < 9; i++) 
		{
			for(int j = 0; j < 9; j++) 
			{
				char s[] = "0";
				fscanf(f, " %c", &s[0]); //gets the char from the file
				buffer[i][j] = atoi(s); //converts char to int and stores into the buffer
			}
		}
	}
	else 
	{
		printf("There is no file with that name. Please enter a valid file name/path\n");
		exit(1); //exits with error
	}
}

//main method
int main(int argc, const char *argv[]) 
{
	//checks if filepath and maxdelay have been provided as comand line args
	if (argc < 3)
	{
		printf("Please include file and maxdelay.\n\n Syntax: mssv_threads.c [FILE PATH] [MAXDELAY]\n");
		return 1;
	}

	//sets the value of the max delay from the command line
	int maxDelay = atoi(argv[2]);
	
	//sets the solution from the file to the buffer array
	setSolution(argv[1]);

	//opens log file for invalid sub sections
	fp = fopen( "log.txt" , "w" );
	fprintf(fp, "LOG FILE\n--------\n");
	fclose(fp);

	//initializes mutex variables for access control
	pthread_mutex_init(&writem, NULL);
	pthread_mutex_init(&bufferm, NULL);
	pthread_mutex_init(&counterm, NULL);
	
	pthread_t threads[11];
	
	int threadIndex = 0;	
	int i,j;

	parameters *data = (parameters *) malloc(sizeof(parameters));	
	data->maxDelay = maxDelay;

	//column thread
	pthread_create(&threads[threadIndex++], NULL, isColumnValid, data); 
	
	//3x3 subsection thread
	pthread_create(&threads[threadIndex++], NULL, is3x3Valid, data); 

	//creates threads for rows
	for (i = 0; i < 9; i++) {
		for (j = 0; j < 9; j++) {						
			if (j == 0) {
				parameters *rowData = (parameters *) malloc(sizeof(parameters));	
				rowData->row = i;		
				rowData->column = j;
				rowData->maxDelay = maxDelay;
				pthread_create(&threads[threadIndex++], NULL, isRowValid, rowData);
			}
		}
	}

	for (i = 0; i < 11; i++) {
		pthread_join(threads[i], NULL); // Wait for all threads to finish
	}

	//destroys all the mutex variables
	pthread_mutex_destroy(&writem);
	pthread_mutex_destroy(&bufferm);
	pthread_mutex_destroy(&counterm);

	//if counter is not 27, then the sudoku solution is invalid
	for (i = 0; i < 11; i++) {
		if (counter != 27) {
			printf("\nThere are only %d valid sub-grids, and thus the sudoku solution is invalid.\n", counter);
			return 1; //returns with error
		}
	}

	//if it reaches here then the counter is 27 and the solution is valid
	printf("\nThere are %d valid sub-grids, and thus the sudoku solution is valid.\n", counter);

	
 	
	// return with success
	return 0;

} 
