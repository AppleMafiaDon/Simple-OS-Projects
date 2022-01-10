#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define Default_Rows 10
#define Default_Columns 10
#define Max_Connections 20

int processBuyResponse(int connID, char *response, long size);
int getRowColResponse(int connID, char *response);
pthread_t tids[Max_Connections * 2];
pthread_mutex_t mutex;
int numRows = 0, numCols = 0, connPos = 0, seatsLeft = 0;
int **venue;
char successMsg[100];
int lenSucc = sizeof(successMsg);
char noSeatsMsg[100] = "There are no more seats available. Sorry!\n";
int lenSeat = sizeof(noSeatsMsg);
char buyTicketMsg[100] = "Would you like to buy a ticket? yes/no\n";
int lenBuy = sizeof(buyTicketMsg);
char thanksMsg[100] = "Thank you for shopping with us.\n";
int lenThanks = sizeof(thanksMsg);
char seatTakenMsg[100] = "Sorry, but that seat is taken. Please select a different seat.\n";
int lenTaken = sizeof(seatTakenMsg);
char rowColMsg[100] = "Please enter the row and column of the seat you wish to purchase.\n";
int lenAsk = sizeof(rowColMsg);
char dataCorruptedMsg[100] = "Somehow the venue data has been corrupted. Closing the connections.\n";
int lenCorrupt = sizeof(dataCorruptedMsg);
char purchaseMsg[100] = "You have successfully purchased a seat. Thank you!\n";
int lenPurchase = sizeof(purchaseMsg);

void seatsAvailable() {
	pthread_mutex_lock(&mutex);
	for (int i = 0; i < numRows; i++) {
		printf("[");
		for (int j = 0; j < numCols; j ++) {
			printf("%d, ", venue[i][j]);
		}
		printf("]\n");
	}
	pthread_mutex_unlock(&mutex);
}

int sendMessage(int conn, char *msg, int len) {
	int bytesSent = 0;
	if ((bytesSent = send(conn, msg, len, 0)) < 0) {
		perror("There was an issue sending the message:");
		return -1;
	}
	else if (bytesSent == 0) {
		printf("Sorry, Nothing was sent on %d. \n", conn); fflush(stdout);
		return 0;
	}
	printf("bytesSent: %d msg: %s\n", bytesSent, msg); fflush(stdout);
	return bytesSent;
}

int recieveMsg(int connID, int flag) {
	int respFlag = 0;
	char respBuff[1024];

	printf("response: %lu, %lu\n", sizeof(respBuff), sizeof(respBuff) / sizeof(char)); fflush(stdout);
	if ((respFlag = recv(connID, &respBuff, sizeof(respBuff), 0)) < 0) {
		perror("There was an error recieving from the client:");
		return -1;
	}
	else if (respFlag == 0) {
		printf("Client connection closed unexpectedly on %d.\n", connID); fflush(stdout);
		return -1;
	}

	printf("%s flag: %d\n", respBuff, flag); fflush(stdout);//thank you message}
	if (flag == 1) {
		if (processBuyResponse(connID, respBuff, sizeof(respBuff)) < 0)
			return -1;
	}
	else if (flag == 2) {
		if (getRowColResponse(connID, respBuff) < 0)
			return -1;
	}
	return 0;
}

//returns the status of the operation
int manBuySeat(int row, int column) {
	pthread_mutex_lock(&mutex); //accessing venue, seatsLeft
	if (venue[row-1][column-1] == 0) {
		venue[row-1][column-1] = 1;
		seatsLeft--;
		pthread_mutex_unlock(&mutex);
		seatsAvailable();
		return 0;	//seat successfully purchased
	}
	else if (venue[row-1][column-1] == 1) {
		pthread_mutex_unlock(&mutex);
		return -2; //seat already taken
	}
	pthread_mutex_unlock(&mutex);
	return -1; //data in the seating is corrupted
}

