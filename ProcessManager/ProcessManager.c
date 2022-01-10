#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>


#define MAX_BURST 10
#define LOWEST_PRI 128

enum State {
	Waiting, Ready, Terminated
};

struct PCB {
	int pid;
	int state;
	int cpuBurst;
	int priority;
};

struct PCB* m, *n;
int inputM, inputN;
pthread_mutex_t mutex;

void swap(struct PCB* a, struct PCB* b) {
	struct PCB temp = *a;
	*a = *b;
	*b = temp;
}

int partitionPri(struct PCB* queue, int left, int right, int pivot) {
	int leftPointer = left - 1;
	int rightPointer = right;

	while (1) {
		while (queue[++leftPointer].priority < pivot && leftPointer < right) {
			//do nothing
		}

		while (rightPointer > 0 && queue[--rightPointer].priority > pivot && rightPointer > left) {
			//do nothing
		}
		if (leftPointer >= rightPointer) {
			break;
		}
		else {
			printf(" item swapped :%d,%d\n", queue[leftPointer].priority, queue[rightPointer].priority);
			swap(&queue[leftPointer], &queue[rightPointer]);
		}
	}
	printf(" pivot swapped :%d,%d\n", queue[leftPointer].priority, queue[right].priority);
	swap(&queue[leftPointer], &queue[right]);
	return leftPointer;
}

void quickSortPri(struct PCB*queue, int left, int right) {
	if (right - left <= 0) {
		return;
	}
	else {
		int pivot = queue[right].priority;
		int partitionPoint = partitionPri(queue, left, right, pivot);
		quickSortPri(queue, left, partitionPoint - 1);
		quickSortPri(queue, partitionPoint + 1, right);
	}
}

int partitionSJF(struct PCB* queue, int left, int right, int pivot) {
	int leftPointer = left - 1;
	int rightPointer = right;

	while (1) {
		while (queue[++leftPointer].cpuBurst < pivot && leftPointer < right) {
			//do nothing
		}

		while (rightPointer > 0 && queue[--rightPointer].cpuBurst > pivot && rightPointer > left) {
			//do nothing
		}
		if (leftPointer >= rightPointer) {
			break;
		}
		else {
			printf(" item swapped :%d,%d\n", queue[leftPointer].cpuBurst, queue[rightPointer].cpuBurst);
			swap(&queue[leftPointer], &queue[rightPointer]);
		}
	}
	printf(" pivot swapped :%d,%d\n", queue[leftPointer].cpuBurst, queue[right].cpuBurst);
	swap(&queue[leftPointer], &queue[right]);
	return leftPointer;
}

void quickSortSJF(struct PCB*queue, int left, int right) {
	if (right - left <= 0) {
		return;
	}
	else {
		int pivot = queue[right].cpuBurst;
		int partitionPoint = partitionSJF(queue, left, right, pivot);
		quickSortSJF(queue, left, partitionPoint - 1);
		quickSortSJF(queue, partitionPoint + 1, right);
	}
}

int comparePri(const void* proc1, const void* proc2) {
	return ((*(struct PCB*)proc1).priority - (*(struct PCB*)proc2).priority);
}

int compareSJF(const void* proc1, const void* proc2) {
	return ((*(struct PCB*)proc1).cpuBurst - (*(struct PCB*)proc2).cpuBurst);
}

int loadBalance(int queueFlag) {
	int count = 0;
	if (queueFlag == 0) {
		for (int i = (inputN + 1)/2; i < inputN; i++) {
			printf("load balancing process %d from Priority to SJF. \n", n[i].pid); fflush(stdout);
			m[count] = n[i];
			count++;
		}
		inputN = ((inputN + 1) / 2);
		inputM = count;
		printf("new SJF Length: %d, new Priority Length: %d. \n", inputM, inputN); fflush(stdout);
		qsort(&m, inputM, sizeof(struct PCB), &compareSJF);
	}
	else {
		for (int i = (inputM + 1) / 2; i < inputM; i++) {
			printf("load balancing process %d from SJF to Priority. \n", m[i].pid); fflush(stdout);
			n[count] = m[i];
			count++;
		}
		inputM = ((inputM + 1) / 2);
		inputN = count;
		printf("new SJF Length: %d, new Priority Length: %d. \n", inputM, inputN); fflush(stdout);
		qsort(&n, inputN, sizeof(struct PCB), &comparePri);
	}
	return 0;
}

