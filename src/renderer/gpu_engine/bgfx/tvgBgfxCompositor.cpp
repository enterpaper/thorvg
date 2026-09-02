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

#include "thorvg.h"
#include "tvgBgfxCompositor.h"

#include <cmath>
#include <cstring>

/*
 * Mask methods resolved by fs_blit (values == tvg MaskMethod enum order):
 *   0 None, 1 Alpha, 2 InvAlpha, 3 Luma, 4 InvLuma,
 *   5 Add, 6 Subtract, 7 Intersect, 8 Difference, 9 Lighten, 10 Darken
 * Methods 5-10 need the destination color and go through the dst-texture path.
 */
static bool bgfxMaskNeedsDst(uint8_t method)
{
    return method >= (uint8_t)MaskMethod::Add;
}

/*
 * Blend methods that fs_blit evaluates with a destination texture. The value
 * is the u_blit.z blend id; 0.0 = Normal (fixed function). Non-separable HSL
 * modes (Hue..Luminosity) degrade to Normal.
 */
static float bgfxBlendId(BlendMethod method)
{
    if (method == BlendMethod::Normal) return 0.0f;
    if ((uint8_t)method >= (uint8_t)BlendMethod::Hue) {
        TVGLOG("BGFX_ENGINE", "HSL blend method %d degrades to Normal", (int)method);
        return 0.0f;
    }
    return (float)method;
}

static RenderRegion bgfxPaintRegion(const BBox& aabb, uint32_t w, uint32_t h)
{
    if (aabb.min.x > aabb.max.x || aabb.min.y > aabb.max.y) return {{0, 0}, {(int32_t)w, (int32_t)h}};
    return {{int32_t(aabb.min.x), int32_t(aabb.min.y)}, {int32_t(aabb.max.x), int32_t(aabb.max.y)}};
}


//***********************************************************************
// draw core
//***********************************************************************

void BgfxCompositor::draw(BgfxContext& context, const DrawParams& p)
{
    if (!p.mesh || p.mesh->ibuffer.empty() || !bgfx::isValid(p.program)) return;

    auto region = RenderRegion::intersect(p.scissor, {{0, 0}, {(int32_t)width, (int32_t)height}});
    if (!region.valid()) return;

    auto layout = BgfxVertex::layout();

    bgfx::TransientVertexBuffer tvb;
    bgfx::allocTransientVertexBuffer(&tvb, p.mesh->vbuffer.count, layout);
    std::memcpy(tvb.data, p.mesh->vbuffer.data, p.mesh->vbuffer.count * sizeof(BgfxVertex));

    bgfx::TransientIndexBuffer tib;
    bgfx::allocTransientIndexBuffer(&tib, p.mesh->ibuffer.count, true);  // meshes index uint32_t
    std::memcpy(tib.data, p.mesh->ibuffer.data, p.mesh->ibuffer.count * sizeof(uint32_t));

    float xform[16];
    bgfxPackViewportMatrix(xform, width, height);
    float depth[4] = {p.z, 0.0f, 0.0f, 0.0f};

    bgfx::setState(p.state);
    bgfx::setStencil(p.stencil.front, p.stencil.back);
    bgfx::setUniform(context.uniform(BgfxUniform::ViewportMatrix), xform);
    bgfx::setUniform(context.uniform(BgfxUniform::Depth), depth);
    if (p.paintMatrix) {
        float paint[16];
        bgfxPackMatrix(paint, *p.paintMatrix);
        bgfx::setUniform(context.uniform(BgfxUniform::PaintMatrix), paint);
    }
    if (p.color) bgfx::setUniform(context.uniform(BgfxUniform::Color), p.color);
    if (p.gradient) bgfx::setUniform(context.uniform(BgfxUniform::Gradient), p.gradient, 2);
    if (p.blit) bgfx::setUniform(context.uniform(BgfxUniform::Blit), p.blit);
    if (p.blur) bgfx::setUniform(context.uniform(BgfxUniform::Blur), p.blur);
    if (p.effect) {
        // fs_effect consumes up to 3 vec4 slots through one uniform handle
        bgfx::setUniform(context.uniform(BgfxUniform::Effect), p.effect, 3);
    }

    if (bgfx::isValid(p.ramp)) bgfx::setTexture(0, context.uniform(BgfxUniform::Ramp), p.ramp);
    if (bgfx::isValid(p.tex)) bgfx::setTexture(1, context.uniform(BgfxUniform::Tex), p.tex);
    if (bgfx::isValid(p.mask)) bgfx::setTexture(2, context.uniform(BgfxUniform::Mask), p.mask);
    if (bgfx::isValid(p.dst)) bgfx::setTexture(3, context.uniform(BgfxUniform::Dst), p.dst);

    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setIndexBuffer(&tib);
    bgfx::setScissor(uint16_t(region.min.x), uint16_t(region.min.y), uint16_t(region.sw()), uint16_t(region.sh()));

    bgfx::submit(view, p.program, order++);
}


