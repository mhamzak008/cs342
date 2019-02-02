#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

typedef enum { false, true } bool;

struct proccess
{
    // Info
    int vruntime;
    int pid;
    int startTime;
    int load;
    int prio;
    int executedLine;   // keeps track of the no of instructions executed
    int cpuBurstExecuted;
    int ioBurstExecuted;
    int ioBurst;
    int timeSlice;
    int timeSliceExecuted;
    int finishTime;
    int turnaroundTime;
    int waitTime;
    int responseTime; 

    struct proccess *next;
}*start;

struct ioQue
{
    struct proccess *p;
    struct ioQue *next;    
}*startIO;

// Checks if a proccess is waiting in IO or not
bool checkIO(int pid)
{
    struct ioQue* temp = startIO;

    while((temp != NULL) && (temp->p->pid != pid))
    {
        temp = temp->next;
    }

    if((temp != NULL) && temp->p->pid == pid)
        return true;

    return false;
}

// Add a proccess to IO que
void addIO(struct proccess *p)
{
    struct ioQue* new = ( struct ioQue* ) malloc( sizeof( struct ioQue ) );

    new->p = p;
    new->next = startIO;
    startIO = new;
}

// Remove a process from the IO que with a particular pid
struct proccess* getIO(int pid)
{
    struct ioQue* q = startIO;
    struct ioQue* temp;
    if ( startIO == NULL )
    {
        return NULL;
    }
    else
    {
        if(q->p->pid == pid)
        {
            temp = q;
            startIO = startIO->next;
        }
        else
        {            
            while ( q->next != NULL && q->next->p->pid == pid)
            {
                q = q->next;
            }
            temp = q->next;
            q->next = q->next->next;
        }
    }

    struct proccess* p = temp->p;
    free(temp);

    return p;
}

void updateIO()
{
    struct ioQue * cur = startIO;

    while (cur != NULL)
    {
        cur->p->ioBurstExecuted += 10000000;
 //       printf("ioBurstExecuted of %i is %i\n",cur->p->pid, cur->p->ioBurstExecuted );
        cur = cur->next;
    }
}

void updateWaitTime()
{
	struct proccess* cur = start;

	while(cur != NULL)
	{
		cur->waitTime += 10000000;
		cur = cur->next;
	}
}

struct proccess* checkIOcompletion()
{
    struct ioQue * cur = startIO;
    struct proccess* temp = NULL;
    while (cur != NULL)
    {
        if(cur->p->ioBurstExecuted >= cur->p->ioBurst)
        {
            temp = getIO(cur->p->pid);
            cur = NULL;
        }
        else
            cur = cur->next;
    }

    return temp;    
}

// Inserts to ready proccess's with increasing order of vruntime
void createProcess( struct proccess* new)
{
    struct proccess* q;

    if ( start == NULL || new->vruntime < start->vruntime )
    {
        new->next = start;
        start = new;
    }
    else
    {
        q = start;
        while ( q->next != NULL && q->next->vruntime <= new->vruntime )
        {
            q = q->next;
        }

        new->next = q->next;
        q->next = new;
    }
}

// Gets the proccess with lowest value of vruntime from the ready queue
struct proccess * getReady()
{
    struct proccess *cpu = NULL;

    if ( start == NULL )
    {
 //       printf( "\nNo Process In The Ready Queue\n" );
    }
    else
    {
        cpu = start;
//        printf( "\nRunning Process Is %d\n", cpu->pid );
        start = start->next;
        cpu->next = NULL;
    }    

    return cpu;
}

// Displays the ready queue
void display()
{
    struct proccess *cur;

    cur = start;
    if ( start == NULL )
        printf( "READY QUEUE IS EMPTY\n" );

    else
    {
        printf( "READY QUEUE IS:\n" );

        while ( cur != NULL )
        {
            printf( "\t%d[vruntime=%d]\n", cur->pid, cur->vruntime );
            cur = cur->next;
        }
    }
}

// Calculates timeSlice of a process
int getTimeSlice(int targetedLatency, int proccessLoad, int totalLoad)
{
    targetedLatency = targetedLatency / 1000000; 

    int timeSlice = (proccessLoad * targetedLatency);
    timeSlice = timeSlice / totalLoad;
    timeSlice = timeSlice * 1000000;

    if (timeSlice > 10000000)
        return timeSlice;

    return 10000000;	// min timeSlice is 10000000 ns
}

// Calculates new vruntime after i/o wait of a process
int updateVruntimeIO(int old, int min_vruntime, int targetedLatency)
{
    int new = min_vruntime - targetedLatency;

    if(old > new)
        new = old;

    return new;
}

