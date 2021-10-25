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
#include <utmp.h>
#include <sys/time.h>
#include <time.h>

bool serverRunning = true;
bool userOnline = false;
//bool userDenied = false;

void stopServer();
void readFifo();
void resetParser(int fd);

const char* readFromFile();
const char* truncateSection();
const char* prepareForSending();
const char* getTime(struct utmp* log);
const char* getLoggedUsers();
const char* getLine(int fd);
const char* searchPidDetails(const char* path, const char* fields[], int n);
const char* preparePid(const char* input);
const char* makePath(const char* part1, const char* part2, const char* part3);


bool processClientJoin();
bool checkCommand();
bool checkFileExists(const char* path);
bool checkPidFormat(const char* pid);


bool waitForClientJoin(bool* serverRunning){

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
    	*serverRunning = strcmp(response, "Quitting!") != 0;
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
    	else if(checkCommand("quit", request)){
    		strcpy(output, "Quitting!");
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
	strcat(endResult, "}");
	return endResult;
}

bool checkCommand(const char* command, const char* input){
	int n = strlen(command);
	for(int i = 0; i < n; i++){
		if(input[i] != command[i]) return false;
	}
	return true;
}

void waitForCommands(bool* userOnline, bool* serverRunning){

	//processClientJoin(userName);
	int sockp[2];

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockp) < 0) { 
      perror("Socketpair"); 
      exit(1); 
    }

	int pid;
	if((pid = fork()) < 0){
    	perror("Fork failed");
    	exit(-1);
    }
    else if(pid > 0){ //parrent

    	close(sockp[0]);

    	const char* request = readFromFile("myFifo2");
		printf("Received from client: %s\n", request);

    	write(sockp[1], request, strlen(request)+1);

    	char* response = malloc(sizeof(char) * 1024);
    	read(sockp[1], response, 1024);
    	close(sockp[1]);
    	*userOnline = strcmp(response, "Client logged out!") != 0;
    	*serverRunning = strcmp(response, "Quitting!") != 0;
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
    }
    else{ //child
    	close(sockp[1]);
    	char* input = malloc(sizeof(char) * 200);
    	read(sockp[0],input,200);
    	char* output = malloc(sizeof(char) * 1024);

    	if(checkCommand("login : ", input)){
    		/*const char* userName = truncateSection(input, "login : ");
    		if(processClientJoin(userName)){
    			strcpy(output, "User name accepted!");
    		}
    		else{
    			strcpy(output, "User name invalid!");
    		}*/
    		strcpy(output, "Client already logged in...");
    	}
    	else if(checkCommand("get-logged-users", input)){
    		strcpy(output, getLoggedUsers());
    	}
    	else if(checkCommand("get-proc-info : ", input)){
    		const char* check[5] = {"Name:", "State:", "Ppid:", "Uid:", "Vmsize:"};
    		//strcpy(output, "get-proc-info : valid!");

    		const char* aux = preparePid(input);
    		printf("Prepared Pid: %s \n\n", aux);

    		if(checkPidFormat(aux)){

    			printf("Making path... \n\n");
    			const char* pidPath = makePath("/proc/", aux, "/status");
    			printf("Pid path: %s \n\n", pidPath);

    			if(checkFileExists(pidPath)){
    				strcpy(output, searchPidDetails(pidPath, check, 5));
    			}
    			else{
    				strcpy(output, "File does not exist!");
    			}

    		}
    		else{

    			strcpy(output, "Pid format invalid!");

    		}

    		//strcpy(output, searchPidDetails("/proc/21/status", check, 5));

    	}
    	else if(checkCommand("logout", input)){
    		strcpy(output, "Client logged out!");
    	}
    	else if(checkCommand("quit", input)){
    		strcpy(output, "Quitting!");
    	}
    	else{
    		strcpy(output, "Command invalid!");
    	}

    	write(sockp[0],output,strlen(output)+1);
    	close(sockp[0]);
    	exit(0);
    }
}

const char* getLoggedUsers(){
	char* result = malloc(sizeof(char) * 1024);

	result[0] = '\0';
	struct utmp* log;
	while((log = getutent()) != NULL){
		char* buffer = malloc(sizeof(char) * 100);
		strcpy(buffer, "Username: ");
		strcat(buffer, log->ut_user);
		strcat(buffer, ", Hostname for remote log in: ");
		strcat(buffer, log->ut_host);
		strcat(buffer, ", Time entry was made: ");
		strcat(buffer, getTime(log));

		strcat(buffer, ";\n");

		strcat(result, buffer);
	}

	return result;
}

const char* getTime(struct utmp* log){
	
	//Cod luat de pe Stackoverflow
	time_t nowtime;
	struct tm *nowtm;
	char* tmbuf = malloc(sizeof(char) * 64);
	
	nowtime = log->ut_tv.tv_sec;
	nowtm = localtime(&nowtime);
	strftime(tmbuf, 64, "%Y-%m-%d %H:%M:%S", nowtm);
	return(tmbuf);

	//Sursa cod: https://stackoverflow.com/questions/2408976/struct-timeval-to-printable-format

}

const char* searchPidDetails(const char* path, const char* fields[], int n){

	int fd;
	fd = open(path, O_RDONLY);
	if(fd == -1) exit(-1);

	int lines_searched = 0;
	bool found = false;
	char* result = malloc(sizeof(char) * 1024);
	*(result) = '\0';

	for(int i = 0; i < n; i++){
			char* result_buf = malloc(sizeof(char) * 128);
			found = false;
			lines_searched = 0;

			while(!found && lines_searched < 20){
				const char* buf_str = getLine(fd);
				found = checkCommand(fields[i], buf_str);
				if(found){
					strcpy(result_buf, buf_str);
				}
				lines_searched++;
			}

			if(found){
				strcat(result, result_buf);
			}
			else{
				strcat(result, fields[i]);
				strcat(result, " Not available");
			}

			strcat(result, "\n");
			resetParser(fd);

	}

	return result;
}

void resetParser(int fd){
	lseek(fd, 0, SEEK_SET);
}

const char* getLine(int fd){
	char* line = malloc(sizeof(char)*1024);
	*line = '\0';
	char parser;
	int len = 0;
	int r_stat;
	r_stat = read(fd, &parser, sizeof(char));
	while(parser != '\n' && r_stat != -1){
		*(line+len) = parser;
		len++;
		r_stat = read(fd, &parser, sizeof(char));
	}
	return line;
}

const char* makePath(const char* part1, const char* part2, const char* part3){
	char* result = malloc(sizeof(char)*1024);
	strcpy(result, part1);
	strcat(result, part2);
	strcat(result, part3);

	return result;
}

const char* preparePid(const char* input){
	char* result = malloc(sizeof(char)*1024);
	strcpy(result, truncateSection(input, "get-proc-info : "));
	return result;
}

bool checkPidFormat(const char* pid){
	int len = strlen(pid);
	char aux;
	for(int i = 0; i < len; i++){
		aux = pid[i];
		if(aux != '0' && aux != '1' && aux != '2' && aux != '3' && aux != '4' && aux != '5' && aux != '6' &&
			aux != '7' && aux != '8' && aux != '9') 
		{
			return false;
		}
	}
	return true;
}

bool checkFileExists(const char* path){
	if(access(path, F_OK) == 0) return true;
	return false;
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
			if(waitForClientJoin(&serverRunning)){
				printf("User is online\n");
				userOnline = true;
			}
			
		}
		else{
			waitForCommands(&userOnline, &serverRunning);
		}
		
	}

	printf("Server stoped!\n");

	return 0;
}