int execute(int queueFlag) {
	if (queueFlag == 0) {
		for (int i = 0; i < inputM; i++) {
			if (m[i].state == Ready) {
				printf("Process %d is executing from SJF queue. \n", m[i].pid); fflush(stdout);
				sleep(1);
				m[i].cpuBurst -= 1;
				printf("Process %d from SJF queue has %d seconds left to run. \n", m[i].pid, m[i].cpuBurst); fflush(stdout);
				if (m[i].cpuBurst == 0) {
					printf("Process %d is finished from SJF queue. Terminating! \n", m[i].pid); fflush(stdout);
					m[i].state = Terminated;
				}
				return 0;
			}
		}
	}
	else {
		for (int i = 0; i < inputM; i++) {
			if (n[i].state == Ready) {
				printf("Process %d is executing from Priority queue. \n", n[i].pid); fflush(stdout);
				sleep(1);
				n[i].cpuBurst -= 1;
				printf("Process %d from Priority queue has %d seconds left to run. \n", n[i].pid, n[i].cpuBurst); fflush(stdout);
				if (n[i].cpuBurst == 0) {
					printf("Process %d is finished from Priority queue. Terminating! \n", n[i].pid); fflush(stdout);
					n[i].state = Terminated;
				}
				return 0;
			}
		}
	}
}

void* stateSwitch() {
	while (1) {
		sleep(5);
		pthread_mutex_lock(&mutex);
		for (int i = 0; i < inputM; i++) {
			if (m[i].state != Terminated) {
				m[i].state = rand() % 2;
				printf("Process %d was set to state %d in SJF queue. \n", m[i].pid, m[i].state); fflush(stdout);
			}
		}
		for (int j = 0; j < inputN; j++) {
			if (n[j].state != Terminated){
				n[j].state = rand() % 2;
				printf("Process %d was set to state %d in Priority queue. \n", n[j].pid, n[j].state); fflush(stdout);
			}
		}
		if (inputN == 0 & inputM == 0) {
			printf("No more processes to run, ending state switcher. \n"); fflush(stdout);
			pthread_mutex_unlock(&mutex);
			return NULL;
		}
		pthread_mutex_unlock(&mutex);
	}
}

void* aging() {
	while (1) {
		sleep(2);
		pthread_mutex_lock(&mutex);
		for (int i = 0; i < inputM; i++) {
			if (m[i].priority > 0) {
				m[i].priority -= 1;
				printf("Process %d was set to priority %d in SJF queue. \n", m[i].pid, m[i].priority); fflush(stdout);
			}
		}
		for (int j = 0; j < inputN; j++) {
			if (n[j].priority > 0) {
				n[j].priority -= 1;
				printf("Process %d was set to priority %d in Priority queue. \n", n[j].pid, n[j].priority); fflush(stdout);
			}
		}
		if (inputN == 0 & inputM == 0) {
			printf("No more processes to run, ending aging. \n"); fflush(stdout);
			pthread_mutex_unlock(&mutex);
			return NULL;
		}
		pthread_mutex_unlock(&mutex);
	}
}

int checkNQueue() {
	for (int i = 0; i < inputN; i++) {
		if (n[i].state != Terminated)
			return 0;
	}
	return 1;
}

int checkMQueue() {
	for (int i = 0; i < inputM; i++) {
		if (m[i].state != Terminated)
			return 0;
	}
	return 1;
}

void* priorityCycle() {
	int empty = 0, queueFlag = 1;
	while (!empty) {
		pthread_mutex_lock(&mutex);
		execute(queueFlag);
		empty = checkNQueue();
		if (empty && !checkMQueue()) {
			loadBalance(queueFlag);
			empty = 0;
		}
		pthread_mutex_unlock(&mutex);
	}
	printf("Both queues are finished. \n"); fflush(stdout);
	pthread_mutex_lock(&mutex);
	inputN = 0;
	pthread_mutex_unlock(&mutex);
}

void* SJFcycle() {
	int empty = 0, queueFlag = 0;
	while (!empty) {
		pthread_mutex_lock(&mutex);
		execute(queueFlag);
		empty = checkMQueue();
		if (empty && !checkNQueue()) {
			loadBalance(queueFlag);
			empty = 0;
		}
		pthread_mutex_unlock(&mutex);
	}
	printf("Both queues are finished. \n"); fflush(stdout);
	pthread_mutex_lock(&mutex);
	inputM = 0;
	pthread_mutex_unlock(&mutex);
}

