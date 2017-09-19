/* Program Description:	Iterative server process that recieves arithmetic expressions from the client,
 * 			evaluates the expressions and sends the result back to the client.
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
	struct sockaddr_in serv_addr, cli_addr;
	int sockfd, newsockfd, clilen, i, operand;
	float result;
	char buffer[100], operator, token;
	
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Unable to create socket");
		exit(1);
	}
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(55393);
	
	if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("Unable to bind local address");
		exit(1);
	}
	
	listen(sockfd, 5);
	
	while(1)
	{
		clilen = sizeof(cli_addr);
		if((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) < 0)
		{
			printf("Unable to accept incoming client connections");
			exit(1);
		}
		
		while(1)
		{
			for(i = 0; i < 100; i++) buffer[i] = '\0';
			recv(newsockfd, buffer, 100, 0);
			
			if(!strcmp(buffer, "-1")) break;
			
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
			
			for(i = 0; i < 100; i++) buffer[i] = '\0';
			sprintf(buffer, "%f", result);
			send(newsockfd, buffer, strlen(buffer) + 1, 0);
		}
		
		close(newsockfd);
	}
	
	return 0;
}