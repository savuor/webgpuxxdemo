cmake_minimum_required(VERSION 3.0...3.25)
project( WebGpuDemo VERSION 0.1.0 LANGUAGES CXX C )

add_subdirectory(3rdparty/glfw-3.3.8)
add_subdirectory(3rdparty/dawn)
add_subdirectory(3rdparty/glfw3webgpu)

add_executable(application app.cpp)

target_link_libraries(application PRIVATE glfw webgpu glfw3webgpu)

set_target_properties(application PROPERTIES CXX_STANDARD 17 )

set_target_properties(application PROPERTIES VS_DEBUGGER_ENVIRONMENT "DAWN_DEBUG_BREAK_ON_ERROR=1")

if (MSVC)
    target_compile_options(application PRIVATE /W4)
else()
    target_compile_options(application PRIVATE -Wall -Wextra -pedantic)
endif()

if(XCODE)
    set_target_properties(application PROPERTIES
        XCODE_GENERATE_SCHEME ON
        XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal")
endif()