void BgfxCompositor::drawQuad(BgfxContext& context, bgfx::ProgramHandle program, uint64_t state, BgfxStencilState stencil,
    float z, const RenderRegion& region, const float* color,
    bgfx::TextureHandle tex, bgfx::TextureHandle mask, bgfx::TextureHandle dst,
    const float* blit, const float* blur, const float* effect)
{
    BgfxMeshData quad;
    quad.bbox({0.0f, 0.0f}, {(float)width, (float)height});

    DrawParams p;
    p.program = program;
    p.mesh = &quad;
    p.state = state;
    p.stencil = stencil;
    p.z = z;
    p.scissor = region;
    p.color = color;
    p.tex = tex;
    p.mask = mask;
    p.dst = dst;
    p.blit = blit;
    p.blur = blur;
    p.effect = effect;
    draw(context, p);
}


void BgfxCompositor::drawQuadProgram(BgfxContext& context, BgfxProgram programId, uint64_t state, BgfxStencilState stencil,
    float z, const RenderRegion& region, const float* color,
    bgfx::TextureHandle tex, bgfx::TextureHandle mask, bgfx::TextureHandle dst,
    const float* blit, const float* blur, const float* effect)
{
    drawQuad(context, context.program(programId), state, stencil, z, region, color, tex, mask, dst, blit, blur, effect);
}


//***********************************************************************
// clips
//***********************************************************************

void BgfxCompositor::renderClipPath(BgfxContext& context, BgfxRenderDataPaint* paint)
{
    if (paint->clips.empty()) return;

    // the clip footprint is limited to the paint's own bounds (see tvgGlRenderer::drawClip)
    auto paintRegion = bgfxPaintRegion(paint->aabb, width, height);

    ARRAY_FOREACH(p, paint->clips) {
        auto clip = (BgfxRenderDataShape*)(*p);

        // 1. winding of the clip geometry into the stencil (no color, no depth)
        if (clip->meshStrokes.ibuffer.count > 0) {
            // stroke-only clip shape (its outline is already resolved by the stroker)
            draw(context, {context.program(BgfxProgram::Solid), &clip->meshStrokes, 0, bgfxStencilDirect(), BGFX_SHAPE_DEPTH_DEFAULT, paintRegion});
        } else if (clip->meshShape.ibuffer.count > 0) {
            auto stencil = (clip->fillRule == FillRule::NonZero) ? bgfxStencilWindingNonZero() : bgfxStencilWindingEvenOdd();
            draw(context, {context.program(BgfxProgram::Solid), &clip->meshShape, 0, stencil, BGFX_SHAPE_DEPTH_DEFAULT, paintRegion});
        }

        // 2. cover: mark the unclipped outside of the winding with a nearer depth
        //    and clear the stencil for the next clip
        draw(context, {context.program(BgfxProgram::Solid), &clip->meshBBox,
            bgfxStateDepthLess(true), bgfxStencilClipOutside(), 0.5f, paintRegion});
    }
}


void BgfxCompositor::clearClipPath(BgfxContext& context, BgfxRenderDataPaint* paint)
{
    if (paint->clips.empty()) return;

    auto paintRegion = bgfxPaintRegion(paint->aabb, width, height);

    // open the depth marks again so later paints are unaffected by this clip
    drawQuad(context, context.program(BgfxProgram::Solid), bgfxStateDepthAlways(true), bgfxStencilKeep(),
        BGFX_CLIP_DEPTH_BASE, paintRegion);
}


//***********************************************************************
// shapes & images
//***********************************************************************

