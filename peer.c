#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>                                                                            
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/select.h>                            
                                                                                
#include <netdb.h>

#define	BUFSIZE 64
#define	BUFLEN 1024
#define	MSG		"Any Message \n"


int
main(int argc, char **argv)
{
	char	*host = "localhost";
	int	port = 3000;
	char	now[100];		/* 32-bit integer to hold time	*/ 
	struct hostent	*phe;	/* pointer to host information entry	*/
	struct hostent	*hp;
	struct sockaddr_in sin;	/* an Internet endpoint address		*/
	struct sockaddr_in client;
	struct sockaddr_in server;
	struct sockaddr_in serverReq;	
	struct pdu {char type; char data[BUFLEN];};
	struct cpdu {char type; char pName[10]; char cName[10]; char address[80];};
	int	s, n, type;	/* socket descriptor and socket type	*/
	int 	sd, sock, new_sd, alen, client_len;
	char regList[20][10];
	
	switch (argc) {
	case 1:
		break;
	case 2:
		host = argv[1];
		break;
	case 3:
		host = argv[1];
		port = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "usage: UDPtime [host [port]]\n");
		exit(1);
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;                                                                
	sin.sin_port = htons(port);
                                                                                        
   	/* Map host name to IP address, allowing for dotted decimal */
        if ( phe = gethostbyname(host) ){
                memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
        }
        else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
		fprintf(stderr, "Can't get host entry \n");
                                                                                
    	/* Allocate a socket */
        s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s < 0)
		fprintf(stderr, "Can't create socket \n");
	
                                                                                
    	/* Connect the socket */
        if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		fprintf(stderr, "Can't connect to %s %s \n", host, "Time");


	

	/* TCP socket */
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}

	/* Bind an address to the socket	*/
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(0);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1);
	}

	/* queue up to 5 connect requests  */
	listen(sd, 5);

	(void) write(s, MSG, strlen(MSG));
	alen = sizeof(struct sockaddr_in);
	getsockname(sd, (struct sockaddr *) &server, &alen);
	
	printf("%d\n", server.sin_port);
	
	fd_set rfds, afds;


	int init;
	for (init = 0; init < sizeof(regList)/sizeof(regList[0]); init++)
		strcpy(regList[init], "");
		
	/* User interface */
	
	struct pdu cFileName, cFileData;
	struct pdu sFileName, sFileData;
	struct stat sb;
	struct cpdu command, searchResult, listResult;
	char input[10];
	char choice;
	char tmp[5];
	char* recvPort; 
	char* recvAddress;
	int retval, i, r;
	char pName[10];
	
	off_t fileSize;
	FILE *fptr;
	fprintf(stderr, "Choose a user name: ");
	scanf("%10s", pName);
	strcpy(command.pName, pName);
	while(1){
		FD_ZERO(&afds);
		FD_SET(sd, &afds);
		FD_SET(0, &afds);
		memcpy(&rfds, &afds, sizeof(rfds));
		fprintf(stderr, "Command: ");
		retval = select(FD_SETSIZE, &rfds, NULL, NULL, NULL);

		if(FD_ISSET(0, &rfds)){	
			scanf("%s", input);	
			choice = input[0];
			switch (choice) {
				case '?': { /* Help */
				 	printf("R-Content Registration; T-Content Deregistration;\n"
				 	       "L-List Local Content; D-Download Content;\n"
				 	       "O-List all the On-Line Content; Q-Quit;\n");
				 	       break;
				}
				case 'R': { /* Register */
					command.type = 'R';
					fprintf(stderr, "Enter Content Name for Registration:  ");
					strcpy(command.pName, pName);
					scanf("%10s", command.cName);
					sprintf(tmp, "%d", server.sin_port);
					memset(command.address, 0 , 80);
					strcpy(command.address, "192.168.2.236\n");
					strcat(command.address, tmp);
					write(s, &command, strlen(command.address)+21);
					
					recv(s, &command, 101, 0);
					if (command.type == 'E') {
						fprintf(stderr, "Content not registered\n");
						break;
					}
					
					/*  Add content to local register  */
					for (i = 0; i < sizeof(regList)/sizeof(regList[0]); i++){
						if (regList[i][0] == '\0'){
							strcpy(regList[i], command.cName);
							break;
						}
					}
					
					printf("Register: %s Added to Server\n", command.cName);
					
					
					break;
				}
				case 'T': { /* Deregister */
					command.type = 'T';
					fprintf(stderr, "Enter Content Name for Deregistration: ");
					strcpy(command.pName, pName);
					scanf("%10s", command.cName);
					write(s, &command, strlen(command.address)+21);
					
					recv(s, &command, 101, 0);
					if (command.type == 'E') {
						fprintf(stderr, "Content not deregistered\n");
						break;
					}
					
					/*  Remove content to local register  */
					for (i = 0; i < sizeof(regList)/sizeof(regList[0]); i++){
						if (!strcmp(regList[i], command.cName)){
							strcpy(regList[i], "");
							break;
						}
					}
					
					printf("Deregister: %s Removed from Server\n", command.cName);
					
					break;
				}
				case 'L': { /* List Local Registered Content */
					printf("Local Registered Content:\n");
					for (i = 0; i < sizeof(regList)/sizeof(regList[0]); i++){
			    			if (regList[i][0] != '\0'){
			    				printf("\t%s\n", regList[i]);
						}
		    			}
					break;
				}
				case 'D': { /* Download */
					command.type = 'S';
					fprintf(stderr, "Enter Content to Download: ");
					scanf("%10s", command.cName);
					write(s, &command, 21);
					
					memset(searchResult.address, 0 , 80);
					recv(s, &searchResult, 101, 0);
					
					/*  Error Handling  */	
					if (searchResult.type == 'E') {
						fprintf(stderr, "Content not registered\n");
						break;
					}
					
					printf("Search: Found '%s' from '%s'\n", searchResult.cName,
					searchResult.pName);
					
					char* token = strtok(searchResult.address,"\n");
					recvAddress = strdup(token);
					token = strtok(NULL,"\n");
					recvPort = strdup(token);

					/* Create a stream socket	*/	
					if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
						fprintf(stderr, "Can't creat a socket\n");
						exit(1);
					}
					
					/* Bind an address to the socket	*/
					bzero((char *)&serverReq, sizeof(struct sockaddr_in));
					serverReq.sin_family = AF_INET;
					serverReq.sin_port = atoi(recvPort);
					if (hp = gethostbyname(host)) 
					  bcopy(hp->h_addr, (char *)&serverReq.sin_addr, hp->h_length);
					else if ( inet_aton(host, (struct in_addr *) &serverReq.sin_addr) ){
					fprintf(stderr, "Can't get server's address\n");
					  exit(1);
					}
					
					/* Connecting to the server */
					if (connect(sock, (struct sockaddr *)&serverReq, sizeof(serverReq)) == -1){
					fprintf(stderr, "Can't connect \n");
					  exit(1);
					}
					
					printf("Download: Connected to IP %s on Port %d\n", recvAddress, atoi(recvPort));
					/*  Request File Data  */
					cFileName.type = 'D';
					strcpy(cFileName.data, command.cName);
					write(sock, &cFileName, strlen(cFileName.data)+1);
					
					fptr = fopen(cFileName.data, "w");
					/*  Read File Data  */
					memset(cFileData.data, 0, BUFLEN);
					while (n = read(sock, &cFileData, BUFLEN+1) > 0) {
						if (cFileData.type == 'C'){		
					  		fprintf(fptr, "%s", cFileData.data);
					  	}
						else if (cFileData.type == 'E') {
							fprintf(stderr, "File Doesn't Exist\n");
							remove(cFileName.data);
							break;
						}	
					}
					
					close(sock);
					fclose(fptr);
					
					/*  Register File  */
					if (cFileData.type == 'C') {
						command.type = 'R';
						sprintf(tmp, "%d", server.sin_port);
						memset(command.address, 0 , 80);
						strcpy(command.address, "192.168.2.236\n");
						strcat(command.address, tmp);
						write(s, &command, strlen(command.address) + 21);
						recv(s, &command, 101, 0);
						
						if (command.type == 'E') {
							fprintf(stderr, "Content not registered\n");
							break;
						}
						
						printf("Register: %s Added to Server\n", command.cName);

					}
					
					/*  Add content to local register  */
					for (i = 0; i < sizeof(regList)/sizeof(regList[0]); i++){
						if (regList[i][0] == '\0'){
							strcpy(regList[i], command.cName);
							break;
						}
					}
					

					break;
				}
				case 'O': { /* List Online Registered Content */
					printf("Online Registered Content:\n");
					memset(listResult.address, 0, 80);
					command.type = 'O';
					write(s, &command, 101);
					
					recv(s, &listResult, 101, 0);
					printf("%s\n", listResult.address);			
					break;
				}
				case 'Q': { /* Deregister and Quit */
					for (i = 0; i < sizeof(regList)/sizeof(regList[0]); i++){
			    			if (regList[i][0] != '\0'){
			    				command.type = 'T';
			    				strcpy(command.pName, pName);
			    				strcpy(command.cName, regList[i]);
			    				write(s, &command, strlen(command.address)+21);
							
							recv(s, &command, 101, 0);
						
							if (command.type == 'E') {
								fprintf(stderr, "Content not deregistered\n");
								break;
							}
							
							printf("Deregister: %s Removed from Server\n", command.cName);
						}
		    			}
					
					exit(0);
				}

				default:
					break;
			}	
			
		}
		if(FD_ISSET(sd, &rfds)){
			client_len = sizeof(client);
	  		new_sd = accept(sd, (struct sockaddr *)&client, &client_len);
	  		sFileData.type = 'C';  		
			
			memset(sFileName.data, 0, BUFLEN);
								
			recv(new_sd, &sFileName, BUFLEN+1, 0);
			if (sFileName.type == 'D'){
			
				memcpy(sFileData.data, sFileData.data, strlen(sFileData.data)-1);
				
				fptr = fopen(sFileName.data, "r");
				if (fptr != NULL) {
					
					stat(sFileName.data, &sb);
					
					for(fileSize = sb.st_size; fileSize > 0; fileSize -= BUFLEN-1) {
						r = fread(sFileData.data, 1, BUFLEN-1, fptr);
						sFileData.data[r] = '\0';
						write(new_sd, &sFileData, strlen(sFileData.data)+2);
					}
					
					fclose(fptr);
					close(new_sd);
										
					printf("\nFile Transfered\n");
				}
				else{
					printf("No file with that name\n");
					sFileData.type = 'E';
					write(new_sd, &sFileData, strlen(sFileData.data)+1);
				}
			}
		}
	}
	exit(0);
}

