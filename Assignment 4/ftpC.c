/* Program Description:	FTP client process that connects to a FTP server process
 * 			over the network to transfer files to or from the server.
 * 
 * Authors:		Asmit De (10/CSE/53)
 * 			Samik Some (10/CSE/93)
 * 
 * Group#:		39
 * 
 * Date:		September 23, 2013
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP		"127.0.0.1"				// Replace with the IP address of the server
#define SERVER_PORT		55393					// Replace with the port number on which the server is running
#define BLOCK_SIZE		65535					// Set the size of the data block to be sent in each pass during file transfer
#define BLOCK_HEADER_SIZE	3					// Set the size of the header fields [1 + (lg(BLOCK_SIZE + 1)) / 8]
#define DATA_BUFFER_SIZE	65538					// Set the size of the data buffer for file transfer [BLOCK_HEADER_SIZE + BLOCK_SIZE]
#define CONTROL_BUFFER_SIZE	80					// Set the size of the control buffer to exchange control messages with server
#define PIPE_BUFFER_SIZE	80					// Set the size of the pipe buffer for communication between client's control & data processes


int main()
{
	struct sockaddr_in	serv_addr,				// Stores the address of the FTP server
				cli_addr;				// Stores the address of the FTP client
	
	int			sockfd_control,				// Stores the TCP socket file descriptor for the client's control process
				sockfd_data_listen,			// Stores the TCP listening socket file descriptor for the client's data process
				sockfd_data_accept,			// Stores the TCP accepting socket file descriptor for the client's data process
				filefd,					// Stores the file descriptor for the file transferred
				pipefd[2],				// Stores the pipe file descriptor for communication between client's control & data processes
				data_port,				// Stores the data port number to bind the data socket to
				dataBytes,				// Stores the number of bytes read from / written to file
				i;					// Iterator
				
	char			control_buffer[CONTROL_BUFFER_SIZE],	// Control buffer for control message exchange between client and server
				data_buffer[DATA_BUFFER_SIZE],		// Data buffer for file transfer between client and server
				pipe_buffer[PIPE_BUFFER_SIZE],		// Pipe buffer for communication between client's control & data processes
				*commandToken,				// Stores the parsed tokens from the command
				dataBlockSize[BLOCK_HEADER_SIZE],	// Stores the number of bytes in data block in string format
				dataBufferContent[DATA_BUFFER_SIZE],	// Contents of the data buffer containing data block and headers
				dataBlockHeader[BLOCK_HEADER_SIZE];	// Contents of the data block header
				
	pid_t			dataProcess;
	
	
	// Open the TCP socket for control  connection
	if((sockfd_control = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Error: Unable to create TCP socket");
		exit(1);
	}
	
	
	// Set the server address parameters
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	serv_addr.sin_port = htons(SERVER_PORT);
	
	
	// Establish a connection with the server process
	if(connect(sockfd_control, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("Error: Unable to connect to FTP server");
		close(sockfd_control);
		exit(1);
	}
	printf("\nConnection with FTP server [IP: %s:%d] established\n", SERVER_IP, SERVER_PORT);
	
	
	// Create the pipe for communication with the client's data process
	pipe(pipefd);
	
	
	// Fork the data process
	dataProcess = fork();
	if(dataProcess == -1) // Handle fork error
	{
		perror("Error: Unable to fork the data process");
		close(sockfd_control);
		exit(1);
	}
	else if(dataProcess == 0) // Code for the data process
	{
		// Close duplicate copy of the socket file descriptor of control process
		close(sockfd_control);
		
		// Open the TCP data socket for listening
		if((sockfd_data_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			perror("Error: Unable to create TCP data socket for listening");
			strcpy(pipe_buffer, "EXIT");
			write(pipefd[1], pipe_buffer, PIPE_BUFFER_SIZE);
			close(pipefd[0]);
			close(pipefd[1]);
			exit(1);
		}
		
		// Check if data port has been set
		for(i = 0; i < PIPE_BUFFER_SIZE; i++) pipe_buffer[i] = '\0';
		read(pipefd[0], pipe_buffer, PIPE_BUFFER_SIZE);
		commandToken = strtok(pipe_buffer, " ");
		
		// Close data process if first command sent is quit
		if(!strcmp(commandToken, "quit"))
		{
			close(sockfd_data_listen);
			close(pipefd[0]);
			close(pipefd[1]);
			exit(0);
		}
		else if(!strcmp(commandToken, "port"))
		{
			// Get data port number
			commandToken = strtok(NULL, " ");
			data_port = atoi(commandToken);
			
			// Set the client address parameters
			cli_addr.sin_family = AF_INET;
			cli_addr.sin_addr.s_addr = INADDR_ANY;
			cli_addr.sin_port = htons(data_port);
			
			// Bind the client's data process to the specified data port with the client address information for the TCP data socket
			if(bind(sockfd_data_listen, (struct sockaddr *)&cli_addr, sizeof(cli_addr)) < 0)
			{
				perror("Error: Unable to bind local address for TCP data socket");
				strcpy(pipe_buffer, "EXIT");
				write(pipefd[1], pipe_buffer, PIPE_BUFFER_SIZE);
				close(sockfd_data_listen);
				close(pipefd[0]);
				close(pipefd[1]);
				exit(1);
			}
			
			// Listen for incoming TCP data connections
			listen(sockfd_data_listen, 1);
		}
		
		// Loop until quit command is recieved from control process
		while(1)
		{
			// Read command from pipe sent by control process
			for(i = 0; i < PIPE_BUFFER_SIZE; i++) pipe_buffer[i] = '\0';
			read(pipefd[0], pipe_buffer, PIPE_BUFFER_SIZE);
			
			// Close data process if quit command is recieved
			if(!strcmp(pipe_buffer, "quit"))
			{
				close(sockfd_data_listen);
				close(pipefd[0]);
				close(pipefd[1]);
				exit(1);
			}
			
			// Accept incoming TCP data connections from FTP server
			servlen = sizeof(serv_addr);
			if((sockfd_data_accept = accept(sockfd_data_listen, (struct sockaddr *)&serv_addr, &servlen)) < 0)
			{
				perror("Error: Unable to accept TCP data connection from FTP server");
				close(sockfd_data_listen);
				continue;
			}
			
			// Check for get/put commands and perform appropriate actions
			commandToken = strtok(pipe_buffer, " ");
			if(!strcmp(commandToken, "get"))
			{
				
				// Open the file for writing
				commandToken = strtok(NULL, " ");
				if((filefd = open(commandToken, O_CREAT | O_WRONLY, 0777) < 0)
				{
					perror("Error: Unable to create file on client storage");
					close(sockfd_data_listen);
					close(sockfd_data_accept);
					continue;
				}
				
				// Read file received block by block
				while(1)
				{
					// Read data sent by server
					for(i = 0; i < DATA_BUFFER_SIZE; i++) data_buffer[i] = '\0';
					recv(sockfd_data_accept, data_buffer, DATA_BUFFER_SIZE, 0);
					
					// Get size of data block
					strncpy(dataBlockSize, data_buffer + 1, BLOCK_HEADER_SIZE - 1);
					dataBlockSize[BLOCK_HEADER_SIZE - 1] = '\0';
					dataBytes = 0;
					for(i = 0; i < BLOCK_HEADER_SIZE; i++)
					{
						dataBytes += (int)dataBlockSize[i] << (8 * i)
					}
					
					// Write data block to file
					write(filefd, data_buffer + BLOCK_HEADER_SIZE, dataBytes);
					
					// Stop reading if last block received
					if(data_buffer[0] == 'L') break;
				}
				
				// Close file
				close(filefd);
			}
			else if(!strcmp(commandToken, "put"))
			{
				// Open the file for reading
				commandToken = strtok(NULL, " ");
				if((filefd = open(commandToken, O_RDONLY, 0777) < 0)
				{
					perror("Error: Unable to read file on client storage");
					close(sockfd_data_listen);
					close(sockfd_data_accept);
					continue;
				}
				
				// Send file block by block
				while(1)
				{
					// Read data block from file
					dataBytes = read(filefd, data_buffer, BLOCK_SIZE, 0);
					
					// Create data block headers and fill the data buffer to be sent
					if(dataBytes < BLOCK_SIZE)
					{
						for(i = )
					}
					
					// Write data block to file
					strncpy(dataBlockSize, data_buffer + 1, BLOCK_HEADER_SIZE - 1);
					dataBlockSize[BLOCK_HEADER_SIZE - 1] = '\0';
					write(filefd, data_buffer + BLOCK_HEADER_SIZE, atoi(dataBlockSize));
					
					// Stop reading if last block received
					if(data_buffer[0] == 'L') break;
				}
				
				// Close file
				close(filefd);
			}
			
			// Close data connection if server's data process has closed the connection
			for(i = 0; i < PIPE_BUFFER_SIZE; i++) pipe_buffer[i] = '\0';
			read(pipefd[0], pipe_buffer, PIPE_BUFFER_SIZE);
			if(!strcmp(pipe_buffer, "250") || !strcmp(pipe_buffer, "550"))
			{
				close(sockfd_data_accept);
				close(sockfd_data_listen);
			}
		}
	}
	
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