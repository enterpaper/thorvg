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

#ifndef _TVG_BGFX_SHADER_TYPES_H_
#define _TVG_BGFX_SHADER_TYPES_H_

#include "tvgRender.h"

/*
 * Per-draw uniform payloads for the bgfx backend. bgfx uploads uniforms
 * individually (setUniform), so these are plain value bundles filled once
 * per draw submission by the compositor.
 *
 * Layout notes:
 *  - matrices are bgfx Mat4 uploads (column-major float[16]);
 *  - u_gradient packs two vec4 slots (bgfx UniformType::Vec4, num = 2);
 *  - u_effect packs three vec4 slots (num = 3).
 */

struct BgfxXform
{
    float viewportMatrix[16];  // viewport space -> NDC (includes the y flip)
    float depth[4];            // x: clip-space z for this draw (clip marking)
    float paintMatrix[16];     // geometry -> paint space (gradients)
    float color[4];            // r,g,b,a premultiplied
};

// linear: p0 = (x0, y0, 1/|d|^2, spread), p1 = (dx, dy, -, 0)
// radial: p0 = (cx, cy, r, spread), p1 = (fx, fy, fr, 1)  -- p1.w selects radial
struct BgfxGradientParams
{
    float p0[4];
    float p1[4];
};

// x: mask method (MaskMethod), y: opacity 0-1, z: blend method (BlendMethod), w: dst texture bound
struct BgfxBlitParams
{
    float p[4];
};

// x: texel width, y: texel height, z: direction (0 both/1 horizontal/2 vertical), w: sigma
struct BgfxBlurParams
{
    float p[4];
};

// fill: p = (r,g,b,a); tint: p0 black, p1 white, p2 intensity; tritone: shadow/mid/high + blender
struct BgfxEffectParams
{
    float p[12];
};

#endif //_TVG_BGFX_SHADER_TYPES_H_
