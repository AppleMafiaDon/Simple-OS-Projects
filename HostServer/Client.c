#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define Default_IP "172.17.0.5"
#define Default_Port 6666    //This is the default port provided to the server Should be the same in the file too
#define Default_Timeout 60   //this is in seconds

char IP[20];
int port = 0, timeout = 0, rows = 0, cols = 0;;
char* buyType;

int processRowsCols(char *response) {
	printf("response: %s\n", response);
	char *args[2];
	char* arg;
	int cntr = 0;
	arg = strtok(response, "\n");
	while (arg) {
		//printf("arg: %s\n", arg); fflush(stdout);
		args[cntr] = arg;
		cntr++;
		arg = strtok(NULL, "\n");
	}
	int rowFlag = 0, colFlag = 0;
	char *value;
	for (int i = 0; i < cntr; i++){
		value = strtok(args[i], "=");
		while (value) {
			//printf("Value: %s\n", value);
			if (rowFlag == 0 && colFlag == 0) {
				if (strcmp(value, "rows") == 0) {
					rowFlag = 1;
				}
				else if (strcmp(value, "columns") == 0) {
					colFlag = 1;
				}
				else {
					printf("Improper response. Closing the connection.\n"); fflush(stdout);
					return -1;
				}
			}
			else if (rowFlag == 1) {
				if ((rows = atoi(value)) == 0) {
					printf("Did not recieve a number value for rows. Closing the Connection. \n"); fflush(stdout);
					return -1;
				}
				rowFlag = 0;
			}
			else if (colFlag == 1) {
				//printf("setting cols"); fflush(stdout);
				if ((cols = atoi(value)) == 0) {
					printf("Did not recieve a number value for columns. Closing the Connection. \n"); fflush(stdout);
					return -1;
				}
				//printf("cols: %d", cols);
				colFlag = 0;
			}
			value = strtok(NULL, "=");
		}
		printf("arg: %s\n", args[i]); fflush(stdout);	
	}
	return 0;
}

