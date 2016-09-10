all: server user worker

server: server.cpp
	g++ server.cpp -o server

user: user.cpp
	g++ user.cpp -o user

worker: worker.cpp
	g++ worker.cpp -lcrypt -o worker

clean:
	rm -f server
	rm -f user
	rm -f worker
