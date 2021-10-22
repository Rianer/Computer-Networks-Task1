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

#define INPUT_LIMIT 60
#define DISCONNECT_CLIENT "disconnect"
#define WRITE_FIFO "send_message"

bool clientActive = true;

void disconnectClient();
void writeFifo();

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

	if(strcmp(input, DISCONNECT_CLIENT) == 0) {
		disconnectClient();
	}
	else if(strcmp(input, WRITE_FIFO) == 0){
		writeFifo(&fd_wr);
	}
}

void disconnectClient(){
	printf("Disconnecting client...\n");
	//...
	clientActive = false;
}

void writeFifo(int* fd_wr){
	if( (*fd_wr = open("myFifo2", O_WRONLY)) == -1 ){
		perror("Open");
		exit(-1);
	}
	else{
		printf("Fifo opened for writing...\n");
		write(*fd_wr, "Test text", 10);
	}
}

int main(){

	char userInput[60];
	
	while(clientActive){
		checkConsoleInput(userInput);
	}

	printf("Client disconnected!\n");
	return 0;
}