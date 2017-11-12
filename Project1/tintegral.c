#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#include "function.h"

void *runner(void *tempIndex);
double areas[1000];
int l; 
int u;
int k;
int n;
int lock;

int main(int argc, char *argv[])
{
	// Getting the input from terminal and converting it to ints
	l = atoi(argv[1]);
	u = atoi(argv[2]);
	k = atoi(argv[3]);
	n = atoi(argv[4]);

	// Checking boundary conditions of 'n'
	if ((n < 1001) && (n > 0))
	{
		// Creating an array to store areas from each thread
//		areas[n];

		// Creating n threads
		pthread_t threads[n];
		pthread_attr_t attr;

		for(int i = 0; i < n; i++)
		{
			pthread_attr_init(&attr);	// setting default attributes

			lock = 1;

			pthread_create(&threads[i], &attr, runner, &i);	
			
			while(lock == 1);	// Waiting for the thread to be created
		}	
		
		// Waiting for n threads to end
		for(int i = 0; i < n; i++)
		{
			pthread_join(threads[i], NULL);
		}

		// Summing up the areas from all the threads
		double sum;		
		for(int i = 0; i < n; i++)
		{				
			sum = sum + areas[i];
		}

		printf("%lf\n", sum);
	}
	else
	{
		printf("%s\n", "Invalid N" );
	}

return 0;
}

// The threads will begin control in this function
void *runner(void *tempIndex)
{
	int index = *(int*) tempIndex; 
	lock = 0;

	// Calculating interval child is responsible for			
	double intervalStart;
	double intervalEnd;
	double tempI = index + 1;
	intervalStart = l + (((tempI-1)*(u-l))  / n);
	intervalEnd	  = l + ((tempI * (u-l))  / n);

	// calculating assigned area			
	double base;
	double totalArea;

	base = (intervalEnd-intervalStart) / k;

	for(int i = 0; i < k; i++)
	{
		double area;
		double intervalStartY;
		double intervalEndY;

		intervalEnd = intervalStart + base;

		intervalStartY = compute_f(intervalStart);
		intervalEndY   = compute_f(intervalEnd);

		area = ((intervalStartY + intervalEndY) / 2) * base;

		totalArea = totalArea + area;

		intervalStart = intervalEnd;
	}

	areas[index] = totalArea;

	pthread_exit(0);
}
