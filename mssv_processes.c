/*
*F.Shabbir - 19201960
*The program implements a muti-processing sudoku solution vaildator
*It has three methods that check for the validity of the row, column and 3x3 sub-grids
*A method is used to read the solution from a text file and load it into an array called 'buffer'
*Access control is implemented via semaphores
*All invalid results are stored a file called 'log.txt' located in the same directory
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <unistd.h>


//holds the solution
static int buffer[9][9];
//semaphores for access control
sem_t semWrite, semCounter, semValid;
//file pointer for the log file
FILE *fp;


//validates rows
void isRowValid (int row, int shmidc, int keyc, int sizec)
{
	//to check if numbers 1-9 only appears once in the row
	int validityArray[9] = {0};
	int validCheck = 0;

	//open and attach to shared memory
	shmidc = shmget(keyc, sizec, IPC_CREAT | 0775);
	int *counter = (int *) shmat(shmidc, NULL, 0);

	//iterates through the entire row
	for (int i = 0; i < 9; i++) 
	{
		int num = buffer[row][i];	
		if (num < 1 || num > 9  || validityArray[num - 1] == 1)
		{
			sem_wait(&semWrite); //locks write semaphore
			fp = fopen( "log.txt" , "a" );
			if (fp == NULL)
			{
			    printf("Error opening log file!");
			    exit(1);
			}
			fprintf(fp, "Process ID-%d: row %d is invalid\n", getpid(), row+1);
			fclose(fp);
			sem_post(&semWrite); //unlocks semaphore
			
			validCheck = 1; //sets to 1 to indicate invalid row
			break;
		} 
		else 
		{
			validityArray[num - 1] = 1; //changes the value in the validity array		
		}
	}
	
	if (validCheck == 0) 
	{
		sem_wait(&semCounter); //locks semaphore
		*counter = *counter + 1; //increases counter
		sem_post(&semCounter); //unlocks semaphore
	
		printf("Validation results from process ID-%d: row %d is valid\n", getpid(), row+1);
	}
	shmdt((void *) counter); //detaches
}

//validates columns
void isColumnValid (int shmidc, int keyc, int sizec) {

	int colCount = 0;
	int invCols[9]; //holds the invalid columns
	int invCC; //invalid col count

	//open and attach to shared memory
	shmidc = shmget(keyc, sizec, IPC_CREAT | 0775);
	int *counter = (int *) shmat(shmidc, NULL, 0);
	
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

				sem_wait(&semCounter); //locks semaphore
				*counter = *counter + 1; //increases counter
				
				sem_post(&semCounter); //unlocks semaphore

			}
		}
	}

	if (invCC > 0)
	{
		sem_wait(&semWrite); //locks write semaphore
		//add to the log file
		fp = fopen( "log.txt" , "a" );
		if (fp == NULL)
		{
		    printf("Error opening log file!");
		    exit(1);
		}	
	
		fprintf(fp, "Process ID-%d: column(s) ", getpid());
		for (int x = 0; x < invCC; x++)
		{
			if (x == invCC - 1)
				fprintf(fp, "%d", invCols[x]+1);
			else
				fprintf(fp, "%d, ", invCols[x]+1);
		}
		fprintf(fp, " is/are invalid\n");
		fclose(fp);
		sem_post(&semWrite); //unlocks semaphore
	}
	
	printf("Validation results from process ID-%d: %d of 9 columns are valid\n", getpid(), colCount);
	shmdt((void *) counter); //detaches
}

//validates sub-grids
void is3x3Valid(int shmidc, int keyc, int sizec) {

	int invalid =  1;
	int row = 0;
	int subSCount = 0;
	int invSubsR[9]; //holds the invalid subsections row
	int invSubsC[9]; //holds the invalid subsections col
	int invSC = 0; //invalid subsection count

	//open and attach to shared memory
	shmidc = shmget(keyc, sizec, IPC_CREAT | 0775);
	int *counter = (int *) shmat(shmidc, NULL, 0);

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
				subSCount++;

				sem_wait(&semCounter); //locks semaphore
				*counter = *counter + 1; //increases counter
				sem_post(&semCounter); //unlocks semaphore
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
		sem_wait(&semWrite); //locks write semaphore
		//add to the log file
		fp = fopen( "log.txt" , "a" );
		if (fp == NULL)
		{
		    printf("Error opening log file!");
		    exit(1);
		}
	
		fprintf(fp, "Process ID-%d: sub-grid(s) ", getpid());
		for (int x = 0; x < invSC; x++)
		{
			if (x == invSC - 1)
				fprintf(fp, "[%d..%d, %d..%d]", invSubsR[x], invSubsR[x]+2, invSubsC[x], invSubsC[x]+2);
			else
				fprintf(fp, "[%d..%d, %d..%d], ", invSubsR[x], invSubsR[x]+2, invSubsC[x], invSubsC[x]+2);
		}
		fprintf(fp, " is/are invalid\n");
		fclose(fp);
		sem_post(&semWrite); //unlocks semaphore
	}

	printf("Validation results from process ID-%d: %d of 9 subs-grids are valid\n", getpid(), subSCount); 
	shmdt((void *) counter); //detaches
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
		printf("Please include file and maxdelay.\n\nSyntax: mssv_threads.c [FILE PATH] [MAXDELAY]\n");
		return 1;
	}

	//sets the value of the max delay from the command line
	int maxDelay = atoi(argv[2]);

	//sets the solution from the file to the buffer array
	setSolution(argv[1]);
	
	//initialize semaphores
	sem_init(&semWrite, 1, 1);
	sem_init(&semCounter, 1, 1);
	
	sem_wait(&semWrite); //locks semaphore
	//opens log file for invalid sub sections
	fp = fopen( "log.txt" , "w" );
	fprintf(fp, "LOG FILE\n--------\n");
	fclose(fp);
	sem_post(&semWrite); //unlocks semaphore


	key_t key; // key to be passed to shmget()
	int shmid; // return value from shmget()
	int *counter;
	int size;

	key = ftok(".",'s'); // defines key for shmget()
	size = 40; //size of shared mem


	shmid = shmget (key, size, IPC_CREAT | 0775);

	//attach to the segment to get a pointer to it
	counter = (int *) shmat(shmid, NULL, 0);
	*counter = 0;

	//variables to hold fork ids
	int fork1, fork2, fork3;
	int stat = 0; //status

	//forks into process for column validation
	if((fork1 = fork()) == 0)
	{
		isColumnValid(shmid, key, size);
		exit(0);
	}

	//forks into process for 3x3 sub-grid validation
	if((fork2 = fork()) == 0)
	{
		is3x3Valid(shmid, key, size);
		exit(0);
	}
	
	//loops and creates 9 processes for row validation
	for (int i = 0; i < 9; i++)
	{
		if ((fork3 = fork()) == 0)
		{
			isRowValid(i, shmid, key, size);
			exit(0);
		}
	}
	
	//waits for all threads to return
	wait(NULL);
	sleep(maxDelay);
	
	//if counter is not 27, then the sudoku solution is invalid
	for (int i = 0; i < 11; i++) {
		if (*counter != 27) {
			printf("\nThere are only %d valid sub-grids, and thus the sudoku solution is invalid.\n", *counter);
			return 1; //returns with error
		}
	}

	//if it reaches here then the counter is 27 and the solution is valid
	printf("\nThere are %d valid sub-grids, and thus the sudoku solution is valid.\n", *counter);
	
 
	//destroys created semaphores
	sem_destroy(&semWrite);
	sem_destroy(&semCounter);

	//detaches
	if (shmdt(counter) == -1) {
		perror("shmdt");
	}
		
	// returns with success
	return 0;
}
