/* Program Description:	Concurrent server process that recieves arithmetic expressions from the client,
 * 			evaluates the expressions and sends the result back to the client.
 * 
 * Auhtors:		Asmit De (10/CSE/53)
 * 			Samik Some (10/CSE/93)
 * 
 * Date:		August 17, 2013
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
#define CONN_QUEUE_SIZE 5					// Set the number of incoming client connections to keep in queue


// Function to evaluate simple expressions in the form
// <result> = <result> <operator> <operand>
float calculate(float result, char operator, int operand)
{
	switch(operator)
	{
		case '+':
			result += operand;
			break;
		case '-':
			result -= operand;
			break;
		case '*':
			result *= operand;
			break;
		case '/':
			result /= operand;
			break;
	}
	
	return result;
}


int main()
{
	struct sockaddr_in	serv_addr,			// Stores the address of the server
				cli_addr;			// Stores the address of the client
				
	pid_t			pid;				// Stores process id of child servers
	
	int			sockfd_listen,			// Stores the socket file descriptor for the listening socket
				sockfd_accept,			// Stores the socket file descriptor for the accepting socket
				clilen,				// Stores the length of the client address
				i,				// Iterator for string manipulation
				operand;			// Stores the operand to be operated with the current result
				
	float			result;				// Stores the result of evaluated expressions
				
	char			buffer[BUFFER_SIZE],		// Buffer for message exchange between client and server
				operator,			// Stores the operator to be operated next
				token;				// Stores a character token from the expression in buffer
	
	
	// Open the socket for listening
	if((sockfd_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Error: Unable to create socket");
		exit(1);
	}
	
	
	// Set the server address
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(SERVER_PORT);
	
	
	// Bind the server process to the specified port with the server address information
	if(bind(sockfd_listen, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("Error: Unable to bind local address");
		close(sockfd_listen);
		exit(1);
	}
	
	
	// Signal handler for SIGINT
	void catch_SIGINT(int signum)
	{
		printf("\nServer has been stopped...\n");
		close(sockfd_listen);
		close(sockfd_accept);
		exit(0);
	}
	
	
	// Listen for incoming connections and place in queue, initialize signal handler
	listen(sockfd_listen, CONN_QUEUE_SIZE);
	signal(SIGINT, catch_SIGINT);
	printf("Server is up and running and ready to accept incoming connections...\n[Press <Ctrl-C> to stop server]\n");
	
	
	// Run the server continuously
	while(1)
	{
		// Accept incoming client connections
		clilen = sizeof(cli_addr);
		if((sockfd_accept = accept(sockfd_listen, (struct sockaddr *)&cli_addr, &clilen)) < 0)
		{
			perror("Error: Unable to accept incoming client connections");
			close(sockfd_listen);
			exit(1);
		}
		printf("\nClient [IP: %s] connected", inet_ntoa(cli_addr.sin_addr));
		fflush(stdout);
		
		// Fork a child server to handle communications with client
		pid = fork();
		if(pid == -1)
		{
			// Handle fork error
			perror("Error: unable to fork a child server for the current client request");
			
			for(i = 0; i < BUFFER_SIZE; i++) buffer[i] = '\0';
			strcpy(buffer, "Error: Unable to process request");
			send(sockfd_accept, buffer, strlen(buffer) + 1, 0);
			close(sockfd_accept);
			
			printf("\nClient [IP: %s] connection terminated\n", inet_ntoa(cli_addr.sin_addr));
		}
		else if(pid == 0) // Child server
		{
			// Communicate with the client
			while(1)
			{
				// Receive expression from client
				for(i = 0; i < BUFFER_SIZE; i++) buffer[i] = '\0';
				recv(sockfd_accept, buffer, BUFFER_SIZE, 0);
				
				// Check for stopping criterion
				if(!strcmp(buffer, "-1")) break;
				
				// Evaluate the arithmetic expression following left to right operator precedence
				result = 0;
				operator = '+';
				operand = 0;
				i = 0;
				while((token = buffer[i++]) != '\0')
				{
					if(token >= '0' && token <= '9')
					{
						operand = operand * 10 + token - '0';
					}
					else if(token != ' ')
					{
						result = calculate(result, operator, operand);
						operand = 0;
						operator = token;
					}
				}
				result = calculate(result, operator, operand);
				
				// Send result to client
				for(i = 0; i < BUFFER_SIZE; i++) buffer[i] = '\0';
				sprintf(buffer, "%f", result);
				send(sockfd_accept, buffer, strlen(buffer) + 1, 0);
			}
			
			// Close the accepting socket
			close(sockfd_accept);
			printf("\nClient [IP: %s] connection closed\n", inet_ntoa(cli_addr.sin_addr));
			
			return 0;
		}
	}
}