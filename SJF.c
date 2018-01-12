//Author Peter Adamson
//CS3413
//Assignment 1 Question 3
//command line format for usage of this program: ./a.out [int] < [input.txt] 
//where [int] is the desired number of processors (1 or more) and [input.txt] is the input file you wish to hand to stdin

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAXLEN 100

//define the node structure for the queue
typedef struct qNode Node;
struct qNode
{
	char *user;
	char *process;
	int arrival;
	int duration;
	int timeLeft;
	int arrivalNum;
	Node *next;	
};

//defines a user summary structure for use upon job completion
typedef struct userSummary Summary;
struct userSummary
{
	char *user;
	int lastCompleted;
	int arrivedAt;
	int arrivalPri;
};

//defines a cpu structure
typedef struct cpuStruct CPU;
struct cpuStruct
{
	int cpuNum;
	int availableAt;
	int idleAt;
	int busy;  //0 if free, 1 if busy
	Node *jobAssigned;
};

//function declarations
void initialize();
void enqueue(Node *newJob);
void dequeue(Node *newJob);
void schedule(int cpus);
void printResult();
void sortArrived(int time);
void printQueue();
int length();
int front();
Node *readJob();
Node* getFirstNode();

//global pointers to the start(head) and end(tail) of the list
Node *head;
Node *tail;

int main(int argc, char **argv)
{
	//check the number of cpus given
	int cpus;
	if(argc == 2)
	{
		cpus = atoi(argv[1]);
	}
	else
	{
		printf("invalid number of arguments\n");
		return;
	}

	//chew up and ignore the header
	char header[MAXLEN];
	fgets(header, sizeof(header), stdin);

	//initialize list pointers
	initialize();

	//load the jobs into the list
	Node *newJob;
	newJob = readJob();
	while(newJob != NULL)
	{
		enqueue(newJob);
		newJob = readJob();
	}
	
	//schedule the jobs
	schedule(cpus);

	//produce a report
	printResult();
}

//sets the head and tail pointers to null and indicates to the user that the pointers are ready to use
void initialize()
{
	head = tail = NULL;
	printf("list initialized\n");
}

//loads a job into the end of the list
void enqueue(Node *newJob)
{
	//set up the job to be added
	Node *temp = NULL;
	temp = (Node*)malloc(sizeof(Node));
	temp->user = newJob->user;
	temp->process = newJob->process;
	temp->arrival = newJob->arrival;
	temp->duration = newJob->duration;
	temp->timeLeft = newJob->timeLeft;
	temp->arrivalNum = newJob->arrivalNum;
	temp->next = NULL;
	if(tail == NULL)	//the list must be empty, so set both head and tail to temp
	{
		head = temp;
		tail = temp;
	}
	else			//the list is not empty, so add the job to the end of the list
	{
		tail->next = temp;
		tail = temp;	
	}
}

//removes a job from the front of the list
void dequeue(Node *newJob)
{
	Node *find = head;
	Node *prev = NULL;

	//loop as long as find has not hit the end of the list
	while(find != NULL)
	{
		if(find == newJob)	//we have found the job to remove
		{
			if(prev == NULL)	//the job to remove is the first job in the list
			{
				find = find->next;
				free(head);
				head = find;
			}
			else	//the job to remove is somewhere other than the first job in the list
			{
				prev->next = find->next;
				free(find);
				find = prev->next;
			}
		}
		else	//we have not found the job to remove
		{
			prev = find;
			find = find->next;
		}
	}
}

//returns the first job in the list
Node* getFirstNode()
{
	//set up the job to be returned
	Node *firstNode;
	firstNode = head;
	if(head == NULL)	//the list is empty
	{
		printf("list is empty\n");
	}
	else			//the list is not empty, so return the first job in the list
	{
		return firstNode;
	}
}

//return the length of the list
int length()
{
	int length = 0;
	Node *temp = head;

	if(tail == NULL)	//the list must be empty
	{
		return length;
	}
	else			//the list is not empty, so increment length and continue until the end is reached
	{
		while(temp)
		{
			length++;
			temp = temp->next;
		}
	}
	return length;
}

//reads in a job from standard input
Node *readJob()
{
	//set up the job to be added
	char user[MAXLEN];
	char process[MAXLEN];
	char a[MAXLEN];
	char b[MAXLEN];
	int arrival;
	int duration;
	static int count = 1;
	Node *newJob = NULL;

	/*check that we are not at the end of the file and that the format is correct.
	if so, set up and return the job*/
	if(!feof(stdin) && (4 == scanf("%s %s %d %d", user, process, &arrival, &duration)))
	{
		newJob = (Node*)malloc(sizeof(Node));
		newJob->user = malloc(strlen(user)+1);
		strcpy(newJob->user, user);
		newJob->process = malloc(strlen(process)+1);
		strcpy(newJob->process, process);
		newJob->arrival = arrival;
		newJob->duration = duration;
		newJob->timeLeft = duration;
		newJob->arrivalNum = count;
		newJob->next = NULL;
	}
	count++;

	return newJob;
}

