$input v_color0, v_uv, v_paint

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

/*
 * One separable gaussian pass between render targets.
 * u_blur = (texel width, texel height, direction, sigma): direction 1
 * blurs horizontally, 2 vertically; sigma is in target pixels.
 */

#include <bgfx_shader.sh>

uniform vec4 u_blur;
SAMPLER2D(s_tex, 1);

void main()
{
    float sigma = max(u_blur.w, 0.25);
    vec2 step2 = (u_blur.z < 1.5) ? vec2(u_blur.x, 0.0) : vec2(0.0, u_blur.y);
    vec2 d = step2 * (sigma * 0.5);

    vec4 sum = texture2D(s_tex, v_uv) * 0.2270270270;
    sum += (texture2D(s_tex, v_uv + d * 1.0) + texture2D(s_tex, v_uv - d * 1.0)) * 0.1945945946;
    sum += (texture2D(s_tex, v_uv + d * 2.0) + texture2D(s_tex, v_uv - d * 2.0)) * 0.1216216216;
    sum += (texture2D(s_tex, v_uv + d * 3.0) + texture2D(s_tex, v_uv - d * 3.0)) * 0.0540540541;
    sum += (texture2D(s_tex, v_uv + d * 4.0) + texture2D(s_tex, v_uv - d * 4.0)) * 0.0162162162;

    gl_FragColor = sum;
}
