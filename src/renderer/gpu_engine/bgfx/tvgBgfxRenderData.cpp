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

#include <algorithm>
#include <cmath>
#include "tvgCommon.h"
#include "tvgFill.h"
#include "tvgGpuCommon.h"
#include "tvgBgfxTessellator.h"
#include "tvgBgfxRenderData.h"

//***********************************************************************
// BgfxRenderSettings
//***********************************************************************

uint8_t BgfxRenderSettings::updateOpacity(tvg::ColorSpace cs, uint8_t opacity)
{
    return static_cast<uint8_t>(opacity * opacityMultiplier);
}


void BgfxRenderSettings::update(BgfxContext& context, const Fill* fill, const Matrix* modelTransform, bool updateColorRamp)
{
    if (!fill) return;

    // geometry(local) -> gradient space: fill transform inverse, composed with
    // the inverse model transform (both gradients and vertices stay in local space)
    Matrix invTransform;
    if (inverse(&fill->transform(), &invTransform)) {
        Matrix invModel;
        if (modelTransform && inverse(modelTransform, &invModel)) invTransform = invTransform * invModel;
    } else invTransform = tvg::identity();
    paintMatrix = invTransform;

    if (fill->type() == Type::LinearGradient) {
        float x0, y0, x1, y1;
        ((LinearGradient*)fill)->linear(&x0, &y0, &x1, &y1);
        auto dx = x1 - x0, dy = y1 - y0;
        auto lenSq = dx * dx + dy * dy;
        gradient.p0[0] = x0;
        gradient.p0[1] = y0;
        gradient.p0[2] = (lenSq > 0.0f) ? 1.0f / lenSq : 0.0f;
        gradient.p0[3] = (float)fill->spread();
        gradient.p1[0] = dx;
        gradient.p1[1] = dy;
        gradient.p1[2] = 0.0f;
        gradient.p1[3] = 0.0f;
        fillType = BgfxFillType::Linear;
    } else if (fill->type() == Type::RadialGradient) {
        float cx, cy, r, fx, fy, fr;
        ((RadialGradient*)fill)->radial(&cx, &cy, &r, &fx, &fy, &fr);
        CONST_RADIAL(fill)->correct(fx, fy, fr);
        // two-circle interpolation, resolved by fs_gradient (wg backend math)
        gradient.p0[0] = cx;
        gradient.p0[1] = cy;
        gradient.p0[2] = r;
        gradient.p0[3] = (float)fill->spread();
        gradient.p1[0] = fx;
        gradient.p1[1] = fy;
        gradient.p1[2] = fr;
        gradient.p1[3] = 1.0f;  // radial selector (fs_gradient)
        fillType = BgfxFillType::Radial;
    }

    if (updateColorRamp) buildRamp(context, fill);
}