int recieveMsg(int connID, int flag) {
	int respFlag = 0;
	char respBuff[1024];
	char *noSeatsMsg = "There are no more seats available. Sorry!\n";
	char *dataCorruptedMsg = "Somehow the venue data has been corrupted. Closing the connections.\n";

	printf("response: %lu, %lu\n", sizeof(respBuff), sizeof(respBuff) / sizeof(char)); fflush(stdout);
	if ((respFlag = recv(connID, &respBuff, sizeof(respBuff), 0)) < 0) {
		perror("There was an error recieving from the server:");
		return -1;
	}
	else if (respFlag == 0) {
		printf("Server connection closed unexpectedly on %d.\n", connID); fflush(stdout);
		return -1;
	}
	else if (strcmp(respBuff, noSeatsMsg)== 0) {
		printf("%s", respBuff); fflush(stdout);
		return -1;
	}
	else if (strcmp(respBuff, dataCorruptedMsg) == 0) {
		printf("%s", respBuff); fflush(stdout);
		return -1;
	}
	printf("%s flag: %d\n", respBuff, flag); fflush(stdout);//message received
	if (flag == 1) {
		printf("Processing rows and columns on %d", connID); fflush(stdout);
		if (processRowsCols(respBuff) < 0)
			return -1;
	}
	return 0;
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

int getRowsCols(int connID) { 
	printf("recieving connectionSuccess message using %d\n", connID); fflush(stdout);
	if (recieveMsg(connID, 1) < 0)
		return -1;
	printf("rows: %d, cols: %d", rows, cols); fflush(stdout);
	return 0;
}

int askBuy(int connID) {
	char sendBuf[100];
	while (1) {
		printf("Please Enter 'yes' or 'no'\n"); fflush(stdout);
		scanf("%s", sendBuf);
		if (strcmp(sendBuf, "no") == 0) {
			printf("sending clientBuyResponse message using %d\n", connID); fflush(stdout);
			if (sendMessage(connID, sendBuf, sizeof(sendBuf)) < 0)//send client response
				return -1;
			printf("recieving thanks message using %d\n", connID); fflush(stdout);
			recieveMsg(connID, 0); //process the next message thanksMsg
			//printf("Thanks received. Returning -1"); fflush(stdout);
			return -1;
		}
		else if (strcmp(sendBuf, "yes") == 0) {
			char* concat;
			sprintf(concat, " %s", buyType);
			strcat(sendBuf, concat);
			printf("sending clientBuyResponse message using %d\n", connID); fflush(stdout);
			if (sendMessage(connID, sendBuf, sizeof(sendBuf)) < 0)//send client response with buy type
				return -1;
			return 0;
		}
		else {
			printf("Incorrect input.\n");
		}
	}
}

int selectRowsCols(int connID) {
	char row[50], col[50], response[1024];
	int rowI = 0, colI = 0;
	printf("recieving rowColumnResponse message using %d\n", connID); fflush(stdout);
	if (recieveMsg(connID, 0) < 0) {//process the row/column response from server
		return -1;
	}
	while (rowI == 0) { //get a proper row
		printf("Enter row from 1 - %d \n", rows); fflush(stdout);
		scanf("%s", row);
		if ((rowI = atoi(row)) < 1 || rowI > rows) {
			printf("Incorect response. \n"); fflush(stdout);
			rowI = 0;
		}
	}
	while (colI == 0){// get a proper column
		printf("Enter column from 1 - %d \n", cols); fflush(stdout);
		scanf("%s", col);
		if ((colI = atoi(col)) < 1 || colI > cols) {
			printf("Incorect response. \n"); fflush(stdout);
			colI = 0;
		}
	}
	sprintf(response, "%s %s", row, col);
	printf("sending rowColumnClientResponse message using %d\n", connID); fflush(stdout);
	if (sendMessage(connID, response, sizeof(response)) < 0)//send the server the row and column
		return -1;
	printf("recieving purchaseOutcomeResponse message using %d\n", connID); fflush(stdout);
	if (recieveMsg(connID, 0) < 0) //get the server response: either seat taken, or success
		return -1;
}

void *buyTickets(void *sockfd) {
	int connID = *((int *)sockfd);
	char *seatTakenMsg = "Sorry, but that seat is taken. Please select a different seat.\n";

	int cont = 1;
	if (getRowsCols(connID) < 0) { //process initial message from server of rows and columns of venue
		return NULL;
	}
	while (cont) {
		if (sendMessage(connID, "continue", sizeof("continue")) < 0)//send the continue message that the server is expecting
			return NULL;
		printf("recieving askBuyResponse message using %d\n", connID); fflush(stdout);
		if (recieveMsg(connID, 0) < 0) { //process the next message either saying no seats or buy tickets
			return NULL;
		}
		if (askBuy(connID) < 0) { //process the ask to buy and client response
			//printf("askbuy < 1"); fflush(stdout);
			return NULL;
		}
		if (strcmp(buyType, "manual") == 0) {//buytype is manual
			if (selectRowsCols(connID) < 0) {//So setup the process to select rows and columns
				return NULL;
			}
		}
		else {// otherwise it's auto if you made it this far
			while (recieveMsg(connID, 0) >= 0) {//process the server sending successful purchase message repeatedly till no more to be made
				printf("recieving autoSuccess message using %d\n", connID); fflush(stdout);
			}
			return NULL;
		}//Done with that purchase, attempt to get the next response from the server, top of while loop
	}
}

int init(int argc, char*argv[]) {
	if (argc == 1) {
		strcpy(IP, Default_IP); //This should default it to the current local host
		port = Default_Port;
		timeout = Default_Timeout;
		buyType = "auto";
		printf("IP: %s, Port: %d, Timeout: %d, buyType: %s\n", IP, port, timeout, buyType); fflush(stdout);
		return 0;
	}
	else if (argc == 3) {
		FILE *init = fopen(argv[1], "r"); //open the file
		if (!init) {
			perror("There was a problem finding your input file: \n");// handle error;
			return -1;
		}
		char line[256]; //this will store the line being read in
		int ipFlag = 0, portFlag = 0, timeoutFlag = 0;
		char* arg;
		while (line[0] != EOF && fgets(line, sizeof(line), init)) { //while we still have lines to read
			printf("line: %s", line); fflush(stdout);
			arg = strtok(line, "= ");//split the line on '='
			while (arg) {
				printf("arg: %s", arg); fflush(stdout);
				if (ipFlag == 0 && portFlag == 0 && timeoutFlag == 0) {//if none of the flags are set
					//check what the first value is
					if (strstr(arg, "IP")) {//if ip let us know the next value will be the ip
						ipFlag = 1;
					}
					else if (strstr(arg, "Port") != NULL) {//if port let us know the next value will be the port number
						portFlag = 1;
					}
					else if (strstr(arg, "Timeout") != NULL) {//if timeout let us know the next value will be the timeout
						timeoutFlag = 1;
					}
				}
				else if (ipFlag == 1) {//process the IP value
					strcpy(IP, arg);
					printf("IP: %s", IP); fflush(stdout);
					ipFlag = 0;
				}
				else if (portFlag == 1) {//process the port value
					if ((port = atoi(arg)) == 0) {//if not an integer or a port of 0, return
						printf("There was an issue reading the port number. Make sure it is a number.\n");
						return -1;
					}
					portFlag = 0;
				}
				else if (timeoutFlag == 1) {//process the timeout value
					if ((timeout = atoi(arg)) == 0) {//if not an integer or a timeout of 0, return
						printf("There was an issue reading the timeout period. Make sure it is a number.\n");
						return -1;
					}
					timeoutFlag = 0;
				}
				arg = strtok(NULL, "= ");
			}
		}
		printf("IP: %s", IP); fflush(stdout);
		buyType = argv[2];
		printf("buyType: %s", buyType); fflush(stdout);
		//check if buyType is valid
		if (strcmp(buyType, "manual") == 0 || strcmp(buyType, "auto") == 0) {
			printf("IP: %s, Port: %d, Timeout: %d, buyType: %s\n", IP, port, timeout, buyType); fflush(stdout);
			return 0;
		}
		printf("Please use 'manual' or 'auto'. Usage: %s <ini filepath> <manual/auto>\n", argv[0]);
		return -1;
	}
	else {
		printf("You supplied an incorrect number of arguments. Usage: %s <ini filepath> <manual/auto>\n", argv[0]);
		return -2;
	}
}

int main(int argc, char *argv[]) {
	if (init(argc, argv) != 0) {
		printf("There was an error with the initialization of the client.\n");
		return -1;
	}

	struct addrinfo hints, *ipAddr;
	int connID;
	// first, load up address structs with getaddrinfo():
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	char portAsStr[50];
	sprintf(portAsStr, "%d", port);
	if (getaddrinfo(IP, portAsStr, &hints, &ipAddr) < 0) {
		perror("There was an error getting the address information:");
		return -2;
	}
	
	int tempTime = timeout;
	//attempt to connect for timeout period
	while (tempTime > 0) {
		//create the socket information
		if ((connID = socket(ipAddr->ai_family, ipAddr->ai_socktype, ipAddr->ai_protocol)) < 0) {
			perror("There was an error creating the socket:");
			return -3;
		}
		if (connect(connID, ipAddr->ai_addr, ipAddr->ai_addrlen) < 0) {
			printf("On %d ", connID); fflush(stdout);
			perror("Trying again. There was an issue connecting to the server:");
			tempTime--;
			sleep(1);
			continue;
		}
		else {//if successfully connected
			pthread_t tid = 0;
			if (pthread_create(&tid, NULL, &buyTickets, &connID) != 0) {//pass socket to function to manage messages and responses
				perror("There was an issue creating the buyTickets thread:");
				return -4;
			}
			if (pthread_join(tid, NULL) != 0) {//wait for the thread to complete
				perror("There was an issue waiting for the thread:");
				return -5;
			}
			printf("Closing the connection %d \n", connID); fflush(stdout);
			close(connID);
			return 0;
		}
	}
	if (tempTime <= 0) 
		printf("Your connection timed out. \n");

	return 0;
}