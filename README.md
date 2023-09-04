# WebGPU C++ Demo

Experiments in the modern graphic API

Based on this tutorial: https://eliemichel.github.io/LearnWebGPU/index.html

# How to build
* Dawn: fix unused var by setting [[maybe_unused]]
* install this: `sudo apt-get install xcb libxcb-xkb-dev x11-xkb-utils libx11-xcb-dev libxkbcommon-x11-dev`

# How to run
On my machine there is an integrated Radeon card (AMD Ryzen 7) and a discrete nVidia RTX.
At first it tried to run on Radeon, skips 3 frames and freezes (Dawn) or dies with surface timeout error.
Switching to LVP ICD helped: added env variable at startup:
`VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json`
By some reason setting nVidia ICD results to no devices found.