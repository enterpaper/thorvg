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

#ifndef _TVG_BGFX_CONTEXT_H_
#define _TVG_BGFX_CONTEXT_H_

#include "tvgBgfxCommon.h"

enum class BgfxProgram : uint8_t
{
    Solid = 0,
    Gradient,
    Image,
    Blit,
    Blur,
    Effect,
    Count
};

enum class BgfxUniform : uint8_t
{
    ViewportMatrix = 0,
    Depth,
    PaintMatrix,
    Color,
    Gradient,
    Blit,
    Blur,
    Effect,
    Ramp,   // sampler
    Tex,    // sampler
    Mask,   // sampler
    Dst,    // sampler
    Count
};

struct BgfxContext
{
    BgfxContext()
    {
        // bgfx 的 kInvalidHandle = UINT16_MAX，idx=0 是有效句柄：
        // 零初始化的句柄数组会被 isValid() 判定为有效，导致 createShaders()
        // 永远不会被触发、绘制以假句柄静默提交。必须显式置为无效。
        for (auto& program : mPrograms) program = BGFX_INVALID_HANDLE;
        for (auto& uniform : mUniforms) uniform = BGFX_INVALID_HANDLE;
    }

    uint16_t viewBase = 0;
    uint16_t viewCount = 0;
    uint16_t viewNext = 0;

    bool initialize(uint16_t base, uint16_t count);
    bool invalid() const
    {
        return !mInitialized;
    }
    void release();

    // cached program handles, created lazily from the embedded shader table
    bgfx::ProgramHandle program(BgfxProgram id);

    bgfx::UniformHandle uniform(BgfxUniform id)
    {
        return mUniforms[uint8_t(id)];
    }

    // view timeline: one view per render-target stay in paint order
    uint16_t allocView()
    {
        if (viewNext >= viewBase + viewCount) {
            TVGERR("BGFX_ENGINE", "view id range exhausted (%u views)", viewCount);
            return viewBase + viewCount - 1;
        }
        return viewNext++;
    }

    void resetViews()
    {
        viewNext = viewBase;
    }

    // configure a view for a frame buffer (or the backbuffer when fb is invalid)
    void setupView(uint16_t view, bgfx::FrameBufferHandle fb, uint16_t w, uint16_t h, bool clear, uint32_t rgba = 0x00000000);

private:
    bool createShaders();

    bool mInitialized = false;
    bgfx::ProgramHandle mPrograms[uint8_t(BgfxProgram::Count)] = {};
    bgfx::UniformHandle mUniforms[uint8_t(BgfxUniform::Count)] = {};
};

#endif //_TVG_BGFX_CONTEXT_H_
