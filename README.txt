SOCKET PROGRAMMING
Computer Networks

Roll NO. 140050002 (Deep Modh)
Roll No. 140050020 (Govind Lahoti)

---------------------------------------------------------------

List of relevant files -->

1. server.cpp
	Contains the code for server

2. worker.cpp
	Contains the code for worker

3. user.cpp
	Contains the code for user

4. Makefile

5. Result.jpg

---------------------------------------------------------------

Running instructions -->
	
	(IN SEQUENCE)

	1. First compile each file and create the executable
		g++ server.cpp -o server
		g++ worker.cpp -o worker -lcrypt
		g++ user.cpp -o user

	2. Run the server by command : ./server <port>
		where <port> is the port number to which server will listen
		example: ./server 5000

	3. Run the workers one after the other each by command : ./worker <server-ip> <server-port>
		where <server-ip> is IP address of the server
			  <server-port> is the port number to which server is listening
		example: ./server 10.105.12.67 5000

	4. Run the users one after the other, each using commad:
		./user <server-ip> <server-port> <hash> <passwd-length> <binary-string>
		where <server-ip> is IP address of the server
			  <server-port> is the port number to which server is listening
			  <hash> is the hash whose corresponding password is to be cracked
			  <passwd-lenght> is the maximum possible length of password. For practical reason it should be <10
			  <binary-string> is a 3 bit binary string, where the three bits indicate the use of lower case, upper case and numerical characters in the password.
		example: ./user 10.105.12.67 5000 aay0SZFBQF/BA 7 001

---------------------------------------------------------------