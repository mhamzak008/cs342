#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Function Definations
void *producerControl(void *tempIndex);
void *consumerControl();

// Global Variables
int syncLock; // to ensure that threads are created in sync
int M;	  	  // Size of each buffer
int N;		  // Number of producers
char *outputFileName;
char *inputFileName;
struct student ***buffers;
struct lock **locks;
struct student * sortedList;	// list used by consumer to hold sorted entries
int totalStudents;	// Total number of students in input file
// Used to wake up and sleep the consumer
pthread_mutex_t totalCountLock;
int totalCount;
pthread_cond_t consumerState;

struct student
{
	int sid;
	char firstname[64];
	char lastname[64];
	double cgpa;

	struct student * next;
};

struct lock
{
	pthread_mutex_t mutex;	// Access to buffer
	int start;	// Where to consume
	int end;	// Where to produce
	pthread_cond_t emptyExists;	// signal buffer not full to producer
	int count;	// number of students in the current buffer
};

// Inserts into sortedList with increasing order of student id
void insertList( struct student* new)
{
	struct student* q;

	if ( sortedList == NULL || new->sid < sortedList->sid )
	{
		new->next = sortedList;
		sortedList = new;
	}
	else
	{
		q = sortedList;
		while ( q->next != NULL && q->next->sid <= new->sid )
		{
			q = q->next;
		}

		new->next = q->next;
		q->next = new;
	}
}

// Displays the sorted list on console
void displayList()
{
	struct student *cur;

	cur = sortedList;
	if ( sortedList == NULL )
		printf( "Sorted List Is Empty\n" );
	else
	{
		while ( cur != NULL )
		{
			printf("%i %s %s %lf\n", cur->sid, cur->firstname, cur->lastname, cur->cgpa);
			cur = cur->next;
		}
	}
}

int main(int argc, char *argv[])
{
	// Read the values of N and M
	N = atoi(argv[1]);	// Number of producers
	M = atoi(argv[2]);

	// Check boundary condition of N
	if((N >= 1) && (N <= 10))
	{
		// Check boundary condition of M
		if((M >= 10) && (M <=1000))
		{
			// Initializing thread shared data
			inputFileName = argv[3];
			outputFileName = argv[4];
			totalStudents = 0;			
			pthread_cond_init(&consumerState, NULL);	

			// Counting the total number of students
			// Opening the input file
			FILE *input = fopen(inputFileName, "r");
			if (input == NULL)
			{
				puts("Error opening input file!");
				exit(1);
			}

			// counting total students
			int ch;
			while(!feof(input))
			{
				ch = fgetc(input);
				if(ch == '\n')
				{
					totalStudents++;
				}
			}

			// Creating the array to hold the buffers	
			buffers = (struct student***) malloc(N*sizeof(struct student **));
			// Creating the array to hold locks
			locks = (struct lock**) malloc(N * sizeof(struct lock*));
			// Initializing the locks and buffers
			for (int i = 0; i < N; i++)
			{	
				// Creating the buffers in the array
				buffers[i] = (struct student**) malloc(M * sizeof(struct student *));

				// Creating and initializing the locks
				struct lock* new = (struct lock*) malloc(sizeof(struct lock));
				new->start = 0;
				new->end = 0;
				new->count = 0;
				pthread_mutex_init(&(new->mutex), NULL);
				pthread_cond_init(&(new->emptyExists), NULL);	
				locks[i] = new;
			}

			// Creating N producer threads
			pthread_t producers[N];
			pthread_attr_t attr;
			pthread_attr_init(&attr);	// setting default attributes
			for(int i = 0; i < N; i++)
			{
				syncLock = 1;

				pthread_create(&producers[i], &attr, producerControl, &i);

				while(syncLock == 1);	// Waiting for the thread to be initialized
			}

			// Creating 1 consumer thread
			pthread_t consumer;
			pthread_create(&consumer, &attr, consumerControl, NULL);

			// Waiting for N producer threads to end before releasing resources
			for(int i = 0; i < N; i++)
			{
				pthread_join(producers[i], NULL);
			}			

			// Waiting for the consumer thread to end
			pthread_join(consumer, NULL);

		//	displayList();	// Display result on console	
		}
		else
			puts("Invalid M");
	}
	else
		puts("Invalid N");

	return EXIT_SUCCESS;
}

