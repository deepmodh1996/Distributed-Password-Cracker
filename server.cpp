#include <iostream>
#include <cstring>          // for strlen
#include <sys/socket.h>     // for socket
#include <arpa/inet.h>      // for inet_addr
#include <sys/types.h>      // for select
#include <unistd.h>         // for close
#include <algorithm>
#include <cassert>
#include <vector>
#define BACK_LOG 10         // maximum no. of connections allowed by server
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

    3. Worker -> Server
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

    4. Server -> Worker
        0th character = length of password (assumed <10 for practical reasons)
        1st/2nd/3rd characters = 3 bit binary string (indicating the type of password)
        next few character = some aplhanumermic characters. Workers to try these
                             characters only as the first character for password
        '$' (indicating the beginning of hash) 
        rest of the characters = hash

*/

// this function cracks the hash given by the user, by distributing the work ammongst workers
// This function returns the password if obtained/else invalid hash message  
string solve(const char *message, const vector<int> &worker_sockets);

// main function
int main(int argc, char *argv[]){

    // Check if the executabel is run with valid number of arguments
    if(argc != 2){
        // display appropriate error message
        cout << "Less/more arguments provided. Total 2 arguments are expected." << endl;
        return -1;
    }
    
    const string port_str = argv[1];
    
    // convert the port_str string to integer, and store it in server_port variable
    int server_port = 0;
    for(int i = 0; i < port_str.size(); i++){
        server_port = server_port*10 + port_str[i] - '0';
    }
 
    // create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1){
        // display appropriate error message
        cout << "Could not create socket. Run again." << endl;
        return -2;
    }
    cout << "Socket created" << endl;
     
    // set server_socket to allow multiple connections.
    int opt = 1;
    if((setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*) &opt, sizeof(int)) == -1 )||
        (setsockopt(server_socket, SOL_SOCKET, SO_KEEPALIVE, (char*) &opt, sizeof(int)) == -1 )){
        // display appropriate error message
        cout << "Could not allow multiple connections at the moment. Run again." << endl;
        return -2;
    }

    // prepare the sockaddr_in structure
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(server_port);
     
    // bind the server_socket
    if(bind(server_socket, (struct sockaddr *) &server, sizeof(server)) < 0){
        // display appropriate error message
        cout << "Bind failed. Try different port." << endl;
        return -2;
    }
    cout << "Bind successful" << endl;
     
    // start listening to the socket
    if(listen(server_socket, BACK_LOG) < 0){
        // display appropriate error message
        cout << "Unable to listen. Try again." << endl;
        return -2;
    };
    cout << "Started listening..." << endl;

    cout << "Waiting for incoming connections..." << endl;
    int addrlen = sizeof(struct sockaddr_in);
    
    fd_set readfds; // socket set to be used in select
    vector<int> client_sockets, worker_sockets;
    // client_sockets -> vector of all the sockets connected to server
    // worker_sockets -> vector of all the workers connected to server

    // run server indefinitely
    while(1){

        // clear the socket set
        FD_ZERO(&readfds);
  
        // add server_socket to socket set
        FD_SET(server_socket, &readfds);

        // add all the connected clients to socket set
        for(int i = 0; i < client_sockets.size(); i++){
            FD_SET(client_sockets[i], &readfds);
        }

        // wait for activity to happend in readfds socket set
        int activity = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);

        // check if some error occured in select
        if(activity < 0){
            // display appropriate error message
            cout << "Error in select. Try again." << endl;
            return -2;
        }

        if(FD_ISSET(server_socket, &readfds)){

            // there is a new incoming connection
            struct sockaddr_in new_address;
            int new_socket = accept(server_socket, (struct sockaddr *) &new_address, (socklen_t*) &addrlen);
            if(new_socket < 0){
                // display appropriate error message
                cout << "Error in acceptance. Try again." << endl;
                return -2;
            }
            cout << "New connection" << endl;
             
            // add new socket to array of sockets
            client_sockets.push_back(new_socket);
        }

        for(int i = 0; i < client_sockets[i]; i++){
             
            if(FD_ISSET(client_sockets[i], &readfds)){
               
                char message[2000]; // to be used to store that message
                memset(message, '\0', 2000);

                // read the message
                int read_size = recv(client_sockets[i], message, 2000, 0);
                
                // Check if it was for closing. If not read the incoming message if w
                if (read_size == 0){

                    // Client disconnected
                    cout << "Client disconnected" << endl;
                    
                    // close the correspoing socket 
                    close(client_sockets[i]);
                    
                    // remove it from worker_sockets if present
                    vector<int>::iterator it = find(worker_sockets.begin(), worker_sockets.end(), client_sockets[i]);
                    if(it != worker_sockets.end())
                        worker_sockets.erase(it);

                    // remove it from client_sockets
                    client_sockets.erase(client_sockets.begin() + i);
                }

                else{
                    // there is a new incoming message       
                    if(message[0] == 'w'){
                        // the message is from worker
                        worker_sockets.push_back(client_sockets[i]);
                        // It should be connection ack message (see message format on top)
                        // By construction, the workers responses regarding passwords won't appear here
                        // They will appear in solve() function only
                        assert(message[1] == '2');
                    }
                    else{
                        // The message is from user

                        // obtain the password corresponding to user request
                        string password = solve(message, worker_sockets);

                        // convert to char* before sending
                        const char *reply = password.c_str();
                        
                        // send the message back to user
                        if(send(client_sockets[i], reply, strlen(reply), 0) < 0){
                            // display appropriate error message
                            cout << "Sending message to user failed. Try again" << endl;
                            return -2;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

// this function cracks the hash given by the user, by distributing the work ammongst workers
// This function returns the password if obtained/else invalid hash message
string solve(const char *message, const vector<int> &worker_sockets){

    string allowed_chars = "";  // store all the allowed characters in password

    if(message[2] == '1'){
        // lower case alphabests are allowed
        for(char c = 'a'; c <= 'z'; c++)
            allowed_chars += c;
    }

    if(message[3] == '1'){
        // upper case alphabests are allowed
        for(char c = 'A'; c <= 'Z'; c++)
            allowed_chars += c;
    }

    if(message[4] == '1'){
        // numbers are allowed
        for(char c = '0'; c <= '9'; c++)
            allowed_chars += c;
    }

    // calculate approx how many starting characters each worker will get
    int n_each = allowed_chars.size() / worker_sockets.size();

    char instruction[2000]; // message that will be sent to workers

    // send tasks (after breaking it in pieces) to all the workers
    for(int i = 0; i < worker_sockets.size(); i++){

        // start the construction of messagee (see message format comment)
        memset(instruction, '\0', 2000);
        for(int j = 0; j < 4; j++)
        instruction[j] = message[j+1];

        int pos = 4;
        // give only a fraction of allowed characters as first characters to each worker
        if(i < worker_sockets.size() - 1){
            for(int j = n_each*i;  j < n_each*(i+1); j++)
                instruction[pos++] = allowed_chars[j];
        }
        else{
            for(int j = n_each*i; j < allowed_chars.size(); j++)
                instruction[pos++] = allowed_chars[j];       
        }

        instruction[pos++] = '$';
        // copy the hash
        for(int j = 5; j < strlen(message); j++)
            instruction[pos++] = message[j];

        // now send this instruction to worker
        if(send(worker_sockets[i], instruction, strlen(instruction), 0) < 0){
            // display appropriate error message
            cout << "Sending message to server failed. Try again" << endl;
            exit(-2);
        }
    }

    string password = "Hash provided is Invalid!!"; // default message if no worker got the password

    char response[2000]; // will store the messages received from workers

    // check for response of each worker
    for(int i = 0; i < worker_sockets.size(); i++){
        // check response from worker-i
        memset(response, '\0', 2000);
        if(recv(worker_sockets[i], response, 2000, 0) <= 0){
            // display appropriate error message
            cout << "Receive of message from worker failed. Try again." << endl;
            exit(-2);
        }
        if(response[1] == '1'){
            // password found by worker
            password = "";
            for(int j = 2; j < strlen(response); j++)
                password += response[j];
        }
    
    }
    
    // return the password obtained/else the invalid hash message
    return password;
}