void BgfxCompositor::renderShape(BgfxContext& context, BgfxRenderDataShape* renderData, BlendMethod blendMethod)
{
    if (renderData->viewport.invalid()) return;
    if (blendMethod != BlendMethod::Normal) {
        // per-shape advanced blending needs a destination copy; scenes get the
        // full implementation via composition (renderScene), shapes degrade.
        TVGLOG("BGFX_ENGINE", "per-shape blend method %d degrades to Normal", (int)blendMethod);
    }

    if (!renderData->clips.empty()) renderClipPath(context, renderData);

    auto fill = [&]() {
        if (renderData->renderSettingsShape.skip || renderData->meshShape.ibuffer.empty()) return;

        auto& settings = renderData->renderSettingsShape;
        float color[4] = {renderData->solidShape.color.r / 255.0f, renderData->solidShape.color.g / 255.0f,
            renderData->solidShape.color.b / 255.0f, renderData->solidShape.packedColor().a / 255.0f};

        auto fastPath = renderData->convex && settings.fillType == BgfxFillType::Solid && renderData->clips.empty();
        if (fastPath) {
            // convex + solid + unclipped: direct premultiplied draw
            draw(context, {context.program(BgfxProgram::Solid), &renderData->meshShape,
                bgfxStatePremultipliedBlend(), bgfxStencilKeep(), BGFX_SHAPE_DEPTH_DEFAULT,
                renderData->viewport, &settings.paintMatrix, color});
            return;
        }

        // 1. winding into the stencil
        auto stencil = (renderData->fillRule == FillRule::NonZero) ? bgfxStencilWindingNonZero() : bgfxStencilWindingEvenOdd();
        draw(context, {context.program(BgfxProgram::Solid), &renderData->meshShape,
            0, stencil, BGFX_SHAPE_DEPTH_DEFAULT, renderData->viewport, &settings.paintMatrix, color});

        // 2. cover: draw color where the stencil is set and clear it
        DrawParams cover;
        cover.mesh = &renderData->meshShapeBBox;
        cover.state = bgfxStatePremultipliedBlend();
        cover.stencil = bgfxStencilCover();
        cover.scissor = renderData->viewport;
        cover.paintMatrix = &settings.paintMatrix;
        cover.color = color;
        if (settings.fillType == BgfxFillType::Solid) {
            cover.program = context.program(BgfxProgram::Solid);
        } else {
            cover.program = context.program(BgfxProgram::Gradient);
            cover.gradient = &settings.gradient;
            cover.ramp = settings.ramp;
        }
        draw(context, cover);
    };

    auto stroke = [&]() {
        if (renderData->renderSettingsStroke.skip || renderData->meshStrokes.ibuffer.empty()) return;

        auto& settings = renderData->renderSettingsStroke;

        // stroke outline is a "direct" stencil marker (REPLACE 255)
        draw(context, {context.program(BgfxProgram::Solid), &renderData->meshStrokes,
            0, bgfxStencilDirect(), BGFX_SHAPE_DEPTH_DEFAULT, renderData->viewport});

        float color[4] = {renderData->solidStroke.color.r / 255.0f, renderData->solidStroke.color.g / 255.0f,
            renderData->solidStroke.color.b / 255.0f, renderData->solidStroke.packedColor().a / 255.0f};

        DrawParams cover;
        cover.mesh = &renderData->meshStrokesBBox;
        cover.state = bgfxStatePremultipliedBlend();
        cover.stencil = bgfxStencilCover();
        cover.scissor = renderData->viewport;
        cover.paintMatrix = &settings.paintMatrix;
        cover.color = color;
        if (settings.fillType == BgfxFillType::Solid) {
            cover.program = context.program(BgfxProgram::Solid);
        } else {
            cover.program = context.program(BgfxProgram::Gradient);
            cover.gradient = &settings.gradient;
            cover.ramp = settings.ramp;
        }
        draw(context, cover);
    };

    if (renderData->strokeFirst) {
        stroke();
        fill();
    } else {
        fill();
        stroke();
    }

    if (!renderData->clips.empty()) clearClipPath(context, renderData);
}


void BgfxCompositor::renderImage(BgfxContext& context, BgfxRenderDataPicture* renderData, BlendMethod blendMethod)
{
    if (renderData->viewport.invalid()) return;
    if (!bgfx::isValid(renderData->imageTexture) || renderData->meshData.ibuffer.empty()) return;

    if (!renderData->clips.empty()) renderClipPath(context, renderData);

    float color[4] = {0.0f, 0.0f, 0.0f, renderData->opacity / 255.0f};

    // images cover exactly their quad: no winding needed, clear leftover stencil
    draw(context, {context.program(BgfxProgram::Image), &renderData->meshData,
        bgfxStatePremultipliedBlend(), bgfxStencilClearOnly(), BGFX_SHAPE_DEPTH_DEFAULT,
        renderData->viewport, &renderData->renderSettings.paintMatrix, color, renderData->imageTexture});

    if (!renderData->clips.empty()) clearClipPath(context, renderData);
}


//***********************************************************************
// composition
//***********************************************************************

