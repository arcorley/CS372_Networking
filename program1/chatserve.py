import socket
import sys

if (len(sys.argv) < 2): #make sure there is a port argument
	print "USAGE: " + sys.argv[0] + " port" #display proper usage to user
	sys.exit(0) #exit the program

serverName = socket.gethostname() #get the host name for creation of the socket
serverPort = int(sys.argv[1]) #get the port number from the arguments
serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM) #declare the socket
serverSocket.bind((serverName,serverPort)) #bind the socket to the port
serverSocket.listen(1) #begin listening

exitProg = 0

while True: #infinite loop. Ensures the program stays running until it receives a SIGINT
	exitProg = 0
	connectionSocket, addr = serverSocket.accept() #accept any connections

	while (exitProg == 0): #main chat loop. continue passing messages until someone exits
		sentence = connectionSocket.recv(525).decode() #receive message from client

		if "/exit" in sentence: #if exit was received from client, close connection and break loop
			exitProg = 1
			connectionSocket.close()
			break
		else:
			print sentence #if exit wasn't received, display the message sent over

		reply = raw_input("chatserve> ") #gather the reply

		if (reply == '/exit'): #check if the chatserve reply is exit
			exitProg = 1
			connectionSocket.send(reply.encode()) #send the reply so the client knows the chat is ending
			connectionSocket.close() #close the socket
			break
		else:
			finalReply = "chatserve> " + reply #append the handle to the reply
			connectionSocket.send(finalReply.encode()) #send the message

		sentence=""