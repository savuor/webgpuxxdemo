# WebGPU C++ Demo

Experiments in the modern graphic API

Based on this tutorial: https://eliemichel.github.io/LearnWebGPU/index.html

# How to build
* Dawn: fix unused var by setting [[maybe_unused]]
* install this: `sudo apt-get install xcb libxcb-xkb-dev x11-xkb-utils libx11-xcb-dev libxkbcommon-x11-dev`

# How to run
On my machine there is an integrated Radeon card (AMD Ryzen 7) and a discrete nVidia RTX 3060.

By default it loads Radeon Vulkan impl. It skips 3 frames and freezes (Dawn) or dies with surface timeout error (wgpu-native).

There is a way to switch it to different impl: use an ICD. I have a choice of LVP (lavapipe) and nVidia ICDs. However, there are different problems here:

* Dawn:
  - both ICDs work in a separate console
  - debugging in VS Code works with LVP ICD only:
`VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json`
    By some reason setting nVidia ICD ends at `vkEnumerateInstanceExtensionProperties()` failed with `ERROR_INITIALIZATION_FAILED` error
* wgpu-native:
  - only nVidia ICD works in a separate console
  - in VS Code both LVP and nVidia ICDs fail at swap chain creation: `Surface does not support the adapter's queue family`

