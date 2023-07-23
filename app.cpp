#include <iostream>
#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>


struct TerminatorGLFW
{
    ~TerminatorGLFW()
    {
        glfwTerminate();
    }
};


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

    while (!glfwWindowShouldClose(window))
    {
        // Check whether the user clicked on the close button (and any other
        // mouse/key event, which we don't use so far)
        glfwPollEvents();
    }

    wgpuInstanceRelease(instance);

    glfwDestroyWindow(window);

    return 0;
}