//returns the seat number of the last seat purchased
int autoBuySeat(int connID, char* response, long size) {
	srand(time(0));
	int randRow = 0, randCol = 0;
	//printf("seeded random, trying to buy seat row: %d col: %d", randRow, randCol); fflush(stdout);
	while (seatsLeft > 0) {
		pthread_mutex_lock(&mutex);//accessing venue, numCols, numRows
		while (1) {
			randRow = rand() % numRows;
			randCol = rand() % numCols;
			//printf("trying to buy seat row: %d col: %d", randRow, randCol); fflush(stdout);
			if (venue[randRow][randCol] == 0) {
				venue[randRow][randCol] = 1;
				seatsLeft--;
				sprintf(response, "Successfully purchased ticket in row: %d, column: %d on %d\n", randRow, randCol, connID);
				printf("%s\n", response); fflush(stdout); //send the row and column purchased : row => result / numCols  column => ((result % numCols) + 1)
				if (sendMessage(connID, response, size) <= 0)
					return -1;
				pthread_mutex_unlock(&mutex);
				seatsAvailable();
				sleep(10);
				break;
			}
		}
	}
	if (sendMessage(connID, noSeatsMsg, lenSeat) <= 0)
		return -1;
	return 0;
}

int getRowColResponse(int connID, char *response) {
	int row = 0, col = 0, cntr = 0;
	char* arg;
	arg = strtok(response, " ");
	while (arg) {
		printf("arg: %s\n", arg); fflush(stdout);
		if (cntr == 0) {
			if ((row = atoi(arg)) == 0) {
				printf("row, Incorrect Response recieved on %d. Closing the connection\n", connID); fflush(stdout);
				return -1;
			}
		}
		else {
			if ((col = atoi(arg)) == 0) {
				printf("col, Incorrect Response recieved on %d. Closing the connection\n", connID); fflush(stdout);
				return -1;
			}
		}
		cntr++;
		arg = strtok(NULL, " ");
	}
	int manBuy = 0;
	if ((manBuy = manBuySeat(row, col)) == -2) {//if -2 returned 
		printf("Sending seatsTaken message using %d\n", connID); fflush(stdout);
		if (sendMessage(connID, seatTakenMsg, lenTaken) < 0)//send message saying seat taken
			return -1;
	}
	else if (manBuy == -1) {//else if -1 returned 
		printf("Sending corruptData message using %d\n", connID); fflush(stdout);
		sendMessage(connID, dataCorruptedMsg, lenCorrupt);//send message saying the venue data is corrupted 
		return -1;//return
	}
	else {//else send message saying they successfully purchased the seat
		printf("Sending success message using %d\n", connID); fflush(stdout);
		if (sendMessage(connID, purchaseMsg, lenPurchase) < 0)//send message saying successful purchase
			return -1;
	}
}

int processBuyResponse(int connID, char *response, long size) {
	printf("response: %s\n", response); fflush(stdout);
	if (strstr(response, "no") != NULL) { //if no
		printf("Sending thanks message using %d\n", connID); fflush(stdout);
		sendMessage(connID, thanksMsg, lenThanks); //send message: saying "Thank you for shopping with us.\n"
		return -1; //connection will be closed by thread manager
	}
	else if (strcmp(response, "yes manual") == 0) { //if yes and the flag is manual
		printf("Sending rowCol message using %d\n", connID); fflush(stdout);
		if (sendMessage(connID, rowColMsg, lenAsk) < 0) //send row col message
			return -1;
		//recieve: row and column values
		printf("recieving rowColumnResponse message using %d\n", connID); fflush(stdout);
		if (recieveMsg(connID, 2) < 0) //the answer will be valid row and column: hopefully guarenteed by client
			return -1;
	}
	else if (strcmp(response, "yes auto") == 0) { //if yes and the flag is auto
		//printf("autobuying seat"); fflush(stdout);
		if (autoBuySeat(connID, response, size) < 0)//run autoBuySeat()
			return -1; 
		
	}
}

