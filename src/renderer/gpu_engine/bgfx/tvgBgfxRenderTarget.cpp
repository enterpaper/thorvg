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

#include "tvgBgfxRenderTarget.h"


void BgfxRenderTarget::initialize(BgfxContext& context, uint32_t w, uint32_t h)
{
    release(context);

    width = w;
    height = h;

    texture = bgfx::createTexture2D(
        uint16_t(w), uint16_t(h), false, 1, bgfx::TextureFormat::BGRA8,
        BGFX_TEXTURE_RT | BGFX_TEXTURE_BLIT_DST | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
    if (!bgfx::isValid(texture)) {
        TVGERR("BGFX_ENGINE", "failed to create render target color texture (%ux%u)", w, h);
        return;
    }

    depthStencil = bgfx::createTexture2D(
        uint16_t(w), uint16_t(h), false, 1, bgfx::TextureFormat::D24S8, BGFX_TEXTURE_RT_WRITE_ONLY);
    if (!bgfx::isValid(depthStencil)) {
        TVGERR("BGFX_ENGINE", "failed to create render target depth/stencil texture (%ux%u)", w, h);
        bgfx::destroy(texture);
        texture = BGFX_INVALID_HANDLE;
        return;
    }

    const bgfx::TextureHandle attachments[2] = { texture, depthStencil };
    frameBuffer = bgfx::createFrameBuffer(2, attachments, false);
    if (!bgfx::isValid(frameBuffer)) {
        TVGERR("BGFX_ENGINE", "failed to create frame buffer (%ux%u)", w, h);
        bgfx::destroy(depthStencil);
        depthStencil = BGFX_INVALID_HANDLE;
        bgfx::destroy(texture);
        texture = BGFX_INVALID_HANDLE;
        return;
    }

    viewport = {{0, 0}, {int32_t(w), int32_t(h)}};
}


void BgfxRenderTarget::release(BgfxContext& context)
{
    if (bgfx::isValid(frameBuffer)) bgfx::destroy(frameBuffer);
    // attachments were created with destroyTextures = false
    if (bgfx::isValid(texture)) bgfx::destroy(texture);
    if (bgfx::isValid(depthStencil)) bgfx::destroy(depthStencil);
    frameBuffer = BGFX_INVALID_HANDLE;
    texture = BGFX_INVALID_HANDLE;
    depthStencil = BGFX_INVALID_HANDLE;
    width = height = 0;
    viewport = {};
}


BgfxRenderTarget* BgfxRenderTargetPool::allocate(BgfxContext& context)
{
    if (pool.count > 0) {
        auto renderTarget = pool.last();
        pool.pop();
        return renderTarget;
    }

    auto renderTarget = new BgfxRenderTarget;
    renderTarget->initialize(context, width, height);
    list.push(renderTarget);
    return renderTarget;
}


void BgfxRenderTargetPool::free(BgfxContext& context, BgfxRenderTarget* renderTarget)
{
    if (!renderTarget) return;
    pool.push(renderTarget);
}


void BgfxRenderTargetPool::initialize(BgfxContext& context, uint32_t w, uint32_t h)
{
    release(context);
    width = w;
    height = h;
}


void BgfxRenderTargetPool::release(BgfxContext& context)
{
    ARRAY_FOREACH(p, list) {
        (*p)->release(context);
        delete (*p);
    }
    list.clear();
    pool.clear();
    width = height = 0;
}
