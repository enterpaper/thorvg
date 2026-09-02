/*
 * Copyright (c) 2026 ThorVG project. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * BgfxCanvas minimal example.
 *
 * Demonstrates the thorvg bgfx backend:
 *   1. the application owns the bgfx lifecycle (bgfx::init / bgfx::frame),
 *   2. BgfxCanvas::target() reserves a bgfx view range and binds the present
 *      target (nullptr = bgfx back buffer),
 *   3. a static scene is rasterized every frame through draw() / sync().
 *
 * Expected output: a dark background, a blue->orange gradient rounded
 * rectangle, and an amber circle with a white stroke.
 */

#include <bgfx/bgfx.h>
#include <GLFW/glfw3.h>

#if defined(_WIN32)
    #define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
    #define GLFW_EXPOSE_NATIVE_COCOA
#else
    #define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>

#include <thorvg.h>

#define EXAMPLE_WIDTH 1280
#define EXAMPLE_HEIGHT 720


int main()
{
    // 1. Create a window with no client API (bgfx owns the graphics context)
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(EXAMPLE_WIDTH, EXAMPLE_HEIGHT, "thorvg bgfx example", nullptr, nullptr);

    // 2. Initialize bgfx with the native window handle
    bgfx::PlatformData platformData;
#if defined(_WIN32)
    platformData.nwh = glfwGetWin32Window(window);
#elif defined(__APPLE__)
    platformData.nwh = glfwGetCocoaWindow(window);
#else
    platformData.nwh = (void*)(uintptr_t)glfwGetX11Window(window);
#endif
    bgfx::Init init;
    init.platformData = platformData;
    init.resolution.width = EXAMPLE_WIDTH;
    init.resolution.height = EXAMPLE_HEIGHT;
    init.resolution.reset = BGFX_RESET_VSYNC;
    bgfx::init(init);

    // 3. Initialize ThorVG and create the bgfx canvas.
    //    Views [0, 64) are reserved for the canvas - the application
    //    must not use them for its own rendering passes.
    tvg::Initializer::init(0);

    auto canvas = tvg::BgfxCanvas::gen();
    if (!canvas) return -1;
    if (canvas->target({0, 64}, nullptr, EXAMPLE_WIDTH, EXAMPLE_HEIGHT, tvg::ColorSpace::ABGR8888) != tvg::Result::Success) return -1;

    // 4. Build a static scene: background + gradient rounded rect + stroked circle
    auto background = tvg::Shape::gen();
    background->appendRect(0, 0, EXAMPLE_WIDTH, EXAMPLE_HEIGHT);
    background->fill(34, 34, 42);
    canvas->add(background);

    auto gradientRect = tvg::Shape::gen();
    gradientRect->appendRect(160, 180, 420, 360, 40, 40);
    tvg::Fill::ColorStop stops[2] = {
        {0.0f,   0,  96, 165, 255},  // blue
        {1.0f, 255,  99,  72, 255},  // orange-red
    };
    auto gradient = tvg::LinearGradient::gen();
    gradient->linear(160, 180, 580, 540);
    gradient->colorStops(stops, 2);
    gradientRect->fill(gradient);  //ownership: the shape takes the gradient
    canvas->add(gradientRect);

    auto circle = tvg::Shape::gen();
    circle->appendCircle(880, 360, 160, 160);
    circle->fill(246, 196, 66);
    circle->strokeWidth(12.0f);
    circle->strokeFill(255, 255, 255);
    canvas->add(circle);

    // 5. Render loop: thorvg submits into bgfx views, bgfx::frame() presents
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        canvas->draw(true);  // update + render the scene (clears the scene target)
        canvas->sync();      // blit the rasterized scene into the present target
        bgfx::frame();       // execute the bgfx frame & present
    }

    // 6. Cleanup in reverse order: canvas, thorvg, bgfx, glfw
    delete canvas;  // releases the reserved views and offscreen targets
    tvg::Initializer::term();
    bgfx::shutdown();
    glfwTerminate();
    return 0;
}
