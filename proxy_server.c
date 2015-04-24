/*
 * CSCE 3530
 * April 23, 2015
 * Project 2 - Web Proxy
 * Danny Stieben, Aaron Tabor, Ahmed Alotaibi (Group 12)
 */

#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>	/* system type defintions */
#include <sys/socket.h>	/* network system functions */
#include <netinet/in.h>	/* protocol & struct definitions */
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BACKLOG	5
#define BUF_SIZE	1024000
#define LISTEN_PORT	60000

int threadCount = BACKLOG;
char theRes [BUF_SIZE];
void *client_handler(void *arg);
int hostname_to_ip(char *, char *);
int sock_send;

int main(int argc, char *argv[]){
    int status, *sock_tmp;
    pthread_t a_thread;
    void *thread_result;

    struct sockaddr_in addr_mine;
    struct sockaddr_in addr_remote;
    int sock_listen;
    int sock_aClient;
    int addr_size;
    int reuseaddr = 1;


    sock_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_listen < 0) {
        perror("socket() failed");
        exit(0);
    }

    memset(&addr_mine, 0, sizeof (addr_mine));
    addr_mine.sin_family = AF_INET;
    addr_mine.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_mine.sin_port = htons((unsigned short)LISTEN_PORT);

    status = bind(sock_listen, (struct sockaddr *) &addr_mine,
    	sizeof (addr_mine));
    if (status < 0) {
        perror("bind() failed");
        close(sock_listen);
        exit(1);
    }

    status = listen(sock_listen, 5);
    if (status < 0) {
        perror("listen() failed");
        close(sock_listen);
        exit(1);
    }

    addr_size = sizeof(struct sockaddr_in);
    printf("waiting for a client\n");
    while(1) {
    	if (threadCount < 1) {
    		sleep(1);
    	}

    	sock_aClient = accept(sock_listen, (struct sockaddr *) &addr_remote,
            &addr_size);
    	if (sock_aClient == -1){
    		close(sock_listen);
        	exit(1);
    	}

		printf("Got a connection from a client.\n");
    	sock_tmp = malloc(1);
    	*sock_tmp = sock_aClient;
    	//printf("thread count = %d\n", threadCount);
    	threadCount--;
 		status = pthread_create(&a_thread, NULL, client_handler,
            (void *) sock_tmp);
 		if (status != 0) {
 			perror("Thread creation failed");
 			close(sock_listen);
 			close(sock_aClient);
 			free(sock_tmp);
        	exit(1);
 		}
    }

    return 0;
}
//method used to send responses back to client
void sendRes(char *res, int sock_desc) {
	printf("sending: %s\n", res);
	fflush(stdout);
	send(sock_desc, res, strlen(res), 0);
}

/* this is used to listen to a server response so the user knows whether task was successful or not */
char* listenRes() {
	char res [BUF_SIZE];
	memset(res,0,sizeof(res));
	memset(theRes,0,sizeof(theRes));
	while (recv(sock_send, res, BUF_SIZE, 0) > 0) {
		strcat(theRes, res);
		memset(res, 0, sizeof(res));
	}
	return theRes;
}

//translates hostnames to IP addresses
int hostname_to_ip(char * hostname , char* ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
         
    if ( (he = gethostbyname( hostname ) ) == NULL) 
    {
        // get the host info
        herror("gethostbyname");
        return 1;
    }
 
    addr_list = (struct in_addr **) he->h_addr_list;
     
    for(i = 0; addr_list[i] != NULL; i++) 
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }
     
    return 1;
}

//replaces substrings of any size with other substrings of any size. Used in bad word filter
char * str_replace ( const char *string, const char *substr, const char *replacement ){
  char *tok = NULL;
  char *newstr = NULL;
  char *oldstr = NULL;
  /* if either substr or replacement is NULL, duplicate string a let caller handle it */
  if ( substr == NULL || replacement == NULL ) return strdup (string);
  newstr = strdup (string);
  while ( (tok = strstr ( newstr, substr ))){
    oldstr = newstr;
    newstr = malloc ( strlen ( oldstr ) - strlen ( substr ) + strlen ( replacement ) + 1 );
    /*failed to alloc mem, free old string and return NULL */
    if ( newstr == NULL ){
      free (oldstr);
      return NULL;
    }
    memcpy ( newstr, oldstr, tok - oldstr );
    memcpy ( newstr + (tok - oldstr), replacement, strlen ( replacement ) );
    memcpy ( newstr + (tok - oldstr) + strlen( replacement ), tok + strlen ( substr ), strlen ( oldstr ) - strlen ( substr ) - ( tok - oldstr ) );
    memset ( newstr + strlen ( oldstr ) - strlen ( substr ) + strlen ( replacement ) , 0, 1 );
    free (oldstr);
  }
  return newstr;
}

