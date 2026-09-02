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

#include "tvgBgfxTextureMgr.h"

bgfx::TextureFormat::Enum BgfxTextureMgr::textureFormat(const RenderSurface* surface)
{
    switch (surface->cs) {
        case ColorSpace::ABGR8888:
        case ColorSpace::ABGR8888S:
            return bgfx::TextureFormat::RGBA8;
        case ColorSpace::ARGB8888:
        case ColorSpace::ARGB8888S:
        default:
            return bgfx::TextureFormat::BGRA8;
    }
}


void BgfxTextureMgr::upload(BgfxContext& context, BgfxTextureEntry& entry, const RenderSurface* surface, FilterMethod filter)
{
    releaseEntry(context, entry);

    const auto straightAlpha = (surface->cs == ColorSpace::ABGR8888S || surface->cs == ColorSpace::ARGB8888S);
    const auto size = surface->stride * surface->h * surface->channelSize;

    pixel_t* data = surface->data;
    pixel_t* managed = nullptr;

    if (straightAlpha) {
        // premultiply straight alpha in a private copy, the source stays untouched
        managed = tvg::malloc<pixel_t>(size);
        for (uint32_t y = 0; y < surface->h; ++y) {
            auto src = surface->buf32 + y * surface->stride;
            auto dst = managed + y * surface->stride;
            for (uint32_t x = 0; x < surface->w; ++x, ++src, ++dst) {
                auto c = *src;
                auto a = (c >> 24) & 0xff;  // alpha stays in the top byte for both S color spaces
                auto c0 = MULTIPLY((c >> 16) & 0xff, a);
                auto c1 = MULTIPLY((c >> 8) & 0xff, a);
                auto c2 = MULTIPLY(c & 0xff, a);
                *dst = (c & 0xff000000) | (c0 << 16) | (c1 << 8) | c2;
            }
        }
        data = managed;
    }

    auto mem = bgfx::copy(data, size);
    tvg::free(managed);

    auto samplerFlags = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
    samplerFlags |= (filter == FilterMethod::Nearest) ? BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT : BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC;

    entry.texture = bgfx::createTexture2D(
        uint16_t(surface->w), uint16_t(surface->h), false, 1,
        textureFormat(surface), samplerFlags, mem);
    entry.filter = filter;
    entry.refCnt = 0;
}


const BgfxTextureEntry* BgfxTextureMgr::retain(BgfxContext& context, const RenderSurface* surface, FilterMethod filter, bool refreshTexture)
{
    if (!surface || surface->w == 0 || surface->h == 0) return nullptr;

    auto entry = find(surface);
    if (!entry) {
        entry = new SurfaceEntry;
        entry->surface = surface;
        surfaces.back(entry);
    }

    BgfxTextureEntry* texture = nullptr;
    INLIST_FOREACH(entry->textures, t) {
        if (t->filter == filter) {
            texture = t;
            break;
        }
    }

    // refresh the entry when the caller detects a content change (stamp bumped)
    if (!texture) {
        texture = new BgfxTextureEntry;
        upload(context, *texture, surface, filter);
        entry->textures.back(texture);
    } else if (refreshTexture) {
        upload(context, *texture, surface, filter);
    }

    texture->refCnt++;
    return texture;
}


void BgfxTextureMgr::release(BgfxContext& context, const RenderSurface* surface, bgfx::TextureHandle texture)
{
    if (!surface || !bgfx::isValid(texture)) return;

    INLIST_FOREACH(surfaces, p) {
        if (p->surface != surface) continue;
        INLIST_SAFE_FOREACH(p->textures, t) {
            if (t->texture.idx == texture.idx) {
                if (--t->refCnt == 0) {
                    releaseEntry(context, *t);
                    p->textures.remove(t);
                    delete t;
                }
                return;
            }
        }
    }
}


BgfxTextureMgr::SurfaceEntry* BgfxTextureMgr::find(const RenderSurface* surface)
{
    INLIST_FOREACH(surfaces, p) {
        if (p->surface == surface) return p;
    }
    return nullptr;
}


void BgfxTextureMgr::releaseEntry(BgfxContext& context, BgfxTextureEntry& entry)
{
    if (bgfx::isValid(entry.texture)) bgfx::destroy(entry.texture);
    entry.texture = BGFX_INVALID_HANDLE;
    entry.refCnt = 0;
}


void BgfxTextureMgr::clear(BgfxContext& context)
{
    INLIST_FOREACH(surfaces, p) {
        INLIST_SAFE_FOREACH(p->textures, t) {
            releaseEntry(context, *t);
            p->textures.remove(t);
            delete t;
        }
    }
    while (auto p = surfaces.front()) delete p;
    stamp++;
}
