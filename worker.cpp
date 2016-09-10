// IMPORTANT - complie using -lcrypt flag

#include <iostream>
#include <cstring>		// for strlen
#include <sys/socket.h> // for socket
#include <arpa/inet.h>	// for inet_addr
#include <unistd.h>		// for crypt and close
#include <cstdlib>		// for exit
#include <pthread.h>	// for threading
using namespace std;

/*
    MESSAGE FORMATS
    ---------------

    1. Worker -> Server
        When it connects for the first time
            0th character = 'w'
            1st character = '2'
        When it responds to some task
            If password found
                0th character = 'w'
                1st character = '1'
                rest of the characters = password
            If password found
                0th character = 'w'
                1st character = '0'

    2. Server -> Worker
        0th character = length of password (assumed <10 for practical reasons)
        1st/2nd/3rd characters = 3 bit binary string (indicating the type of password)
        next few character = some aplhanumermic characters. Workers to try these
                             characters only as the first character for password
        '$' (indicating the beginning of hash) 
        rest of the characters = hash

*/

// returns the password after processing the server's message.
// returns empty string in case  no password is found
string process_task(const char *message);

// following are auxiliary functions used process_task() function
// explained later 
void crack(string &password, const string &hash, const string &allowed_chars, const int len, bool &found);
bool check(const string &password, const string &hash);

// main function
int main(int argc, char *argv[]){
	// Check if the executabel is run with valid number of arguments
    if(argc != 3){
    	// display appropriate error message
        cout << "Less/more arguments provided. Total 3 arguments are expected." << endl;
        return -1;
    }
   	
    const char* server_ip = argv[1];	// first argument
    const string port_str = argv[2];	// second argument

    // convert the port_str string to integer, and store it in server_port variable
    int server_port = 0;
    for(int i = 0; i < port_str.size(); i++){
        server_port = server_port*10 + port_str[i] - '0';
    }

    // construct the sockaddr_in structure for server
    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(server_ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);
     
    // create socket
    int sock = socket(AF_INET, SOCK_STREAM , 0);
    if (sock == -1){
    	// display appropriate error message
        cout << "Could not create socket. Run again." << endl;
        return -2;
    }
    cout << "Socket created" << endl;
     
    // connect to remote server
    if(connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0){
    	// display appropriate error message
        cout << "Connection with server failed. First run the server, then the worker." << endl;
        return -2;
    }
    cout << "Connected to server" << endl;
 	
 	char message[2000];	// will be used for all communications
 	memset(message, '\0', 2000);

 	// construct the initial connection ack message (see messgae format comment on top)
 	message[0] = 'w';
 	message[1] = '2';
 	
 	// send message
    if(send(sock, message, strlen(message), 0) < 0){
    	// display appropriate error message
        cout << "Sending message to server failed. Try again" << endl;
        return -2;
    }
    
    // now work as per the instruction of server indefinetly
    while(1){

    	// receive the message from server
    	memset(message, '\0', 2000);
    	int read_size = recv(sock , message , 2000 , 0);

    	// check if the server has closed the connection
    	if(read_size == 0){
    		cout  << "Server has closed the connection. Bye." << endl;
    		break;
    	}

    	// check for error
    	if(read_size < 0){
    		// display appropriate error message
    		cout << "Receive of message from server failed. Try again." << endl;
        	return -2;
    	}

    	cout << "Message from server received successfully" << endl;
    	cout << "Processing..." << endl;

    	// process the task given by server
    	string password = process_task(message);

    	cout << "Processing completed." << endl;

    	// reply to the server 
	   	// construct the response message (see messgae format comment on top)
	   	memset(message, '\0', 2000);

	 	message[0] = 'w';
	 	
	 	if(password.size() == 0){
	 		// no password found
	 		message[1] = '0';
	 	}
	 	else{
	 		// password found
	 		message[1] = '1';
	 		for(int i = 0; i < password.size(); i++)
	 			message[i+2] = password[i];
	 	}

	    // send message
	    if(send(sock, message, strlen(message), 0) < 0){
	    	// display appropriate error message
	        cout << "Sending message to server failed. Try again" << endl;
	        return -2;
	    }

	    cout << "Reply sent to server" << endl;
    }

    // close the socket and exit the program
    close(sock);
    return 0;
}

// returns the password after processing the server's message.
// returns empty string in case  no password is found
string process_task(const char *message){
	// first extract information out of message 
	
	// see messgae format comment on top
	int password_len = message[0] - '0';	// maximum length of password

	string allowed_chars = "";	// store all the allowed characters in password

	if(message[1] == '1'){
		// lower case alphabests are allowed
		for(char c = 'a'; c <= 'z'; c++)
			allowed_chars += c;
	}

	if(message[2] == '1'){
		// upper case alphabests are allowed
		for(char c = 'A'; c <= 'Z'; c++)
			allowed_chars += c;
	}

	if(message[3] == '1'){
		// numbers are allowed
		for(char c = '0'; c <= '9'; c++)
			allowed_chars += c;
	}

	// obtain set of first characters in password (from message)
	string allowed_first_chars = "";
	int pos;
	for(pos = 4; message[pos] != '$'; pos++)
		allowed_first_chars += message[pos];

	// obtain the hash (from message) which has to be cracked
	string original_hash = "";
	for(pos = pos + 1; pos < strlen(message); pos++)
		original_hash += message[pos];

	// now crack the password :-)
	bool found = 0;	// it will be set to true as soon as password is found
	string password = ""; // it will store passowrd if found = True

	// first fix length of password
	for(int len = 1; len <= password_len; len++){
		
		// assign first character
		for(int i = 0; i < allowed_first_chars.size(); i++){
			password += allowed_first_chars[i];
			// initial recursive call
			crack(password, original_hash, allowed_chars, len, found);
			if(found)
				return password;
			password.erase(password.end() - 1);
		}	

	}
	
	// nothing found, return empty string
	return "";
}

// simple recursive implementation of brute-force solution
void crack(string &password, const string &hash, const string &allowed_chars, const int len, bool &found){
	if(password.size() == len){
		// base case reached
		found = check(password, hash);
		return;
	}
	else{
		for(int i = 0; i < allowed_chars.size(); i++){
			password += allowed_chars[i];
			// recursive step
			crack(password, hash, allowed_chars, len, found);
			if(found)
				return;
			password.erase(password.end() - 1);
		}
	}
}

// this function return true iff crypt of password is the given hash
bool check(const string &password, const string &hash){
	
	// find the salt
	string salt = "";
	salt += hash[0];
	salt += hash[1];

	// check whether crpyt of password is the given hash or not
	const char *temp_hash = crypt(password.c_str(), salt.c_str());
    return (strcmp(hash.c_str(), temp_hash) == 0);
}
