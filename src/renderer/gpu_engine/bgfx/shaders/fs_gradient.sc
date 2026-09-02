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

#include <bgfx_shader.sh>

uniform vec4 u_color;
uniform vec4 u_gradient[2];
SAMPLER2D(s_ramp, 0);

void main()
{
    vec2 pos = v_paint;
    vec4 p0 = u_gradient[0];
    vec4 p1 = u_gradient[1];
    float t;

    if (p1.w < 0.5) {
        // linear: projection onto the gradient vector (p0.z = 1/|d|^2)
        t = dot(pos - p0.xy, p1.xy) * p0.z;
    } else {
        // radial: two-circle interpolation around center/focal
        // (math ported from the wg backend shader)
        vec2 d0 = pos - p0.xy;
        vec2 d1 = p0.xy - p1.xy;
        float r0 = p0.z;
        float rd = p1.z - p0.z;
        float a = dot(d1, d1) - rd * rd;
        float b = 2.0 * dot(d0, d1) - 2.0 * r0 * rd;
        float c = dot(d0, d0) - r0 * r0;
        float d = b * b - 4.0 * a * c;
        t = 0.0;
        if (d >= 0.0) t = min(1.0, (-b + sqrt(d)) / (2.0 * a));
        if ((c > 0.0) && (t >= 1.0)) t = 0.0;
        t = 1.0 - t;
    }

    // spread is resolved by the ramp sampler wrap mode (pad/mirror/repeat)
    vec4 Sc = texture2D(s_ramp, vec2(t, 0.5));
    float alpha = Sc.a * u_color.a;
    gl_FragColor = vec4(Sc.rgb * alpha, alpha);
}