bool BgfxRenderSettings::buildRamp(BgfxContext& context, const Fill* fill)
{
    const Fill::ColorStop* stops = nullptr;
    auto stopCnt = fill->colorStops(&stops);
    if (stopCnt == 0) return false;

    static Array<Fill::ColorStop> sstops(stopCnt);
    sstops.clear();
    sstops.push(stops[0]);

    // filter by increasing offset
    for (uint32_t i = 1; i < stopCnt; i++) {
        if (sstops.last().offset < stops[i].offset) sstops.push(stops[i]);
        else if (sstops.last().offset == stops[i].offset) sstops.last() = stops[i];
    }

    auto rampSize = BGFX_TEXTURE_GRADIENT_SIZE;
    auto data = tvg::malloc<uint8_t>(rampSize * 4);

    auto assign = [](uint8_t* dst, const Fill::ColorStop& color) {
        std::memcpy(dst, &color.r, 4);
    };

    // head
    uint32_t range_s = 0;
    uint32_t range_e = uint32_t(sstops[0].offset * (rampSize - 1));
    auto dst = data + range_s * 4;
    for (uint32_t ti = range_s; (ti < range_e) && (ti < rampSize); ti++, dst += 4) {
        assign(dst, sstops[0]);
    }

    // body
    for (uint32_t di = 1; di < sstops.count; di++) {
        range_s = uint32_t(sstops[di - 1].offset * (rampSize - 1));
        range_e = uint32_t(sstops[di - 0].offset * (rampSize - 1));
        float delta = 1.0f/(range_e - range_s);
        dst = data + range_s * 4;
        for (uint32_t ti = range_s; (ti < range_e) && (ti < rampSize); ti++, dst += 4) {
            assign(dst, tvg::lerp(sstops[di - 1], sstops[di], (ti - range_s) * delta));
        }
    }

    // tail
    const tvg::Fill::ColorStop& colorStopLast = sstops.last();
    range_s = uint32_t(colorStopLast.offset * (rampSize - 1));
    range_e = rampSize;
    dst = data + range_s * 4;
    for (uint32_t ti = range_s; ti < range_e; ti++, dst += 4) {
        assign(dst, colorStopLast);
    }

    // spread modes are resolved by sampler flags baked into the ramp texture
    uint64_t flags = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC;
    if (fill->spread() == FillSpread::Reflect) flags = BGFX_SAMPLER_U_MIRROR | BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC;
    else if (fill->spread() == FillSpread::Repeat) flags = BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC;

    auto mem = bgfx::copy(data, rampSize * 4);
    tvg::free(data);

    release(context);
    ramp = bgfx::createTexture2D(uint16_t(rampSize), 1, false, 1, bgfx::TextureFormat::RGBA8, flags, mem);
    if (!bgfx::isValid(ramp)) {
        TVGERR("BGFX_ENGINE", "failed to create gradient ramp texture");
        return false;
    }
    return true;
}


void BgfxRenderSettings::release(BgfxContext& context)
{
    if (bgfx::isValid(ramp)) bgfx::destroy(ramp);
    ramp = BGFX_INVALID_HANDLE;
}

//***********************************************************************
// BgfxRenderDataPaint
//***********************************************************************

void BgfxRenderDataPaint::release(BgfxContext& context)
{
    clips.clear();
};

void BgfxRenderDataPaint::updateClips(const Array<RenderData>& clips)
{
    this->clips.clear();
    // RenderData == BgfxRenderDataPaint*, just copy it.
    this->clips = *((Array<BgfxRenderDataPaint*>*)&clips);
}

//***********************************************************************
// BgfxRenderDataShape
//***********************************************************************

void BgfxRenderDataShape::updateBBox(BBox bb)
{
    bbox.min = tvg::min(bbox.min, bb.min);
    bbox.max = tvg::max(bbox.max, bb.max);
}


void BgfxRenderDataShape::updateVisibility(const RenderShape& rshape, uint8_t opacity)
{
    renderSettingsShape.skip = (rshape.color.a * opacity == 0) && (!rshape.fill);
    renderSettingsStroke.skip = rshape.stroke ? (rshape.stroke->color.a * opacity == 0) && (!rshape.stroke->fill) : true;
}


