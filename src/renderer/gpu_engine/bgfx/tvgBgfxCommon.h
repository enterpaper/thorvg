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

#ifndef _TVG_BGFX_COMMON_H_
#define _TVG_BGFX_COMMON_H_

#include "tvgRender.h"
#include "bgfx/bgfx.h"

/*
 * bgfx render backend shared building blocks.
 *
 * State model (kept aligned with the wg backend's pipelines):
 *  - fills resolve their fill rule through the stencil buffer; the cover pass
 *    both draws color and clears the stencil it tested (TEST_NOTEQUAL + ZERO).
 *  - strokes/markup use the "direct" stencil marker (REPLACE with ref 255).
 *  - clipping accumulates AND-coverage in the depth buffer: every clip marks
 *    the still-unclipped outside of its winding with a nearer depth value;
 *    a paint's cover passes the depth test only on unclipped pixels.
 *  - all blending assumes premultiplied alpha (ONE, INV_SRC_ALPHA).
 */

// depth starts fully "open" and clip marks bring it nearer to 0
#define BGFX_CLIP_DEPTH_BASE 1.0f
#define BGFX_SHAPE_DEPTH_DEFAULT 0.5f

// pack a ThorVG 3x3 matrix for a bgfx Mat4 uniform (same layout as the wg backend)
inline void bgfxPackMatrix(float out[16], const Matrix& m)
{
    out[0]  = m.e11; out[1]  = m.e21; out[2]  = 0.0f;  out[3]  = m.e31;
    out[4]  = m.e12; out[5]  = m.e22; out[6]  = 0.0f;  out[7]  = m.e32;
    out[8]  = 0.0f;  out[9]  = 0.0f;  out[10] = 1.0f;  out[11] = 0.0f;
    out[12] = m.e13; out[13] = m.e23; out[14] = 0.0f;  out[15] = m.e33;
}

// viewport space -> NDC, flipping the y axis
inline void bgfxPackViewportMatrix(float out[16], uint32_t w, uint32_t h)
{
    out[0]  = +2.0f / w; out[1]  = +0.0f;      out[2]  = +0.0f; out[3]  = +0.0f;
    out[4]  = +0.0f;     out[5]  = -2.0f / h;  out[6]  = +0.0f; out[7]  = +0.0f;
    out[8]  = +0.0f;     out[9]  = +0.0f;      out[10] = -1.0f; out[11] = +0.0f;
    out[12] = -1.0f;     out[13] = +1.0f;      out[14] = +0.0f; out[15] = +1.0f;
}

struct BgfxVertex
{
    float x, y;
    float u, v;
    uint32_t rgba;

    static bgfx::VertexLayout layout()
    {
        bgfx::VertexLayout decl;
        decl.begin()
            .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();
        return decl;
    }
};

// solid alpha blending for premultiplied color/coverage
inline uint64_t bgfxStateColorWrite()
{
    return BGFX_STATE_WRITE_MASK | BGFX_STATE_DEPTH_TEST_ALWAYS;
}

inline uint64_t bgfxStatePremultipliedBlend()
{
    return bgfxStateColorWrite() | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA);
}

// opaque overwrite (blit/compose passes that compute the color themselves)
inline uint64_t bgfxStateOpaqueWrite()
{
    return bgfxStateColorWrite() | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ZERO);
}

// depth test for clip-marked pixels: nearer values clip later covers
inline uint64_t bgfxStateDepthLess(bool write)
{
    return (write ? BGFX_STATE_WRITE_Z : 0) | BGFX_STATE_DEPTH_TEST_LESS;
}

inline uint64_t bgfxStateDepthGreater(bool write)
{
    return (write ? BGFX_STATE_WRITE_Z : 0) | BGFX_STATE_DEPTH_TEST_GREATER;
}

inline uint64_t bgfxStateDepthEqual(bool write)
{
    return (write ? BGFX_STATE_WRITE_Z : 0) | BGFX_STATE_DEPTH_TEST_EQUAL;
}

inline uint64_t bgfxStateDepthAlways(bool write)
{
    return (write ? BGFX_STATE_WRITE_Z : 0) | BGFX_STATE_DEPTH_TEST_ALWAYS;
}

struct BgfxStencilState
{
    uint32_t front = BGFX_STENCIL_NONE;
    uint32_t back = BGFX_STENCIL_NONE;  // BGFX_STENCIL_NONE: front applies to both faces
};

