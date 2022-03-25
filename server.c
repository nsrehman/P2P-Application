/* time_server.c - main */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>


/*------------------------------------------------------------------------
 * main - Iterative UDP server for TIME service
 *------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
	struct  sockaddr_in fsin;	/* the from address of a client	*/
	char	buf[100];		/* "input" buffer; any size > 0	*/
	char    *pts;
	int	sock;			/* server socket		*/
	time_t	now;			/* current time			*/
	int	alen;			/* from-address length		*/
	struct  sockaddr_in sin; /* an Internet endpoint address         */
	//struct pdu {char type; char data[100];};
	struct pdu {char type; char pName[10]; char cName[10]; char address[80];};
	int 	rSize = 20; 
	struct reg {struct pdu content; int usage;} r[20]; /* Local List of Registered Content */
	int     s, type;        /* socket descriptor and socket type    */
	int 	port=3000;
                                                                       

	switch(argc){
		case 1:
			break;
		case 2:
			port = atoi(argv[1]);
			break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
			exit(1);
	}

        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = htons(port);
                                                                                                 
    /* Allocate a socket */
        s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s < 0)
		fprintf(stderr, "can't creat socket\n");
                                                                                
    /* Bind the socket */
        if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		fprintf(stderr, "can't bind to %d port\n",port);
        listen(s, 5);	
	alen = sizeof(fsin);

    /* Initialize Local List of Registered Content */
    	int reg_init = 0;
      	for(reg_init = 0; reg_init < rSize; reg_init++){
		strcpy(r[reg_init].content.pName, "");
		strcpy(r[reg_init].content.cName, "");
		strcpy(r[reg_init].content.address, "");
		r[reg_init].usage = 0;
    	} 
    
    	struct pdu command, returnStr, ack;
    	int i, j;
    	int error=0;
	int index, usage;
    	while(1){
    		/* Receive request */
    		memset(command.address, 0 , 80);
		if (recvfrom(s, &command, 101, 0,
				(struct sockaddr *)&fsin, &alen) < 0)
			fprintf(stderr, "recvfrom error\n");
		
		switch (command.type){
	  		case 'R':		/* Register */
		  		printf("Uploading file\n");
		  		error = 0;
		  		for (i = 0; i < rSize; i++){
					if (!strcmp(r[i].content.cName, command.cName)){
		    				if (!strcmp(r[i].content.pName, command.pName)){
							error = 1;
		    				}
		    			}
		    		}
				if (!error){
		    			for (i = 0; i < rSize; i++){	    			
			    			if (r[i].content.pName[0] == '\0'){
			    				strcpy(r[i].content.pName, command.pName);
			    				strcpy(r[i].content.cName, command.cName);
			    				strcpy(r[i].content.address, command.address);
		    									
		    					ack.type = 'A';
		    					strcpy(ack.cName, command.cName);
		    					(void) sendto(s, &ack, 21, 0, (struct sockaddr 
		    					*)&fsin, sizeof(fsin));
			    				break;
			    			}
		    			}
		    		}
		    		/* If no space or peer name conflict send error */
		    		if (error || (i == rSize)){
	    				ack.type = 'E';
    					strcpy(ack.cName, command.cName);
    					(void) sendto(s, &ack, 21, 0, (struct sockaddr 
    					*)&fsin, sizeof(fsin));
		    		}
	  			break;
			case 'S':		/* Search for Content */
				usage = 9001;
		    		for (i = 0; i < rSize; i++){
		    			if (!strcmp(r[i].content.cName, command.cName)){
		    				if (r[i].usage < usage){
		    					usage = r[i].usage;
							index = i;
						}
		    			}
		    		}

		    		if (usage == 9001){
		    			ack.type = 'E';
		    			(void) sendto(s, &ack, 1, 0, (struct sockaddr *)&fsin, 
		    			sizeof(fsin));
		    			break;
		    		}
				r[index].content.type = 'S';
		    		r[index].usage++;
	  			(void) sendto(s, &r[index].content, strlen(r[index].content.address)
	  			+ 21, 0, (struct sockaddr *)&fsin, sizeof(fsin));
				break;
			case 'T':		/* Deregister */
				for (i = 0; i < rSize; i++){
					if (!strcmp(r[i].content.cName, command.cName)){
		    				if (!strcmp(r[i].content.pName, command.pName)){
		    					strcpy(r[i].content.pName, "");
		    					strcpy(r[i].content.cName, "");
		    					strcpy(r[i].content.address, "");
		    					r[i].usage = 0;
		    					
		    					ack.type = 'A';
		    					strcpy(ack.cName, command.cName);
		    					(void) sendto(s, &ack, 21, 0, (struct sockaddr 
		    					*)&fsin, sizeof(fsin));
	  						break;
		    				}
		    			}
		    		}
		    		
		    		if (i == rSize){
			    		ack.type = 'E';
					(void) sendto(s, &ack, 1, 0, (struct sockaddr *)&fsin, sizeof(fsin));
				}
				break;
			case 'O':		/* List Online Content */
				returnStr.type = 'O';
				strcpy(returnStr.address, "");
		    		for (i = 0; i < rSize; i++){
		    			if (strcmp(r[i].content.cName, "")){
		    				for (j = 0; j < i; j++){
			    				if (!strcmp(r[i].content.cName, r[j].content.cName)){
			    					break;	
							}
						}
						if (j==i){
						strcat(returnStr.address, r[i].content.cName);
			    			strcat(returnStr.address, "\n");
						}
					}
		    		}
	  			(void) sendto(s, &returnStr, strlen(returnStr.address)
	  			+ 21, 0, (struct sockaddr *)&fsin, sizeof(fsin));
				break;
		  	default:		
				continue;
		}
    	
    	}

}
