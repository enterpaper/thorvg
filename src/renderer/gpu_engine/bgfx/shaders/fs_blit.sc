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
 * Composition pass: resolves masking, blending and presentation of an
 * offscreen scene onto the current channel.
 *
 * u_blit = (mask method, opacity, blend id, flag):
 *   - mask method: MaskMethod enum order; 0 = none. Methods >= Add(5)
 *     modify the composed alpha.
 *   - blend id: BlendMethod enum order for the separable modes
 *     (1 Multiply .. 11 Exclusion); 0 = Normal (fixed function). HSL modes
 *     and Add degrade to Normal on the CPU side.
 *   - flag: s_dst is bound (1) for the shader-evaluated paths; for the
 *     present pass (method 0, blend 0) it requests un-premultiplication
 *     for straight-alpha targets.
 */

#include <bgfx_shader.sh>

uniform vec4 u_blit;
SAMPLER2D(s_tex, 1);
SAMPLER2D(s_mask, 2);
SAMPLER2D(s_dst, 3);

vec3 blendChannels(float id, vec3 S, vec3 D)
{
    // per-component selects use mix(step(...)); id selects use scalar
    // ternaries so the source translates to every backend; scalars are
    // splatted with vec3_splat() since HLSL constructors take exactly one
    // argument per component
    vec3 r1, r2;
    // Multiply(1) / Screen(2)
    if (id < 2.5) return (id < 2.0) ? (S * D) : (S + D - S * D);
    // Overlay(3): 2SD if D < 0.5 else 1 - 2(1-S)(1-D)
    r1 = 2.0 * S * D;
    r2 = 1.0 - 2.0 * (1.0 - S) * (1.0 - D);
    if (id < 3.5) return mix(r1, r2, step(vec3_splat(0.5), D));
    // Darken(4) / Lighten(5)
    if (id < 5.5) return (id < 5.0) ? min(S, D) : max(S, D);
    // ColorDodge(6): D / (1 - S)
    if (id < 6.5) return min(vec3_splat(1.0), D / max(vec3_splat(1e-6), 1.0 - S));
    // ColorBurn(7): 1 - (1 - D) / S
    if (id < 7.5) return 1.0 - min(vec3_splat(1.0), (1.0 - D) / max(vec3_splat(1e-6), S));
    // HardLight(8): overlay with the color roles reversed
    r1 = 2.0 * S * D;
    r2 = 1.0 - 2.0 * (1.0 - S) * (1.0 - D);
    if (id < 8.5) return mix(r1, r2, step(vec3_splat(0.5), S));
    // SoftLight(9)
    if (id < 9.5) {
        vec3 g = mix(sqrt(D), ((16.0 * D - 12.0) * D + 4.0) * D, step(D, vec3_splat(0.25)));
        r1 = D - (1.0 - 2.0 * S) * D * (1.0 - D);
        r2 = D + (2.0 * S - 1.0) * (g - D);
        return mix(r1, r2, step(S, vec3_splat(0.5)));
    }
    // Difference(10)
    if (id < 10.5) return abs(S - D);
    // Exclusion(11)
    return S + D - 2.0 * S * D;
}

void main()
{
    float method = u_blit.x;
    float opacity = u_blit.y;
    float blendId = u_blit.z;
    float flag = u_blit.w;

    vec4 src = texture2D(s_tex, v_uv) * opacity;

    // masking (mask math ported from the wg backend scene compose shader)
    if (method > 0.5) {
        vec4 msk = texture2D(s_mask, v_uv);
        if (method < 1.5) {
            src *= msk.a;                                    // Alpha
        } else if (method < 2.5) {
            src *= (1.0 - msk.a);                            // InvAlpha
        } else if (method < 3.5) {
            src *= dot(msk.rgb, vec3(0.2126, 0.7152, 0.0722));   // Luma
        } else if (method < 4.5) {
            src *= (1.0 - dot(msk.rgb, vec3(0.2126, 0.7152, 0.0722)));  // InvLuma
        } else {
            float a;
            if (method < 5.5)      a = src.a + msk.a * (1.0 - src.a);       // Add
            else if (method < 6.5) a = src.a * (1.0 - msk.a);               // Subtract
            else if (method < 7.5) a = src.a * msk.a;                       // Intersect
            else if (method < 8.5) a = src.a * (1.0 - msk.a) + msk.a * (1.0 - src.a);  // Difference
            else if (method < 9.5) a = max(src.a, msk.a);                   // Lighten
            else                   a = min(src.a, msk.a);                   // Darken
            src.a = a;
        }
    }

    bool useDst = (method >= 5.0) || (blendId > 0.5);
    if (!useDst) {
        // the state applies premultiplied source-over;
        // the present flag un-premultiplies for straight-alpha targets
        if (flag > 0.5) src.rgb = src.rgb / max(src.a, 1e-6);
        gl_FragColor = src;
        return;
    }

    // shader-evaluated path: the state writes opaquely, composite manually
    vec4 dst = texture2D(s_dst, v_uv);
    if (blendId < 0.5) {
        // masked source-over
        gl_FragColor = vec4(src.rgb + dst.rgb * (1.0 - src.a), src.a + dst.a * (1.0 - src.a));
        return;
    }

    // separable blend in straight space, then source-over
    vec3 s = src.rgb / max(src.a, 1e-6);
    vec3 d = dst.rgb / max(dst.a, 1e-6);
    vec3 b = blendChannels(blendId, s, d);
    gl_FragColor = vec4(src.a * b + dst.rgb * (1.0 - src.a), src.a + dst.a * (1.0 - src.a));
}