//method that talks with web server
char* web_handler(char* ip, char* buf) {
    struct sockaddr_in	addr_send;
    int	i;
    //char text[80],buf[BUF_SIZE];
    int	send_len,bytes_sent;

	/* create socket for sending data */
    sock_send=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_send < 0) {
        printf("socket() failed\n");
        exit(0);
    }

	/* create socket address structure to connect to */
    memset(&addr_send, 0, sizeof (addr_send)); /* zero out structure */
    addr_send.sin_family = AF_INET; /* address family */
    addr_send.sin_addr.s_addr = inet_addr(ip);
    addr_send.sin_port = htons((unsigned short)80);

	/* connect to the server */
    i=connect(sock_send, (struct sockaddr *) &addr_send, sizeof (addr_send));
    if (i < 0) {
        printf("connect() failed\n");
	close(sock_send);
        exit(0);
    }
	//send our request
	send_len=strlen(buf);
	bytes_sent=send(sock_send,buf,send_len,0);
	//capture response
	char* res = listenRes();
	close(sock_send);
	
	return res;
}

//method that talks with the browser
void *client_handler(void *sock_desc) {
	int msg_size;
	char buf[BUF_SIZE], request[BUF_SIZE];
	int sock = *(int*)sock_desc;
	
	while ((msg_size = recv(sock, buf, BUF_SIZE, 0)) > 0) {
        buf[msg_size] = 0;
		
		//figure out what we want to get
		char tokenize[msg_size+1];
		strcpy(tokenize, buf);
		
		char * token = strtok(tokenize, " ");
		token = strtok(NULL, " ");
		//complete address
		char * pch = token + 1;
		char address[200];
		if(pch)
		{
			strcpy(address, pch);
		}
		
		//document path
		char * pch2 = strchr(pch, '/');
		char document[200];
		if(pch2)
		{
			strcpy(document, pch2);
		}
		else
		{
			strcpy(document, "/");
		}
		
		//host or "domain"
		char host[200], ip[100];
		int diff;
		if(pch && pch2)
		{
			diff = pch2 - pch;
		}
		else
		{
			diff = strlen(address);
		}
			
		strncpy(host, address, diff);
		
		//check if site is blocked
		FILE * blacklist;
		char blBuffer[100];
		blacklist = fopen("blacklist.txt", "r");
		int blocked = 0;
		while (fgets(blBuffer, 100, blacklist) != NULL )
		{
			if(strncmp(host, blBuffer, diff) == 0)
			{
				blocked = 1;
				break;
			}
		}
		fclose(blacklist);
		
		if(blocked == 1)
		{
			//this is blocked
			//this whole block of code will read "blocked_response.txt" and send the contents to the browser
			FILE * isBlocked;
			isBlocked = fopen("blocked_response.txt", "r");
			char blockedSend[BUF_SIZE];
			char blockedBuffer[10000];
			while(fgets(blockedBuffer, 10000, isBlocked) != NULL)
			{
				strcat(blockedSend, blockedBuffer);
				memset(blockedBuffer,0,strlen(blockedBuffer));
			
			}
			strcat(blockedSend, "\r\n\r\n");
			sendRes(blockedSend, sock);
			memset(blockedSend,0,strlen(blockedSend));
			
			printf("%s", "This is blocked!");
			fflush(stdout);
			fclose(isBlocked);
		}
		else
		{
			//CHECK FOR CACHE FILE
			FILE * fp;
			fp = fopen(host, "r");
			if(!fp)
			{
				//make the file
				fp = fopen(host, "w");
				//build our request
				strncpy(request, "GET ", 4);
				strcat(request, document);
				strcat(request, " HTTP/1.1\r\nHost: ");
				strncat(request, host, diff);
				strcat(request, "\r\nUser-Agent: WebProxy/0.99");
				strcat(request, "\r\n\r\n");
				host[diff] = '\0';
				hostname_to_ip(host, ip);
		
				printf("%s\n", request);
				
				//send our request
				char * returned = web_handler(ip, request);
				//filter for bad words
				FILE * badwords;
				badwords = fopen("badwords.txt", "r");
				char badBuffer[100];
				while(fgets(badBuffer, 100, badwords) != NULL)
				{
					int length = strlen(badBuffer);
					badBuffer[length-1] = '\0';
					returned = str_replace(returned, badBuffer, "***REMOVED***");
				}
				sendRes(returned, sock);
				//print final version to cache file
				fprintf(fp, "%s", returned);
			}
			else
			{
				//read the file
				char toSend[BUF_SIZE];
				char fileBuffer[10000];
				while(fgets(fileBuffer, 10000, fp) != NULL)
				{
					strcat(toSend, fileBuffer);
					memset(fileBuffer,0,strlen(fileBuffer));
				
				}
				//send cached content to browser
				sendRes(toSend, sock);
				memset(toSend,0,strlen(toSend));
			}
		
			fclose(fp);
		}
		
		memset(buf,0,strlen(buf));
		//memset(address,0,strlen(address));
		memset(document,0,strlen(document));
		memset(ip,0,strlen(ip));
    }

    close(sock);
	free(sock_desc);
	threadCount++;
}
