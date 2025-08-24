#include <cstdint>
#include <cstring>
#include <iostream>
#include <optional>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;
using namespace cv;

const int WIDTH = 640;
const int HEIGHT = 480;
const int CHANNELS = 3;
const size_t SIZE = WIDTH * HEIGHT * CHANNELS;

bool recv_all(int fd, uint8_t *buf, size_t nbytes)
{
    size_t received = 0;
    while (received < nbytes)
    {
        size_t n = recv(fd, buf + received, nbytes - received, 0);
        if (n <= 0)
        {
            cerr << "recv error: " << (n == 0 ? "closed" : strerror(errno)) << endl;
            return false;
        }
        received += n;
    }
    return true;
}

// Compares original and current and returns a vec
// of the where the differences are 255
std::optional<std::vector<uint8_t>> get_diff(
    const std::vector<uint8_t> &original,
    const std::vector<uint8_t> &current)
{
    if (original.size() != current.size())
        return std::nullopt;

    std::vector<uint8_t> diff(original.size());

    for (size_t i = 0; i < original.size(); i += 3)
    {
        bool is_different = false;
        for (size_t j = 0; j < 3; j++)
        {

            if (original[i + j] != current[i + j])
            {
                is_different = true;
                break;
            }
        }

        for (size_t j = 0; j < 3; j++)
        {
            diff[i + j] = is_different ? 255 : 0;
        }
    }

    return diff;
}

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
    namedWindow("diff", WINDOW_AUTOSIZE);

    int original = 0;
    vector<uint8_t> images[2];
    images[0].resize(SIZE);
    images[1].resize(SIZE);

    while (true)
    {
        if (send(fd, "ready", 6, 0) < 0)
        {
            cerr << "Send failed" << strerror(errno) << endl;
            break;
        }

        if (!recv_all(fd, images[original ^ 1].data(), SIZE))
            break;

        Mat frame(480, 640, 16, images[original ^ 1].data());
        blur(frame, frame, Size(9, 9));
        memcpy(images[original ^ 1].data(), frame.data, SIZE);

        for (auto &pixel : images[original ^ 1])
        {
            int round = pixel & 10;
            if (round >= 5)
            {
                pixel += 10 - round;
            }
            else
            {
                pixel -= round;
            }
        }

        auto diff = get_diff(images[original], images[original ^ 1]);
        if (!diff.has_value())
            return 0;

        vector<uint8_t> image_diff = diff.value();
        Mat frame2(480, 640, CV_8UC3, image_diff.data());

        imshow("diff", frame2);
        imshow("debug", frame);
        waitKey(1000);

        original = original ^ 1;
    }

    close(fd);
}