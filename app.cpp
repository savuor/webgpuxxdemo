#include <iostream>
#include <vector>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <glfw3webgpu.h>

// redefined, commenting it out
//#define WEBGPU_BACKEND_DAWN

struct TerminatorGLFW
{
    ~TerminatorGLFW()
    {
        glfwTerminate();
    }
};


WGPUAdapter requestAdapter(WGPUInstance instance, WGPURequestAdapterOptions const * options)
{
    struct UserData
    {
        WGPUAdapter adapter = nullptr;
        bool requestEnded = false;

    };
    UserData userData;

    //TODO: promise & future for wait
    auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const * message, void * pUserData)
    {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestAdapterStatus_Success)
        {
            userData.adapter = adapter;
        } else
        {
            std::cout << "Could not get WebGPU adapter: " << message << std::endl;
        }
        userData.requestEnded = true;
    };

    wgpuInstanceRequestAdapter(instance, options, onAdapterRequestEnded, (void*)&userData);

    // In theory we should wait until onAdapterReady has been called, which
    // could take some time (what the 'await' keyword does in the JavaScript
    // code). In practice, we know that when the wgpuInstanceRequestAdapter()
    // function returns its callback has been called.
    if(!userData.requestEnded)
    {
        throw std::runtime_error("Failed to get an adapter");
    }

    return userData.adapter;
}


WGPUDevice requestDevice(WGPUAdapter adapter, WGPUDeviceDescriptor const * descriptor)
{
    struct UserData
    {
        WGPUDevice device = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    //TODO: promise&future for wait
    auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, char const * message, void * pUserData)
    {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestDeviceStatus_Success) {
            userData.device = device;
        } else {
            std::cout << "Could not get WebGPU device: " << message << std::endl;
        }
        userData.requestEnded = true;
    };

    wgpuAdapterRequestDevice(adapter, descriptor, onDeviceRequestEnded, (void*)&userData);

    if(!userData.requestEnded)
    {
        throw std::runtime_error("Failed to open a device");
    }

    return userData.device;
}


int main (int /* argc */, char** /* argv */)
{
    if (!glfwInit())
    {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return 1;
    }

    TerminatorGLFW terminatorGlfw;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(640, 480, "WebGPU C++ Demo", NULL, NULL);

    if (!window)
    {
        std::cerr << "Could not open window!" << std::endl;
        return 1;
    }

    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;
    WGPUInstance instance = wgpuCreateInstance(&desc);
    if (!instance)
    {
        std::cerr << "Could not initialize WebGPU!" << std::endl;
        return 1;
    }

    // WGPUInstance is a simple pointer, it may be copied around without worrying about its size
    std::cout << "WGPU instance: " << instance << std::endl;

    WGPURequestAdapterOptions adapterOpts = {};
    WGPUAdapter adapter = requestAdapter(instance, &adapterOpts);

    std::cout << "WGPU adapter: " << adapter << std::endl;

    std::vector<WGPUFeatureName> features;

    // First call for a size, second call for actual features list
    size_t featureCount = wgpuAdapterEnumerateFeatures(adapter, nullptr);
    features.resize(featureCount);
    wgpuAdapterEnumerateFeatures(adapter, features.data());

    std::cout << "Adapter features:" << std::endl;
    for (const auto& f : features)
    {
        std::cout << " - " << f << std::endl;
    }

    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = "My Device"; // anything works here, that's your call
    deviceDesc.requiredFeaturesCount = 0; // we do not require any specific feature
    deviceDesc.requiredLimits = nullptr; // we do not require any specific limit
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = "The default queue";
    WGPUDevice device = requestDevice(adapter, &deviceDesc);

    std::cout << "Got device: " << device << std::endl;

    auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */)
    {
        std::cout << "Uncaptured device error: type " << type;
        if (message)
        {
            std::cout << " (" << message << ")";
        }
        std::cout << std::endl;
    };
    wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr /* pUserData */);

    auto onDeviceLost = [](WGPUDeviceLostReason reason, char const * message, void * /*userdata*/)
    {
        std::cout << "Device is lost: reason " << reason;
        if (message)
        {
            std::cout << " (" << message << ")";
        }
        std::cout << std::endl;
    };

    wgpuDeviceSetDeviceLostCallback(device, onDeviceLost, nullptr /*pUserData*/ );

    WGPUQueue queue = wgpuDeviceGetQueue(device);

    auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void* /* pUserData */)
    {
        std::cout << "Queued work finished with status: " << status << std::endl;
    };
    wgpuQueueOnSubmittedWorkDone(queue, /* signalValue -- no idea what it is */ 0, onQueueWorkDone, nullptr /* pUserData */);

    WGPUCommandEncoderDescriptor encoderDesc = {};
    encoderDesc.nextInChain = nullptr;
    encoderDesc.label = "My command encoder";
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

    wgpuCommandEncoderInsertDebugMarker(encoder, "Do one thing");
    wgpuCommandEncoderInsertDebugMarker(encoder, "Do another thing");

    WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
    cmdBufferDescriptor.nextInChain = nullptr;
    cmdBufferDescriptor.label = "Command buffer";
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);

    // Finally submit the command queue
    std::cout << "Submitting command..." << std::endl;
    wgpuQueueSubmit(queue, 1, &command);

    WGPUSurface surface = glfwGetWGPUSurface(instance, window);

    while (!glfwWindowShouldClose(window))
    {
        // Check whether the user clicked on the close button (and any other
        // mouse/key event, which we don't use so far)
        glfwPollEvents();
    }

    wgpuSurfaceRelease(surface);

    wgpuCommandBufferRelease(command);

    wgpuCommandEncoderRelease(encoder);

    wgpuQueueRelease(queue);

    wgpuDeviceRelease(device);

    wgpuAdapterRelease(adapter);

    wgpuInstanceRelease(instance);

    glfwDestroyWindow(window);

    return 0;
}