#include <stdio.h> 
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>

bool clientActive = true;
bool sendingRequest = false;
bool receivingAnswer = false;


void disconnectClient(){
	printf("Disconnecting client...\n");
	//...
	clientActive = false;
}

void sendConsoleInput(){
	char* inputLine = NULL;
	size_t inputLen = 0;
	getline(&inputLine, &inputLen, stdin);
	inputLine[strlen(inputLine)] = '\0'; 

	int fd;

	if( (fd = open("myFifo2", O_WRONLY)) == -1 ){
		perror("Open FIFO");
		exit(-1);
	}
	else{
		//printf("Fifo opened for writing...\n");
		write(fd, inputLine, strlen(inputLine) + 1);
		//printf("Information sent: %s %ld \n", inputLine, strlen(inputLine));
	}

	close(fd);
}


int main(){

	char userInput[60];
	
	sendingRequest = true;
	while(clientActive){
		if(sendingRequest){
			sendConsoleInput();
			sendingRequest = false;
			receivingAnswer = true;
			printf("\n\n");
		}
		if(receivingAnswer){
			printf("Server feedback:\n");
			sendingRequest = true;
			receivingAnswer = false;
			printf("\n\n");
		}
	}
	sendConsoleInput();

	printf("Client disconnected!\n");
	return 0;
}