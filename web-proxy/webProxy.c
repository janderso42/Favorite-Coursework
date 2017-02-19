#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <errno.h>

#define MAXBUFSIZE 1024
#define SMALLBUF 100
#define MAXCLIENTS 25

int request(int csock);
int error(int csock, char meth[SMALLBUF], char path[SMALLBUF], char prot[SMALLBUF], char host[SMALLBUF]);
int forward(int csock, char meth[SMALLBUF], char path[SMALLBUF], char prot[SMALLBUF], char host[SMALLBUF], char req[MAXBUFSIZE]);
int hostname_to_ip(char * hostname , char* ip); //www.umich.edu/~chemh215/W09HTML/SSG4/ssg6/html/Website/DREAMWEAVERPGS/first.html

int main (int argc, char * argv[])
{
  int exitSwitch=0;
	int sock, csock;                           //This will be our socket

	struct sockaddr_in sin, remote;     //"Internet socket address structure"
	unsigned int remote_length;         //length of the sockaddr_in structure

  int pid, status=0;

	if (argc != 2)
	{
		printf ("USAGE: <port>\n");
		//exit(1);
	}

	/******************
	  This code populates the sockaddr_in struct with
	  the information about our socket
	 ******************/
	bzero(&sin,sizeof(sin));                    //zero the struct
	sin.sin_family = AF_INET;                   //address family
	sin.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine

	//Causes the system to create a generic socket
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\nunable to create socket\n\n");
		exit(1);
	} else {
		printf("\nSocket succesfully created: %d\n\n", sock);
	}

	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) //need to bind socket to port number once made
	{
		perror("unable to bind socket\n\n");
		exit(1);
	} else {
		printf("Socket succesfully bound: \n\n");
	}

	if (listen(sock, MAXCLIENTS)<0){
		printf("\nUnable to listen, exitng\n");
		exit(1);
	}

    while(exitSwitch!=1) {
      //waits for an incoming message

      remote_length=sizeof(remote);
      csock=accept(sock, (struct sockaddr*) &remote, &remote_length);

      pid=fork();
      if (pid==0){
        status=request(csock);
        if(status!=0){
          printf("\nError with request\n");
        }
        close(csock);
        exit(0);
      }
      close(csock);
  	}
    close(sock);
  printf("Socket is closed\n\n");
  return 0;
}

int request(int csock){
  char buffer[MAXBUFSIZE], meth[SMALLBUF], path[SMALLBUF], prot[SMALLBUF], host[SMALLBUF];   //a buffer to store our received message
  int valid=0;
  bzero(buffer,sizeof(buffer));
  bzero(meth,SMALLBUF);
  bzero(path,SMALLBUF);
  bzero(prot,SMALLBUF);
  bzero(host,SMALLBUF);

  while((recv(csock, buffer, MAXBUFSIZE, 0)) > 0) { //first connection is auth
    sscanf(buffer, "%s %s %s %*s %s", meth, path, prot, host);
    valid=error(csock, meth, path, prot, host);
    if(valid!=0){
      //printf("\nError encountered\n");
      return -1;
    } else{
      valid=forward(csock, meth, path, prot, host, buffer);
      if(valid!=0){
        //printf("\nError encountered\n");
        return -1;
      }
    }
    bzero(buffer, MAXBUFSIZE);
  }
  return 0;
}

int error(int csock, char meth[SMALLBUF], char path[SMALLBUF], char prot[SMALLBUF], char host[SMALLBUF]){
  char resp[SMALLBUF];
  if((strncmp(meth, "GET", 3)!=0)){
    printf("\nError: 400 Bad Request - Invalid Method: %s", meth);
    strcpy(resp, prot);
    strcat(resp, " 400	Bad	Request: Invalid Method: ");
    strcat(resp, meth);
    strcat(resp, "\n");
    send(csock, resp, strlen(resp), 0);
    return -1;
  }
  else if((strncmp(prot, "HTTP/1.1", 8)!=0) && (strncmp(prot, "HTTP/1.0", 8)!=0)) {   //501 not implemented version
    printf("\nError: 501 HTTP Version Not Implemented: %s", prot);
    strcpy(resp, prot);
    strcat(resp, " 501 HTTP Version Not Implemented: ");
    strcat(resp, prot);
    strcat(resp, "\n");
    send(csock, resp, strlen(resp), 0);
    return -1;
  }
  printf("\nNo Errors, Accessing: %s", path);
  return 0;
}

int forward(int csock, char meth[SMALLBUF], char path[SMALLBUF], char prot[SMALLBUF], char host[SMALLBUF], char req[MAXBUFSIZE]){
  char hostAddr[100], resp[MAXBUFSIZE];
  int ssock;
  struct sockaddr_in serv;             //"Internet socket address structure"
	//unsigned int serv_length=sizeof(serv);

  hostname_to_ip(host, hostAddr);

  if (hostAddr==NULL){
    char resp1[SMALLBUF];
    printf("\nError: 404 Host Not Found: %s", host);
    strcpy(resp1, prot);
    strcat(resp1, " 404 Host Not Found: ");
    strcat(resp1, host);
    strcat(resp1, "\n");
    send(csock, resp1, strlen(resp1), 0);
    return -1;
  } else {
    if ((ssock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
  		printf("\nunable to create socket\n\n");
  		return -1;
  	}
    serv.sin_family = AF_INET;                 //address family
    serv.sin_port = htons(80);      //sets port to network byte order
    serv.sin_addr.s_addr = inet_addr(hostAddr); //sets remote IP address

  	if (connect(ssock, (struct sockaddr *)&serv, sizeof(serv)) < 0) //need to bind socket to port number once made
  	{
      char resp1[SMALLBUF];
  		perror("unable to connect to server \n\n");
      strcpy(resp1, prot);
      strcat(resp1, " 404 Host Not Found: ");
      strcat(resp1, host);
      strcat(resp1, "\n");
      send(csock, resp1, strlen(resp1), 0);
      close(ssock);
  		return -1;
  	}
    send(ssock, req, strlen(req), 0);
    int bytes=1;
    bzero(resp, MAXBUFSIZE);
    while(bytes>0){
      bzero(resp, MAXBUFSIZE);
      bytes=recv(ssock, resp, MAXBUFSIZE, 0);
      send(csock, resp, bytes, 0);
    }
    printf("\nAll data forwarded:   %s\n\n", path);
    close(ssock);
  }

  return 0;
}

int hostname_to_ip(char * hostname , char* ip)   //from http://www.binarytides.com/hostname-to-ip-address-c-sockets-linux/
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;

    he = gethostbyname( hostname );

    addr_list = (struct in_addr **) he->h_addr_list;

    for(i = 0; addr_list[i] != NULL; i++)
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }

    return 1;
}
