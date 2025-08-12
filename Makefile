server: server.cpp
	g++ server.cpp -g -Wall `pkg-config --cflags --libs opencv4` -o server.out

client: client.cpp
	g++ client.cpp -g -Wall `pkg-config --cflags --libs opencv4` -o clinet.out

clean:
	rm -r *.out