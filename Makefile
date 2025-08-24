SOURCEDIR=src
BUILDDIR=build
OBJDIR=$(BUILDDIR)/obj

server:
	g++ $(SOURCEDIR)/server.cpp -g -Wall `pkg-config --cflags --libs opencv4` -o server.out

client:
	g++ $(SOURCEDIR)/client.cpp -g -Wall -Wextra -Wconversion `pkg-config --cflags --libs opencv4` -o clinet.out

station:
	g++ $(SOURCEDIR)/station.cpp -g -Wall `pkg-config --cflags --libs libcamera`

clean:
	rm -r *.out