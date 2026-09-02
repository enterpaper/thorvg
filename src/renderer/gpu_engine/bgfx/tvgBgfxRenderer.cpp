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

#include "tvgTaskScheduler.h"
#include "tvgMath.h"
#include "tvgBgfxRenderer.h"
#include <cmath>
#include <algorithm>
#include <cassert>

/************************************************************************/
/* Internal Class Implementation                                        */
/************************************************************************/

static int32_t _rendererCnt = -1;
static StrictKey _rendererMtx;

// channel z values: clips mark nearer, paints stay above the mark
#define BGFX_Z_CLIP_MARK 0.5f


void BgfxRenderer::release()
{
    if (mContext.invalid()) return;

    disposeObjects();
    mTextures.clear(mContext);

    mRenderTargetPool.release(mContext);
    mTempPool.release(mContext);
    mRenderTargetRoot.release(mContext);

    ARRAY_FOREACH(p, mEffectList) delete (*p);
    mEffectList.clear();
    mEffectPool.clear();

    mRenderTargetStack.clear();
    mCompositorList.clear();
    mCompositor.tempTarget0 = mCompositor.tempTarget1 = mCompositor.tempTarget2 = nullptr;

    mContext.release();
}


void BgfxRenderer::disposeObjects()
{
    ScopedLock lock(mDisposeKey);
    ARRAY_FOREACH(p, mDisposeRenderDatas) {
        auto renderData = (BgfxRenderDataPaint*)(*p);
        if (renderData->type() == Type::Shape) {
            mShapePool.free(mContext, (BgfxRenderDataShape*)renderData);
        } else {
            auto rdp = (BgfxRenderDataPicture*)renderData;
            rdp->releaseTexture(mTextures, mContext);
            mPicturePool.free(mContext, rdp);
        }
    }
    mDisposeRenderDatas.clear();
}


void BgfxRenderer::enterTarget(BgfxRenderTarget* renderTarget, bool clear)
{
    auto fb = renderTarget ? renderTarget->frameBuffer : mExternalFb;
    auto w = renderTarget ? renderTarget->width : mTargetSurface.w;
    auto h = renderTarget ? renderTarget->height : mTargetSurface.h;

    // reuse the current channel when the target stays the same
    if (!clear && mCompositor.frameBuffer.idx == fb.idx && mCompositor.width == w && mCompositor.height == h) return;

    auto view = mContext.allocView();
    mContext.setupView(view, fb, uint16_t(w), uint16_t(h), clear);
    mCompositor.setChannel(view, fb, w, h);
}


//***********************************************************************
/* post effects: orchestration over the compositor primitives           */
//***********************************************************************/

