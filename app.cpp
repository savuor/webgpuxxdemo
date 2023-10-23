#include <iostream>
#include <vector>
#include <map>
#include <chrono>
#include <thread>

#include <GLFW/glfw3.h>

// turn on if you want old C-style interface
//#include <webgpu/webgpu.h>
#include <webgpu/webgpu.hpp>
#include <glfw3webgpu.h>

struct TerminatorGLFW
{
    ~TerminatorGLFW()
    {
        glfwTerminate();
    }
};


wgpu::Adapter requestAdapter(wgpu::Instance instance, const wgpu::RequestAdapterOptions& options)
{
    struct UserData
    {
        wgpu::Adapter adapter = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    //TODO: promise & future for wait
    instance.requestAdapter(options, [&userData](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, char const * message) -> void
    {
        if (status == wgpu::RequestAdapterStatus::Success)
        {
            userData.adapter = adapter;
        } else
        {
            std::cout << "Could not get WebGPU adapter: " << message << std::endl;
        }
        userData.requestEnded = true;
    });

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


wgpu::Device requestDevice(wgpu::Adapter adapter, const wgpu::DeviceDescriptor& descriptor)
{
    struct UserData
    {
        wgpu::Device device = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    //TODO: promise&future for wait
    adapter.requestDevice(descriptor, [&userData](wgpu::RequestDeviceStatus status, wgpu::Device device, char const * message) -> void
    {
        if (status == wgpu::RequestDeviceStatus::Success)
        {
            userData.device = device;
        }
        else
        {
            std::cout << "Could not get WebGPU device: " << message << std::endl;
        }
        userData.requestEnded = true;
    });

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

    wgpu::InstanceDescriptor desc;
    wgpu::Instance instance = wgpu::createInstance(desc);
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

    wgpu::Surface surface = glfwGetWGPUSurface(instance, window);

    wgpu::RequestAdapterOptions adapterOpts;
    adapterOpts.setDefault();
    wgpu::Adapter adapter = requestAdapter(instance, adapterOpts);

    if (!adapter)
    {
        std::cerr << "Could not request an adapter!" << std::endl;
        return 1;
    }

    std::cout << "WGPU adapter: " << adapter << std::endl;

    const std::map<wgpu::FeatureName, std::string> featuresNames =
    {
        {wgpu::FeatureName::Undefined,                     "Undefined"},
        {wgpu::FeatureName::DepthClipControl,              "DepthClipControl"},
        {wgpu::FeatureName::Depth32FloatStencil8,          "Depth32FloatStencil8"},
        {wgpu::FeatureName::TimestampQuery,                "TimestampQuery"},
        {wgpu::FeatureName::PipelineStatisticsQuery,       "PipelineStatisticsQuery"},
        {wgpu::FeatureName::TextureCompressionBC,          "TextureCompressionBC"},
        {wgpu::FeatureName::TextureCompressionETC2,        "TextureCompressionETC2"},
        {wgpu::FeatureName::TextureCompressionASTC,        "TextureCompressionASTC"},
        {wgpu::FeatureName::IndirectFirstInstance,         "IndirectFirstInstance"},
        {wgpu::FeatureName::ShaderF16,                     "ShaderF16"},
        {wgpu::FeatureName::RG11B10UfloatRenderable,       "RG11B10UfloatRenderable"},
        {wgpu::FeatureName::BGRA8UnormStorage,             "BGRA8UnormStorage"},
        {wgpu::FeatureName::Float32Filterable,             "Float32Filterable"},

#ifdef WEBGPU_BACKEND_DAWN
        {wgpu::FeatureName::DawnShaderFloat16,             "DawnShaderFloat16"},
        {wgpu::FeatureName::DawnInternalUsages,            "DawnInternalUsages"},
        {wgpu::FeatureName::DawnMultiPlanarFormats,        "DawnMultiPlanarFormats"},
        {wgpu::FeatureName::DawnNative,                    "DawnNative"},
        {wgpu::FeatureName::ChromiumExperimentalDp4a,      "ChromiumExperimentalDp4a"},
        {wgpu::FeatureName::TimestampQueryInsidePasses,    "TimestampQueryInsidePasses"},
        {wgpu::FeatureName::ImplicitDeviceSynchronization, "ImplicitDeviceSynchronization"},
        {wgpu::FeatureName::SurfaceCapabilities,           "SurfaceCapabilities"},
        {wgpu::FeatureName::TransientAttachments,          "TransientAttachments"},
        {wgpu::FeatureName::MSAARenderToSingleSampled,     "MSAARenderToSingleSampled"},
#endif

        {wgpu::FeatureName::Force32, "Force32"},
    };

    std::vector<wgpu::FeatureName> features;
    // First call for a size, second call for actual features list
    size_t featureCount = adapter.enumerateFeatures(nullptr);
    features.resize(featureCount, wgpu::FeatureName::Undefined);
    adapter.enumerateFeatures(features.data());

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

    wgpu::DeviceDescriptor deviceDesc;
    deviceDesc.setDefault();

    deviceDesc.label = "My Device"; // anything works here, that's your call
    deviceDesc.requiredFeaturesCount = 0; // we do not require any specific feature
    deviceDesc.requiredLimits = nullptr; // we do not require any specific limit
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = "The default queue";

    auto onDeviceLost = [](wgpu::DeviceLostReason reason, char const * message)
    {
        std::map<wgpu::DeviceLostReason, std::string> reasonNames =
        {
            {wgpu::DeviceLostReason::Undefined, "Undefined"},
            {wgpu::DeviceLostReason::Destroyed, "Destroyed"},

            {wgpu::DeviceLostReason::Force32,   "Force32"}
        };

        std::string rs;
        if (reasonNames.count(reason))
        {
            rs = reasonNames.at(reason);
        }
        else
        {
            rs = std::to_string(reason);
        }

        std::cout << "Device is lost: reason " << rs;
        if (message)
        {
            std::cout << " (" << message << ")";
        }
        std::cout << std::endl;
    };

#ifdef WEBGPU_BACKEND_WGPU
    auto deviceLostHandle = std::make_unique<wgpu::DeviceLostCallback>(onDeviceLost);
    auto deviceLostCallback = [](WGPUDeviceLostReason reason, char const * message, void * userdata) -> void
    {
        wgpu::DeviceLostCallback& callback = *reinterpret_cast<wgpu::DeviceLostCallback*>(userdata);
        callback(static_cast<wgpu::DeviceLostReason>(reason), message);
    };
    deviceDesc.deviceLostCallback = deviceLostCallback;
    deviceDesc.deviceLostUserdata = reinterpret_cast<void*>(deviceLostHandle.get());
#endif

    wgpu::Device device = requestDevice(adapter, deviceDesc);

    std::cout << "Got device: " << device << std::endl;

    device.setUncapturedErrorCallback([](wgpu::ErrorType type, char const* message)
    {
        std::cout << "Uncaptured device error: type " << type;
        if (message)
        {
            std::cout << " (" << message << ")";
        }
        std::cout << std::endl;
    });

#ifdef WEBGPU_BACKEND_DAWN
    device.setDeviceLostCallback(onDeviceLost);

    device.setLoggingCallback([](wgpu::LoggingType type, char const *message)
    {
        const std::map<wgpu::LoggingType, std::string> logTypeNames =
        {
            {wgpu::LoggingType::Verbose, "Verbose"},
            {wgpu::LoggingType::Info,    "Info"},
            {wgpu::LoggingType::Warning, "Warning"},
            {wgpu::LoggingType::Error,   "Error"},

            {wgpu::LoggingType::Force32, "Force32"}
        };
        std::string mt;
        if (logTypeNames.count(type))
        {
            mt = logTypeNames.at(type);
        }
        else
        {
            mt = std::to_string(type);
        }

        std::cout << "Device log: type " << mt;

        if (message)
        {
            std::cout << " (" << message << ")";
        }
        std::cout << std::endl;
    });

#endif

    std::vector<wgpu::FeatureName> deviceFeatures;

    int nDeviceFeatures = device.enumerateFeatures(nullptr);
    deviceFeatures.resize(nDeviceFeatures, wgpu::FeatureName::Undefined);
    device.enumerateFeatures(deviceFeatures.data());

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

    wgpu::Queue queue = device.getQueue();

    auto onQueueWorkDone = [](wgpu::QueueWorkDoneStatus status)
    {
        std::string st;
        std::map<wgpu::QueueWorkDoneStatus, std::string> queueWdsNames =
        {
            {wgpu::QueueWorkDoneStatus::Success,    "Success"},
            {wgpu::QueueWorkDoneStatus::Error,      "Error"},
            {wgpu::QueueWorkDoneStatus::Unknown,    "Unknown"},
            {wgpu::QueueWorkDoneStatus::DeviceLost, "DeviceLost"},
            
            {wgpu::QueueWorkDoneStatus::Force32,    "Force32"}
        };
        if (queueWdsNames.count(status))
        {
            st = queueWdsNames.at(status);
        }
        else
        {
            st = std::to_string(status);
        }

        std::cout << "Queued work finished with status: " << st << std::endl;
    };

#ifdef WEBGPU_BACKEND_DAWN
    queue.onSubmittedWorkDone(/* signalValue -- no idea what it is */ 0, onQueueWorkDone);
#elif defined(WEBGPU_BACKEND_WGPU)
    queue.onSubmittedWorkDone(onQueueWorkDone);
#endif

    std::cout << "Queue: " << queue << std::endl;

    wgpu::SwapChainDescriptor swapChainDesc;
    swapChainDesc.setDefault();

    swapChainDesc.width = 640;
    swapChainDesc.height = 480;
    swapChainDesc.format = wgpu::TextureFormat::BGRA8Unorm;
    swapChainDesc.usage = wgpu::TextureUsage::RenderAttachment;
    swapChainDesc.presentMode = wgpu::PresentMode::Fifo;
    swapChainDesc.label = "Our swap chain";
    wgpu::SwapChain swapChain = device.createSwapChain(surface, swapChainDesc);

    std::cout << "Swapchain: " << swapChain << std::endl;

    std::cout << "Running frame loop..." << std::endl;

    int nFrame = 0;
    while (!glfwWindowShouldClose(window))
    {
        // Check whether the user clicked on the close button (and any other
        // mouse/key event, which we don't use so far)
        glfwPollEvents();

        wgpu::TextureView nextTexture = swapChain.getCurrentTextureView();

        if (!nextTexture)
        {
            std::cerr << "Cannot acquire next swap chain texture" << std::endl;
            break;
        }

        // Turn it on if you need it
        //std::cout << "frame #" << nFrame << std::endl;

        wgpu::CommandEncoderDescriptor encoderDesc;
        encoderDesc.label = "My command encoder";
        wgpu::CommandEncoder encoder = device.createCommandEncoder(encoderDesc);

        // encoder.insertDebugMarker("Do one thing");
        // encoder.insertDebugMarker("Do another thing");

        wgpu::RenderPassDescriptor renderPassDesc;
        wgpu::RenderPassColorAttachment renderPassColorAttachment;
        renderPassColorAttachment.view = nextTexture;
        renderPassColorAttachment.resolveTarget = nullptr;
        renderPassColorAttachment.loadOp = wgpu::LoadOp::Clear;
        renderPassColorAttachment.storeOp = wgpu::StoreOp::Store;
        renderPassColorAttachment.clearValue = wgpu::Color{ 0.9, 0.1, 0.2, 1.0 };

        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &renderPassColorAttachment;
        renderPassDesc.depthStencilAttachment = nullptr;
        renderPassDesc.timestampWriteCount = 0;
        renderPassDesc.timestampWrites = nullptr;
        renderPassDesc.nextInChain = nullptr;

        wgpu::RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);
        renderPass.end();

        nextTexture.release();

        wgpu::CommandBufferDescriptor cmdBufferDescriptor;
        cmdBufferDescriptor.label = "Command buffer";

        wgpu::CommandBuffer commandBuffer = encoder.finish(cmdBufferDescriptor);
        // as an option, a vector of commandBuffers can be submitted
        queue.submit(commandBuffer);

        //commandBuffer.release();
        //encoder.release();

        swapChain.present();

        nFrame++;
    }

    swapChain.release();

    surface.release();

    queue.release();

    device.release();

    adapter.release();

    glfwDestroyWindow(window);

    instance.release();

    return 0;
}