//performs the scheduling of the arrived jobs
void schedule(int cpus)
{
	//set up a log file for auditing or reporting purposes (e.g. to produce a summary)
	FILE *file = fopen("TaskLog.txt","w");
	int time = 0;
	int maxUsers = length();
	int i;
	int j;
	int found = 0;

	//an array of Summary structs
	Summary *summaries[maxUsers];

	//an array of cpu structs
	CPU *cpuList[cpus];

	//memory allocation and array initialization for summaries
	for(i = 0; i < maxUsers; i++)
	{
		summaries[i] = malloc(sizeof(Summary));
		summaries[i] = NULL;
	}

	//memory allocation and array initialization for summaries
	for(i = 0; i < cpus; i++)
	{
		cpuList[i] = malloc(sizeof(CPU));
		cpuList[i]->busy = 0;
		cpuList[i]->cpuNum = i+1;
		cpuList[i]->jobAssigned = NULL;
		cpuList[i]->idleAt = 0;
	}
	//reset i to 0 for next section
	i = 0;

	//scheduling loop as long as list has jobs remaining	
	while(1)
	{
		//free all cpus and remove any completed jobs
		for(j = 0; j < cpus; ++j)
		{
			if(cpuList[j]->jobAssigned)	//the cpu has an assigned job
			{
				if(cpuList[j]->jobAssigned->timeLeft == 0)	//the job assigned to the cpu has finished
				{
					dequeue(cpuList[j]->jobAssigned);
				}
			}	
			cpuList[j]->busy = 0;
			cpuList[j]->jobAssigned = NULL;
		}

		//if after enqueueing all assigned jobs, the queue is still empty, we are done
		if(length() == 0)
		{
			break;
		}

		//sort the arrived jobs as long as there is more than one job
		if(length() > 1)
		{
			sortArrived(time);
		}

		//loop as long as there is another job in the queueu
		Node *curr = head;
		while(curr)
		{
			//check if there are any free cpus
			int allBusy = 1;
			for(j = 0; j < cpus; j++)
			{
				if(cpuList[j]->busy == 0) //we have a free cpu
				{
					allBusy = 0;
				}
			}
			//if all cpu's are busy, we can't do anything so increase time
			if(allBusy == 1)	//all cpus are busy
			{
				time++;
				break;
			}
			else			//at least one cpu is free
			{
				if(curr->arrival > time) //the job has not arrived yet
				{
					time++;
					break;
				}
				else			//we can schedule the job
				{
					//find the first available cpu
					int cpuCount = 0;
					while(cpuList[cpuCount]->busy == 1)
					{
						cpuCount++;
					}
					cpuList[cpuCount]->busy = 1;
					cpuList[cpuCount]->idleAt = time;
					curr->timeLeft = curr->timeLeft - 1;
					cpuList[cpuCount]->jobAssigned = curr;
					fprintf(file,"%d %s %s %d\n",time,curr->process,curr->user,time+1);
					if(curr->timeLeft == 0)	//this job is done
					{
						Summary *tempUser = NULL;
						tempUser = (Summary*)malloc(sizeof(Summary));
						tempUser->arrivedAt = curr->arrival;
						tempUser->user = curr->user;
						tempUser->lastCompleted = time;
						tempUser->arrivalPri = curr->arrivalNum;
						summaries[i] = tempUser;
						i++;
					}	
				}
			}
			curr = curr->next;
			if(!curr)	//have hit the end of the list without filling all the cpus
			{
				time++;
			}
		}
	}

	//check for duplicates 
	for(i = 0; i < maxUsers - 1; i++)
	{
		for(j = i + 1; j < maxUsers; j++)
		{
			if(summaries[i] == NULL)
			{
				continue;
			}
			else if(summaries[j] != NULL) //we have data present
			{
				if(strcmp(summaries[i]->user,summaries[j]->user) == 0)	//we have a duplicate
				{
					if(summaries[i]->lastCompleted > summaries[j]->lastCompleted)  //summaries[i] is the more recently completed job
					{
						summaries[j] = NULL;
					}
					else								//summaries[j] is the more recently completed job
					{
						summaries[j]->arrivalPri = summaries[i]->arrivalPri;
						summaries[i] = summaries[j];
						summaries[j] = NULL;
					}
				}
			}
		}
	}
	
	//sort summaries
	for(i = 0; i < maxUsers -1; i++)
	{
		for(j=i+1; j< maxUsers; j++)
		{
			if(summaries[i] == NULL)
			{
				continue;
			}
			else if(summaries[j] != NULL)
			{
				if(summaries[i]->arrivalPri > summaries[j]->arrivalPri) //swap
				{
					Summary *temp = summaries[i];
					summaries[i] = summaries[j];
					summaries[j] = temp;
				}
			}
		}
	}	
	
	//sort cpuList
	for(i = 0; i < cpus -1; i++)
	{
		for(j=i+1; j< cpus; j++)
		{
			if(cpuList[i]->availableAt > cpuList[j]->availableAt) //swap
			{
				CPU* temp = cpuList[i];
				cpuList[i] = cpuList[j];
				cpuList[j] = temp;
			}
		}
	}

	//indicate that the job scheduling has completed
	for(i = 0; i < cpus; i++)
	{
		fprintf(file,"%d CPU%dIDLE CPU%dIDLE 0\n",cpuList[i]->idleAt,cpuList[i]->cpuNum,cpuList[i]->cpuNum);
	}

	//print out summary to audit/log file
	for(i = 0; i < maxUsers; i++)
	{
		if(summaries[i] != NULL)	//we have data
		{
			fprintf(file,"%s %d\n",summaries[i]->user, summaries[i]->lastCompleted + 1);
		}
	}

	//free summaries
	for(i = 0; i < maxUsers; i++)
	{
		summaries[i] = NULL;
		free(summaries[i]);
	}

	//free cpus
	for(i = 0; i < cpus; i++)
	{
		cpuList[i] = NULL;
		free(cpuList[i]);
	}

	//close the audit/log file
	fclose(file);
}