struct PCB createPCB() {
	int cpuburst = 0;
	while (cpuburst == 0) {
		cpuburst = rand() % MAX_BURST;
	}
	struct PCB newPCB = { rand() % 1000000, 
		Ready, 
		cpuburst,
		rand() % LOWEST_PRI };
	return newPCB;
}

int loadProcesses() {
	srand(time(0));
	for (int i = 0; i < inputM; i++) {
		m[i] = createPCB();
		printf("%d process loaded in SJF: pid: %d, cpuburst: %d : priority: %d \n", i, m[i].pid, m[i].cpuBurst, m[i].priority ); fflush(stdout);
	}
	for (int j = 0; j < inputN; j++) {
		n[j] = createPCB();
		printf("%d process loaded in Priority: pid: %d, cpuburst: %d : priority: %d \n", j, n[j].pid, n[j].cpuBurst, n[j].priority); fflush(stdout);
	}
}

int checkArgs(int argc, char* argv[]) {
	//check that a number is supplied to the command line,
	if (argc == 3) {
		inputM = atoi(argv[1]);
		inputN = atoi(argv[2]);
		if (inputM <= 0 || inputN <= 0) {
			printf("Please enter a positive numbers for the input arguments of the function. Usage: <command name> <numSJFprocesses> <numPBprocesses> \n"); fflush(stdout);
			return -1;
		}
		if (inputM < inputN)
			return inputN;
		else
			return inputM;
	}
	else {
		printf("Invalid number of arguments. Please input two positive numbers for number of threads to process. Usage: <command name> <numSJFprocesses> <numPBprocesses> \n"); fflush(stdout);//print message saying please provide a number 
		return -1;
	}
}

void printPriority() {
	printf("[ ");
	for (int i = 0; i < inputN; i++) {
		printf("%d, ", n[i].priority);
	}
	printf("] \n"); fflush(stdout);
}

void printSJF() {
	printf("[ ");
	for (int i = 0; i < inputM; i++) {
		printf("%d, ", m[i].cpuBurst);
	}
	printf("] \n"); fflush(stdout);
}

int main(int argc, char* argv[]) {
	int procLength = 0;
	if((procLength = checkArgs(argc, argv)) < 0)
		return -1;
	if (pthread_mutex_init(&mutex, NULL) != 0) {
		perror("There was an issue trying to initialize the mutex:");
		return -2;
	}
	//make sure each array of PCBs is large enough to store the larger number of processes given in initialization
	//this is in case of load balancing
	m = malloc(procLength * sizeof(struct PCB)); 
	n = malloc(procLength * sizeof(struct PCB));
	loadProcesses();
	quickSortSJF(m, 0, inputM - 1);
	printSJF();
	quickSortPri(n, 0, inputN - 1);
	printPriority();
	pthread_t pthreads[4];
	memset(&pthreads, 0, sizeof(pthreads));
	if (pthread_create(&pthreads[0], NULL, &SJFcycle, NULL) < 0) {
		perror("There was an error creating the SJF thread:");
		pthread_mutex_destroy(&mutex);
		free(m);
		free(n);
		return -1;
	}
	if (pthread_create(&pthreads[1], NULL, &priorityCycle, NULL) < 0) {
		perror("There was an error creating the priority thread:");
		pthread_mutex_destroy(&mutex);
		free(m);
		free(n);
		return -1;
	}
	if (pthread_create(&pthreads[2], NULL, &aging, NULL) < 0) {
		perror("There was an error creating the aging thread:");
		pthread_mutex_destroy(&mutex);
		free(m);
		free(n);
		return -1;
	}
	if (pthread_create(&pthreads[3], NULL, &stateSwitch, NULL) < 0) {
		perror("There was an error creating the state thread:");
		pthread_mutex_destroy(&mutex);
		free(m);
		free(n);
		return -1;
	}

	for (int i = 0; i < 4; i++) {
		if (pthread_join(pthreads[i], NULL) < 0) {
			perror("There was an error joining the thread:");
			pthread_mutex_destroy(&mutex);
			free(m);
			free(n);
			return -1;
		}
	}
	pthread_mutex_destroy(&mutex);
	free(m);
	free(n);
	printf("Application finished, closing! \n"); fflush(stdout);
	return 0;
}