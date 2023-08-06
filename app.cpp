#include <iostream>
#include <vector>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>

#define WEBGPU_BACKEND_DAWN

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


int main (int /* argc */, char** /* argv */)
{
    if (!glfwInit())
    {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return 1;
    }

    TerminatorGLFW terminatorGlfw;

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

    while (!glfwWindowShouldClose(window))
    {
        // Check whether the user clicked on the close button (and any other
        // mouse/key event, which we don't use so far)
        glfwPollEvents();
    }

    wgpuAdapterRelease(adapter);

    wgpuInstanceRelease(instance);

    glfwDestroyWindow(window);

    return 0;
}