
-- color-space
#ifndef __BLEND_COLOR_SPACE__
#define __BLEND_COLOR_SPACE__
void rgbToHsv(vec4 rgb, out vec4 col2)
{
    float cmax, cmin, h, s, v, cdelta;
    vec3 c;

    cmax = max(rgb[0], max(rgb[1], rgb[2]));
    cmin = min(rgb[0], min(rgb[1], rgb[2]));
    cdelta = cmax-cmin;

    v = cmax;
    if (cmax!=0.0)
        s = cdelta/cmax;
    else {
        s = 0.0;
        h = 0.0;
    }

    if (s == 0.0) {
        h = 0.0;
    }
    else {
        c = (vec3(cmax, cmax, cmax) - rgb.xyz)/cdelta;

        if (rgb.x==cmax) h = c[2] - c[1];
        else if (rgb.y==cmax) h = 2.0 + c[0] -  c[2];
        else h = 4.0 + c[1] - c[0];

        h /= 6.0;

        if (h<0.0)
            h += 1.0;
    }
    col2 = vec4(h, s, v, rgb.w);
}

void hsvToRgb(vec4 hsv, out vec4 col2)
{
    float i, f, p, q, t, h, s, v;
    vec3 rgb;

    h = hsv[0];
    s = hsv[1];
    v = hsv[2];

    if(s==0.0) {
        rgb = vec3(v, v, v);
    }
    else {
        if(h==1.0)
            h = 0.0;

        h *= 6.0;
        i = floor(h);
        f = h - i;
        rgb = vec3(f, f, f);
        p = v*(1.0-s);
        q = v*(1.0-(s*f));
        t = v*(1.0-(s*(1.0-f)));

        if (i == 0.0) rgb = vec3(v, t, p);
        else if (i == 1.0) rgb = vec3(q, v, p);
        else if (i == 2.0) rgb = vec3(p, v, t);
        else if (i == 3.0) rgb = vec3(p, q, v);
        else if (i == 4.0) rgb = vec3(t, p, v);
        else rgb = vec3(v, p, q);"
    }

    col2 = vec4(rgb, hsv.w);
}
#endif // __BLEND_COLOR_SPACE__

-- srcAlpha
void blend_srcAlpha(vec4 src, inout vec4 dst, float factor)
{
    dst = dst*(1.0 - src.a) + src*src.a;
}

-- mix
void blend_mix(vec4 src, inout vec4 dst, float factor) { dst = mix(dst, src, factor); }
void blend_mix(vec3 src, inout vec3 dst, float factor) { dst = mix(dst, src, factor); }
void blend_mix(vec2 src, inout vec2 dst, float factor) { dst = mix(dst, src, factor); }
void blend_mix(float src, inout float dst, float factor) { dst = mix(dst, src, factor); }

-- add
void blend_add(vec4 src, inout vec4 dst, float factor) { dst += src; }
void blend_add(vec3 src, inout vec3 dst, float factor) { dst += src; }
void blend_add(vec2 src, inout vec2 dst, float factor) { dst += src; }
void blend_add(float src, inout float dst, float factor) { dst += src; }

-- smoothAdd
void blend_smoothAdd(vec4 src, inout vec4 dst, float factor) { dst = factor*(dst+src); }
void blend_smoothAdd(vec3 src, inout vec3 dst, float factor) { dst = factor*(dst+src); }
void blend_smoothAdd(vec2 src, inout vec2 dst, float factor) { dst = factor*(dst+src); }
void blend_smoothAdd(float src, inout float dst, float factor) { dst = factor*(dst+src); }

-- sub
void blend_sub(vec4 src, inout vec4 dst, float factor) { dst -= src; }
void blend_sub(vec3 src, inout vec3 dst, float factor) { dst -= src; }
void blend_sub(vec2 src, inout vec2 dst, float factor) { dst -= src; }
void blend_sub(float src, inout float dst, float factor) { dst -= src; }

-- reverseSub
void blend_reverseSub(vec4 src, inout vec4 dst, float factor) { dst = src-dst; }
void blend_reverseSub(vec3 src, inout vec3 dst, float factor) { dst = src-dst; }
void blend_reverseSub(vec2 src, inout vec2 dst, float factor) { dst = src-dst; }
void blend_reverseSub(float src, inout float dst, float factor) { dst = src-dst; }

-- mul
void blend_mul(vec4 src, inout vec4 dst, float factor) { dst *= src; }
void blend_mul(vec3 src, inout vec3 dst, float factor) { dst *= src; }
void blend_mul(vec2 src, inout vec2 dst, float factor) { dst *= src; }
void blend_mul(float src, inout float dst, float factor) { dst *= src; }

-- diff
void blend_diff(vec4 src, inout vec4 dst, float factor) { dst = abs(dst - src); }
void blend_diff(vec3 src, inout vec3 dst, float factor) { dst = abs(dst - src); }
void blend_diff(vec2 src, inout vec2 dst, float factor) { dst = abs(dst - src); }
void blend_diff(float src, inout float dst, float factor) { dst = abs(dst - src); }