//produces a formatted report for the scheduler
void printResult()
{
	//open and read the audit/log
	FILE *file = fopen("TaskLog.txt","r");
	int time;
	char job[MAXLEN];
	char user[MAXLEN];
	int completed;

	//set up a header
	printf("Time\tJob\n");

	//read and print loop
	while(1)
	{
		if(!feof(file) && (4 == fscanf(file, "%d %s %s %d", &time, job, user, &completed))) //format is correct
		{
			printf("%d\t%s\n",time, job);
		}
		else										   //we are done with this section of the report
		{
			break;
		}
	}
	
	//set up the next header
	printf("\n");
	printf("Summary\n");

	//read and print loop
	while(1)
	{
		if(!feof(file) && (2 == fscanf(file, "%s %d", user, &time)))	//format is correct
		{
			printf("%s\t%d\n",user,time);
		}
		else								//we are done
		{
			break;
		}
	}	

	//close the audit/log file
	fclose(file);
}

void sortArrived(int time)
{
	Node *start = head;
	Node *comp;
	Node *swap; 
	int counter = 0;
	int i;

	if(start == NULL)	//list is empty
	{
		return;
	}

	//loop as long as there is another node
	while(start->next)
	{
		counter = 0;
		swap = start;
		comp = start->next;

		//loop as long as there is another node
		while(comp)
		{
			if(comp->arrival <= time)	//the job has arrived
			{

				if(swap->duration > comp->duration)	//set a new swap
				{
					swap = comp;
				}

				else if(swap->duration == comp->duration)	//both jobs have same duration
				{
					if(swap->arrival > comp->arrival)	//set a new swap
					{
						swap = comp;
					}
				}
			}
			comp = comp->next;
		}

		if(swap != start)	//we have changed swap
		{
			Node *push = start;
			char *tempUser1 = start->user;
			char *tempProcess1 = start->process;
			int tempArrival1 = start->arrival;
			int tempDuration1 = start->duration;
			int tempTimeLeft1 = start->timeLeft;
			char *tempUser2;
			char *tempProcess2;
			int tempArrival2;
			int tempDuration2;
			int tempTimeLeft2;
			start->user = swap->user; 
			start->process = swap->process;
			start->arrival = swap->arrival;
			start->duration = swap->duration;
			start->timeLeft = swap->timeLeft;

			//loop as long as we have no reached swap
			while(push != swap)
			{
				if(push->next != swap)	//the next node is not swap
				{
					tempUser2 = push->next->user;
					tempProcess2 = push->next->process;
					tempArrival2 = push->next->arrival;
					tempDuration2 = push->next->duration;
					tempTimeLeft2 = push->next->timeLeft;
					push = push->next;
					push->user = tempUser1;
					push->process = tempProcess1;
					push->arrival = tempArrival1;
					push->duration = tempDuration1;
					push->timeLeft = tempTimeLeft1;
					tempUser1 = tempUser2;
					tempProcess1 = tempProcess2;
					tempArrival1 = tempArrival2;
					tempDuration1 = tempDuration2;
					tempTimeLeft1 = tempTimeLeft2;
				}
				else	//the next node is swap
				{
					push->next->user = tempUser1;
					push->next->arrival = tempArrival1;
					push->next->process = tempProcess1;
					push->next->duration = tempDuration1;
					push->next->timeLeft = tempTimeLeft1;
					push = push->next;
				}
			}
		}
		start = start->next;	
	}
}

void printQueue()
{
	Node *start = head;
	
	//loop while there is data
	while(start)
	{
		printf("%s %s %d %d %d\n",start->user, start->process, start->arrival, start->duration, start->timeLeft);
		start = start->next;
	}
}
