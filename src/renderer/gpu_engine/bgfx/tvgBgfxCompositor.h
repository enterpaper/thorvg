/*
 * Copyright (c) 2023 - 2026 ThorVG project. All rights reserved.

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

#ifndef _TVG_BGFX_COMPOSITOR_H_
#define _TVG_BGFX_COMPOSITOR_H_

#include "tvgBgfxContext.h"
#include "tvgBgfxRenderData.h"
#include "tvgBgfxRenderTarget.h"

/*
 * Effect payload kept alive between prepare(RenderEffect*) and the immediate
 * effect passes (the wg backend stages this in a uniform buffer, bgfx binds it
 * per draw).
 */
struct BgfxEffectData
{
    SceneEffect type{};      // effect selector (mirrors RenderEffect::type)
    float sigma = 0.0f;      // blur sigma in local units
    float scale = 1.0f;      // transform scaling factor
    float extend = 0.0f;     // region extension in target pixels
    Point offset = {0.0f, 0.0f};
    float color[4] = {0.0f, 0.0f, 0.0f, 0.0f};   // premultiplied (drop shadow, fill)
    float black[3] = {0.0f, 0.0f, 0.0f};
    float white[3] = {1.0f, 1.0f, 1.0f};
    float intensity = 0.0f;
    float midtone[3] = {0.5f, 0.5f, 0.5f};
    float highlight[3] = {1.0f, 1.0f, 1.0f};
    float blender = 1.0f;
};

/*
 * Composition record created by target() and driven by
 * beginComposite()/endComposite(). `pushes` counts the allocated render
 * targets: 2 when masking (mask target + content target), 1 otherwise.
 */
struct BgfxCompose : RenderCompositor
{
    RenderRegion aabb{};
    CompositionFlag flags = CompositionFlag::Invalid;
    BlendMethod blend = BlendMethod::Normal;
    int pushes = 0;
};

/*
 * Immediate-mode compositor: draws shapes/images and the composition/effect
 * passes straight into the current bgfx view. A "channel" (view id + frame
 * buffer) is assigned by the renderer whenever the render target changes.
 */
struct BgfxCompositor
{
    uint16_t view = 0;
    bgfx::FrameBufferHandle frameBuffer = BGFX_INVALID_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t order = 0;  // draw order sort key inside the view

    BgfxRenderTarget* tempTarget0 = nullptr;  // scratch for blends/effects
    BgfxRenderTarget* tempTarget1 = nullptr;
    BgfxRenderTarget* tempTarget2 = nullptr;

    void reset(TVG_UNUSED BgfxContext& context)
    {
        order = 0;
    }

    void setChannel(uint16_t v, bgfx::FrameBufferHandle fb, uint32_t w, uint32_t h)
    {
        view = v;
        frameBuffer = fb;
        width = w;
        height = h;
        order = 0;
    }

    // shapes & images
    void renderShape(BgfxContext& context, BgfxRenderDataShape* renderData, BlendMethod blendMethod);
    void renderImage(BgfxContext& context, BgfxRenderDataPicture* renderData, BlendMethod blendMethod);

    // clips (AND-accumulated in the depth buffer)
    void renderClipPath(BgfxContext& context, BgfxRenderDataPaint* paint);
    void clearClipPath(BgfxContext& context, BgfxRenderDataPaint* paint);

    // composition of an offscreen target onto the current channel
    void renderScene(BgfxContext& context, BgfxRenderTarget& src, BgfxCompose* compose);
    void composeScene(BgfxContext& context, BgfxRenderTarget& src, BgfxRenderTarget& msk, BgfxCompose* compose);
    // present the root scene into the current channel (back buffer / external FB)
    void blit(BgfxContext& context, BgfxRenderTarget& src, bool premultiplied);

    // effect/composition primitives used by the renderer orchestration
    void drawQuadProgram(BgfxContext& context, BgfxProgram programId, uint64_t state, BgfxStencilState stencil,
                         float z, const RenderRegion& region, const float* color = nullptr,
                         bgfx::TextureHandle tex = BGFX_INVALID_HANDLE, bgfx::TextureHandle mask = BGFX_INVALID_HANDLE,
                         bgfx::TextureHandle dst = BGFX_INVALID_HANDLE, const float* blit = nullptr,
                         const float* blur = nullptr, const float* effect = nullptr);
    // one separable blur pass between render targets
    void blurPass(BgfxContext& context, BgfxRenderTarget& src, BgfxRenderTarget& dst, float sigma, uint8_t direction);
    // gpu copy of the whole color attachment
    void copyTarget(BgfxRenderTarget& src, BgfxRenderTarget& dst);

private:
    struct DrawParams
    {
        bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
        const BgfxMeshData* mesh = nullptr;
        uint64_t state = 0;
        BgfxStencilState stencil;
        float z = BGFX_SHAPE_DEPTH_DEFAULT;
        RenderRegion scissor = {};
        const Matrix* paintMatrix = nullptr;      // uploads u_paintMatrix when set
        const float* color = nullptr;             // uploads u_color when set
        bgfx::TextureHandle tex = BGFX_INVALID_HANDLE;
        bgfx::TextureHandle ramp = BGFX_INVALID_HANDLE;
        const BgfxGradientParams* gradient = nullptr;
        bgfx::TextureHandle mask = BGFX_INVALID_HANDLE;
        bgfx::TextureHandle dst = BGFX_INVALID_HANDLE;
        const float* blit = nullptr;
        const float* blur = nullptr;
        const float* effect = nullptr;            // up to 3 vec4 slots
    };

    // uber draw: uploads the transient mesh + uniforms and submits into the channel
    void draw(BgfxContext& context, const DrawParams& p);

    // full-viewport helper pass over a bounding quad
    void drawQuad(BgfxContext& context, bgfx::ProgramHandle program, uint64_t state, BgfxStencilState stencil,
                  float z, const RenderRegion& region, const float* color = nullptr,
                  bgfx::TextureHandle tex = BGFX_INVALID_HANDLE, bgfx::TextureHandle mask = BGFX_INVALID_HANDLE,
                  bgfx::TextureHandle dst = BGFX_INVALID_HANDLE, const float* blit = nullptr,
                  const float* blur = nullptr, const float* effect = nullptr);
};

#endif //_TVG_BGFX_COMPOSITOR_H_