// Updates vruntime
// Run after everytime a process runs
void updateVruntime(int processLoad, int* vruntime)
{
    int delta = 1024 / processLoad;
    delta = 10000000 * delta;
    *vruntime = (*vruntime) + delta;
}

// checks if a new process has arrived at current time
// returns -1 if error, otherwise returns the number of proccess created
int checkInput(char *fileName, int time, int* totalProcesses, int min_vruntime)
{
    FILE *input;
    int startTime;
    int pid;
    int prio;    
    int status = 0;
    input = fopen(fileName, "r");

    // prio to weight table
    static const int prio_to_weight[40] = { 88761, 71755, 56483, 
        46273, 36291, 29154, 23254, 18705, 14949, 11916, 9548, 
        7620, 6100, 4904, 3906, 3121, 2501, 1991, 1586, 1277, 
        1024, 820, 655, 526, 423, 335, 272, 215, 172, 110, 87, 
        70, 56, 45, 36, 29, 23, 18, 15,};

        char buffer[30];

        if (NULL == input)
        {
           perror("cant open input file");
           return (-1);
       }

       while (EOF != fscanf(input, "%30[^\n]\n", buffer))
       {
        // Search for "start time"
        char * posS; 
        posS = strchr(buffer, 's');

        if (posS != NULL)   // start time found
        {
            sscanf(buffer, "%i start %i prio %i", &pid, &startTime, &prio);
            
            if (startTime == time)
            {
                int vruntime = min_vruntime - 10000000;

                if (vruntime < 0)
                    vruntime = 0;

                int load = prio_to_weight[prio];

                struct proccess* new = ( struct proccess* ) malloc( sizeof( struct proccess ) );
                new->pid = pid;
                new->startTime = startTime;
                new->vruntime = vruntime;
                new->load = load;
                new->prio = prio;
                new->executedLine = 1;
                new->ioBurstExecuted = 0;
                new->cpuBurstExecuted = 0;
                new->timeSlice = 0;
                new->waitTime = 0;
                new->responseTime = 0;              

                createProcess(new);
                (*totalProcesses)++;
                status++;
            }
        }
    }

    fclose(input);
    return status;
}

void updateTotalLoad(int* totalLoad, struct proccess* cpu)
{
    struct proccess *cur = start;
    *totalLoad = 0;
    while (cur != NULL)
    {
        *totalLoad = (*totalLoad) + cur->load;
        cur = cur->next;
    }
    if( cpu != NULL)
    	*totalLoad = (*totalLoad) + cpu->load;
}