void * processConnection(void *connFD) {
	int connID = *((int *)connFD);
	printf("connID: %d\n", connID);
	sprintf(successMsg, "rows=%d\ncolumns=%d", numRows, numCols);

	char recBuff[1024];
	memset(&recBuff, 0, sizeof(recBuff));
	printf("Sending connection message using %d\n", connID); fflush(stdout);
	
	if (sendMessage(connID, successMsg, lenSucc) <= 0)
		return NULL;
	printf("Recieving continue message using %d\n", connID); fflush(stdout);
	if (recieveMsg(connID, 0) < 0) //the answer will be continue
		return NULL;
	
	while (seatsLeft > 0) {
		printf("Sending buyTickets message using %d\n", connID); fflush(stdout);
		if (sendMessage(connID, buyTicketMsg, lenBuy) <= 0) //send message: ask if they want to buy a ticket
			return NULL;
		//recieve: answer
		printf("Recieving buyResponse message using %d\n", connID); fflush(stdout);
		if (recieveMsg(connID, 1) < 0) //the answer will be yes or no: hopefully guarenteed by client
			return NULL;
		//the buy response should be getting processed and finish processing
		printf("Recieving continue message using %d\n", connID); fflush(stdout);
		if (recieveMsg(connID, 0) < 0) //the answer will be continue
			return NULL;
	}
	if (sendMessage(connID, noSeatsMsg, lenSeat) <= 0)
		return NULL;
}

void *waitForDone(void *connfd) {
	pthread_mutex_lock(&mutex);//accessing connPos and tids
	int threadPos = connPos; //store the position of this thread in the list of connection threads locally
	int connFD = *((int *)connfd);
	printf("connFD: %d\n", connFD); fflush(stdout);
	pthread_create(&(tids[threadPos + 1]), NULL, &processConnection, &connFD);//create a thread to process the connection
	pthread_mutex_unlock(&mutex);//make sure the mutex isn't locked for the entirety of the thread

	pthread_join((tids[threadPos + 1]), NULL); //wait for that thread to complete processing the connection
	
	pthread_mutex_lock(&mutex);//accessing tids
	printf("Closing connection %d\n", connFD); fflush(stdout);
	close(connFD); //finally close the connection as the last thing to do so more connections can be created.
	tids[threadPos] = 0; //set the thread to be available
	pthread_mutex_unlock(&mutex);
}

int init(int argc, char* argv[]) {
	if (argc == 1) {
		numRows = Default_Rows;
		numCols = Default_Columns;
	}
	else if (argc == 3) {
		if ((numRows = atoi(argv[1])) <= 0 || (numCols = atoi(argv[2])) <= 0) {
			printf("Please enter a valid set of positive non-zero integers which represent the number of rows and columns in the venue.\n");
			return -1;
		}
	}
	else {
		printf("You supplied an incorrect number of arguments. Usage: %s <numRows> <numColumns>\n", argv[0]);
		return -2;
	}
	venue = calloc(numRows, sizeof(int *));
	for (int i = 0; i < numRows; i++) {
		venue[i] = calloc(numCols, sizeof(int));
	}
	seatsLeft = numRows * numCols;
	printf("Rows: %d, Columns: %d, SeatsLeft: %d\n", numRows, numCols, seatsLeft);
	seatsAvailable();
	return 0;
}


