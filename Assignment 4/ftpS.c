/* Program Description:	Iterative server process that recieves a string from the client, evaluates whether
 * 			the string is palindrome or not and sends the result back to the client.
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
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE	100					// Set the size of the buffer for communication with clients
#define SERVER_PORT	55393					// Replace with the port number on which the server is to be run
#define CONN_QUEUE_SIZE 5					// Set the number of incoming TCP client connections to keep in queue


// Function to check for palindromes
int isPalindrome(char *str)
{
	int	length,						// Stores the length of the string
		isPal,						// Flag to check palindrome
		i;						// Iterator for string manipulation 
	
	length = strlen(str);
	isPal = 1;
	for(i = 0; i < length / 2; i++)
	{
		if(str[i] != str[length - 1 - i])
		{
			isPal = 0;
			break;
		}
	}
	
	return(isPal);
}


int main()
{
	struct sockaddr_in	serv_addr,			// Stores the address of the server
				cli_addr;			// Stores the address of the client
	
	int			sockfd_tcp,			// Stores the TCP socket file descriptor for listening
				sockfd_udp,			// Stores the UDP socket file descriptor
				sockfd_accept,			// Stores the socket file descriptor for the accepting socket (for TCP only)
				clilen,				// Stores the length of the client address
				i;				// Iterator for string manipulation
				
	fd_set			readfds;			// Set of file descriptors to read from
				
	char			buffer[BUFFER_SIZE];		// Buffer for message exchange between client and server
	
	
	// Open the TCP socket for listening
	if((sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Error: Unable to create TCP socket");
		exit(1);
	}
	
	
	// Open the UDP socket for listening
	if((sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("Error: Unable to create UDP socket");
		close(sockfd_tcp);
		exit(1);
	}
	
	
	// Set the server address
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(SERVER_PORT);
	
	
	// Bind the server process to the specified port with the server address information for the TCP socket
	if(bind(sockfd_tcp, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("Error: Unable to bind local address for TCP socket");
		close(sockfd_tcp);
		close(sockfd_udp);
		exit(1);
	}
	
	
	// Bind the server process to the specified port with the server address information for the UDP socket
	if(bind(sockfd_udp, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("Error: Unable to bind local address for UDP socket");
		close(sockfd_tcp);
		close(sockfd_udp);
		exit(1);
	}
	
	
	// Signal handler for SIGINT
	void catch_SIGINT(int signum)
	{
		printf("\nServer has been stopped...\n");
		close(sockfd_tcp);
		close(sockfd_udp);
		close(sockfd_accept);
		exit(0);
	}


	// Listen for incoming TCP connections and place in queue, initialize signal handler
	listen(sockfd_tcp, CONN_QUEUE_SIZE);
	signal(SIGINT, catch_SIGINT);
	printf("Server is up and running and ready to accept incoming connections...\n[Press <Ctrl-C> to stop server]\n");
	
	
	// Run the server continuously
	while(1)
	{
		// Initialize the file descriptor set for reading
		FD_ZERO(&readfds);
		FD_SET(sockfd_tcp, &readfds);
		FD_SET(sockfd_udp, &readfds);
		
		
		// Monitor file descriptor set for incoming connections
		select(sockfd_udp + 1, &readfds, NULL, NULL, NULL);
		
		
		// Check for available connections
		if(FD_ISSET(sockfd_tcp, &readfds))		// For TCP connections
		{
			// Accept incoming TCP client connections
			clilen = sizeof(cli_addr);
			if((sockfd_accept = accept(sockfd_tcp, (struct sockaddr *)&cli_addr, &clilen)) < 0)
			{
				perror("Error: Unable to accept TCP client connection");
				close(sockfd_tcp);
				continue;
			}
			printf("\nTCP Client [IP: %s] connected", inet_ntoa(cli_addr.sin_addr));
			fflush(stdout);
			
			// Receive string from client
			for(i = 0; i < BUFFER_SIZE; i++) buffer[i] = '\0';
			recv(sockfd_accept, buffer, BUFFER_SIZE, 0);
			
			// Check for palindrome
			if(isPalindrome(buffer))
			{
				for(i = 0; i < BUFFER_SIZE; i++) buffer[i] = '\0';
				strcpy(buffer, "The given string is a palindrome");
			}
			else
			{
				for(i = 0; i < BUFFER_SIZE; i++) buffer[i] = '\0';
				strcpy(buffer, "The given string is not a palindrome");
			}
			
			// Send result to client
			send(sockfd_accept, buffer, strlen(buffer) + 1, 0);
			printf("\nTCP Client [IP: %s] request processed", inet_ntoa(cli_addr.sin_addr));
			
			// Close the accepting socket
			close(sockfd_accept);
			printf("\nTCP Client [IP: %s] connection closed\n", inet_ntoa(cli_addr.sin_addr));
		}
		else if(FD_ISSET(sockfd_udp, &readfds))		// For UDP connections
		{
			// Receive string from client
			for(i = 0; i < BUFFER_SIZE; i++) buffer[i] = '\0';
			clilen = sizeof(cli_addr);
			if(recvfrom(sockfd_udp, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&cli_addr, &clilen) < 0)
			{
				perror("Error: Unable to recieve message from client");
				continue;
			}
			printf("\nUDP Client [IP: %s] request accepted", inet_ntoa(cli_addr.sin_addr));
			
			// Check for palindrome
			if(isPalindrome(buffer))
			{
				for(i = 0; i < BUFFER_SIZE; i++) buffer[i] = '\0';
				strcpy(buffer, "The given string is a palindrome");
			}
			else
			{
				for(i = 0; i < BUFFER_SIZE; i++) buffer[i] = '\0';
				strcpy(buffer, "The given string is not a palindrome");
			}
			
			// Send result to client
			if(sendto(sockfd_udp, buffer, strlen(buffer) + 1, 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr)) < 0)
			{
				perror("Error: Unable to send message to client");
				continue;
			}
			printf("\nUDP Client [IP: %s] request processed\n", inet_ntoa(cli_addr.sin_addr));
		}
	}
}