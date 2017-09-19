/* Program Description:	Client process that accepts arithmetic expressions from the user,
 * 			sends to the server and displays the result recieved from the server.
 * 
 * Authors:		Asmit De (10/CSE/53)
 * 			Samik Some (10/CSE/93)
 * 
 * Date:		August 17, 2013
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main()
{
	struct sockaddr_in serv_addr;
	int sockfd, i;
	char buffer[100];
	
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Unable to create socket");
		exit(1);
	}
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_addr.sin_port = htons(55393);
	
	if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("Unable to connect to server");
		exit(1);
	}
	
	while(1)
	{
		printf("\nEnter arithmetic expression or -1 to exit: ");
		for(i = 0; i < 100; i++) buffer[i] = '\0';
		fgets(buffer, 100, stdin);
		buffer[strlen(buffer) - 1] = '\0';
		fflush(stdin);
		
		send(sockfd, buffer, strlen(buffer) + 1, 0);
		
		if(!strcmp(buffer, "-1")) break;
		
		for(i = 0; i < 100; i++) buffer[i] = '\0';
		recv(sockfd, buffer, 100, 0);
		
		printf("Result: %s\n", buffer);
	}
	
	close(sockfd);
	
	return 0;
}