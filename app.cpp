#include <iostream>
#include <vector>
#include <map>
#include <chrono>
#include <thread>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <glfw3webgpu.h>

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
#ifdef WEBGPU_BACKEND_DAWN
    const std::string backendName = "Dawn";
#elif defined(WEBGPU_BACKEND_WGPU)
    const std::string backendName = "WGPU-Native";
#endif

    std::cout << "Running with " << backendName << " backend" << std::endl;

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

    if (!glfwInit())
    {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return 1;
    }

    TerminatorGLFW terminatorGlfw;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(640, 480, "WebGPU C++ Demo", NULL, NULL);

    if (!window)
    {
        std::cerr << "Could not open window!" << std::endl;
        return 1;
    }

    WGPUSurface surface = glfwGetWGPUSurface(instance, window);

    WGPURequestAdapterOptions adapterOpts = {};
    WGPUAdapter adapter = requestAdapter(instance, &adapterOpts);

    std::cout << "WGPU adapter: " << adapter << std::endl;

    const std::map<int, std::string> featuresNames =
        {
            {         0, "Undefined"},
            {         1, "DepthClipControl"},
            {         2, "Depth32FloatStencil8"},
            {         3, "TimestampQuery"},
            {         4, "PipelineStatisticsQuery"},
            {         5, "TextureCompressionBC"},
            {         6, "TextureCompressionETC2"},
            {         7, "TextureCompressionASTC"},
            {         8, "IndirectFirstInstance"},
            {         9, "ShaderF16"},
            {        10, "RG11B10UfloatRenderable"},
            {        11, "BGRA8UnormStorage"},
            {        12, "Float32Filterable"},

            {0x000003E9, "DawnShaderFloat16"},
            {0x000003EA, "DawnInternalUsages"},
            {0x000003EB, "DawnMultiPlanarFormats"},
            {0x000003EC, "DawnNative"},
            {0x000003ED, "ChromiumExperimentalDp4a"},
            {0x000003EE, "TimestampQueryInsidePasses"},
            {0x000003EF, "ImplicitDeviceSynchronization"},
            {0x000003F0, "SurfaceCapabilities"},
            {0x000003F1, "TransientAttachments"},
            {0x000003F2, "MSAARenderToSingleSampled"},

            {0x7FFFFFFF, "Force32"},
        };

    std::vector<WGPUFeatureName> features;
    // First call for a size, second call for actual features list
    size_t featureCount = wgpuAdapterEnumerateFeatures(adapter, nullptr);
    features.resize(featureCount);
    wgpuAdapterEnumerateFeatures(adapter, features.data());

    std::cout << "Adapter features:" << std::endl;
    for (const auto& f : features)
    {
        std::string fs;
        if (featuresNames.count(f))
            fs = featuresNames.at(f);
        else
            fs = std::to_string(f);

        std::cout << " - " << fs << std::endl;
    }

    auto onDeviceLost = [](WGPUDeviceLostReason reason, char const * message, void * /*userdata*/)
    {
        std::cout << "Device is lost: reason " << reason;
        if (message)
        {
            std::cout << " (" << message << ")";
        }
        std::cout << std::endl;
    };

    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = "My Device"; // anything works here, that's your call
    deviceDesc.requiredFeaturesCount = 0; // we do not require any specific feature
    deviceDesc.requiredLimits = nullptr; // we do not require any specific limit
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = "The default queue";
#ifdef WEBGPU_BACKEND_WGPU
    deviceDesc.deviceLostCallback = onDeviceLost;
    deviceDesc.deviceLostUserdata = nullptr;
#endif
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

#ifdef WEBGPU_BACKEND_DAWN
    wgpuDeviceSetDeviceLostCallback(device, onDeviceLost, nullptr /*pUserData*/ );

    auto onDeviceLog = [](WGPULoggingType type, char const * message, void * /* userdata */)
    {
        std::cout << "Device log: type ";
        if (type < 4)
        {
            const std::map<int, std::string> logTypeNames =
            {
                { 0, "Verbose"},
                { 1, "Info"},
                { 2, "Warning"},
                { 3, "Error"},
            };
            std::cout << logTypeNames.at(type);
        }
        else
        {
            std::cout << type;
        }

        if (message)
        {
            std::cout << " (" << message << ")";
        }
        std::cout << std::endl;
    };

    wgpuDeviceSetLoggingCallback(device, onDeviceLog, nullptr /* pUserData*/);