void BgfxRenderDataShape::updateMeshes(const RenderShape &rshape, RenderUpdateFlag flag, const Matrix& matrix)
{
    releaseMeshes();  //Optimize: bad idea to reset meshes always. it could re-use the meshes if there haven't been any path changes.

    convex = false;
    strokeFirst = rshape.strokeFirst();
    renderSettingsShape.opacityMultiplier = 1.0f;
    renderSettingsStroke.opacityMultiplier = 1.0f;

    // optimize path
    auto& optPath = RenderPath::scratch();
    RenderPath optStrokePath;
    bool optPathThin = false;
    bool optPathSkipFill = false;
    auto strokeWidth = rshape.strokeWidth();
    auto localOut = (std::isfinite(strokeWidth) && !tvg::zero(strokeWidth)) ? &optStrokePath : nullptr;
    if (rshape.trimpath()) {
        auto& trimmed = RenderPath::scratch();
        if (rshape.stroke->trim.trim(rshape.path, trimmed)) {
            GpuOptimizeResult result{&optPath, localOut};
            gpuOptimize(trimmed, result, matrix);
            optPathThin = result.thin;
            optPathSkipFill = result.skipFill;
        }
        else optPath.clear();
    } else {
        GpuOptimizeResult result{&optPath, localOut};
        gpuOptimize(rshape.path, result, matrix);
        optPathThin = result.thin;
        optPathSkipFill = result.skipFill;
    }

    auto updatePath = flag & (RenderUpdateFlag::Transform | RenderUpdateFlag::Path);

    // update fill shapes
    if (updatePath || (flag & (RenderUpdateFlag::Color | RenderUpdateFlag::Gradient))) {
        if (optPathSkipFill) {
            // Too-thin fills are suppressed instead of going through thin fallback.
            meshShape.clear();
        } else {
            BBox bbox;
            // Drawable thin fills are tessellated as a minimal-width stroke.
            if (optPathThin && tvg::zero(rshape.strokeWidth())) {
                BgfxStroker stroker(&meshShape, MIN_BGFX_STROKE_WIDTH, StrokeCap::Butt, StrokeJoin::Bevel);
                stroker.run(optPath);
                bbox = stroker.getBBox();
                renderSettingsShape.opacityMultiplier = MIN_BGFX_STROKE_ALPHA;
            } else {
                BgfxBWTessellator bwTess{&meshShape};
                bwTess.tessellate(optPath);
                convex = bwTess.convex;
                bbox = bwTess.getBBox();
            }
            if (meshShape.ibuffer.empty()) {
                meshShape.clear();
            } else {
                meshShapeBBox.bbox(bbox.min, bbox.max);
                updateBBox(bbox);
            }
        }
    }
    // update strokes shapes
    if (rshape.stroke && (updatePath || (flag & (RenderUpdateFlag::Stroke | RenderUpdateFlag::GradientStroke)))) {
        auto qualityScale = scaling(matrix);
        auto strokeWidthWorld = strokeWidth * qualityScale;
        if (!std::isfinite(strokeWidthWorld)) strokeWidthWorld = strokeWidth;
        if (!std::isfinite(strokeWidthWorld)) strokeWidthWorld = 0.0f;
        if (!std::isfinite(qualityScale) || tvg::zero(qualityScale)) qualityScale = 1.0f;

        //run stroking only if it's valid
        if (!tvg::zero(strokeWidthWorld)) {
            BgfxStroker stroker(&meshStrokes, strokeWidth, rshape.strokeCap(), rshape.strokeJoin(), rshape.strokeMiterlimit(), qualityScale);
            auto& dashed = RenderPath::scratch();
            if (gpuStrokeDash(rshape, dashed, nullptr)) stroker.run(dashed);
            else stroker.run(optStrokePath);
            renderSettingsStroke.opacityMultiplier = 1.0f;
            if (meshStrokes.ibuffer.empty()) {
                meshStrokes.clear();
            } else {
                auto bbox = stroker.getBBox();
                meshStrokesBBox.bbox(bbox.min, bbox.max);
                auto strokeBounds = gpuTransformBounds(stroker.bounds(), matrix);
                updateBBox({{(float)strokeBounds.min.x, (float)strokeBounds.min.y}, {(float)strokeBounds.max.x, (float)strokeBounds.max.y}});
            }
        }
    }
    // update shapes bbox (with empty path handling)
    if (!meshShape.vbuffer.empty() || !meshStrokes.vbuffer.empty()) updateAABB();
    else bbox = aabb = {{0, 0}, {0, 0}};
    meshBBox.bbox(bbox.min, bbox.max);
}


void BgfxRenderDataShape::releaseMeshes()
{
    meshStrokes.clear();
    meshStrokesBBox.clear();
    meshShape.clear();
    meshShapeBBox.clear();
    meshBBox.clear();
    bbox.min = {FLT_MAX, FLT_MAX};
    bbox.max = {0.0f, 0.0f};
    aabb = {{0, 0}, {0, 0}};
    clips.clear();
}


