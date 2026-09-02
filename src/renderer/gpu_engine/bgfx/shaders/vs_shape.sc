$input a_position, a_texcoord0, a_color0
$output v_color0, v_uv, v_paint

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

#include <bgfx_shader.sh>

uniform mat4 u_viewportMatrix;
uniform vec4 u_depth;
uniform mat4 u_paintMatrix;

void main()
{
    vec4 pos4 = mul(u_viewportMatrix, vec4(a_position, 0.0, 1.0));
    pos4.z = u_depth.x;
    pos4.w = 1.0;
    gl_Position = pos4;
    v_color0 = a_color0;
    v_uv = a_texcoord0;
    // geometry(local) -> paint space; consumed by the gradient fragment shader
    v_paint = mul(u_paintMatrix, vec4(a_position, 0.0, 1.0)).xy;
}
