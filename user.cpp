#include <iostream>
#include <cstring>      // for strlen
#include <sys/socket.h> // for socket
#include <arpa/inet.h>  // for inet_addr
#include <unistd.h>     // for close(socket)
#include <sys/time.h>   // for gettimeofday()
using namespace std;

/*
    MESSAGE FORMATS
    ---------------

    1. User -> Server
        0th character = 'u'
        1st character = length of password (assumed <10 for practical reasons)
        2nd/3rd/4th characters = 3 bit binary string (indicating the type of password)
        rest of the characters = hash

    2. Server -> User
        Just the password
        In case no password is found (happens when hash provided is invalid), message returned is 'Hash provided is Invalid!!'

*/

// main function
int main(int argc , char *argv[]){
    // Check if the executabel is run with valid number of arguments
    if(argc != 6){
        // display appropriate error message
        cout << "Less/more arguments provided. Total 6 arguments are expected." << endl;
        return -1;
    }
    
    // extract information from arguments provided
    const char* server_ip = argv[1];
    const string port_str = argv[2];
    const string hash = argv[3];
    const string passlen_str = argv[4];
    const string pass_type = argv[5];

    // check whether password length is within practical limit
    if(passlen_str.size() > 1){
        // display appropriate objection message
        cout << "Passwords of such large length can not be cracked. Give password length <= 9." << endl;
        return -1;
    }

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
    if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0){
        // display appropriate error message
        cout << "Connection with server failed. First run the server, then the user." << endl;
        return -2;
    }
    cout << "Connected to server" << endl;

    char message[2000]; // will be used for all communication
    memset(message, '\0', 2000);

    // construct the message (see messgae format comment on top)
    message[0] = 'u';
    message[1] = passlen_str[0];
    message[2] = pass_type[0];
    message[3] = pass_type[1];
    message[4] = pass_type[2];

    for(int i = 0; i < hash.size(); i++){
        message[i+5] = hash[i];
    }

    // send message
    if(send(sock, message, strlen(message), 0) < 0){
        // display appropriate error message
        cout << "Sending message to server failed. Try again" << endl;
        return -2;
    }
    
    // measure start time 
    struct timeval t1;
    gettimeofday(&t1, NULL);

    // check reply from the server
    memset(message, '\0', 2000);
    if(recv(sock, message, 2000 ,0) <= 0){
        // display appropriate error message
        cout << "Receive of message from server failed. Try again." << endl;
        return -2;
    }

    // mesasure end time
    struct timeval t2;
    gettimeofday(&t2, NULL);

    // compute tike taken by server
    double time_taken = (t2.tv_sec - t1.tv_sec) + double(t2.tv_usec - t1.tv_usec)/1000000.0;

    cout << "Password : " << message << endl;
    cout << "Time taken by server to crack password : " << time_taken << " seconds" << endl; 
    
    // close the socket and exit the program
    close(sock);
    return 0;
}