void BgfxCompositor::renderScene(BgfxContext& context, BgfxRenderTarget& src, BgfxCompose* compose)
{
    float blit[4] = {(float)MaskMethod::None, compose->opacity / 255.0f, bgfxBlendId(compose->blend), 0.0f};
    auto region = RenderRegion::intersect(compose->aabb, {{0, 0}, {(int32_t)width, (int32_t)height}});

    if (compose->blend == BlendMethod::Normal) {
        // fixed-function premultiplied source-over
        drawQuadProgram(context, BgfxProgram::Blit, bgfxStatePremultipliedBlend(), bgfxStencilKeep(),
            BGFX_SHAPE_DEPTH_DEFAULT, region, nullptr, src.texture, BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE, blit);
        return;
    }

    // advanced blend: copy the destination, evaluate the blend in the shader
    // and overwrite the area opaquely; degraded when no scratch target is
    // available or the channel renders to the back buffer
    if (!tempTarget0 || !bgfx::isValid(frameBuffer)) {
        blit[2] = 0.0f;
        drawQuadProgram(context, BgfxProgram::Blit, bgfxStatePremultipliedBlend(), bgfxStencilKeep(),
            BGFX_SHAPE_DEPTH_DEFAULT, region, nullptr, src.texture, BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE, blit);
        return;
    }

    bgfx::TextureRegion dstRegion, srcRegion;
    dstRegion.init(tempTarget0->texture);
    srcRegion.init(bgfx::getTexture(frameBuffer));
    bgfx::blit(view, dstRegion, srcRegion);

    blit[3] = 1.0f;  // dst texture bound
    drawQuadProgram(context, BgfxProgram::Blit, bgfxStateOpaqueWrite(), bgfxStencilKeep(),
        BGFX_SHAPE_DEPTH_DEFAULT, region, nullptr, src.texture, BGFX_INVALID_HANDLE, tempTarget0->texture, blit);
}


void BgfxCompositor::composeScene(BgfxContext& context, BgfxRenderTarget& src, BgfxRenderTarget& msk, BgfxCompose* compose)
{
    float blit[4] = {(float)compose->method, compose->opacity / 255.0f, bgfxBlendId(compose->blend), 0.0f};
    auto region = RenderRegion::intersect(compose->aabb, {{0, 0}, {(int32_t)width, (int32_t)height}});

    auto needsDst = bgfxMaskNeedsDst((uint8_t)compose->method) || (blit[2] != 0.0f);

    if (!needsDst) {
        // factor-based masks compose with fixed-function source-over
        drawQuadProgram(context, BgfxProgram::Blit, bgfxStatePremultipliedBlend(), bgfxStencilKeep(),
            BGFX_SHAPE_DEPTH_DEFAULT, region, nullptr, src.texture, msk.texture, BGFX_INVALID_HANDLE, blit);
        return;
    }

    if (!tempTarget0 || !bgfx::isValid(frameBuffer)) {
        // degrade: alpha-style composition without the destination
        blit[0] = (float)MaskMethod::Alpha;
        blit[2] = 0.0f;
        drawQuadProgram(context, BgfxProgram::Blit, bgfxStatePremultipliedBlend(), bgfxStencilKeep(),
            BGFX_SHAPE_DEPTH_DEFAULT, region, nullptr, src.texture, msk.texture, BGFX_INVALID_HANDLE, blit);
        return;
    }

    bgfx::TextureRegion dstRegion, srcRegion;
    dstRegion.init(tempTarget0->texture);
    srcRegion.init(bgfx::getTexture(frameBuffer));
    bgfx::blit(view, dstRegion, srcRegion);

    blit[3] = 1.0f;
    drawQuadProgram(context, BgfxProgram::Blit, bgfxStateOpaqueWrite(), bgfxStencilKeep(),
        BGFX_SHAPE_DEPTH_DEFAULT, region, nullptr, src.texture, msk.texture, tempTarget0->texture, blit);
}


void BgfxCompositor::blit(BgfxContext& context, BgfxRenderTarget& src, bool premultiplied)
{
    // present: the current channel is the back buffer / external frame buffer
    // (the renderer sets it up before calling)
    // the quad is built in viewport pixel space, like every other draw:
    // vs_shape always applies u_viewportMatrix (pixel -> NDC)
    float blit[4] = {(float)MaskMethod::None, 1.0f, 0.0f, premultiplied ? 0.0f : 1.0f};
    drawQuadProgram(context, BgfxProgram::Blit, bgfxStateOpaqueWrite(), bgfxStencilKeep(),
        BGFX_SHAPE_DEPTH_DEFAULT, {{0, 0}, {(int32_t)width, (int32_t)height}},
        nullptr, src.texture, BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE, blit);
}


//***********************************************************************
// post effects
//***********************************************************************

void BgfxCompositor::copyTarget(BgfxRenderTarget& src, BgfxRenderTarget& dst)
{
    bgfx::TextureRegion dstRegion, srcRegion;
    dstRegion.init(dst.texture);
    srcRegion.init(src.texture);
    bgfx::blit(view, dstRegion, srcRegion);
}


void BgfxCompositor::blurPass(BgfxContext& context, BgfxRenderTarget& src, BgfxRenderTarget& dst, float sigma, uint8_t direction)
{
    float blur[4] = {1.0f / src.width, 1.0f / src.height, (float)direction, sigma};
    drawQuadProgram(context, BgfxProgram::Blur, bgfxStateOpaqueWrite(), bgfxStencilKeep(),
        BGFX_SHAPE_DEPTH_DEFAULT, dst.viewport, nullptr, src.texture, BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE, nullptr, blur);
}
