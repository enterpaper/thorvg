/*
 * Copyright (c) 2020 - 2026 ThorVG project. All rights reserved.

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

#ifndef _TVG_BGFX_RENDERER_H_
#define _TVG_BGFX_RENDERER_H_

#include "tvgBgfxCompositor.h"
#include "tvgBgfxRenderTarget.h"
#include "tvgBgfxRenderData.h"
#include "tvgBgfxTextureMgr.h"

struct BgfxRenderer : RenderMethod
{
    //main features
    bool preUpdate() override;
    RenderData prepare(const RenderShape& rshape, RenderData data, const Matrix& transform, const Array<RenderData>& clips, uint8_t opacity, RenderUpdateFlag flags, bool clipper) override;
    RenderData prepare(RenderSurface* surface, RenderData data, const Matrix& transform, const Array<RenderData>& clips, uint8_t opacity, FilterMethod filter, RenderUpdateFlag flags) override;
    bool postUpdate() override;
    bool preRender() override;
    bool renderShape(RenderData data) override;
    bool renderImage(RenderData data) override;
    bool postRender() override;
    void dispose(RenderData data) override;
    RenderRegion region(RenderData data) override;
    bool bounds(RenderData data, Point* pt4, const Matrix& m) override;
    bool blend(BlendMethod method) override;
    ColorSpace colorSpace() override;
    const RenderSurface* mainSurface() override;
    bool clear() override;
    bool sync() override;
    bool intersectsImage(RenderData data, const RenderRegion& region) override;
    bool intersectsShape(RenderData data, const RenderRegion& region) override;

    // called by BgfxCanvas::target(); target == nullptr → back buffer,
    // otherwise a pointer to a bgfx::FrameBufferHandle
    Result target(const BgfxCanvas::Context& ctx, void* target, uint32_t w, uint32_t h, ColorSpace cs);

    //composition
    RenderCompositor* target(const RenderRegion& region, ColorSpace cs, CompositionFlag flags) override;
    bool beginComposite(RenderCompositor* cmp, MaskMethod method, uint8_t opacity) override;
    bool endComposite(RenderCompositor* cmp) override;

    //post effects
    void prepare(RenderEffect* effect, const Matrix& transform) override;
    bool region(RenderEffect* effect) override;
    bool render(RenderCompositor* cmp, const RenderEffect* effect, bool direct) override;
    void dispose(RenderEffect* effect) override;

    //partial rendering
    void damage(RenderData rd, const RenderRegion& region) override;
    bool partial(bool disable) override;

    BgfxRenderer(uint32_t threads, EngineOption op);
    ~BgfxRenderer() override;

    static bool term();

private:
    void release();
    void disposeObjects();

    // switch the compositor channel when the render target changes
    void enterTarget(BgfxRenderTarget* renderTarget, bool clear);
    // channel for the present pass (back buffer when the fb is invalid)
    void enterPresent();

    // post effects orchestration over the compositor primitives
    bool applyEffect(BgfxRenderTarget& target, BgfxEffectData* ed);
    BgfxEffectData* effectData(RenderEffect* effect);

    BgfxContext mContext;
    BgfxCompositor mCompositor;

    BgfxRenderTarget mRenderTargetRoot;         // offscreen scene
    BgfxRenderTargetPool mRenderTargetPool;     // composition targets
    BgfxRenderTargetPool mTempPool;             // effect/blend scratch
    BgfxTextureMgr mTextures;

    BgfxRenderDataShapePool mShapePool;
    BgfxRenderDataPicturePool mPicturePool;

    Array<BgfxRenderTarget*> mRenderTargetStack;  // content & mask targets
    Array<BgfxCompose*> mCompositorList;
    Array<RenderData> mDisposeRenderDatas;
    Array<BgfxEffectData*> mEffectList;           // all created effect data
    Array<BgfxEffectData*> mEffectPool;           // unused effect data
    Key mDisposeKey;

    RenderSurface mTargetSurface;
    BlendMethod mBlendMethod = BlendMethod::Normal;
    bool mClearBuffer = false;

    // present configuration (from BgfxCanvas::target)
    bgfx::FrameBufferHandle mExternalFb = BGFX_INVALID_HANDLE;
};

#endif //_TVG_BGFX_RENDERER_H_