-- darken
void blend_darken(vec4 src, inout vec4 dst, float factor) { dst = min(dst, src*factor); }
void blend_darken(vec3 src, inout vec3 dst, float factor) { dst = min(dst, src*factor); }
void blend_darken(vec2 src, inout vec2 dst, float factor) { dst = min(dst, src*factor); }
void blend_darken(float src, inout float dst, float factor) { dst = min(dst, src*factor); }

-- lighten
void blend_lighten(vec4 src, inout vec4 dst, float factor) { dst = max(dst, src*factor); }
void blend_lighten(vec3 src, inout vec3 dst, float factor) { dst = max(dst, src*factor); }
void blend_lighten(vec2 src, inout vec2 dst, float factor) { dst = max(dst, src*factor); }
void blend_lighten(float src, inout float dst, float factor) { dst = max(dst, src*factor); }

-- div
void blend_div(float src, inout float dst, float factor)
{
    if(src != 0.0) dst = (1.0-factor)*dst + factor*dst/src;
}
void blend_div(vec4 src, inout vec4 dst, float factor)
{
    blend_div(src.r, dst.r, factor);
    blend_div(src.g, dst.g, factor);
    blend_div(src.b, dst.b, factor);
}
void blend_div(vec3 src, inout vec3 dst, float factor)
{
    blend_div(src.r, dst.r, factor);
    blend_div(src.g, dst.g, factor);
    blend_div(src.b, dst.b, factor);
}
void blend_div(vec2 src, inout vec2 dst, float factor)
{
    blend_div(src.r, dst.r, factor);
    blend_div(src.g, dst.g, factor);
}

-- overlay
void blend_overlay(float src, inout float dst, float factor)
{
    float facm = 1.0 - factor;
    if(dst < 0.5) {
        dst *= facm + 2.0* factor *src;
    } else {
        dst = 1.0 - (facm + 2.0* factor *(1.0 - src))*(1.0 - dst);
    }
}
void blend_overlay(vec4 src, inout vec4 dst, float factor)
{
    blend_overlay(src.r, dst.r, factor);
    blend_overlay(src.g, dst.g, factor);
    blend_overlay(src.b, dst.b, factor);
}
void blend_overlay(vec3 src, inout vec3 dst, float factor)
{
    blend_overlay(src.r, dst.r, factor);
    blend_overlay(src.g, dst.g, factor);
    blend_overlay(src.b, dst.b, factor);
}
void blend_overlay(vec2 src, inout vec2 dst, float factor)
{
    blend_overlay(src.r, dst.r, factor);
    blend_overlay(src.g, dst.g, factor);
}

-- dodge
void blend_dodge(float col1, inout float col2, float factor)
{
    if(col2 != 0.0) {
        float tmp = 1.0 - factor*col1;
        if(tmp <= 0.0) {
            col2 = 1.0;
        } else if((tmp = col2/tmp) > 1.0) {
            col2 = 1.0;
        } else {
            col2 = tmp;
        }
    }
}
void blend_dodge(vec4 col1, inout vec4 col2, float factor)
{
    dodgeBlender(col1.r, col2.r, factor);
    dodgeBlender(col1.g, col2.g, factor);
    dodgeBlender(col1.b, col2.b, factor);
}
void blend_dodge(vec3 col1, inout vec3 col2, float factor)
{
    dodgeBlender(col1.r, col2.r, factor);
    dodgeBlender(col1.g, col2.g, factor);
    dodgeBlender(col1.b, col2.b, factor);
}
void blend_dodge(vec2 col1, inout vec2 col2, float factor)
{
    dodgeBlender(col1.r, col2.r, factor);
    dodgeBlender(col1.g, col2.g, factor);
}

-- burn
void blend_burn(float col1, inout float col2, float factor)
{
    float tmp = (1.0 - factor) + factor*col1;
    if(tmp <= 0.0) {
        col2 = 0.0;
    } else if((tmp = (1.0 - (1.0 - col2)/tmp)) < 0.0) {
        col2 = 0.0;
    } else if(tmp > 1.0) {
        col2 = 1.0;
    } else {
        col2 = tmp;
    }
}
void blend_burn(vec4 col1, inout vec4 col2, float factor)
{
    blend_burn(col1.r, col2.r, factor);
    blend_burn(col1.g, col2.g, factor);
    blend_burn(col1.b, col2.b, factor);
}
void blend_burn(vec3 col1, inout vec3 col2, float factor)
{
    blend_burn(col1.r, col2.r, factor);
    blend_burn(col1.g, col2.g, factor);
    blend_burn(col1.b, col2.b, factor);
}
void blend_burn(vec2 col1, inout vec2 col2, float factor)
{
    blend_burn(col1.r, col2.r, factor);
    blend_burn(col1.g, col2.g, factor);
}

