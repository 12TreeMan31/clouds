#include <poll.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

using namespace std;
using namespace cv;

const char *window_name = "debug";
const int MAX_CONNECTIONS = 8;
const int PORT = 8888;

void camera_init(VideoCapture *cam, int deviceID)
{
    int apiID = cv::CAP_ANY;

    cam->open(deviceID, apiID);
    if (!cam->isOpened())
    {
        cerr << "ERROR: Unable to open camera " << deviceID << endl;
        throw std::runtime_error("camera");
    }
}

/*unsigned char **mat_to_array(Mat mat)
{

    unsigned char *data = mat.data;
}*/

void new_connection(int serverfd, struct pollfd *fds, int *nfds)
{
    for (int i = 0; i < 8; i++)
    {
        // Null since we don't care about the address
        int peerfd = accept(serverfd, NULL, NULL);

        // No more new peers
        if (peerfd < 0)
            return;

        cout << "New client" << endl;
        fds[*nfds] = {.fd = peerfd, .events = POLLIN};
        *nfds += 1;
    }
}
void handle_network(int fd, Mat frame)
{
    char buffer[1024] = {0};
    recv(fd, buffer, 1024, 0);
    string code(buffer);
    if (code != "ready")
        return;

    unsigned int size = frame.rows * frame.cols * frame.channels();
    unsigned int bytes = 0;

    while (bytes != size)
    {
        auto remaining = size - bytes;
        auto n = send(fd, frame.data + bytes, remaining, 0);
        bytes += n;
    }
}

int new_socket(int socket_type, int max_connections, int port)
{
    int fd = socket(AF_INET, socket_type, 0);
    struct sockaddr_in info;
    info.sin_addr.s_addr = INADDR_ANY;
    info.sin_family = AF_INET;
    info.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&info, sizeof(info)) < 0)
    {
        perror("Could not bind");
        return -1;
    }

    if (listen(fd, max_connections) < 0)
    {
        perror("Failed to listen");
        return -1;
    }

    return fd;
}

int main()
{
    // Set SOCK_NONBLOCK to not block on accept
    int serverfd = new_socket(SOCK_STREAM | SOCK_NONBLOCK, MAX_CONNECTIONS, PORT);
    cout << "0.0.0.0:" << PORT << endl;

    VideoCapture cam;
    camera_init(&cam, 0);
    // namedWindow(window_name, WINDOW_AUTOSIZE);
    Mat frame;
    cam.read(frame);
    cout
        << frame.cols
        << "x"
        << frame.rows
        << "x"
        << frame.channels()
        << " Type: "
        << frame.type()
        << endl;

    // Sets fds[0] to be the servers socket
    struct pollfd fds[MAX_CONNECTIONS] = {{.fd = serverfd, .events = POLLIN}};
    int nfds = 1;

    while (true)
    {
        if (poll(fds, nfds, -1) <= 0)
        {
            perror("Poll timed out");
            continue;
        }

        int currentnfds = nfds;
        for (int i = 0; i <= currentnfds; i++)
        {
            struct pollfd current = fds[i];
            // No new event or no watched fd at that index
            if (current.revents != POLLIN || current.fd < 0)
                continue;

            // There is a new connection
            if (current.fd == serverfd)
            {
                new_connection(serverfd, fds, &nfds);
                continue;
            }

            if (frame.empty())
            {
                cerr << "ERROR! blank frame grabbed\n";
                break;
            }

            cam.read(frame);

            waitKey(100);
            cout << "writiable" << endl;
            handle_network(current.fd, frame);
        }
    }

    close(serverfd);

    return 0;
}