// stencil winding ops (color/depth untouched)
//
// BGFX_STENCIL_FUNC_RMASK(0xff) is mandatory here: bgfx derives the stencil
// *write* mask from the very same RMASK field (see unpackStencilWriteMask in
// bgfx_p.h), so leaving it unset means StencilWriteMask == 0 and the INCR/DECR/
// INVERT ops become no-ops -- the stencil stays 0, the cover pass (TEST_NOTEQUAL
// against ref 0) never passes and every stencil-routed fill silently vanishes.
// The wg backend sets the equivalent mask to 0xFFFFFFFF in tvgWgPipelines.cpp.
inline BgfxStencilState bgfxStencilWindingNonZero()
{
    const uint32_t mask = BGFX_STENCIL_FUNC_RMASK(0xff);
    return {
        mask | BGFX_STENCIL_TEST_ALWAYS | BGFX_STENCIL_OP_FAIL_S_KEEP | BGFX_STENCIL_OP_FAIL_Z_KEEP | BGFX_STENCIL_OP_PASS_Z_INCR,
        mask | BGFX_STENCIL_TEST_ALWAYS | BGFX_STENCIL_OP_FAIL_S_KEEP | BGFX_STENCIL_OP_FAIL_Z_KEEP | BGFX_STENCIL_OP_PASS_Z_DECR,
    };
}

inline BgfxStencilState bgfxStencilWindingEvenOdd()
{
    auto both = BGFX_STENCIL_FUNC_RMASK(0xff) | BGFX_STENCIL_TEST_ALWAYS | BGFX_STENCIL_OP_FAIL_S_KEEP | BGFX_STENCIL_OP_FAIL_Z_KEEP | BGFX_STENCIL_OP_PASS_Z_INVERT;
    return {both, both};
}

// stroke / clip "direct" marker: inside the stroked outline the stencil
// becomes the reference value (255), everything else stays 0
inline BgfxStencilState bgfxStencilDirect(uint32_t ref = 255)
{
    auto both = BGFX_STENCIL_TEST_ALWAYS | BGFX_STENCIL_FUNC_REF(ref) | BGFX_STENCIL_FUNC_RMASK(0xff) |
        BGFX_STENCIL_OP_FAIL_S_KEEP | BGFX_STENCIL_OP_FAIL_Z_KEEP | BGFX_STENCIL_OP_PASS_Z_REPLACE;
    return {both, both};
}

// cover pass: draws where the stencil test passes and zeroes the stencil
inline BgfxStencilState bgfxStencilCover(uint32_t mask = 0xff)
{
    auto both = BGFX_STENCIL_TEST_NOTEQUAL | BGFX_STENCIL_FUNC_REF(0) | BGFX_STENCIL_FUNC_RMASK(mask) |
        BGFX_STENCIL_OP_FAIL_S_ZERO | BGFX_STENCIL_OP_FAIL_Z_ZERO | BGFX_STENCIL_OP_PASS_Z_ZERO;
    return {both, both};
}

// stencil always, no ops (pure depth helper passes)
inline BgfxStencilState bgfxStencilKeep()
{
    auto both = BGFX_STENCIL_TEST_ALWAYS | BGFX_STENCIL_FUNC_REF(0) | BGFX_STENCIL_FUNC_RMASK(0xff) |
        BGFX_STENCIL_OP_FAIL_S_KEEP | BGFX_STENCIL_OP_FAIL_Z_KEEP | BGFX_STENCIL_OP_PASS_Z_KEEP;
    return {both, both};
}

// stencil always + zero: clears the stencil while drawing (exact quad coverage)
inline BgfxStencilState bgfxStencilClearOnly()
{
    auto both = BGFX_STENCIL_TEST_ALWAYS | BGFX_STENCIL_FUNC_REF(0) | BGFX_STENCIL_FUNC_RMASK(0xff) |
        BGFX_STENCIL_OP_FAIL_S_ZERO | BGFX_STENCIL_OP_FAIL_Z_ZERO | BGFX_STENCIL_OP_PASS_Z_ZERO;
    return {both, both};
}

// clip pass: marks the unclipped outside of the winding (EQUAL 0) with depth;
// inside the winding the stencil is cleared
inline BgfxStencilState bgfxStencilClipOutside()
{
    auto both = BGFX_STENCIL_TEST_EQUAL | BGFX_STENCIL_FUNC_REF(0) | BGFX_STENCIL_FUNC_RMASK(0xff) |
        BGFX_STENCIL_OP_FAIL_S_REPLACE | BGFX_STENCIL_OP_FAIL_Z_KEEP | BGFX_STENCIL_OP_PASS_Z_KEEP;
    return {both, both};
}

#endif //_TVG_BGFX_COMMON_H_
