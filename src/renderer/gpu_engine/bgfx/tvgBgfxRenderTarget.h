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

#ifndef _TVG_BGFX_RENDER_TARGET_H_
#define _TVG_BGFX_RENDER_TARGET_H_

#include "tvgBgfxContext.h"

struct BgfxRenderTarget
{
    bgfx::FrameBufferHandle frameBuffer = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle texture = BGFX_INVALID_HANDLE;       // color attachment, sampleable
    bgfx::TextureHandle depthStencil = BGFX_INVALID_HANDLE;  // D24S8 attachment
    uint32_t width = 0;
    uint32_t height = 0;
    RenderRegion viewport = {};

    /*
     * The depth/stencil attachment is explicit: bgfx only provides one
     * automatically for the back buffer, and the fill winding / stroke /
     * clip passes all resolve through the stencil and depth buffers.
     */
    void initialize(BgfxContext& context, uint32_t w, uint32_t h);
    void release(BgfxContext& context);
};


class BgfxRenderTargetPool
{
private:
    Array<BgfxRenderTarget*> list;   // all created targets
    Array<BgfxRenderTarget*> pool;   // currently unused targets
    uint32_t width = 0;
    uint32_t height = 0;
public:
    BgfxRenderTarget* allocate(BgfxContext& context);
    void free(BgfxContext& context, BgfxRenderTarget* renderTarget);

    void initialize(BgfxContext& context, uint32_t w, uint32_t h);
    void release(BgfxContext& context);
};
#endif //_TVG_BGFX_RENDER_TARGET_H_