// The producer threads will begin control in this function
void *producerControl(void *tempIndex)
{
	int id = *(int*) tempIndex;
	syncLock = 0;

	// Opening file
	FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(inputFileName, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);    

    while ((read = getline(&line, &len, fp)) != -1)	// read till end of file
	{
		int tempId;
		sscanf(line, "%i", &tempId);

        if (id == tempId)   // relevant student found
        {
            // import student info from the input file
     	  	int garbage;
        	struct student* new = ( struct student* ) malloc( sizeof( struct student ) );
        	sscanf(line, "%i %i %s %s %lf",&garbage, &new->sid, new->firstname, new->lastname, &new->cgpa);

            // Acquire lock on the buffer
        	pthread_mutex_lock(&(locks[id]->mutex));

        	// Wait till there is any space in the buffer to put data into
        	while(locks[id]->count >= M)
        	{
        		pthread_cond_wait(&(locks[id]->emptyExists), &(locks[id]->mutex));
        	}

            // Add student to the buffer and updating related lock info
        	buffers[id][locks[id]->end] = new;
        	locks[id]->count++;
        	locks[id]->end++;
        	if ((locks[id]->end) >= M)
        	{
        		locks[id]->end = 0;
        	}       	

            // unlock the buffer 
        	pthread_mutex_unlock(&(locks[id]->mutex));    
        }   

        // Wake up the consumer, if sleeping
        pthread_mutex_lock(&totalCountLock);
        totalCount++;
        if(totalCount > 0)	
        {
        	pthread_cond_signal(&consumerState);
        }
        pthread_mutex_unlock(&totalCountLock);
    }    

    // Releasing resources
    fclose(fp);
    if (line)
        free(line);

    pthread_exit(0);
}

// The consumer thread will begin control in this function
void *consumerControl()
{	
	int j = 0;	
	do
	{
		// Sleep until there is atleast 1 student in all buffers combined
		pthread_mutex_lock(&totalCountLock);
		while(totalCount < 1)	
		{
			pthread_cond_wait(&consumerState, &totalCountLock);
		}
		pthread_mutex_unlock(&totalCountLock);		

		for (int i = 0; i < N; i++)
		{
			// Try to acquire lock on the buffer
			if(pthread_mutex_trylock(&(locks[i]->mutex)) == 0)
			{
				if(locks[i]->count > 0)	//If there is any data in the current buffer
				{
					// Consume the data
					struct student* temp = buffers[i][locks[i]->start];
					locks[i]->start++;
					if(locks[i]->start >= M)
						locks[i]->start = 0;

					insertList(temp);        	
					j++;
					locks[i]->count--;

					pthread_mutex_lock(&totalCountLock);
					totalCount--;
					pthread_mutex_unlock(&totalCountLock);		

        		 	// if buffer was full then signal the relevant producer to wake up
					if (locks[i]->count < M)
					{
						pthread_cond_signal(&locks[i]->emptyExists);				
					}
				}

				pthread_mutex_unlock(&(locks[i]->mutex));	// unlock the buffer
			}
		}

	} while(j < totalStudents); // Run until all students are processed

	// Opening the output file for the consumer
	FILE *output = fopen(outputFileName, "w");
	if (output == NULL)
	{
		puts("Error opening output file!");
		exit(1);
	}

	// Printing the sorted students' list into the output file
	struct student *cur;
	cur = sortedList;
	if ( sortedList == NULL )
		fprintf(output, "No Student Records Found\n" );
	else
	{
		while ( cur != NULL )
		{
			fprintf(output, "%i %s %s %.2lf\n", cur->sid, cur->firstname, cur->lastname, cur->cgpa);
			cur = cur->next;
		}
	}

	// Closing output file
	fclose(output);

	pthread_exit(0);
}

