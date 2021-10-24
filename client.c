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

const char* readFromFile(const char* path);

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

void getServerResponse(){
	/*int fd;
	fd = open("myFifo2", O_RDONLY);
	char* getResponse = malloc(sizeof(char) * 100);

	read(fd, getResponse, 100);
	close(fd);

	printf("R: %s\n", getResponse);*/

	const char* serverResponse = readFromFile("myFifo");

	printf("%s\n", serverResponse);

}

const char* readFromFile(const char* path){
	int counter = 0;
	int char_num = 20;
	char aux = '.';
	char *buffer = malloc(sizeof(char)*(char_num + 1));
	int fd = open(path, O_RDONLY);
	while(aux != '\n' && aux != EOF){
		read(fd,&aux,1);
		//printf("%c_",aux);
		
		if(counter >= char_num){
			{
				//printf("Realocating buffer...\n");
				char aux_buffer[char_num];
				strcpy(aux_buffer, buffer);
				char_num *= 2;
				buffer = malloc(sizeof(char)*(char_num + 1));
				strcpy(buffer, aux_buffer);

			}
		}
		if(aux != '\n') {
			*(buffer+counter) = aux;
			counter++;
		}
	}
	close(fd);
	return buffer;
}


int main(){

	if( mkfifo("myFifo", 0666) == -1 ){ //creating a FIFO File

		if(access("myFifo", F_OK) == 0){ //checking if file is already created
			printf("FIFO file has been detected!\n");
		}
		else{ //mkfifo didn't work, handling error
			perror("Error at Fifo");
			exit(-1);
		}
		
	}

	sendingRequest = true;
	while(clientActive){
		if(sendingRequest){
			printf("Type command: ");
			sendConsoleInput();
			sendingRequest = false;
			receivingAnswer = true;
			printf("\n\n");
		}
		if(receivingAnswer){
			printf("Server feedback: ");
			getServerResponse();
			sendingRequest = true;
			receivingAnswer = false;
			printf("\n\n");
		}
	}
	sendConsoleInput();

	printf("Client disconnected!\n");
	return 0;
}