-- linear
void blend_linear(float col1, inout float col2, float factor)
{
    if(col1 > 0.5) {
        col2= col2 + factor*(2.0*(col1 - 0.5));
    } else {
        col2= col2 + factor*(2.0*(col1 - 1.0));
    }
}
void blend_linear(vec4 col1, inout vec4 col2, float factor)
{
    blend_linear(col1.r, col2.r, factor);
    blend_linear(col1.g, col2.g, factor);
    blend_linear(col1.b, col2.b, factor);
}
void blend_linear(vec3 col1, inout vec3 col2, float factor)
{
    blend_linear(col1.r, col2.r, factor);
    blend_linear(col1.g, col2.g, factor);
    blend_linear(col1.b, col2.b, factor);
}
void blend_linear(vec2 col1, inout vec2 col2, float factor)
{
    blend_linear(col1.r, col2.r, factor);
    blend_linear(col1.g, col2.g, factor);
}

-- screen
void blend_screen(vec4 src, inout vec4 dst, float factor)
{
    float facm = 1.0 - factor;
    dst = vec4(1.0) - (vec4(facm) + factor*(vec4(1.0) - src))*(vec4(1.0) - dst);
}
void blend_screen(vec3 src, inout vec3 dst, float factor)
{
    float facm = 1.0 - factor;
    dst = vec3(1.0) - (vec3(facm) + factor*(vec3(1.0) - src))*(vec3(1.0) - dst);
}
void blend_screen(vec2 src, inout vec2 dst, float factor)
{
    float facm = 1.0 - factor;
    dst = vec2(1.0) - (vec2(facm) + factor*(vec2(1.0) - src))*(vec2(1.0) - dst);
}
void blend_screen(float src, inout float dst, float factor)
{
    float facm = 1.0 - factor;
    dst = 1.0 - (facm + factor*(1.0 - src))*(1.0 - dst);
}

-- soft
void blend_soft(vec4 col1, inout vec4 col2, float factor)
{
    vec4 one = vec4(1.0);
    vec4 scr = one - (one - col1)*(one - col2);
    col2 = (1.0 - factor)*col2 + factor *((one - col2)*col1*col2 + col2*scr);
}
void blend_soft(vec3 col1, inout vec3 col2, float factor)
{
    vec3 one = vec3(1.0);
    vec3 scr = one - (one - col1)*(one - col2);
    col2 = (1.0 - factor)*col2 + factor *((one - col2)*col1*col2 + col2*scr);
}
void blend_soft(vec2 col1, inout vec2 col2, float factor)
{
    vec2 one = vec2(1.0);
    vec2 scr = one - (one - col1)*(one - col2);
    col2 = (1.0 - factor)*col2 + factor *((one - col2)*col1*col2 + col2*scr);
}
void blend_soft(float col1, inout float col2, float factor)
{
    float one = float(1.0);
    float scr = one - (one - col1)*(one - col2);
    col2 = (1.0 - factor)*col2 + factor *((one - col2)*col1*col2 + col2*scr);
}

-- hue
#include blender.color-space

void blend_hue(vec4 col1, inout vec4 col2, float factor)
{
    vec4 buf = col2;

    vec4 hsv, hsv2, tmp;
    rgbToHsv(col1, hsv2);

    if(hsv2.y != 0.0) {
        rgbToHsv(col2, hsv);
        hsv.x = hsv2.x;
        hsvToRgb(hsv, tmp);

        col2 = mix(buf, tmp, factor );
        col2.a = buf.a;
    }
}

-- sat
#include blender.color-space

void blend_sat( vec4 col1, inout vec4 col2, float factor)
{
    vec4 hsv = vec4(0.0);
    vec4 hsv2 = vec4(0.0);
    rgbToHsv(col2, hsv);

    if(hsv.y != 0.0) {
        rgbToHsv(col1, hsv2);
        hsv.y = (1.0 - factor)*hsv.y + factor*hsv2.y;
        hsvToRgb(hsv, col2);
    }
}

-- val
#include blender.color-space

void blend_val(vec4 col1, inout vec4 col2, float factor)
{
    vec4 hsv, hsv2;
    rgbToHsv(col2, hsv);
    rgbToHsv(col1, hsv2);

    hsv.z = (1.0 - factor)*hsv.z + factor*hsv2.z;
    hsvToRgb(hsv, col2);
}

-- col
#include blender.color-space

void blend_col(vec4 col1, inout vec4 col2, float factor)
{
    vec4 hsv, hsv2, tmp;
    rgbToHsv(col1, hsv2);

    if(hsv2.y != 0.0) {
        rgbToHsv(col2, hsv);
        hsv.x = hsv2.x;
        hsv.y = hsv2.y;
        hsvToRgb(hsv, tmp);

        col2.rgb = mix(col2.rgb, tmp.rgb, factor );
    }
}

