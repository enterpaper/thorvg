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
 * Scene post effects, selected by u_color.x:
 *   0 Fill      u_effect[0] = (r, g, b, a) straight
 *   1 Tint      u_effect[0] = (black.rgb, intensity), u_effect[1] = (white.rgb, -)
 *   2 Tritone   u_effect[0] = shadow.rgb, u_effect[1] = midtone.rgb,
 *               u_effect[2] = (highlight.rgb, blender)
 *   3 DropShadow u_effect[0] = (premultiplied color), u_effect[1] = (offset uv, -)
 * The input texture holds the (blurred) scene copy; alpha is coverage.
 */

#include <bgfx_shader.sh>

uniform vec4 u_color;
uniform vec4 u_effect[3];
SAMPLER2D(s_tex, 1);

float luma(vec3 c)
{
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

void main()
{
    float mode = u_color.x;
    vec4 src = texture2D(s_tex, v_uv);

    if (mode < 0.5) {
        // Fill: override the content color, keep the coverage
        vec4 fill = u_effect[0];
        float a = src.a * fill.a;
        gl_FragColor = vec4(fill.rgb * a, a);
    } else if (mode < 1.5) {
        // Tint: map the luminance between black and white
        vec3 black = u_effect[0].rgb;
        vec3 white = u_effect[1].rgb;
        float intensity = u_effect[0].a;
        vec3 straight = src.rgb / max(src.a, 1e-6);
        float l = clamp(luma(straight), 0.0, 1.0);
        vec3 tinted = black + (white - black) * l;
        gl_FragColor = vec4(mix(straight, tinted, intensity) * src.a, src.a);
    } else if (mode < 2.5) {
        // Tritone: shadow / midtone / highlight by luminance
        vec3 shadow = u_effect[0].rgb;
        vec3 midtone = u_effect[1].rgb;
        vec3 highlight = u_effect[2].rgb;
        float blender = u_effect[2].a;
        vec3 straight = src.rgb / max(src.a, 1e-6);
        float l = clamp(luma(straight), 0.0, 1.0);
        vec3 lo = mix(shadow, midtone, clamp(l * 2.0, 0.0, 1.0));
        vec3 hi = mix(midtone, highlight, clamp(l * 2.0 - 1.0, 0.0, 1.0));
        vec3 tri = mix(lo, hi, step(0.5, l));
        gl_FragColor = vec4(mix(straight, tri, blender) * src.a, src.a);
    } else {
        // DropShadow: colorize the blurred silhouette, offset in uv
        vec4 color = u_effect[0];  // premultiplied
        vec2 offset = u_effect[1].xy;
        float a = texture2D(s_tex, v_uv + offset).a;
        gl_FragColor = vec4(color.rgb * a, color.a * a);
    }
}
