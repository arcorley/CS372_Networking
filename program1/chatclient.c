#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h>
int main(int argc, char *argv[])
{
	//declare all the variables required for setting up the connection
	int socketFD, portNumber, charsWritten = 0, charsRead, exitProg = 0;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char message[501];
	char handle[13];
	char finalMessage[525];
	memset(finalMessage, '\0', sizeof(finalMessage));

	//check the usage and arguments
	if (argc < 3)
	{
		fprintf(stderr, "USAGE: %s hostname port\n", argv[0]);
		exit(1);
	}

	//get a handle from the user
	printf("Enter a handle to use for this chat session (Max 10 characters): ");
	memset(handle, '\0', sizeof(handle)); //zero out the string to get rid of any noise
	fgets(handle, sizeof(handle) - 1, stdin); //place the user's input in the variable
	handle[strcspn(handle, "\n")] = '\0'; //remove the trailing newline added by fgets

	//set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); //clear out the address struct
	portNumber = atoi(argv[2]); //grab the port number from the args
	serverAddress.sin_family = AF_INET; //create the socket
	serverAddress.sin_port = htons(portNumber); //store the port number
	serverHostInfo = gethostbyname(argv[1]); //convert the machine name into a special form of address
	if (serverHostInfo == NULL)
	{
		fprintf(stderr, "ERROR: no such host\n");
		exit(0);
	}
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); //copy in the address

	//set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); //set up TCP socket
	if (socketFD < 0)
	{
		fprintf(stderr, "ERROR: Error opening socket\n");
		exit(1);
	}
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
	{
		fprintf(stderr, "ERROR: could not contact server process on port %s\n", argv[2]);
		exit(2);
	}

/*******************CHAT LOOP STARTS HERE*****************************/
	while (exitProg == 0)
	{
		memset(finalMessage, '\0', sizeof(finalMessage)); //blank out the final message
		//get a message from the user
		printf("%s> ", handle); //prompt the user
		memset(message, '\0', sizeof(message)); //zero out the message buffer
		fgets(message, sizeof(message)-1, stdin); //copy user input into message buffer
		message[strcspn(message, "\n")] = '\0'; //remove trailing newline added by fgets

		if (strcmp(message, "/exit") == 0) //if the message is exit, tell the chatserve process, then exit
		{
			exitProg = 1;
			charsWritten = send(socketFD, message, strlen(message), 0);
			break;
		}
		else //this block is for a normal message
		{
			strcat(finalMessage, handle); //add the handle to the string we're sending
			strcat(finalMessage,"> "); //add the separator
			strcat(finalMessage, message); //add the message from the user to the string
			//send message to server
			charsWritten = send(socketFD, finalMessage, strlen(finalMessage), 0); //write to server
			if (charsWritten < 0)
			{
				fprintf(stderr, "ERROR: Could not write to socket");
				if (charsWritten < strlen(finalMessage))
				{
					printf("WARNING: Not all data written to socket!\n");
				}
			}

			//get return message from server
			memset(finalMessage, '\0', sizeof(finalMessage)); //clear out the buffer again for reuse
			charsRead = recv(socketFD, finalMessage, sizeof(finalMessage) - 1, 0); //Read data from the socket, leaving \0 at the end
			if (charsRead < 0)
			{
				fprintf(stderr, "ERROR: couldn't read from socket");
				exit(1);
			}

			if (strcmp(finalMessage, "/exit") == 0) //if exit was received from server, close the connection and exit
			{
				exitProg = 1;
				printf("The other person has exited the chat\n"); //let the user know that the chat was cancelled
				break;
			}
			else
			{
				printf("%s\n", finalMessage); //if exit wasn't received, print out the message
			}
		}
	}

	close(socketFD); //close the socket connection
	return 0;
}