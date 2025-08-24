#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>

#include <libcamera/libcamera.h>

using namespace libcamera;
using namespace std;

static shared_ptr<Camera> camera;

int main()
{
    // See https://libcamera.org/guides/application-developer.html
    unique_ptr<CameraManager> cm = make_unique<CameraManager>();
    cm->start();

    // Inits camera but does not start it
    auto cameras = cm->cameras();
    if (cameras.empty())
    {
        cout << "No cameras were identified on the system."
             << endl;
        cm->stop();
        return EXIT_FAILURE;
    }

    for (auto const &camera : cm->cameras())
        cout << camera->id() << endl;

    string cameraId = cameras[0]->id();
    camera = cm->get(cameraId);

    camera->acquire();

    unique_ptr<CameraConfiguration> config = camera->generateConfiguration({StreamRole::Viewfinder});
    StreamConfiguration &streamConfig = config->at(0);
    cout << "Default viewfinder configuration is: " << streamConfig.toString() << endl;

    config->validate();
    camera->configure(config.get());

    // Now set up framebuffer

    FrameBufferAllocator *allocator = new FrameBufferAllocator(camera);
    for (StreamConfiguration &cfg : *config)
    {
        cout << cfg << endl;
        int ret = allocator->allocate(cfg.stream());
        if (ret < 0)
        {
            std::cerr << "Can't allocate buffers" << std::endl;
            return -ENOMEM;
        }

        size_t allocated = allocator->buffers(cfg.stream()).size();
        std::cout << "Allocated " << allocated << " buffers for stream" << std::endl;
    }

    Stream *stream = streamConfig.stream();
    const std::vector<std::unique_ptr<FrameBuffer>> &buffers = allocator->buffers(stream);
    std::vector<std::unique_ptr<Request>> requests;

    for (unsigned int i = 0; i < buffers.size(); ++i)
    {
        std::unique_ptr<Request> request = camera->createRequest();
        if (!request)
        {
            std::cerr << "Can't create request" << std::endl;
            return -ENOMEM;
        }

        const std::unique_ptr<FrameBuffer> &buffer = buffers[i];
        int ret = request->addBuffer(stream, buffer.get());
        if (ret < 0)
        {
            std::cerr << "Can't set buffer for request"
                      << std::endl;
            return ret;
        }

        requests.push_back(std::move(request));
    }

    camera->stop();
    camera->release();
    camera.reset();
    cm->stop();

    return 0;
}