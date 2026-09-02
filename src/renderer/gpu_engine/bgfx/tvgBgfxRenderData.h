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

#ifndef _TVG_BGFX_RENDER_DATA_H_
#define _TVG_BGFX_RENDER_DATA_H_

#include "tvgBgfxContext.h"
#include "tvgBgfxGeometry.h"
#include "tvgBgfxShaderTypes.h"
#include "tvgBgfxTextureMgr.h"

#define BGFX_TEXTURE_GRADIENT_SIZE 1024

enum class BgfxFillType { None = 0, Solid = 1, Linear = 2, Radial = 3 };

static_assert(sizeof(RenderColor) == 4, "Solid color data must remain tightly packed RGBA8");

struct BgfxSolidData
{
    uint32_t colorInd{};
    RenderColor color{};
    uint8_t opacity = 255;

    RenderColor packedColor() const { return {color.r, color.g, color.b, MULTIPLY(color.a, opacity)}; }
};

struct BgfxRenderSettings
{
    BgfxGradientParams gradient{};
    Matrix paintMatrix = tvg::identity();  // geometry(local) -> gradient space
    bgfx::TextureHandle ramp = BGFX_INVALID_HANDLE;
    BgfxFillType fillType{};
    float opacityMultiplier = 1.0f;
    bool skip{};

    uint8_t updateOpacity(TVG_UNUSED tvg::ColorSpace cs, uint8_t opacity);
    void update(BgfxContext& context, const Fill* fill, const Matrix* modelTransform, bool updateColorRamp);
    void release(BgfxContext& context);

private:
    bool buildRamp(BgfxContext& context, const Fill* fill);
};

struct BgfxRenderDataPaint
{
    BBox aabb{{},{}};
    RenderRegion viewport{};
    Array<BgfxRenderDataPaint*> clips;
    Matrix transform;

    virtual ~BgfxRenderDataPaint() {};
    virtual void release(BgfxContext& context);
    virtual Type type() { return Type::Undefined; };

    void updateClips(const Array<RenderData>& clips);
};

struct BgfxRenderDataShape: public BgfxRenderDataPaint
{
    BgfxRenderSettings renderSettingsShape{};
    BgfxSolidData solidShape{};
    BgfxRenderSettings renderSettingsStroke{};
    BgfxSolidData solidStroke{};
    BgfxMeshData meshBBox{};
    BgfxMeshData meshShape{};
    BgfxMeshData meshShapeBBox{};
    BgfxMeshData meshStrokes{};
    BgfxMeshData meshStrokesBBox{};
    bool strokeFirst{};
    FillRule fillRule{};
    bool convex{};
    BBox bbox;

    void updateBBox(BBox bb);
    void updateAABB() { aabb = bbox; }
    void updateVisibility(const RenderShape& rshape, uint8_t opacity);
    void updateMeshes(const RenderShape& rshape, RenderUpdateFlag flag, const Matrix& matrix);
    void releaseMeshes();
    void release(BgfxContext& context) override;
    Type type() override { return Type::Shape; };
};

class BgfxRenderDataShapePool
{
private:
    Array<BgfxRenderDataShape*> mPool;
    Array<BgfxRenderDataShape*> mList;
public:
    BgfxRenderDataShape* allocate(BgfxContext& context);
    void free(BgfxContext& context, BgfxRenderDataShape* renderData);
    void release(BgfxContext& context);
};

struct BgfxRenderDataPicture: public BgfxRenderDataPaint
{
    BgfxRenderSettings renderSettings{};
    bgfx::TextureHandle imageTexture = BGFX_INVALID_HANDLE;
    const RenderSurface* imageSource = nullptr;
    FilterMethod imageFilter = FilterMethod::Bilinear;
    uint16_t imageStamp = 0;
    uint8_t opacity = 255;
    BgfxMeshData meshData{};

    void updateSurface(const RenderSurface* surface, const Matrix& transform);
    void setImage(bgfx::TextureHandle texture, const RenderSurface* surface, FilterMethod filter, uint16_t stamp);
    void releaseTexture(BgfxTextureMgr& textures, BgfxContext& context);
    void clearImage();
    void release(BgfxContext& context) override;
    Type type() override { return Type::Picture; };
};

class BgfxRenderDataPicturePool
{
private:
    Array<BgfxRenderDataPicture*> mPool;
    Array<BgfxRenderDataPicture*> mList;
public:
    BgfxRenderDataPicture* allocate(BgfxContext& context);
    void free(BgfxContext& context, BgfxRenderDataPicture* dataPicture);
    void release(BgfxContext& context);
};

struct BgfxIntersector
{
    bool isPointInTriangle(const Point& p, const Point& a, const Point& b, const Point& c);
    bool isPointInTris(const Point& p, const BgfxMeshData& mesh);
    bool isPointInMesh(const Point& p, const BgfxMeshData& mesh);
    bool intersectClips(const Point& pt, const Array<BgfxRenderDataPaint*>& clips);
    bool intersectShape(const RenderRegion region, const BgfxRenderDataShape* shape);
    bool intersectImage(const RenderRegion region, const BgfxRenderDataPicture* image);
};

#endif //_TVG_BGFX_RENDER_DATA_H_
