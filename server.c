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



bool serverRunning = false;
bool waitingClient = false;
bool respondingToClient = false;
bool allClientsLeft = false;
bool readyToProcessRequest = false;
bool userOnline = false;
bool userDenied = false;

void stopServer();
void readFifo();
const char* readFromFile();
const char* truncateSection();

void waitForClientJoin(){


	printf("Server online, waiting for client to join...\n\n");

	const char* request = readFromFile("myFifo2");
	printf("Received from client: %s\n", request);
	const char* userName = truncateSection(request, "login : ");
	printf("Searching for: %s\n\n", userName);
	int fd;
	if( (fd = open("users.config", O_RDONLY|O_CREAT, 0777)) == -1 ){
		perror("Open FIFO");
		exit(-1);
	}
	bool userFound = false;
	bool userInvalid = false;
	int status;
	char parser = '.';
	bool reachedEOF = false;
	while(!userFound && !reachedEOF){


		int buffer_size = 20, count = 0;
		char* buffer = malloc(sizeof(char)*(buffer_size + 1));
		parser = '.';
				while(parser != '\n' && parser != '}'){

					read(fd,&parser,1);
					if(count >= buffer_size){
						{
							//printf("Realocating buffer...\n");
							char aux_buffer[buffer_size];
							strcpy(aux_buffer, buffer);
							buffer_size *= 2;
							buffer = malloc(sizeof(char)*(buffer_size + 1));
							strcpy(buffer, aux_buffer);
						}
					}
		
					if(parser != '{' && parser != '}' && parser != '\n'){
						*(buffer+count) = parser;
						count++;
					}
				}

		if(parser == '}') reachedEOF = true;
		printf("Checking for: %s\n", buffer);
		status = strcmp(buffer, userName);
		if(status == 0) userFound = true; else printf("No match...\n\n");
		if(!userFound && reachedEOF) userInvalid = true;
	}

	if(userFound){
		printf("The user has been found!\n\n");
		userOnline = true;
		userDenied = false;
	} 
	if(userInvalid) {
		printf("The username is invalid!\n\n");
		userDenied = true;
		userOnline = false;
	}
	close(fd);
}

void stopServer(){
	printf("Stoping server...\n");
	//...
	serverRunning = false;
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
	return buffer;
}

const char* truncateSection(const char* string, const char* section){

	int buf_size = 20;
	int counter = 0;
	char* buffer = malloc(sizeof(char)*(buf_size + 1));

	for(int i=strlen(section); i < strlen(string); i++){
		if(counter >= buf_size){
			{
				//printf("Realocating buffer...\n");
				char aux_buffer[buf_size];
				strcpy(aux_buffer, buffer);
				buf_size *= 2;
				buffer = malloc(sizeof(char)*(buf_size + 1));
				strcpy(buffer, aux_buffer);

			}
		}
		*(buffer+counter) = string[i];
		counter++;
	}
	return buffer;
}

int main(){

	if( mkfifo("myFifo2", 0666) == -1 ){ //creating a FIFO File

		if(access("myFifo2", F_OK) == 0){ //checking if file is already created
			printf("FIFO file has been detected!\n");
		}
		else{ //mkfifo didn't work, handling error
			perror("Error at Fifo");
			exit(-1);
		}
		
	}

	//First Phase: Waiting for a client to join the server

	waitForClientJoin();



	while(serverRunning){ //Phase 2: At least one client is online, serving the client
		//checkConsoleInput(consoleInput);
		if(waitingClient){ //Waiting for an input in the FIFO

		}
		if(readyToProcessRequest){ //Processing the request

		}
		if(respondingToClient){ //Providing the client with the result

		}
		if(allClientsLeft){ //Waiting for another client or exit the server

		}
		
	}

	printf("Server stoped!\n");

	return 0;
}