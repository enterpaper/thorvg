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

#ifndef _TVG_BGFX_GEOMETRY_H_
#define _TVG_BGFX_GEOMETRY_H_

#include "tvgBgfxCommon.h"

struct BgfxMeshData
{
    Array<BgfxVertex> vbuffer;
    Array<uint32_t> ibuffer;

    // quad (pmin -> pmax) with uv corners (0,0)-(1,1); used for covers/blits
    void bbox(const Point pmin, const Point pmax)
    {
        clear();
        vbuffer.push({pmin.x, pmin.y, 0.0f, 0.0f, 0xffffffff});
        vbuffer.push({pmax.x, pmin.y, 1.0f, 0.0f, 0xffffffff});
        vbuffer.push({pmax.x, pmax.y, 1.0f, 1.0f, 0xffffffff});
        vbuffer.push({pmin.x, pmax.y, 0.0f, 1.0f, 0xffffffff});
        ibuffer.push(0);
        ibuffer.push(1);
        ibuffer.push(2);
        ibuffer.push(0);
        ibuffer.push(2);
        ibuffer.push(3);
    }

    // unit quad mapped onto an image rect (image space: x right, y down)
    void imageBox(float w, float h, const Matrix& transform)
    {
        clear();
        Point pts[4] = {{0.0f, 0.0f}, {w, 0.0f}, {w, h}, {0.0f, h}};
        float uv[4][2] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
        for (uint32_t i = 0; i < 4; ++i) {
            auto p = pts[i] * transform;
            vbuffer.push({p.x, p.y, uv[i][0], uv[i][1], 0xffffffff});
        }
        ibuffer.push(0);
        ibuffer.push(1);
        ibuffer.push(2);
        ibuffer.push(0);
        ibuffer.push(2);
        ibuffer.push(3);
    }

    void clear()
    {
        vbuffer.clear();
        ibuffer.clear();
    }

    bool invalid()
    {
        return vbuffer.empty();
    }
};

#endif //_TVG_BGFX_GEOMETRY_H_
