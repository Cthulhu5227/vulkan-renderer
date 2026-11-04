## Vulkan Renderer

A small real-time renderer written from scratch in C++20 and the raw Vulkan 1.3 API. No engine and no
wrapper library. It loads a textured OBJ model and draws it spinning with lighting and depth testing, and
handles window resizing/minimizing.

![Spot the cow!](docs/spot.png)

*Keenan Crane's Spot model, about 5,900 triangles*

# Features

- **Modern Vulkan 1.3:** dynamic rendering (no render pass or framebuffer objects) and `synchronization2`
  (`vkCmdPipelineBarrier2`, `vkQueueSubmit2`).
- **Two frames in flight**, each with its own command buffer, fence, uniform buffer and descriptor set, so the CPU
  records the next frame while the GPU draws the current one.
- **Swap chain recreation** on resize and minimize. The pipeline survives because viewport and scissor are dynamic state.
- **Depth buffer** shared by both frames in flight. A barrier orders each frame's depth writes after the previous frame's.
- **OBJ loader** written for this project. Splits polygons into triangle fans.

# Design notes

A few decisions that took the most thought:

- `renderFinished` semaphores are per swap-chain image, not per frame in flight.
- The fence is reset *after* `vkAcquireNextImageKHR`, not before.
- One `transitionImage()` helper maps each layout transition (`UNDEFINED → COLOR_ATTACHMENT`,`TRANSFER_DST → SHADER_READ_ONLY`,...) 
- **Clip-space conventions:** GLM is set to Vulkan's 0..1 depth range, the projection's Y is flipped for Vulkan's
  Y-down clip space.


# Building (Windows)

The project builds with GCC from [MSYS2](https://www.msys2.org) (UCRT64) and a Windows install of CMake 3.20 or newer.

1. Install the dependencies from an MSYS2 UCRT64 shell:
   ```bash
   pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-glfw mingw-w64-ucrt-x86_64-glm mingw-w64-ucrt-x86_64-stb mingw-w64-ucrt-x86_64-shaderc mingw-w64-ucrt-x86_64-vulkan-headers mingw-w64-ucrt-x86_64-vulkan-loader mingw-w64-ucrt-x86_64-vulkan-validation-layers mingw-w64-ucrt-x86_64-vulkan-memory-allocator
   ```
2. Put `C:\msys64\ucrt64\bin` on your `PATH`.
3. Build and run from PowerShell. The script configures, builds, and starts the app with the validation layer enabled:
   ```powershell
   .\run.ps1
   ```

You need a GPU and driver that support Vulkan 1.3.

# What's next

Next I'm implementing **fur in the style of *Shadow of the Colossus***: shell texturing with fins at the silhouette,
following [Lengyel et al. 2001, "Real-Time Fur over Arbitrary Surfaces"](https://www.hhoppe.com/fur.pdf). 

## How this was built

I learned Vulkan on this project, following [vulkan-tutorial.com](https://vulkan-tutorial.com) and then moving to
Vulkan 1.3 features. I wrote the device and swap chain setup by hand. I used Claude as a tutor and
code reviewer.

## Credits

- Spot model and texture by [Keenan Crane](https://www.cs.cmu.edu/~kmcrane/Projects/ModelRepository/), public domain.
- [GLFW](https://www.glfw.org), [GLM](https://github.com/g-truc/glm), [stb_image](https://github.com/nothings/stb),
  [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator).
