#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include "function.h"

int main(int argc, char *argv[])
{
	// Getting the input from terminal and converting it to ints
	int l; 
	int u;
	int k;
	int n;
	
	l = atoi(argv[1]);
	u = atoi(argv[2]);
	k = atoi(argv[3]);
	n = atoi(argv[4]);

	// Checking boundary conditions of 'n'
	if ((n < 51) && (n > 0))
	{
		// Creating n pipes
		int fd[2 * n];
		for (int i = 0; i < n; i++) 
		{
    		pipe(&fd[2*i]);
		}

		// Creating n child processes
		pid_t process[n];
		pid_t self;
		int selfIndex;
		for(int i = 0; i < n; i++)
		{
			process[i] = fork();
			self = process[i];
			selfIndex = i;

			if (process[i] < 0) // error occured
			{
				fprintf(stderr, "Fork Failed!" );
			}
			else if (process[i] == 0)	// child process
			{
				// Close read end of the respective pipe
				close(fd[i * 2]);

				i = n;	// exit loop of children creation
			}
			else	// parent process
			{			
				// Close write end of the respective pipe
				close(fd[(i*2)+1]);
			}
		}	

		// Parent Process
		if(self > 0)
		{
			// Waiting for n child processes to end
			for(int i = 0; i < n; i++)
			{
				wait(0);
			}

			// Summing up the areas from all proccesses
			double sum;
			for(int i = 0; i < n; i++)
			{				
				double area;

				read(fd[2*i], &area, sizeof(area));

				sum = sum + area;
			}

			printf("%lf\n", sum);
		}

		// Child Process
		if(self == 0) 
		{
			// Calculating interval child is responsible for			
			double intervalStart;
			double intervalEnd;
			double tempI = selfIndex+1;
			intervalStart = l + (((tempI-1)*(u-l))  / n);
			intervalEnd   = l + ((tempI * (u-l))  / n);

			// calculating assigned area			
			double base;
			double totalArea;

			base = (intervalEnd-intervalStart) / k;
			totalArea = 0;

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

			// Send the result to the parent
			write(fd[(2*selfIndex) + 1], &totalArea, sizeof(totalArea));
		}
	}
	else
	{
		printf("%s\n", "Invalid N" );
	}

return 0;
}
