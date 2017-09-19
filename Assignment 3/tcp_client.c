/* Program Description:	TCP Client process that accepts a string from the user, sends to the server
 * 			and displays the result (palindrome or not) recieved from the server.
 * 
 * Authors:		Asmit De (10/CSE/53)
 * 			Samik Some (10/CSE/93)
 * 
 * Group#:		39
 * 
 * Date:		September 3, 2013
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE	100					// Set the size of the buffer for communication with the server
#define SERVER_IP	"127.0.0.1"				// Replace with the IP address of the server
#define SERVER_PORT	55393					// Replace with the port number on which the server is running


int main()
{
	struct sockaddr_in	serv_addr;			// Stores the address of the server
	
	int			sockfd,				// Stores the TCP socket file descriptor
				i;				// Iterator for string manipulation
				
	char			buffer[BUFFER_SIZE];		// Buffer for message exchange between client and server
	
	
	// Open the TCP socket
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Error: Unable to create TCP socket");
		exit(1);
	}
	
	
	// Set the server address
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	serv_addr.sin_port = htons(SERVER_PORT);
	
	
	// Establish a connection with the server process
	if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("Error: Unable to connect to server");
		close(sockfd);
		exit(1);
	}
	printf("\nConnection with server [IP: %s:%d] established\n", SERVER_IP, SERVER_PORT);
	
	
	// Input expression from user
	printf("\nEnter a string (less than 100 characters): ");
	for(i = 0; i < BUFFER_SIZE; i++) buffer[i] = '\0';
	fflush(stdin);
	fgets(buffer, BUFFER_SIZE, stdin);
	buffer[strlen(buffer) - 1] = '\0';
	
	
	// Send expression to server
	send(sockfd, buffer, strlen(buffer) + 1, 0);
	
	
	// Receive result from server
	for(i = 0; i < BUFFER_SIZE; i++) buffer[i] = '\0';
	recv(sockfd, buffer, BUFFER_SIZE, 0);
	
	
	// Display result on the standard output
	printf("%s\n", buffer);
	
	
	// Close the TCP socket
	close(sockfd);
	printf("\nConnection with server [IP: %s:%d] closed\n", SERVER_IP, SERVER_PORT);
	
	
	return 0;
}