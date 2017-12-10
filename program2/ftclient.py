import socket
import sys

def validateArgs():
	#check that host is valid
	try:
		socket.gethostbyname(sys.argv[1])
	except:
		print "ERROR: invalid hostname supplied"
		print "USAGE: " + sys.argv[0] + " host port command [filename] dataport" #display proper usage to user
		sys.exit(0)

	#check that port number was provided is a valid int
	try:
		int(sys.argv[2])
	except:
		print "ERROR: invalid port number supplied"
		print "USAGE: " + sys.argv[0] + " host port command [filename] dataport" #display proper usage to user
		sys.exit(0)

	#check that a valid command was supplied
	if (sys.argv[3] != "-l" and sys.argv[3] != "-g"):
		print "ERROR: invalid command supplied"
		print "USAGE: " + sys.argv[0] + " host port command [filename] dataport" #display proper usage to user
		sys.exit(0)

	if (sys.argv[3] == "-l"): #check arguments if the -l command is supplied
		if (len(sys.argv) > 5): #check that the number of arguments is correct
			print "ERROR: too many arguments supplied"
			print "USAGE (-l command): " + sys.argv[0] + " host port -l dataport" #display proper usage to user
			sys.exit(0)
		else:
			try:
				int(sys.argv[4]) #make sure valid port number is supplied
			except:
				print "ERROR: invalid dataport number supplied"
				print "USAGE (-l command): " + sys.argv[0] + " host port -l dataport" #display proper usage to user
				sys.exit(0)

			if ((int(sys.argv[4]) < 40000) or (int(sys.argv[4]) > 65000)): #make sure data port is a value likely not in use
				print "ERROR: dataport must be a port number between 40000 and 65000"
				sys.exit(0)
			elif (int(sys.argv[2]) == int(sys.argv[4])): #ensure data port is different than the host port
				print "ERROR: dataport cannot be the same as the host port"
				sys.exit(0)

	elif (sys.argv[3] == "-g"): #check arguments if the -g command is supplied
		if (len(sys.argv) != 6):
			print "ERROR: incorrect number of arguments supplied"
			print "USAGE (-g command): " + sys.argv[0] + " host port -g filename dataport" #display proper usage to user
			sys.exit(0)
		else:
			try:
				int(sys.argv[5])
			except:
				print "ERROR: invalid dataport number supplied"
				print "USAGE (-g command): " + sys.argv[0] + " host port -g filename dataport" #display proper usage to user
				sys.exit(0)

			if ((int(sys.argv[5]) < 40000) or (int(sys.argv[5]) > 65000)): #make sure data port is a value likely not in use
				print "ERROR: dataport must be a port number between 40000 and 65000"
				sys.exit(0)
			elif (int(sys.argv[2]) == int(sys.argv[5])): #ensure data port is different than the host port
				print "ERROR: dataport cannot be the same as the host port"
				sys.exit(0)

	return

def main():
	validateArgs()

	#set up the socket to connect to the server process
	serverName = sys.argv[1]
	serverPort = int(sys.argv[2])
	clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

	#set up the listening socket for the data connection
	listenServerName = socket.gethostname()

	#get the data port
	if (sys.argv[3] == "-l"):
		listenServerPort = int(sys.argv[4])
	else:
		listenServerPort = int(sys.argv[5])

	#set up the listening socket
	listenServerSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM) #declare the socket
	listenServerSocket.bind((listenServerName,listenServerPort)) #bind the socket to the port
	listenServerSocket.listen(1) #begin listening

	#connect to the server process
	try:
		clientSocket.connect((serverName, serverPort))
	except:
		print "ERROR: connection could not be made to supplied hostname"
		sys.exit(1)

	#send over the requested operation and dataport to the server
	if (sys.argv[3] == "-l"):
		reply = "l\n" + str(listenServerPort) + "\n@@"
	else:
		reply = "g\n" + sys.argv[4] + "\n" + sys.argv[5] + "\n@@"

	clientSocket.send(reply.encode()) #send the requested operation to the server

	connectionSocket, addr = listenServerSocket.accept() #accept any connections to listening socket
	clientName = socket.gethostbyaddr(addr[0])
	if (sys.argv[3] == "-l"):
		print "Receiving directory structure from " + clientName[0] + ":" + str(listenServerPort)
		charsInMessage = 9999999 #set the number of chars in message to be something very high
		charsReceived = 0
		iteration = 0
		results = []

		while (charsReceived < charsInMessage):
			recvdBuffer = connectionSocket.recv(4096).decode() #receive a buffer from the client

			if (iteration == 0): #if this is the first iteration, get the total chars in message
				for line in recvdBuffer.splitlines():
					line = line.strip()
					if not line:continue
					results.append(line)
				charsInMessage = int(results[0])
			else:
				results.append(recvdBuffer)

			charsReceived += len(recvdBuffer)
			iteration += 1
			
		for x in range(1, len(results)):
			print results[x]

		listenServerSocket.close()

	else:
		charsInMessage = 9999999 #set the number of chars in message to be something very high
		charsReceived = 0
		iteration = 0
		results = ""

		while (charsReceived < charsInMessage):
			recvdBuffer = connectionSocket.recv(4096).decode() #receive a buffer from the client

			if (iteration == 0):
				bufsize = recvdBuffer.split('\n', 1)[0] #get the first line of the buffer, which will be the total length
				try:
					charsInMessage = int(bufsize) #set the actual length of the buffer so we know when to stop receiving
					print 'Receiving "' + sys.argv[4] + '" from ' + clientName[0] + ":" + str(listenServerPort)
					results += recvdBuffer
					results = results.split('\n', 1)[-1] #cut off the first newline
				except:
					print clientName[0] + ":" + str(listenServerPort) + " says " + str(recvdBuffer)
					break
			else:
				results += recvdBuffer

			charsReceived += len(recvdBuffer)
			iteration +=1

		file = open(sys.argv[4], "w")
		file.write("%s" % results)
		file.close()
		print "File transfer complete."

	clientSocket.close()

	return

main()