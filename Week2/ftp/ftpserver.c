#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <sys/sendfile.h>
#include <fcntl.h>

void syserr(char *msg){
	perror(msg);
	exit(-1);
}

int main(int argc, char *argv[]){
	int sockfd, newsockfd, portno, fp, fileSize; 
	struct sockaddr_in serv_addr, clt_addr;
	socklen_t addrlen;
	char msgBuffer[256];
	char fileSizeBuffer[256];
	char clAddr[INET6_ADDRSTRLEN]; // luu dia chi ip cua client
	DIR *dir;
	struct dirent *directory;
	dir = opendir("./folder-remote");

	// check port
	if (argc != 2){
		fprintf(stderr, "Usage: %s <port>\n", argv[0]);
		return 1;
	}
	//portno = atoi(argv[1]); // if port is fine, convert it
	portno = 21; // Dat port la 21

	sockfd = socket(AF_INET, SOCK_STREAM, 0); // ipv4 and tcp
	if (sockfd < 0)
		syserr("can't open socket");
	printf("create socket...\n");

	// clean buffer
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;			
	serv_addr.sin_addr.s_addr = INADDR_ANY; 
	serv_addr.sin_port = htons(portno);	

	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		syserr("can't bind");

	printf("bind socket to port %d...\n", portno);
	listen(sockfd, 5); // this socket handles incoming requests

	for (;;){
		printf("wait on port %d...\n", portno);
		addrlen = sizeof(clt_addr);
		// newsockfd picks up that specific phone call:
		newsockfd = accept(sockfd, (struct sockaddr *)&clt_addr, &addrlen);

		if (newsockfd < 0) 
			syserr("can't accept");

		// client IP
		void *clientIP;
		struct in_addr ip = clt_addr.sin_addr;
		clientIP = &ip.s_addr;

		inet_ntop(AF_INET, clientIP, clAddr, sizeof(clAddr));
		printf("\nIP %s connected ", clAddr);

		// fork:
		pid_t pID = fork();

		if (pID < 0) // if forking fails:
		{
			perror("failed to fork!");
			exit(1);
		}
		if (pID == 0) // child process:
		{
			printf("Handler assigned for client %s\n", clAddr);
			close(sockfd); 

			// each fork requires its own buffer so that multiple clients don't read each others' info.
			int b;
			do
			{
				memset(&msgBuffer, 0, sizeof(msgBuffer));
				// read client message:
				printf("new incoming connection, block on receive\n");

				// receive on specific socket:
				b = recv(newsockfd, msgBuffer, sizeof(msgBuffer), 0);

				// server blocks on receive (waiting)
				if (b < 0)
					syserr("can't receive from client");
				else
					msgBuffer[b] = '\0';

				printf("server got message: %s\n", msgBuffer);

				if (strcmp(msgBuffer, "exit") == 0)
				{
					// send exit back to client
					send(newsockfd, msgBuffer, sizeof(msgBuffer), 0);

					printf("Terminating connection...\n");
					close(newsockfd);
					exit(0);
				}

				// handle ls-remote
				else if (strcmp(msgBuffer, "ls-remote") == 0)
				{
					// clean buffer with request
					memset(msgBuffer, 0, sizeof(msgBuffer));

					printf("Files at server:");
					if (dir) // if directory opens successfully
					{
						while ((directory = readdir(dir)) != NULL) // while in dir.
						{
							if (sizeof(msgBuffer) == 0) // if buffer is empty
							{
								if (strcmp(directory->d_name, ".") == 0 ||
									strcmp(directory->d_name, "..") == 0)

									printf("\nCAUGHT!"); // catch unnecessary info
								else
								{
									printf("\n%s", directory->d_name);
									sprintf(msgBuffer, "\n%s", directory->d_name);
								}
							}
							else // if buffer not full, pick up where we left off
							{
								if (strcmp(directory->d_name, ".") == 0 ||
									strcmp(directory->d_name, "..") == 0)

									printf("\nCAUGHT!");
								else
								{
									printf("\n%s", directory->d_name);
									sprintf(msgBuffer + strlen(msgBuffer),
											"\n%s", directory->d_name);
								}
							}
						}
						//send after we catch all files
						b = send(newsockfd, msgBuffer, sizeof(msgBuffer), 0);

					
						rewinddir(dir);
					}
					else // could not open directory
					{
						sprintf(msgBuffer, "server could not open directory");
					}
				}
				// user calls 'get file' (download)
				// Send user a file
				else if (msgBuffer[0] == 'g' &&
						 msgBuffer[1] == 'e' &&
						 msgBuffer[2] == 't' &&
						 msgBuffer[3] == ' ')
				{
					printf("User called get\n");

					// parse the string
					int j = 0;
					for (int i = 4; i <= strlen(msgBuffer); i++)
					{
						msgBuffer[j] = msgBuffer[i];
						j++;
					}
					char address[256] = "./folder-remote/";
					// get file path
					strcat(address, msgBuffer); 

					// mo file
					FILE *fp;
					fp = fopen(address, "rb");
					if (fp == NULL)
						printf("error opening file in: %s\n", msgBuffer);

					printf("File opened successfully!\n");

					// Doc file
					// figure out file size:
					int file_size = 0;
					if (fseek(fp, 0, SEEK_END) != 0)
						printf("Error determining file size\n");

					file_size = ftell(fp);
					rewind(fp);
					printf("File size: %d bytes\n", file_size);

					// pass this size to a buffer so we can send it:
					memset(&fileSizeBuffer, 0, sizeof(fileSizeBuffer));
					sprintf(fileSizeBuffer, "%d", file_size);

					// send file size:
					b = send(newsockfd, fileSizeBuffer, sizeof(fileSizeBuffer), 0);
					if (b < 0) // n < 0
						printf("Error sending file size.\n");

					// receive an ACK from client;
					b = recv(newsockfd, fileSizeBuffer, sizeof(fileSizeBuffer), 0);
					if (b < 0)
						printf("Error receiving handshake");

					// we create a byte array:
					char byteArray[256];
					memset(&byteArray, 0, sizeof(byteArray));

					int buffRead = 0;
					int bytesRemaining = file_size;

					// while there are still bytes to be sent:
					while (bytesRemaining != 0)
					{
						// we fill in the byte array
						// with slabs smaller than 256 bytes:
						if (bytesRemaining < 256)
						{
							buffRead = fread(byteArray, 1, bytesRemaining, fp);
							bytesRemaining = bytesRemaining - buffRead;
							b = send(newsockfd, byteArray, 256, 0);
							if (b < 0)
								printf("Error sending small slab\n");

							printf("sent %d slab\n", buffRead);
						}
						else
						{
							buffRead = fread(byteArray, 1, 256, fp);
							bytesRemaining = bytesRemaining - buffRead;
							b = send(newsockfd, byteArray, 256, 0);
							if (b < 0)
								printf("Error sending slab\n");
							printf("sent %d slab\n", buffRead);
						}
					}
					printf("File sent!\n");
					// clean buffers
					memset(&msgBuffer, 0, sizeof(msgBuffer));
					memset(&byteArray, 0, sizeof(byteArray));
				}
				// user calls 'put file'
				else if (msgBuffer[0] == 'p' &&
						 msgBuffer[1] == 'u' &&
						 msgBuffer[2] == 't' &&
						 msgBuffer[3] == ' ')
				{
					printf("User called put\n");

					// acknowledge the client we got the file name
					b = send(newsockfd, msgBuffer, sizeof(msgBuffer), 0);
					if (b < 0)
						printf("Error sending file ACK\n");

					// receive on the fileSizeBuffer
					memset(&fileSizeBuffer, 0, sizeof(fileSizeBuffer));
					b = recv(newsockfd, fileSizeBuffer, sizeof(fileSizeBuffer), 0);
					if (b < 0)
						printf("Error receiving file size\n");
					printf("size should be: %s\n", fileSizeBuffer);

					// send an ACK for file size
					b = send(newsockfd, fileSizeBuffer, sizeof(fileSizeBuffer), 0);
					if (b < 0)
						printf("Error sending ACK for file size\n");

					// catch the file name
					char fileName[256];
					memset(&fileName, 0, sizeof(fileName));

					// parse
					int j = 0;
					for (int i = 4; i <= strlen(msgBuffer); i++)
					{
						// pass to name buffer
						fileName[j] = msgBuffer[i];
						j++;
					}
					fileSize = atoi(fileSizeBuffer);

					// print file name and size:
					printf("File: '%s' (%d bytes)\n", fileName, fileSize);

					// receive data
					memset(&msgBuffer, 0, sizeof(msgBuffer));
					int remainingData = 0;
					ssize_t len;
					char path[256] = "./folder-remote/";
					strcat(path, fileName);
					printf("path: %s\n", path);
					FILE *fileprocessor;
					fileprocessor = fopen(path, "wb"); // overwrite if existing
													   // create if not
					remainingData = fileSize;
					// while(((len = recv(newsockfd, msgBuffer, 256, 0)) > 0) && (remainingData > 0))
					while (remainingData != 0)
					{
						if (remainingData < 256)
						{
							len = recv(newsockfd, msgBuffer, remainingData, 0);
							fwrite(msgBuffer, sizeof(char), len, fileprocessor);
							remainingData -= len;
							printf("Received %lu bytes, expecting %d bytes\n", len, remainingData);
							break;
						}
						else
						{
							len = recv(newsockfd, msgBuffer, 256, 0); // 256
							fwrite(msgBuffer, sizeof(char), len, fileprocessor);
							remainingData -= len;
							printf("Received %lu bytes, expecting: %d bytes\n", len, remainingData);
						}
					}
					fclose(fileprocessor);
					b = recv(newsockfd, msgBuffer, 256, 0); // receive bizarre lingering packet.
					// clean buffer
					memset(&msgBuffer, 0, sizeof(msgBuffer));
				} 
				else
				{
					b = send(newsockfd, msgBuffer, sizeof(msgBuffer), 0);
					if (b < 0)
						syserr("can't send to server");
					printf("send message...%s\n", msgBuffer);
				} 

				// clean buffer
				memset(&msgBuffer, 0, sizeof(msgBuffer));
			} while (strcmp(msgBuffer, "exit") != 0); 
		}
		else			
			close(newsockfd); // close specific socket
	}						 
	close(sockfd);			 
	return 0;
}