void BgfxRenderDataShape::release(BgfxContext& context)
{
    releaseMeshes();
    renderSettingsStroke.release(context);
    renderSettingsShape.release(context);
    BgfxRenderDataPaint::release(context);
};

//***********************************************************************
// BgfxRenderDataShapePool
//***********************************************************************

BgfxRenderDataShape* BgfxRenderDataShapePool::allocate(BgfxContext& context)
{
    BgfxRenderDataShape* renderData{};
    if (mPool.count > 0) {
        renderData = mPool.last();
        mPool.pop();
    } else {
        renderData = new BgfxRenderDataShape();
        mList.push(renderData);
    }
    return renderData;
}


void BgfxRenderDataShapePool::free(BgfxContext& context, BgfxRenderDataShape* renderData)
{
    renderData->releaseMeshes();
    renderData->clips.clear();
    mPool.push(renderData);
}


void BgfxRenderDataShapePool::release(BgfxContext& context)
{
    ARRAY_FOREACH(p, mList) {
        (*p)->release(context);
        delete(*p);
    }
    mPool.clear();
    mList.clear();
}

//***********************************************************************
// BgfxRenderDataPicture
//***********************************************************************

void BgfxRenderDataPicture::updateSurface(const RenderSurface* surface, const Matrix& transform)
{
    meshData.imageBox(surface->w, surface->h, transform);
}

void BgfxRenderDataPicture::setImage(bgfx::TextureHandle texture, const RenderSurface* surface, FilterMethod filter, uint16_t stamp)
{
    imageTexture = texture;
    imageSource = bgfx::isValid(texture) ? surface : nullptr;
    imageFilter = filter;
    imageStamp = bgfx::isValid(texture) ? stamp : 0;
}

void BgfxRenderDataPicture::releaseTexture(BgfxTextureMgr& textures, BgfxContext& context)
{
    if (bgfx::isValid(imageTexture) && imageStamp == textures.stamp) textures.release(context, imageSource, imageTexture);
    clearImage();
}

void BgfxRenderDataPicture::clearImage()
{
    imageTexture = BGFX_INVALID_HANDLE;
    imageSource = nullptr;
    imageFilter = FilterMethod::Bilinear;
    imageStamp = 0;
}

void BgfxRenderDataPicture::release(BgfxContext& context)
{
    renderSettings.release(context);
    clearImage();
    BgfxRenderDataPaint::release(context);
}

//***********************************************************************
// BgfxRenderDataPicturePool
//***********************************************************************

BgfxRenderDataPicture* BgfxRenderDataPicturePool::allocate(BgfxContext& context)
{
    BgfxRenderDataPicture* renderData{};
    if (mPool.count > 0) {
        renderData = mPool.last();
        mPool.pop();
    } else {
        renderData = new BgfxRenderDataPicture();
        mList.push(renderData);
    }
    return renderData;
}


void BgfxRenderDataPicturePool::free(BgfxContext& context, BgfxRenderDataPicture* renderData)
{
    renderData->clips.clear();
    mPool.push(renderData);
}


void BgfxRenderDataPicturePool::release(BgfxContext& context)
{
    ARRAY_FOREACH(p, mList) {
        (*p)->release(context);
        delete(*p);
    }
    mPool.clear();
    mList.clear();
}

//***********************************************************************
// BgfxIntersector
//***********************************************************************

bool BgfxIntersector::isPointInTriangle(const Point& p, const Point& a, const Point& b, const Point& c)
{
    auto d1 = tvg::cross(p - a, p - b);
    auto d2 = tvg::cross(p - b, p - c);
    auto d3 = tvg::cross(p - c, p - a);
    auto has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    auto has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
    return !(has_neg && has_pos);
}


