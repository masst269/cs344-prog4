/*
Becky Solomon
decrypt server
otp_enc_d.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg){
    perror(msg);
    exit(1);
}

int charToInt (char c){
	if (c == ' '){
		return 26;
	}
	else {
		return (c - 'A');
	}
	return 0;
}

char intToChar(int i){
	if (i == 26){
		return ' ';
	}
	else {
		return (i + 'A');
	}
}

void decode(char message[], char key[]){
	  int i;
	  char n;

	  for (i=0; message[i] != '\n' ; i++){
	  		//if (i == 0){
	  		//	printf("message[i] before decode: %s\n", message[i]);
	  		//}
	  		n = charToInt(message[i]) - charToInt(key[i]);
	  		if (n<0){
	  			n += 27;
	  		}
	  		message[i] = intToChar(n);
	  		//if (i == 0){
	  			//printf("message[i] after decode: %s\n", message[i]);
	  		//}
	  }
	  message[i] = '\0';
	  return;
}

int main(int argc, char *argv[])
{

     int sockfd, newsockfd, portno, optval, n, status;
     socklen_t clilen;
     char buffer[100000];
     char key[100000];
     struct sockaddr_in serv_addr, cli_addr; //socket addresses
     pid_t pid, sid; //process id 
     
     
     if (argc != 2) {
         fprintf(stderr,"usage %s <port>\n", argv[0]);
         exit(EXIT_FAILURE);
     }  
     
     sockfd = socket(AF_INET, SOCK_STREAM, 0); //create the socket
     if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
     }
     optval = 1;
     setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
        
     bzero((char *) &serv_addr, sizeof(serv_addr)); //clear out garbage from address
     portno = atoi(argv[1]); //get port number
     serv_addr.sin_family = AF_INET;	//fill in host address
     serv_addr.sin_addr.s_addr = INADDR_ANY; //any client can connect
     serv_addr.sin_port = htons(portno); //fill in port number
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr, 
              sizeof(serv_addr)) < 0){ //bind address to socket
              perror("Decrypt Server: ERROR on binding");
              exit(1);
     }
              
     listen(sockfd,5); //wait for client to call

	while(1){   
		//loop for handling up to 5 connections 
		  clilen = sizeof(cli_addr); 
		  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); //answer call
		  if (newsockfd < 0){ 
				 perror("ERROR on accept");
				 exit(1);
		  }
		  
		  //fork
		  pid = fork();
		  
		  if (pid < 0){
				perror("ERROR forking process");
				exit(1);
		  } 
		  
		  if (pid == 0){ 
		  //child process. child will handle connection
		  	 bzero(buffer, sizeof(buffer));
		  	 int bytes_remaining = sizeof(buffer);
		  	 int bytes_read = 0;
		  	 
		  	 char *keyStart;
		  	 int numNewLines = 0;
		  	 int i;
		  	 
		  	 //receive authentication message and reply
          read(newsockfd, buffer, sizeof(buffer)-1);
          if (strcmp(buffer, "dec_bs") != 0) {
                //perror("invalid client");
                //write error back to client
                char response[]  = "invalid";
                write(newsockfd, response, sizeof(response));
                _Exit(2);
          } 
          else {
                //write confirmation back to client
                char response[] = "dec_d_bs";
                write(newsockfd, response, sizeof(response));
          }
          bzero(buffer, sizeof(buffer));
          char *p = buffer; //keep track of where in buffer we are
          
			 while (1){ 

				 bytes_read = read(newsockfd, p, bytes_remaining);
	
				 if (bytes_read == 0){ // We're done reading 
				 		//printf("Bytes_read = %d, breaking...\n", bytes_read);
					  break;
				 }
				 if (bytes_read < 0) {
					  perror("Server: ERROR reading from socket");
					  exit(1);
				 }
				 for (i=0; i < bytes_read ; i++){ //search for newlines in buffer //create char* for start of key  
					 if(p[i] == '\n'){
						numNewLines++;
						 if (numNewLines == 1){  //first newline signals start of key
					 	 	keyStart = p+i+1;
							//printf("keystart = %d", keyStart - buffer);					 	 
					 	 }
					 }
				 }
				 
				 if (numNewLines == 2){	//second newline signals end of transmission
					  break;			 
				 }

				 p += bytes_read;
				 //printf("bytesread = %d\n", bytes_read);
				 bytes_remaining -= bytes_read;
	 		 }
	 		 char message[100000];
	 		 bzero(message, sizeof(message));
	 		 strncpy(message, buffer, keyStart-buffer); //separate message and key
	 		 decode(message, keyStart);	//decode message
	 		 
	 		 write(newsockfd, message, sizeof(message));
		  }
		  close(newsockfd);
		  
		  while (pid > 0){ 
		  //parent process. wait for children to finish
				pid = waitpid(-1, &status, WNOHANG);
		  }
	  }
	//end loop
     close(sockfd);
     
     
     return 0; 
}
