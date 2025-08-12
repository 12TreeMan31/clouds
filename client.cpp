#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;
using namespace cv;

const int SIZE = 640 * 480 * 3;

int main(int argc, char *argv[])
{
    if (argc == 0)
        return -1;

    in_addr addr;
    inet_aton(argv[1], &addr);
    // Converts string to int than to network byte order
    int port = htons(atoi(argv[2]));

    struct sockaddr_in remote;
    remote.sin_addr = addr;
    remote.sin_family = AF_INET;
    remote.sin_port = port;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    connect(fd, (struct sockaddr *)&remote, sizeof(remote));

    namedWindow("debug", WINDOW_AUTOSIZE);

    while (true)
    {
        char buffer[SIZE] = {0};

        send(fd, "ready", 6, 0);

        int bytes = 0;
        while (bytes != SIZE)
        {
            int remaining = SIZE - bytes;
            int n = recv(fd, buffer + bytes, remaining, 0);
            bytes += n;
        }

        Mat frame(480, 640, 16, buffer);

        imshow("debug", frame);
        waitKey(1000);
    }
}