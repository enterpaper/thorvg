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

#ifndef _TVG_BGFX_TEXTURE_MGR_H_
#define _TVG_BGFX_TEXTURE_MGR_H_

#include "tvgBgfxContext.h"
#include "tvgInlist.h"

struct BgfxTextureEntry
{
    INLIST_ITEM(BgfxTextureEntry);
    bgfx::TextureHandle texture = BGFX_INVALID_HANDLE;
    uint32_t refCnt = 0;
    FilterMethod filter = FilterMethod::Bilinear;
};

struct BgfxTextureMgr
{
    /*
     * Zero-copy upload strategy (little-endian layouts):
     *  - ABGR8888/S surfaces store bytes (R,G,B,A) -> TextureFormat::RGBA8
     *  - ARGB8888/S surfaces store bytes (B,G,R,A) -> TextureFormat::BGRA8
     * Straight-alpha (S) sources are premultiplied once on upload; the shader
     * pipeline always works with premultiplied texels afterwards.
     */
    const BgfxTextureEntry* retain(BgfxContext& context, const RenderSurface* surface, FilterMethod filter, bool refreshTexture);
    void release(BgfxContext& context, const RenderSurface* surface, bgfx::TextureHandle texture);
    void clear(BgfxContext& context);

    struct SurfaceEntry
    {
        INLIST_ITEM(SurfaceEntry);
        const RenderSurface* surface = nullptr;
        tvg::Inlist<BgfxTextureEntry> textures;
    };

    SurfaceEntry* find(const RenderSurface* surface);
    static void upload(BgfxContext& context, BgfxTextureEntry& entry, const RenderSurface* surface, FilterMethod filter);
    static void releaseEntry(BgfxContext& context, BgfxTextureEntry& entry);
    static bgfx::TextureFormat::Enum textureFormat(const RenderSurface* surface);

    tvg::Inlist<SurfaceEntry> surfaces;
    uint16_t stamp = 1;
};

#endif //_TVG_BGFX_TEXTURE_MGR_H_
