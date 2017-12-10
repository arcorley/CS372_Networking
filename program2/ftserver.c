//NOTE: The general structure for this code was taken from program 4 in CS344 with Prof Brewster
//		It was refitted to meet the requirements of this program

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <dirent.h>
#include <math.h>

void error(const char *msg) { fprintf(stderr, msg); exit(1); } // Error function used for reporting issues

int getArrayLength(char arr[][256], int numFiles)
{
	int i, totalLen = 0, len = 0;

	for (i = 0; i < numFiles; i++)
	{
		len = strlen(arr[i]);
		totalLen += len;
	}

	return totalLen;
}

int main(int argc, char *argv[])
{
	int listenSocketFD, portNumber;
	char* filename;
	
	struct sockaddr_in serverAddress;

	if (argc < 2) //check that valid inputs were provided to the program
	{
		fprintf(stderr, "USAGE: %s port\n", argv[0]);
		exit(1);
	}

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("Error: could not create socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("Error: could not bind to socket");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	printf("Server open on port %d\n", portNumber);

	while(1)
	{
		int establishedConnectionFD =-1;
		struct sockaddr_in clientAddress;
		struct hostent* clientInfo;
		socklen_t sizeOfClientInfo;
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		memset((char *)&clientAddress, '\0', sizeof(clientAddress)); //clear out the address struct	
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) error("Error: could not accept connection");

		socklen_t sizeOfAddress = sizeof(struct in_addr);
		clientInfo = gethostbyaddr(&clientAddress.sin_addr, sizeOfAddress, AF_INET);
		char* clientHost;
		sprintf(clientHost, "%s", clientInfo->h_name);

		printf("Connection from %s\n", clientHost); //display on the screen that a connection was received

		//get the operation requested
		char buffer[65000];
		char recvdBuffer[150000];
		memset(recvdBuffer, '\0', sizeof(recvdBuffer)); //clear out recvdBuffer
		int charsRead = 0, charsWritten = 0, dataPort;;
		char operation = 'a';

		while(strstr(recvdBuffer, "@@") == NULL)
		{
			memset(buffer, '\0', sizeof(buffer)); //clear out the buffer
			charsRead = recv(establishedConnectionFD, buffer, sizeof(buffer) - 1, 0); //read the requested operation from the client
			if (charsRead < 0)
			{
				fprintf(stderr, "ERROR: Could not read from socket\n");
			}

			strcat(recvdBuffer, buffer); //concatenate what we received to the final buffer
		}

		close(establishedConnectionFD);

		if (recvdBuffer[0] == 'l') //processing for l command
		{
			char* token;
			char* dataPortString = NULL;
			token = strtok(recvdBuffer, "\n"); //this puts the command into token
			token = strtok(NULL, "\n"); //go to the next argument
			dataPortString = strdup(token);
			dataPort = atoi(dataPortString);
			operation = 'l';
			printf("List directory requested on port %d.\n", dataPort);
		}
		else //processing for g command
		{
			char* token;
			char* dataPortString = NULL;
			int argument = 1;
			filename = NULL;
			token = strtok(recvdBuffer, "\n"); //this puts the command into the token

			while (token != NULL) //iterate through received string until terminator is met
			{
				if (argument == 1) //place the first argument in filename var
				{
					token = strtok(NULL, "\n");
					filename = strdup(token);
				}
				else if (argument == 2) //place second argument in dataport var
				{
					dataPortString = strdup(token);
					dataPort = atoi(dataPortString);
					break;
				}

				token = strtok(NULL, "\n");
				argument++;
			}

			operation = 'g';
			printf("File \"%s\" requested on port %d.\n", filename, dataPort);
		}

		//connect to the listening data conenction from the client process
		int clientSocketFD, listenPortNumber;
		struct sockaddr_in listenServerAddress;
		struct hostent* listenHostInfo;
		char* listenHostName = strdup(clientHost);
		listenPortNumber = dataPort;
		listenServerAddress.sin_family = AF_INET; //create the socket
		listenServerAddress.sin_port = htons(listenPortNumber);
		listenHostInfo = gethostbyname(listenHostName);

		if (listenHostInfo == NULL)
		{
			fprintf(stderr, "ERROR: Couldn't find listening host with that name\n");
			exit(0);
		}
		memcpy((char*)&listenServerAddress.sin_addr.s_addr, (char*)listenHostInfo->h_addr, listenHostInfo->h_length); //copy in the address

		//set up the socket
		clientSocketFD = socket(AF_INET, SOCK_STREAM, 0);
		if (clientSocketFD < 0)
		{
			fprintf(stderr, "ERROR: Couldn't open listening socket\n");
			exit(1);
		}

		//connect to server
		if (connect(clientSocketFD, (struct sockaddr*)&listenServerAddress, sizeof(listenServerAddress)) < 0) //connect socket to address
		{
			fprintf(stderr, "ERROR: could not contact server process on port %d\n", dataPort);
			exit(2);
		}

		//first get .txt files in the directory
		char filenames[256][256];
		DIR *d;
		struct dirent *dir;
		int fileCount = 0;
		d = opendir("."); //open the directory the process is running in
		if (d)
		{
			while ((dir = readdir(d)) != NULL) //go through each file in the directory 
			{
				if (strstr(dir->d_name, ".txt") != NULL) //if the filename contains .txt
				{
					strcpy(filenames[fileCount], dir->d_name); //copy the filename into the array
					fileCount++; //increment the filecount
				}	
			}

			closedir(d);
		}


		//send back the requested data
		if (operation == 'l')
		{
			int i = 0, totalLength, lengthDigits;
			//get everything ready to send
			totalLength = getArrayLength(filenames, fileCount); //get the length of the filenames

			if (totalLength == 0) 
			{ 
				lengthDigits = 0; 
			} //get the number of digits in total length
			else 
			{ 
				lengthDigits = floor(log10(abs(totalLength))) + 1; 
			}
			
			totalLength += (lengthDigits + 1 + fileCount); //account for newlines and lengthDigits appended to front of string

			//prepare the final string we'll be sending
			memset(buffer, '\0', sizeof(buffer));
			sprintf(buffer, "%d\n", totalLength); //throw the total length and a newline at the beginning so the receiving process knows how much to receive

			for (i = 0; i < fileCount; i++) //this loop adds the filenames separated by newlines
			{
				strcat(buffer, filenames[i]);
				strcat(buffer, "\n");
			}

			//send the data
			int start = 0, charsRemaining = totalLength;
			charsWritten = 0;

			printf("Sending directory contents to %s:%d\n", listenHostName, dataPort);
			while (charsWritten < totalLength)
			{
				charsWritten = send(clientSocketFD, buffer + start, charsRemaining, 0);
				if (charsWritten < 0)
				{
					fprintf(stderr, "Error: could not write to socket\n");
					exit(1);
				}

				start += charsWritten;
				charsRemaining -= charsWritten;
			}

			close(clientSocketFD);
		}
		else if (operation == 'g')
		{
			//check that the filename requested is in the filename list
			int i = 0, fileNameMatch = 0;
			for (i = 0; i < fileCount; i++)
			{
				if ((strcmp(filenames[i], filename)) == 0) //if the filename matches a file in the directory, the program can continue
				{
					fileNameMatch = 1;
					break;
				}
			}

			if (fileNameMatch == 1)
			{
				char* fileText = NULL;
				long bufsize;

				/************* The following stack overflow post was used for this portion: https://stackoverflow.com/questions/2029103/correct-way-to-read-a-text-file-into-a-buffer-in-c **/

				//open the file and get its length
				FILE* fp = fopen(filename, "r");

				if (fp != NULL) //if there wasn't an error, continue
				{
					if (fseek(fp, 0L, SEEK_END) == 0) //go to the end of the file
					{
						bufsize = ftell(fp); //get the length of the file
						if (bufsize == -1)  //handle error
						{
							fprintf(stderr, "ERROR: Could not get length of input file\n");
							exit(1);
						}

						fileText = malloc(sizeof(char) * (bufsize + 1)); //allocate memory for full file size
						memset(fileText,'\0', bufsize + 1);

						if (fseek(fp, 0L, SEEK_SET) != 0) //go back to the beginning of the file, handle errors
						{
							fprintf(stderr, "ERROR: Could not seek back to beginning of input file\n");
							exit(1);
						}

						//this portion reads the entire file into memory
						size_t newLen = fread(fileText, sizeof(char), bufsize, fp);
						if (ferror(fp) != 0)
						{
							fprintf(stderr, "ERROR: Could not read input file into memory");
							exit(1);
						}
						else
						{
							fileText[newLen++] = '\0'; //add the null terminator to the end of the string
						}
					}

					fclose(fp); //close the input file

					int totalLength = bufsize + 1; //get the total size of the input file buffer
					int lengthDigits;
					if (totalLength == 0) 
					{ 
						lengthDigits = 0; 
					} //get the number of digits in total length
					else 
					{ 
						lengthDigits = floor(log10(abs(totalLength))) + 1; 
					}

					totalLength += (lengthDigits + 1); //add the number of digits in the buffer length plus a newline

					//create a new buffer that we can append buffer length to
					char* finalBuffer = NULL;
					finalBuffer = malloc(sizeof(char) * (totalLength + 1)); //allocate the memory for the new buffer
					memset(finalBuffer, '\0', totalLength + 1);

					sprintf(finalBuffer, "%d\n", totalLength); //throw the total length and a newline at the beginning so the receiving process knows how much to receive
					strcat(finalBuffer, fileText); //append the input file text to the final buffer

					int start = 0, charsRemaining = totalLength;
					charsWritten = 0;

					//send the message over to the client
					printf("Sending %s to %s:%d\n", filename, listenHostName, dataPort);
					while (charsWritten < totalLength)
					{
						charsWritten = send(clientSocketFD, finalBuffer + start, charsRemaining, 0);
						if (charsWritten < 0)
						{
							fprintf(stderr, "Error: could not write to socket\n");
							exit(1);
						}

						start += charsWritten;
						charsRemaining -= charsWritten;
					}					

					free(fileText);
					free(finalBuffer);

					close(clientSocketFD);
				}
				else
				{
					fprintf(stderr, "ERROR: Could not open input file\n");
					exit(1);
				}
				
			}
			else //send an error
			{
				printf("File not found. Sending error message to %s:%d\n", listenHostName, portNumber);
				memset(buffer, '\0', sizeof(buffer));
				sprintf(buffer, "FILE NOT FOUND");
				charsWritten = send(clientSocketFD, buffer, 14, 0);
				close(clientSocketFD);
			}
		}

	}


	close(listenSocketFD);

	return 0;
}