// triangle list
bool BgfxIntersector::isPointInTris(const Point& p, const BgfxMeshData& mesh)
{
    for (uint32_t i = 0; i < mesh.ibuffer.count; i += 3) {
        auto p0 = mesh.vbuffer[mesh.ibuffer[i+0]];
        auto p1 = mesh.vbuffer[mesh.ibuffer[i+1]];
        auto p2 = mesh.vbuffer[mesh.ibuffer[i+2]];
        if (isPointInTriangle(p, {p0.x, p0.y}, {p1.x, p1.y}, {p2.x, p2.y})) return true;
    }
    return false;
}


// even-odd triangle list
bool BgfxIntersector::isPointInMesh(const Point& p, const BgfxMeshData& mesh)
{
    uint32_t crossings = 0;
    for (uint32_t i = 0; i < mesh.ibuffer.count; i += 3) {
        Point triangle[3] = {
            {mesh.vbuffer[mesh.ibuffer[i+0]].x, mesh.vbuffer[mesh.ibuffer[i+0]].y},
            {mesh.vbuffer[mesh.ibuffer[i+1]].x, mesh.vbuffer[mesh.ibuffer[i+1]].y},
            {mesh.vbuffer[mesh.ibuffer[i+2]].x, mesh.vbuffer[mesh.ibuffer[i+2]].y}
        };
        for (uint32_t j = 0; j < 3; j++) {
            auto p1 = triangle[j];
            auto p2 = triangle[(j + 1) % 3];
            if (p1.y == p2.y) continue;
            if (p1.y > p2.y) std::swap(p1, p2);
            if ((p.y > p1.y) && (p.y <= p2.y)) {
                auto intersectionX = (p2.x - p1.x) * (p.y - p1.y) / (p2.y - p1.y) + p1.x;
                if (intersectionX > p.x) crossings++;
            }
        }
    }
    return (crossings % 2) == 1;
}

bool BgfxIntersector::intersectClips(const Point& pt, const Array<BgfxRenderDataPaint*>& clips)
{
    for (uint32_t i = 0; i < clips.count; i++) {
        auto clip = (BgfxRenderDataShape*)clips[i];
        if (!isPointInMesh(pt, clip->meshShape)) return false;
    }
    return true;
}


bool BgfxIntersector::intersectShape(const RenderRegion region, const BgfxRenderDataShape* shape)
{
    if (!shape || ((shape->meshShape.ibuffer.count == 0) && (shape->meshStrokes.ibuffer.count == 0))) return false;
    Matrix inverseModel;
    auto testStroke = !shape->renderSettingsStroke.skip && inverse(&shape->transform, &inverseModel);
    auto sizeX = region.sw();
    auto sizeY = region.sh();
    for (int32_t y = 0; y <= sizeY; y++) {
        for (int32_t x = 0; x <= sizeX; x++) {
            Point pt{(float)x + region.min.x, (float)y + region.min.y};
            if (y % 2 == 1) pt.y = (float) sizeY - y - sizeY % 2 + region.min.y;
            if (intersectClips(pt, shape->clips)) {
                if (!shape->renderSettingsShape.skip && isPointInMesh(pt, shape->meshShape)) return true;
                if (testStroke && isPointInTris(pt * inverseModel, shape->meshStrokes)) return true;
            }
        }
    }
    return false;
}


bool BgfxIntersector::intersectImage(const RenderRegion region, const BgfxRenderDataPicture* image)
{
    if (!image) return false;
    auto sizeX = region.sw();
    auto sizeY = region.sh();
    for (int32_t y = 0; y <= sizeY; y++) {
        for (int32_t x = 0; x <= sizeX; x++) {
            Point pt{(float)x + region.min.x, (float)y + region.min.y};
            if (y % 2 == 1) pt.y = (float) sizeY - y - sizeY % 2 + region.min.y;
            if (intersectClips(pt, image->clips)) {
                if (isPointInTris(pt, image->meshData)) return true;
            }
        }
    }
    return false;
}
