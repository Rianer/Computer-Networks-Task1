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

#define STOP_SERVER "stop_server"
#define INPUT_LIMIT 30
#define INITIATE_FIFO "start_fifo"

bool serverRunning = true;

void stopServer();
void readFifo();

const char* processInputLength(int maxChar){
	bool limitExceded = false;
	char c;
	char* inputStream = malloc((sizeof(char)) * (maxChar + 1));
	int counter = 0;
	while( (c=fgetc(stdin)) != '\n' && c != EOF){
		if(counter >= maxChar){
			limitExceded = true;
		}
		if(counter < maxChar){
			inputStream[counter] = c;
		}
		counter++;
	}
	if(limitExceded){

		printf("--Warning: Your input exceded the limit of %d characters, the command is being ignored!\n", maxChar);
		strcpy(inputStream,"");
	}
	return inputStream;
}

void checkConsoleInput(char* input){

	int fd_rd, fd_wr;

	printf("\nProvide input: ");
	//scanf("%s", input);
	//input = processInputLength(100);
	strcpy(input, processInputLength(INPUT_LIMIT));
	printf("Your input was: %s\n", input);

	if(strcmp(input, STOP_SERVER) == 0) {
		stopServer();
	}
	else if(strcmp(input, INITIATE_FIFO) == 0){
		readFifo(&fd_rd);
	}
}

void stopServer(){
	printf("Stoping server...\n");
	//...
	serverRunning = false;
}

void readFifo(int* fd_rd){
	if( (*fd_rd = open("myFifo2", O_RDONLY)) == -1 ){
		perror("Open error:");
		exit(-1);
	}
	else{
		printf("Fifo opened for reading...\n");
	}
}

int main(){

	if( mkfifo("myFifo2", 0666) == -1 ){ //creating a FIFO fyle

		if(access("myFifo2", F_OK) == 0){ //checking if file is already created
			printf("Warning: FIFO fyle has already been created!\n");
		}
		else{ //mkfifo didn't work, handling error
			perror("Error at Fifo");
			exit(-1);
		}
		
	}

	
	char * clientInput;
	char consoleInput[INPUT_LIMIT];

	while(serverRunning){
		checkConsoleInput(consoleInput);
		
	}

	printf("Server stoped!\n");

	return 0;
}