bool BgfxRenderer::applyEffect(BgfxRenderTarget& target, BgfxEffectData* ed)
{
    auto t0 = mCompositor.tempTarget0;
    auto t1 = mCompositor.tempTarget1;
    auto t2 = mCompositor.tempTarget2;

    switch (ed->type) {
        case SceneEffect::GaussianBlur: {
            auto sigma = ed->sigma * ed->scale;
            if (sigma <= 0.0f || !t0 || !t1) return false;
            mCompositor.copyTarget(target, *t0);
            enterTarget(t1, false);
            mCompositor.blurPass(mContext, *t0, *t1, sigma, 1);
            enterTarget(t0, false);
            mCompositor.blurPass(mContext, *t1, *t0, sigma, 2);
            enterTarget(&target, false);
            mCompositor.copyTarget(*t0, target);
            return true;
        }
        case SceneEffect::DropShadow: {
            if (!t0 || !t1 || !t2) return false;
            auto sigma = ed->sigma * ed->scale;
            mCompositor.copyTarget(target, *t2);
            mCompositor.copyTarget(target, *t0);
            if (sigma > 0.0f) {
                enterTarget(t1, false);
                mCompositor.blurPass(mContext, *t0, *t1, sigma, 1);
                enterTarget(t0, false);
                mCompositor.blurPass(mContext, *t1, *t0, sigma, 2);
            }
            enterTarget(&target, false);
            float mode[4] = {3.0f, 0.0f, 0.0f, 0.0f};  // fs_effect mode 3 = shadow
            float params[12] = {
                ed->color[0], ed->color[1], ed->color[2], ed->color[3],
                ed->offset.x / target.width, -ed->offset.y / target.height, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f,
            };
            mCompositor.drawQuadProgram(mContext, BgfxProgram::Effect, bgfxStateOpaqueWrite(), bgfxStencilKeep(),
                BGFX_SHAPE_DEPTH_DEFAULT, target.viewport, mode, t0->texture,
                BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE, nullptr, nullptr, params);
            mCompositor.drawQuadProgram(mContext, BgfxProgram::Blit, bgfxStatePremultipliedBlend(), bgfxStencilKeep(),
                BGFX_SHAPE_DEPTH_DEFAULT, target.viewport, nullptr, t2->texture);
            return true;
        }
        case SceneEffect::Fill: {
            if (!t0) return false;
            mCompositor.copyTarget(target, *t0);
            enterTarget(&target, false);
            float mode[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            float params[12] = {ed->color[0], ed->color[1], ed->color[2], ed->color[3], 0, 0, 0, 0, 0, 0, 0, 0};
            mCompositor.drawQuadProgram(mContext, BgfxProgram::Effect, bgfxStateOpaqueWrite(), bgfxStencilKeep(),
                BGFX_SHAPE_DEPTH_DEFAULT, target.viewport, mode, t0->texture,
                BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE, nullptr, nullptr, params);
            return true;
        }
        case SceneEffect::Tint: {
            if (!t0) return false;
            mCompositor.copyTarget(target, *t0);
            enterTarget(&target, false);
            float mode[4] = {1.0f, 0.0f, 0.0f, 0.0f};
            float params[12] = {ed->black[0], ed->black[1], ed->black[2], ed->intensity,
                ed->white[0], ed->white[1], ed->white[2], 0.0f, 0, 0, 0, 0};
            mCompositor.drawQuadProgram(mContext, BgfxProgram::Effect, bgfxStateOpaqueWrite(), bgfxStencilKeep(),
                BGFX_SHAPE_DEPTH_DEFAULT, target.viewport, mode, t0->texture,
                BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE, nullptr, nullptr, params);
            return true;
        }
        case SceneEffect::Tritone: {
            if (!t0) return false;
            mCompositor.copyTarget(target, *t0);
            enterTarget(&target, false);
            float mode[4] = {2.0f, 0.0f, 0.0f, 0.0f};
            float params[12] = {ed->black[0], ed->black[1], ed->black[2], 0.0f,
                ed->midtone[0], ed->midtone[1], ed->midtone[2], 0.0f,
                ed->highlight[0], ed->highlight[1], ed->highlight[2], ed->blender};
            mCompositor.drawQuadProgram(mContext, BgfxProgram::Effect, bgfxStateOpaqueWrite(), bgfxStencilKeep(),
                BGFX_SHAPE_DEPTH_DEFAULT, target.viewport, mode, t0->texture,
                BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE, nullptr, nullptr, params);
            return true;
        }
        default:
            TVGERR("BGFX_ENGINE", "unsupported effect type = %d", (int)ed->type);
            return false;
    }
}


BgfxEffectData* BgfxRenderer::effectData(RenderEffect* effect)
{
    if (!effect->rd) {
        BgfxEffectData* ed{};
        if (mEffectPool.count > 0) {
            ed = mEffectPool.last();
            mEffectPool.pop();
        } else {
            ed = new BgfxEffectData;
            mEffectList.push(ed);
        }
        effect->rd = ed;
    }
    auto ed = (BgfxEffectData*)effect->rd;
    ed->type = effect->type;
    return ed;
}


/************************************************************************/
/* External Class Implementation                                        */
/************************************************************************/

bool BgfxRenderer::preUpdate()
{
    if (mContext.invalid()) return false;
    return true;
}


RenderData BgfxRenderer::prepare(const RenderShape& rshape, RenderData data, const Matrix& transform, const Array<RenderData>& clips, uint8_t opacity, RenderUpdateFlag flags, bool clipper)
{
    auto rds = data ? (BgfxRenderDataShape*)data : mShapePool.allocate(mContext);

    // update geometry
    if (!data || (flags & (RenderUpdateFlag::Transform | RenderUpdateFlag::Path | RenderUpdateFlag::Stroke))) {
        rds->updateMeshes(rshape, flags, transform);
    }

    // update transform
    if ((!data) || (flags & RenderUpdateFlag::Transform)) {
        rds->transform = transform;
        rds->updateAABB();
    }

    // update paint settings
    if ((!data) || (flags & (RenderUpdateFlag::Transform | RenderUpdateFlag::Blend | RenderUpdateFlag::Color))) {
        rds->solidShape.opacity = rds->renderSettingsShape.updateOpacity(mTargetSurface.cs, opacity);
        rds->solidStroke.opacity = rds->renderSettingsStroke.updateOpacity(mTargetSurface.cs, opacity);
        rds->fillRule = rshape.rule;
    }

    // setup fill settings
    rds->viewport = vport;
    rds->updateVisibility(rshape, opacity);

    // update shape render settings
    if (!rds->renderSettingsShape.skip) {
        if (rshape.fill && (!data || (flags & (RenderUpdateFlag::Gradient | RenderUpdateFlag::Transform)))) {
            bool updateColorRamp = !data || ((flags & RenderUpdateFlag::Gradient) != RenderUpdateFlag::None);
            rds->renderSettingsShape.update(mContext, rshape.fill, &transform, updateColorRamp);
        } else if (!data || (flags & (RenderUpdateFlag::Color | RenderUpdateFlag::Gradient))) {
            rds->solidShape.color = rshape.color;
            rds->renderSettingsShape.fillType = BgfxFillType::Solid;
        }
    }

    // update strokes render settings
    if (rshape.stroke && !rds->renderSettingsStroke.skip) {
        if (rshape.stroke->fill && (!data || (flags & (RenderUpdateFlag::GradientStroke | RenderUpdateFlag::Transform)))) {
            bool updateColorRamp = !data || ((flags & RenderUpdateFlag::GradientStroke) != RenderUpdateFlag::None);
            rds->renderSettingsStroke.update(mContext, rshape.stroke->fill, nullptr, updateColorRamp);
        } else if (!data || (flags & (RenderUpdateFlag::Stroke | RenderUpdateFlag::GradientStroke))) {
            rds->solidStroke.color = rshape.stroke->color;
            rds->renderSettingsStroke.fillType = BgfxFillType::Solid;
        }
    }

    if (flags & RenderUpdateFlag::Clip) rds->updateClips(clips);

    return rds;
}


RenderData BgfxRenderer::prepare(RenderSurface* surface, RenderData data, const Matrix& transform, const Array<RenderData>& clips, uint8_t opacity, FilterMethod filter, RenderUpdateFlag flags)
{
    auto rdp = data ? (BgfxRenderDataPicture*)data : mPicturePool.allocate(mContext);

    // update paint settings
    rdp->viewport = vport;
    rdp->transform = transform;
    if (!data || (flags & (RenderUpdateFlag::Blend | RenderUpdateFlag::Color))) {
        rdp->opacity = rdp->renderSettings.updateOpacity(surface->cs, opacity);
    }

    auto updateSurface = !data || (flags & (RenderUpdateFlag::Transform | RenderUpdateFlag::Path | RenderUpdateFlag::Image));
    if (updateSurface) rdp->updateSurface(surface, transform);

    // reload texture
    auto cacheStale = bgfx::isValid(rdp->imageTexture) && (rdp->imageStamp != mTextures.stamp);
    auto refreshTexture = ((flags & (RenderUpdateFlag::Path | RenderUpdateFlag::Image)) != RenderUpdateFlag::None);
    auto needsImage = !bgfx::isValid(rdp->imageTexture) || (rdp->imageSource != surface) || (rdp->imageFilter != filter) || refreshTexture || cacheStale;
    if (needsImage) {
        rdp->releaseTexture(mTextures, mContext);
        auto* entry = mTextures.retain(mContext, surface, filter, refreshTexture);
        if (entry) rdp->setImage(entry->texture, surface, filter, mTextures.stamp);
        else rdp->clearImage();
    }

    if (flags & RenderUpdateFlag::Clip) rdp->updateClips(clips);

    return rdp;
}


bool BgfxRenderer::postUpdate()
{
    return true;
}


bool BgfxRenderer::preRender()
{
    if (mContext.invalid()) return false;

    mCompositor.reset(mContext);
    mCompositor.tempTarget0 = mTempPool.allocate(mContext);
    mCompositor.tempTarget1 = mTempPool.allocate(mContext);
    mCompositor.tempTarget2 = mTempPool.allocate(mContext);
    mContext.resetViews();

    assert(mRenderTargetStack.count == 0);
    mRenderTargetStack.push(&mRenderTargetRoot);

    // root scene target; the root content is always refreshed
    enterTarget(&mRenderTargetRoot, true);

    // create root compose settings
    auto compose = new BgfxCompose();
    compose->aabb = {{0, 0}, {(int32_t)mTargetSurface.w, (int32_t)mTargetSurface.h}};
    compose->blend = BlendMethod::Normal;
    compose->method = MaskMethod::None;
    compose->opacity = 255;
    mCompositorList.push(compose);

    return true;
}


bool BgfxRenderer::renderShape(RenderData data)
{
    if (mContext.invalid() || !data) return false;
    mCompositor.renderShape(mContext, (BgfxRenderDataShape*)data, mBlendMethod);
    return true;
}


bool BgfxRenderer::renderImage(RenderData data)
{
    if (mContext.invalid() || !data) return false;
    mCompositor.renderImage(mContext, (BgfxRenderDataPicture*)data, mBlendMethod);
    return true;
}


bool BgfxRenderer::postRender()
{
    if (mRenderTargetStack.count > 0) mRenderTargetStack.pop();

    // recycle the effect/blend scratch targets back to the pool
    if (mCompositor.tempTarget0) { mTempPool.free(mContext, mCompositor.tempTarget0); mCompositor.tempTarget0 = nullptr; }
    if (mCompositor.tempTarget1) { mTempPool.free(mContext, mCompositor.tempTarget1); mCompositor.tempTarget1 = nullptr; }
    if (mCompositor.tempTarget2) { mTempPool.free(mContext, mCompositor.tempTarget2); mCompositor.tempTarget2 = nullptr; }

    ARRAY_FOREACH(p, mCompositorList) delete (*p);
    mCompositorList.clear();
    return true;
}


void BgfxRenderer::dispose(RenderData data)
{
    if (!data) return;
    ScopedLock lock(mDisposeKey);
    mDisposeRenderDatas.push(data);
}


RenderRegion BgfxRenderer::region(RenderData data)
{
    if (!data) return {};
    auto renderData = (BgfxRenderDataPaint*)data;
    if (renderData->type() == Type::Shape) {
        auto& v1 = renderData->aabb.min;
        auto& v2 = renderData->aabb.max;
        return {{int32_t(nearbyint(v1.x)), int32_t(nearbyint(v1.y))}, {int32_t(nearbyint(v2.x)), int32_t(nearbyint(v2.y))}};
    }
    return {{0, 0}, {(int32_t)mTargetSurface.w, (int32_t)mTargetSurface.h}};
}


bool BgfxRenderer::bounds(RenderData data, Point* pt4, const Matrix& m)
{
    if (data) {
        auto renderDataPaint = (BgfxRenderDataPaint*)data;
        if (renderDataPaint->type() == Type::Shape) {
            auto renderData = (BgfxRenderDataShape*)data;
            if (!renderData->renderSettingsStroke.skip) {
                tvg::BBox bbox;
                bbox.init();
                auto& vertexes = renderData->meshStrokes.vbuffer;

                for (uint32_t i = 0; i < vertexes.count; i++) {
                    Point vert = {vertexes[i].x, vertexes[i].y};
                    vert *= m;
                    bbox.min = min(bbox.min, vert);
                    bbox.max = max(bbox.max, vert);
                }

                pt4[0] = bbox.min;
                pt4[1] = {bbox.max.x, bbox.min.y};
                pt4[2] = bbox.max;
                pt4[3] = {bbox.min.x, bbox.max.y};
                return true;
            }
        }
    }
    return false;
}


bool BgfxRenderer::blend(BlendMethod method)
{
    mBlendMethod = (method == BlendMethod::Composition ? BlendMethod::Normal : method);
    return true;
}


ColorSpace BgfxRenderer::colorSpace()
{
    return mTargetSurface.cs;
}


const RenderSurface* BgfxRenderer::mainSurface()
{
    return &mTargetSurface;
}


bool BgfxRenderer::clear()
{
    mClearBuffer = true;
    return true;
}


bool BgfxRenderer::sync()
{
    if (mContext.invalid()) return false;

    disposeObjects();

    auto targetSize = mRenderTargetRoot.width > 0 && mRenderTargetRoot.height > 0;
    if (!targetSize) return false;

    // present the root scene into the back buffer / external frame buffer
    enterPresent();
    mCompositor.blit(mContext, mRenderTargetRoot, mTargetSurface.premultiplied);

    return true;
}


Result BgfxRenderer::target(const BgfxCanvas::Context& ctx, void* target, uint32_t w, uint32_t h, ColorSpace cs)
{
    // teardown request from the BgfxCanvas destructor (explicit empty context)
    if (!ctx.viewCount) {
        release();
        return Result::Success;
    }

    if (cs != ColorSpace::ABGR8888 && cs != ColorSpace::ABGR8888S && cs != ColorSpace::ARGB8888 && cs != ColorSpace::ARGB8888S) {
        return Result::NonSupport;
    }

    if (w == 0 || h == 0) return Result::InvalidArguments;
    if (!bgfx::getCaps()) {
        TVGERR("BGFX_ENGINE", "bgfx is not initialized. Call bgfx::init() before BgfxCanvas::target()");
        return Result::Unknown;
    }

    // context has been changed, recreate all instances
    if (mContext.viewCount != ctx.viewCount || mContext.viewBase != ctx.viewBase) {
        release();
        if (!mContext.initialize(ctx.viewBase, ctx.viewCount)) return Result::Unknown;
        mRenderTargetPool.initialize(mContext, w, h);
        mTempPool.initialize(mContext, w, h);
        mRenderTargetRoot.initialize(mContext, w, h);
    } else if ((mTargetSurface.w != w) || (mTargetSurface.h != h)) {
        mRenderTargetPool.release(mContext);
        mTempPool.release(mContext);
        mRenderTargetRoot.release(mContext);
        mRenderTargetPool.initialize(mContext, w, h);
        mTempPool.initialize(mContext, w, h);
        mRenderTargetRoot.initialize(mContext, w, h);
    }

    mTargetSurface.setup(nullptr, w, w, h, CHANNEL_SIZE(cs), cs);
    mTargetSurface.premultiplied = (cs == ColorSpace::ABGR8888 || cs == ColorSpace::ARGB8888);

    // present target: 0 = back buffer, 1 = external frame buffer
    // note: BGFX_INVALID_HANDLE expands to a braced-init-list, which is not
    // an expression and cannot appear in a ternary branch — construct the
    // handle type explicitly instead.
    mExternalFb = (target) ? *(bgfx::FrameBufferHandle*)target : bgfx::FrameBufferHandle{ bgfx::kInvalidHandle };

    return Result::Success;
}


RenderCompositor* BgfxRenderer::target(const RenderRegion& region, TVG_UNUSED ColorSpace cs, TVG_UNUSED CompositionFlag flags)
{
    // create and setup compose data
    auto compose = new BgfxCompose();
    compose->aabb = region;
    compose->flags = flags;
    mCompositorList.push(compose);
    return compose;
}


bool BgfxRenderer::beginComposite(RenderCompositor* cmp, MaskMethod method, uint8_t opacity)
{
    auto compose = (BgfxCompose*)cmp;
    if (!compose) return false;

    // allocate a render target and push it to the stack
    auto allocate = [&]() {
        auto renderTarget = mRenderTargetPool.allocate(mContext);
        mRenderTargetStack.push(renderTarget);
        enterTarget(renderTarget, true);
        compose->pushes++;
        return renderTarget;
    };

    if ((compose->flags & CompositionFlag::Masking) && compose->pushes == 0) {
        // first call: the mask content pass renders into its own target
        allocate();
        return true;
    }

    // second call (masking) or single call (blending / opacity / post processing)
    auto content = allocate();
    compose->method = method;
    compose->opacity = opacity;
    compose->blend = (method == MaskMethod::None) ? mBlendMethod : BlendMethod::Normal;
    (void)content;
    return true;
}


bool BgfxRenderer::endComposite(RenderCompositor* cmp)
{
    auto compose = (BgfxCompose*)cmp;
    if (!compose || compose->pushes == 0) return false;

    auto content = mRenderTargetStack.last();
    mRenderTargetStack.pop();
    compose->pushes--;

    if (compose->pushes > 0) {
        auto mask = mRenderTargetStack.last();
        mRenderTargetStack.pop();
        compose->pushes--;

        // composite the masked content onto the parent target
        enterTarget(mRenderTargetStack.count > 0 ? mRenderTargetStack.last() : nullptr, false);
        mCompositor.composeScene(mContext, *content, *mask, compose);
        mRenderTargetPool.free(mContext, mask);
    } else {
        enterTarget(mRenderTargetStack.count > 0 ? mRenderTargetStack.last() : nullptr, false);
        mCompositor.renderScene(mContext, *content, compose);
    }

    mRenderTargetPool.free(mContext, content);
    return true;
}


void BgfxRenderer::prepare(RenderEffect* effect, const Matrix& transform)
{
    auto ed = effectData(effect);
    if (!ed) return;

    switch (effect->type) {
        case SceneEffect::GaussianBlur: {
            auto gaussian = (RenderEffectGaussianBlur*)effect;
            ed->sigma = gaussian->sigma;
            ed->scale = std::sqrt(transform.e11 * transform.e11 + transform.e12 * transform.e12);
            ed->extend = 4.0f * ed->sigma * ed->scale;  // 2x kernel radius
            gaussian->valid = ed->extend > 0;
            break;
        }
        case SceneEffect::DropShadow: {
            auto dropShadow = (RenderEffectDropShadow*)effect;
            ed->scale = std::sqrt(transform.e11 * transform.e11 + transform.e12 * transform.e12);
            auto radian = tvg::deg2rad(90.0f - dropShadow->angle) - tvg::radian(transform);
            ed->offset = {dropShadow->distance * cosf(radian) * ed->scale, -dropShadow->distance * sinf(radian) * ed->scale};
            ed->sigma = dropShadow->sigma;
            auto alpha = dropShadow->color[3] / 255.0f;
            // premultiplied to avoid the multiplication in the fragment shader
            ed->color[0] = dropShadow->color[0] / 255.0f * alpha;
            ed->color[1] = dropShadow->color[1] / 255.0f * alpha;
            ed->color[2] = dropShadow->color[2] / 255.0f * alpha;
            ed->color[3] = alpha;
            ed->extend = 2.0f * std::max(ed->sigma * ed->scale + std::abs(ed->offset.x), ed->sigma * ed->scale + std::abs(ed->offset.y));
            dropShadow->valid = (ed->extend >= 0);
            break;
        }
        case SceneEffect::Fill: {
            auto fill = (RenderEffectFill*)effect;
            ed->color[0] = fill->color[0] / 255.0f;
            ed->color[1] = fill->color[1] / 255.0f;
            ed->color[2] = fill->color[2] / 255.0f;
            ed->color[3] = fill->color[3] / 255.0f;
            fill->valid = true;
            break;
        }
        case SceneEffect::Tint: {
            auto tint = (RenderEffectTint*)effect;
            ed->black[0] = tint->black[0] / 255.0f;
            ed->black[1] = tint->black[1] / 255.0f;
            ed->black[2] = tint->black[2] / 255.0f;
            ed->white[0] = tint->white[0] / 255.0f;
            ed->white[1] = tint->white[1] / 255.0f;
            ed->white[2] = tint->white[2] / 255.0f;
            ed->intensity = tint->intensity / 255.0f;
            tint->valid = (tint->intensity > 0);
            break;
        }
        case SceneEffect::Tritone: {
            auto tritone = (RenderEffectTritone*)effect;
            ed->black[0] = tritone->shadow[0] / 255.0f;
            ed->black[1] = tritone->shadow[1] / 255.0f;
            ed->black[2] = tritone->shadow[2] / 255.0f;
            ed->midtone[0] = tritone->midtone[0] / 255.0f;
            ed->midtone[1] = tritone->midtone[1] / 255.0f;
            ed->midtone[2] = tritone->midtone[2] / 255.0f;
            ed->highlight[0] = tritone->highlight[0] / 255.0f;
            ed->highlight[1] = tritone->highlight[1] / 255.0f;
            ed->highlight[2] = tritone->highlight[2] / 255.0f;
            ed->blender = tritone->blender / 255.0f;
            tritone->valid = tritone->blender < 255;
            break;
        }
        default:
            TVGERR("BGFX_ENGINE", "Missing effect type? = %d", (int)effect->type);
            return;
    }
}


bool BgfxRenderer::region(RenderEffect* effect)
{
    auto ed = (BgfxEffectData*)effect->rd;
    if (!ed) return false;

    if (effect->type == SceneEffect::GaussianBlur) {
        auto gaussian = (RenderEffectGaussianBlur*)effect;
        if (gaussian->direction != 2) {
            gaussian->extend.min.x = -ed->extend;
            gaussian->extend.max.x = +ed->extend;
        }
        if (gaussian->direction != 1) {
            gaussian->extend.min.y = -ed->extend;
            gaussian->extend.max.y = +ed->extend;
        }
        return true;
    } else if (effect->type == SceneEffect::DropShadow) {
        auto dropShadow = (RenderEffectDropShadow*)effect;
        dropShadow->extend.min.x = -std::ceil(ed->extend + std::abs(ed->offset.x));
        dropShadow->extend.min.y = -std::ceil(ed->extend + std::abs(ed->offset.y));
        dropShadow->extend.max.x = +std::floor(ed->extend + std::abs(ed->offset.x));
        dropShadow->extend.max.y = +std::floor(ed->extend + std::abs(ed->offset.y));
        return true;
    }
    return false;
}


bool BgfxRenderer::render(RenderCompositor* cmp, const RenderEffect* effect, TVG_UNUSED bool direct)
{
    if (mRenderTargetStack.empty() || !effect || !effect->rd) return false;
    auto ed = (BgfxEffectData*)effect->rd;
    (void)cmp;
    return applyEffect(*mRenderTargetStack.last(), ed);
}


void BgfxRenderer::dispose(RenderEffect* effect)
{
    auto ed = (BgfxEffectData*)effect->rd;
    if (ed) mEffectPool.push(ed);
    effect->rd = nullptr;
};


void BgfxRenderer::damage(TVG_UNUSED RenderData rd, TVG_UNUSED const RenderRegion& region)
{
    //TODO: partial rendering is not supported yet
}


bool BgfxRenderer::partial(TVG_UNUSED bool disable)
{
    //TODO: partial rendering is not supported yet
    return false;
}


bool BgfxRenderer::intersectsShape(RenderData data, TVG_UNUSED const RenderRegion& region)
{
    if (!data) return false;
    auto shape = (BgfxRenderDataShape*)data;
    RenderRegion bbox = {
        {(int32_t)shape->aabb.min.x, (int32_t)shape->aabb.min.y},
        {(int32_t)shape->aabb.max.x, (int32_t)shape->aabb.max.y}
    };
    if (region.intersected(bbox)) {
        if (region.contained(bbox)) return true;
        BgfxIntersector intersector;
        return intersector.intersectShape(RenderRegion::intersect(region, bbox), shape);
    }
    return false;
}


bool BgfxRenderer::intersectsImage(RenderData data, TVG_UNUSED const RenderRegion& region)
{
    if (!data) return false;
    auto picture = (BgfxRenderDataPicture*)data;
    BgfxIntersector intersector;
    if (intersector.intersectImage(region, picture)) return true;
    return false;
}


void BgfxRenderer::enterPresent()
{
    enterTarget(nullptr, false);
}


BgfxRenderer::BgfxRenderer(TVG_UNUSED uint32_t threads, TVG_UNUSED EngineOption op)
{
    _rendererMtx.lock();
    if (_rendererCnt == -1) {
        //TODO: initialize the global engine
        _rendererCnt = 0;
    }
    ++_rendererCnt;
    _rendererMtx.unlock();
}


BgfxRenderer::~BgfxRenderer()
{
    release();

    _rendererMtx.lock();
    --_rendererCnt;
    _rendererMtx.unlock();
}


bool BgfxRenderer::term()
{
    _rendererMtx.lock();
    if (_rendererCnt > 0) {
        _rendererMtx.unlock();
        return false;
    }
    _rendererCnt = -1;
    _rendererMtx.unlock();

    //TODO: clean up global resources

    return true;
}