#endif

    std::vector<WGPUFeatureName> deviceFeatures;
    int nDeviceFeatures = wgpuDeviceEnumerateFeatures(device, nullptr);
    deviceFeatures.resize(nDeviceFeatures);
    wgpuDeviceEnumerateFeatures(device, deviceFeatures.data());

    std::cout << "Adapter features:" << std::endl;
    for (const auto& f : features)
    {
        std::string fs;
        if (featuresNames.count(f))
            fs = featuresNames.at(f);
        else
            fs = std::to_string(f);

        std::cout << " - " << fs << std::endl;
    }

    WGPUQueue queue = wgpuDeviceGetQueue(device);

    auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void* /* pUserData */)
    {
        std::cout << "Queued work finished with status: " << status << std::endl;
    };

#ifdef WEBGPU_BACKEND_DAWN
    wgpuQueueOnSubmittedWorkDone(queue, /* signalValue -- no idea what it is */ 0, onQueueWorkDone, nullptr /* pUserData */);
#elif defined(WEBGPU_BACKEND_WGPU)
    wgpuQueueOnSubmittedWorkDone(queue, onQueueWorkDone, nullptr /* pUserData */);
#endif

    std::cout << "Queue: " << queue << std::endl;

    WGPUSwapChainDescriptor swapChainDesc = {};
    swapChainDesc.nextInChain = nullptr;
    swapChainDesc.width = 640;
    swapChainDesc.height = 480;
    swapChainDesc.format = WGPUTextureFormat_BGRA8Unorm;
    swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
    swapChainDesc.presentMode = WGPUPresentMode_Fifo;
    swapChainDesc.label = "Our swap chain";
    WGPUSwapChain swapChain = wgpuDeviceCreateSwapChain(device, surface, &swapChainDesc);
    std::cout << "Swapchain: " << swapChain << std::endl;

    std::cout << "Running frame loop..." << std::endl;
    while (!glfwWindowShouldClose(window))
    {
        // Check whether the user clicked on the close button (and any other
        // mouse/key event, which we don't use so far)
        glfwPollEvents();

        WGPUTextureView nextTexture = wgpuSwapChainGetCurrentTextureView(swapChain);

        if (!nextTexture)
        {
            std::cerr << "Cannot acquire next swap chain texture" << std::endl;
            break;
        }

        WGPUCommandEncoderDescriptor encoderDesc = {};
        encoderDesc.nextInChain = nullptr;
        encoderDesc.label = "My command encoder";
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

        //wgpuCommandEncoderInsertDebugMarker(encoder, "Do one thing");
        //wgpuCommandEncoderInsertDebugMarker(encoder, "Do another thing");

        WGPURenderPassDescriptor renderPassDesc = {};

        WGPURenderPassColorAttachment renderPassColorAttachment = {};
        renderPassColorAttachment.view = nextTexture;
        renderPassColorAttachment.resolveTarget = nullptr;
        renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
        renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
        renderPassColorAttachment.clearValue = WGPUColor{ 0.9, 0.1, 0.2, 1.0 };

        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &renderPassColorAttachment;
        renderPassDesc.depthStencilAttachment = nullptr;
        renderPassDesc.timestampWriteCount = 0;
        renderPassDesc.timestampWrites = nullptr;
        renderPassDesc.nextInChain = nullptr;

        WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
        wgpuRenderPassEncoderEnd(renderPass);

        wgpuTextureViewRelease(nextTexture);

        WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
        cmdBufferDescriptor.nextInChain = nullptr;
        cmdBufferDescriptor.label = "Command buffer";
        WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
        wgpuQueueSubmit(queue, 1, &commandBuffer);

        //wgpuCommandBufferRelease(commandBuffer);
        //wgpuCommandEncoderRelease(encoder);

        wgpuSwapChainPresent(swapChain);
    }

    wgpuSwapChainRelease(swapChain);

    wgpuSurfaceRelease(surface);

    wgpuQueueRelease(queue);

    wgpuDeviceRelease(device);

    wgpuAdapterRelease(adapter);

    glfwDestroyWindow(window);

    wgpuInstanceRelease(instance);

    return 0;
}