all:client.cpp server.cpp 
	g++ -o server server.cpp
	g++ -o client client.cpp

clean:
	rm -f server client

test:
	./server