int main(int argc, char* argv[]) {
	if (init(argc, argv) < 0)
		return -1;
	if (pthread_mutex_init(&mutex, NULL) != 0) {
		perror("There was an issue trying to initialize the mutex:");
		return -2;
	}

	struct addrinfo hints, *ipAddr; 
	int listenfd = 0, connfd = 0;
	// first, load up address structs with getaddrinfo():
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;  // use IPv4
	hints.ai_socktype = SOCK_STREAM; //use TCP
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
	
	//get the address info
	if (getaddrinfo(NULL, "6666", &hints, &ipAddr) < 0) {//change the port value here if you want to listen and connect ot a different port in your ini file for the client
		perror("There was an error getting the address information:");
		return -3;
	}
	// make a socket
	if ((listenfd = socket(ipAddr->ai_family, ipAddr->ai_socktype, ipAddr->ai_protocol)) < 0) {
		perror("There was an error creating the socket:");
		return -4;
	}
	//bind the socket
	if (bind(listenfd, ipAddr->ai_addr, ipAddr->ai_addrlen) < 0) {
		perror("There was an error binding:");
		return -5;
	}
	//listen on the socket
	if (listen(listenfd, Max_Connections) < 0) {
		perror("There was an error listening:");
		return -6;
	}
	printf("Listening on %d.\n", listenfd);
	while (1) {
		//printf("Accepting on %d", listenfd);  fflush(stdout);
		if ((connfd = accept(listenfd, NULL, NULL)) < 0) {//accept the connection
			perror("An error occurred while establishing connection:");
			continue;
		}
		else if(connfd != 0 && tids[connPos] == 0) {
			pthread_mutex_lock(&mutex);//accessing tids
			printf("locked mutex"); fflush(stdout);
			printf("Connection established, sending to the waiting thread on %d.\n", connfd); fflush(stdout);
			pthread_create(&(tids[connPos]), NULL, &waitForDone, &connfd);
			pthread_detach((tids[connPos]));//detach the waiting thread
			pthread_mutex_unlock(&mutex);//accessed tids
			printf("unlocked mutex"); fflush(stdout);
			sleep(1); //sleep for a second to make sure that connPos has been stored locally in the threaded method.
			pthread_mutex_lock(&mutex);//accessing connPos
			printf("locked mutex"); fflush(stdout);
			connPos += 2;
			printf("inner, connPos: %d, connfd: %d", connPos, connfd); fflush(stdout);
			pthread_mutex_unlock(&mutex);//accessed connPos
			printf("unlocked mutex"); fflush(stdout);
		}
		else if (connfd != 0 ) { //Else we need to find a place for the thread to be stored 
			int origPos = connPos;
			while (tids[connPos] > 0) {
				pthread_mutex_lock(&mutex);//accessing connPos
				printf("locked mutex"); fflush(stdout);
				connPos += 2;
				printf("connPos: %d", connPos); fflush(stdout);
				if (connPos >= Max_Connections) {//We need to cycle back to the beginning of the thread list
					connPos = 0;
				}
				if (connPos == origPos) { //There was a really weird error where it allowed a connection to be accepted beyond the defined bounds of acceptable connections
					printf("No available connection slots.\n"); fflush(stdout);
					pthread_mutex_unlock(&mutex);//accessed connPos
					printf("unlocked mutex"); fflush(stdout);
					break;
				}
				pthread_mutex_unlock(&mutex);//accessed connPos
				printf("unlocked mutex"); fflush(stdout);
			}
			if (tids[connPos] == 0) {
				pthread_mutex_lock(&mutex);//accessing tids and connPos
				printf("locked mutex"); fflush(stdout);
				printf("Connection established, sending to the waiting thread on %d.\n", connfd); fflush(stdout);
				pthread_create(&(tids[connPos]), NULL, &waitForDone, &connfd);
				pthread_detach((tids[connPos]));//detach the waiting thread
				pthread_mutex_unlock(&mutex);//accessed tids
				printf("unlocked mutex"); fflush(stdout);
				sleep(1); //sleep for a second to make sure that connPos has been stored locally in the threaded method.
				pthread_mutex_lock(&mutex);//accessing connPos
				printf("locked mutex"); fflush(stdout);
				connPos += 2;
				pthread_mutex_unlock(&mutex);//accessed connPos
				printf("unlocked mutex"); fflush(stdout);
			}
		}
		pthread_mutex_lock(&mutex);//accessing connPos
		printf("locked mutex"); fflush(stdout);
		if (connPos >= Max_Connections) {//We need to cycle back to the beginning of the thread list
			connPos = 0;
		}
		printf("outer connPos: %d, connfd: %d\n", connPos, connfd);
		pthread_mutex_unlock(&mutex);//accessed connPos
		printf("unlocked mutex"); fflush(stdout);
	}

	//Clean-Up:
	pthread_mutex_destroy(&mutex);
	for (int i = 0; i < numRows; i++) {
		free(venue[i]);
	}
	free(venue);
	return 0;
}