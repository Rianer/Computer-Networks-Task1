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
#include <sys/socket.h>


bool serverRunning = true;
bool userOnline = false;
//bool userDenied = false;

void stopServer();
void readFifo();

const char* readFromFile();
const char* truncateSection();
const char* prepareForSending();

bool processClientJoin();
bool checkCommand();



bool waitForClientJoin(){

	bool status = false;

	printf("Server online, waiting for client to join...\n\n");

	const char* request = readFromFile("myFifo2");
	printf("Received from client: %s\n", request);
	

	//processClientJoin(userName);
	int fd1[2], fd2[2];

	if (pipe(fd1)==-1)
    {
        fprintf(stderr, "Pipe Failed" );
        exit(-1);
    }
    if (pipe(fd2)==-1)
    {
        fprintf(stderr, "Pipe Failed" );
        exit(-1);
    }

	int pid, rv;
	if((pid = fork()) < 0){
    	perror("Fork failed");
    	exit(-1);
    }
    else if(pid > 0){ //parrent
    	close(fd1[0]);

    	const char* userName = truncateSection(request, "login : ");

    	write(fd1[1], userName, strlen(userName)+1);
    	close(fd1[1]);

    	wait(&rv);

    	close(fd2[1]);
    	char* response = malloc(sizeof(char) * 100);
    	read(fd2[0], response, 100);
    	close(fd2[0]);

    	status = strcmp(response, "User name accepted!") == 0;
    	//strcpy(response, prepareForSending(response));
    	//printf("%s\n", response);

    	int fd;
    	if( (fd = open("myFifo", O_WRONLY)) == -1 ){
			perror("Open FIFO");
			exit(-1);
		}
		else{
			printf("Fifo opened for writing...\n");
			strcpy(response, prepareForSending(response));
			write(fd, response, strlen(response) + 1);
			printf("Wrote: %s\n",response);
			//printf("Information sent: %s %ld \n", inputLine, strlen(inputLine));
		}

		close(fd);

		return status;
    }
    else{ //child
    	close(fd1[1]);
    	char* name = malloc(sizeof(char) * 100);
    	read(fd1[0],name,100);
    	close(fd1[0]);
    	char* output = malloc(sizeof(char) * 100);

    	if(checkCommand("login : ", request)){
    		if(processClientJoin(name)){
    			strcpy(output, "User name accepted!");
    		}
    		else{
    			strcpy(output, "User name invalid!");
    		}
    	}
    	else{
    		strcpy(output, "Command invalid!");
    	}

    	

    	close(fd2[0]);

    	write(fd2[1],output,strlen(output)+1);
    	close(fd2[1]);
    	exit(rv);
    }
}

bool processClientJoin(const char* userName){
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
		
		userOnline = true;
		if(userOnline) printf("The user has been found!\n\n");
		return true;
	} 
	if(userInvalid) {
		printf("The username is invalid!\n\n");
		userOnline = false;
		if(!userOnline) printf("The username is invalid!\n\n");
		return false;
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
	close(fd);
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

const char* prepareForSending(const char* string){
	int size_def = strlen(string);
	char* endResult = malloc(sizeof(char) * (size_def + 10));
	char* buffer = malloc(sizeof(char) * 10);
	sprintf(buffer, "%d", size_def);
	strcpy(endResult, buffer);
	strcat(endResult, " ");
	strcat(endResult, string);
	strcat(endResult, "\n");
	return endResult;
}

bool checkCommand(const char* command, const char* input){
	int n = strlen(command);
	for(int i = 0; i < n; i++){
		if(input[i] != command[i]) return false;
	}
	return true;
}

void waitForCommands(){


	
	

	//processClientJoin(userName);
	int fd1[2], fd2[2];

	if (pipe(fd1)==-1)
    {
        fprintf(stderr, "Pipe Failed" );
        exit(-1);
    }
    if (pipe(fd2)==-1)
    {
        fprintf(stderr, "Pipe Failed" );
        exit(-1);
    }

	int pid, rv;
	if((pid = fork()) < 0){
    	perror("Fork failed");
    	exit(-1);
    }
    else if(pid > 0){ //parrent

    	close(fd1[0]);

    	const char* request = readFromFile("myFifo2");
		printf("Received from client: %s\n", request);

    	write(fd1[1], request, strlen(request)+1);
    	close(fd1[1]);

    	wait(&rv);

    	close(fd2[1]);
    	char* response = malloc(sizeof(char) * 100);
    	read(fd2[0], response, 100);
    	close(fd2[0]);

    	status = strcmp(response, "User name accepted!") == 0;
    	//strcpy(response, prepareForSending(response));
    	//printf("%s\n", response);

    	int fd;
    	if( (fd = open("myFifo", O_WRONLY)) == -1 ){
			perror("Open FIFO");
			exit(-1);
		}
		else{

			printf("Fifo opened for writing...\n");
			strcpy(response, prepareForSending(response));
			write(fd, response, strlen(response) + 1);
			printf("Wrote: %s\n",response);
			//printf("Information sent: %s %ld \n", inputLine, strlen(inputLine));

		}

		close(fd);

		return status;
    }
    else{ //child
    	close(fd1[1]);
    	char* input = malloc(sizeof(char) * 100);
    	read(fd1[0],input,100);
    	close(fd1[0]);
    	char* output = malloc(sizeof(char) * 100);

    	if(checkCommand("login : ", request)){
    		if(processClientJoin(input)){
    			strcpy(output, "User name accepted!");
    		}
    		else{
    			strcpy(output, "User name invalid!");
    		}
    	}
    	else if(checkCommand("get-logged-users", request)){

    	}
    	else if(checkCommand("get-proc-info : ", request)){

    	}
    	else if(checkCommand("logout", request)){
    		
    	}
    	else if(checkCommand("quit", request)){
    		
    	}
    	else{
    		strcpy(output, "Command invalid!");
    	}

    	

    	close(fd2[0]);

    	write(fd2[1],output,strlen(output)+1);
    	close(fd2[1]);
    	exit(rv);
    }
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

	

	

	while(serverRunning){ //Phase 2: At least one client is online, serving the client

		if(!userOnline){ //Waiting for an input in the FIFO
			if(waitForClientJoin()){
				printf("User is online\n");
				userOnline = true;
			}
			
		}
		else{
			
		}
		
	}

	printf("Server stoped!\n");

	return 0;
}