int main(int argc, char *argv[])
{
	// // Statistics
	// int cpuUtilization;

	// Others
	int time;
	int targetedLatency;	// update when totalProcesses > 20
	int totalProcesses;		// update when a new process arrives
	int totalLoad;	// update when a new process arrives
	int min_vruntime;	// update when a new process arrives
    struct proccess *cpu; // process running in the cpu    
    int idleTime;
    int cpuTime;
    bool idleTimeFlag;
    
    // Initializations
    time = 0;
    idleTime = 0;
	targetedLatency = 200000000;	// ns
	totalProcesses = 0;
    min_vruntime = 0;
    totalLoad = 0;
    cpu = NULL;
    startIO = NULL;
    start = NULL;
    cpuTime = 0;
    idleTimeFlag = false;

    FILE *output = fopen(argv[2], "w");
	if (output == NULL)
	{
    	printf("Error opening file!\n");
    	exit(1);
	}

    do // timer tick
    {
        // Put the processes which arrive at current time in the ready que
        checkInput(argv[1], time, &totalProcesses, min_vruntime);        

        // if io wait of any process is finished, bring it back to ready que                    
        struct proccess* ioTemp = NULL;
        do
        {
            ioTemp = checkIOcompletion();
            if(ioTemp != NULL)
            {
                ioTemp->vruntime = updateVruntimeIO(ioTemp->vruntime, min_vruntime, targetedLatency);
                createProcess(ioTemp);
            }
        }while(ioTemp != NULL);

        // Makes the neccessary updates due to new proccesses
        updateTotalLoad(&totalLoad, cpu);
        if(totalProcesses > 20)
        	targetedLatency = totalProcesses * 10000000;
        if(start != NULL)
        	min_vruntime = start->vruntime;

 //       printf("totalLoad: %i\n", totalLoad );
//        printf("targetedLatency: %i\n",targetedLatency );

 //       display();
//        printf("\nTotalProcesses: %i\n", totalProcesses);

        if(cpu == NULL)
        	cpu = getReady();

        if (cpu != NULL)
        {
//        	printf("timeSlice: %i\n",cpu->timeSlice );

        	if(cpu->timeSlice == 0)
        	{
  //      		printf("getTimeSlice: %i\n", getTimeSlice(targetedLatency, cpu->load, totalLoad) );
            	cpu->timeSlice = cpu->vruntime + getTimeSlice(targetedLatency, cpu->load, totalLoad);
//        		printf("timeSlice: %i\n",cpu->timeSlice );
        	}

            // Run the Proccess
            FILE *input;
            input = fopen(argv[1], "r");
            const char pidChar = cpu->pid + '0';
            int curLine = 1;
            bool found = false;

            char buffer[30];

            if (NULL == input)
            {
                perror("can't open input file");
                return (-1);
            }

            // Execute the relevant instruction in the input file for this timer tick
            while ((EOF != fscanf(input, "%30[^\n]\n", buffer)) && (found == false)) 
            {
                // Search for process associated tasks        
                char * flag;
                bool ioWait = checkIO(cpu->pid);
                flag = strchr(buffer, pidChar);
				
				// process associated task found
                // task is not in iowait
                // and the correct instruction found
                if ((flag != NULL) && (ioWait == false) && (cpu->executedLine < curLine))   
                {
                    char * cpuCheck;
                    char * io;
                    char * end; 
                    cpuCheck = strstr(buffer, "cpu");
                    io = strstr(buffer, " io ");
                    end = strstr(buffer, "end");
    //                printf("instruction is: %s\n", buffer );

                    if (cpuCheck != NULL)    // cpu burst of the task
                    {
                        int cpuBurst;   
                        int temp;          
                        sscanf(buffer, "%i cpu %i", &temp, &cpuBurst);
                        cpuBurst = cpuBurst * 1000000;
                        if(cpuBurst <= cpu->cpuBurstExecuted)
                        {
  //                          printf("%s\n", "cpu burst finished!" );
                            cpu->executedLine++;
                            cpu->cpuBurstExecuted = 0;
                        }
                        else
                        {
                            cpu->cpuBurstExecuted = cpu->cpuBurstExecuted + 10000000;                         
        					cpuTime += 10000000;
                            updateVruntime(cpu->load, &(cpu->vruntime));
  //                          printf("cpuBurstExecuted of proccess %i is %i\n", cpu->pid, cpu->cpuBurstExecuted );
                           	
                            if(cpu->vruntime >= cpu->timeSlice)
                            {
                            	cpu->timeSlice = 0;
                            	fprintf(output, "%i %i\n", cpu->pid, (cpuTime/1000000));
                            	cpuTime = 0;                          	
                            	createProcess(cpu);
                        		cpu = NULL;
                            }

                        	found = true;
                        }
                    }   
                    else if (io != NULL)    // io burst of the task
                    {
                        int ioBurst;
                        int temp;                
                        sscanf(buffer, "%i io %i", &temp, &ioBurst);
                        ioBurst = ioBurst * 1000000;
                        cpu->ioBurst = ioBurst;

                        if(ioBurst <= cpu->ioBurstExecuted)
                        {
                            cpu->ioBurstExecuted = 0;
                            createProcess(cpu);
                        }
                        else
                        {
//                        	printf("%s\n", "adding to the ioQue" );
                            addIO(cpu);
                        }                        
                        cpu->executedLine++;
                        found = true;
                        cpu = NULL;
                    }   
                    else if (end != NULL) // Task has finished
                    {
                        cpu->executedLine++;
                        found = true;

                        cpu->finishTime = (time/1000000) - 30;
                        cpu->turnaroundTime = (time/1000000) - 30;

                        // Printing the stats
                        printf("%i %i %i %i %i %i %i\n", 
                        	cpu->pid, cpu->prio, cpu->startTime, cpu->finishTime,
                        	cpu->turnaroundTime, ((cpu->waitTime)/1000000), cpu->responseTime);

                        free(cpu);
                        cpu = NULL;
                        totalProcesses--;
                    }         
                }

                if(flag != NULL)
                    curLine++;
            }
            fclose(input);
        }       

        time    += 10000000;

        // update the entire io que with 10 ms
        updateIO();

        if((start == NULL) && (startIO != NULL) && (cpu == NULL))	// Idle Time
        {
        	idleTime += 10000000;
        	idleTimeFlag = true;
//        	printf("Idle time is: %i\n", idleTime );
        }

        if((cpu != NULL) && (idleTimeFlag))
        {
        	fprintf(output, "idle %i\n",(idleTime/1000000) );
        	idleTime = 0;
        	idleTimeFlag = false;
        }

        updateWaitTime();

    } while ((start != NULL) || (startIO != NULL) || (cpu != NULL));    // Timer tick
    
	fclose(output);

    return 0;
}