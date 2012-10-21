

-- rgbToHsv
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

-- hsvToRgb
static const char* hsvToRgb =
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


-- invertColor
void invertColor(inout vec4 col)
{
    col.xyz = vec3(1.0) - col.xyz;
}

-- brightnessBlend
void brightnessBlend(inout vec4 col, float factor)
{
    col.xyz = col.xyz * factor;
}

-- contrastBlender
void contrastBlender(inout vec4 col, float factor)
{
    float buf = factor * 0.5 - 0.5;
    if (col.x > 0.5) {
        col.x = clamp( col.x + buf, 0.5, 1.0);
    } else {
        col.x = max( 0.0, 0.5*(2.0*col.x + 1.0 - factor) );
    }
    if (col.y > 0.5) {
        col.y = clamp( col.y + buf, 0.5, 1.0);
    } else {
        col.y = max( 0.0, 0.5*(2.0*col.y + 1.0 - factor) );
    }
    if (col.z > 0.5) {
        col.z = clamp( col.x + buf, 0.5, 1.0);
    } else {
        col.z = max( 0.0, 0.5*(2.0*col.z + 1.0 - factor) );
    }
}

-- alphaBlender
void alphaBlender(vec4 src, inout vec4 dst, float factor)
{
    dst = dst*(1.0 - src.a) + src*src.a;
}

-- mixBlender
void mixBlender(vec4 src, inout vec4 dst, float factor)
{
    dst = mix(dst, src, factor);
}

-- addBlender
void addBlender(vec4 src, inout vec4 dst, float factor)
{
    dst += src;
}

-- mulBlender
void mulBlender(vec4 src, inout vec4 dst, float factor)
{
   dst *= src;
}

-- screenBlender
void screenBlender(vec4 src, inout vec4 dst, float factor)
{
    float facm = 1.0 - factor;
    dst = vec4(1.0) - (vec4(facm) + factor*(vec4(1.0) - src))*(vec4(1.0) - dst);
}

-- overlayBlender
void overlayBlender(vec4 src, inout vec4 dst, float factor)
{
    float facm = 1.0 - factor;

    if(dst.r < 0.5)
        dst.r *= facm + 2.0* factor *src.r;
    else
        dst.r = 1.0 - (facm + 2.0* factor *(1.0 - src.r))*(1.0 - dst.r);

    if(dst.g < 0.5)
        dst.g *= facm + 2.0* factor *src.g;
    else
        dst.g = 1.0 - (facm + 2.0* factor *(1.0 - src.g))*(1.0 - dst.g);

    if(dst.b < 0.5)
        dst.b *= facm + 2.0* factor *src.b;
    else
        dst.b = 1.0 - (facm + 2.0* factor *(1.0 - src.b))*(1.0 - dst.b);
}

-- subBlender
void subBlender(vec4 src, inout vec4 dst, float factor)
{
    dst -= src;
}

-- divBlender
void divBlender(vec4 src, inout vec4 dst, float factor)
{
    float facm = 1.0 - factor;

    if(src.r != 0.0) dst.r = facm*dst.r + factor *dst.r/src.r;
    if(src.g != 0.0) dst.g = facm*dst.g + factor *dst.g/src.g;
    if(src.b != 0.0) dst.b = facm*dst.b + factor *dst.b/src.b;
}

-- diffBlender
void diffBlender(vec4 src, inout vec4 dst, float factor)
{
    dst = abs(dst - src);
}

-- darkBlender
void darkBlender(vec4 src, inout vec4 dst, float factor)
{
    dst = min(dst, src*factor);
}

-- lightBlender
void lightBlender(vec4 src, inout vec4 dst, float factor)
{
    dst = max(dst, src*factor);
}

-- dodgeBlender
void dodgeBlender(vec4 col1, inout vec4 col2, float factor)
{
    if(col2.r != 0.0) {
        float tmp = 1.0 - factor *col1.r;
        if(tmp <= 0.0)
            col2.r = 1.0;
        else if((tmp = col2.r/tmp) > 1.0)
            col2.r = 1.0;
        else
            col2.r = tmp;
    }
    if(col2.g != 0.0) {
        float tmp = 1.0 - factor *col1.g;
        if(tmp <= 0.0)
            col2.g = 1.0;
        else if((tmp = col2.g/tmp) > 1.0)
            col2.g = 1.0;
        else
            col2.g = tmp;
    }
    if(col2.b != 0.0) {
        float tmp = 1.0 - factor *col1.b;
        if(tmp <= 0.0)
            col2.b = 1.0;
        else if((tmp = col2.b/tmp) > 1.0)
            col2.b = 1.0;
        else
            col2.b = tmp;
    }
}

-- burnBlender
void burnBlender(vec4 col1, inout vec4 col2, float factor)
{
    float tmp, facm = 1.0 - factor ;

    tmp = facm + factor *col1.r;
    if(tmp <= 0.0)
        col2.r = 0.0;
    else if((tmp = (1.0 - (1.0 - col2.r)/tmp)) < 0.0)
        col2.r = 0.0;
    else if(tmp > 1.0)
        col2.r = 1.0;
    else
        col2.r = tmp;

    tmp = facm + factor *col1.g;
    if(tmp <= 0.0)
        col2.g = 0.0;
    else if((tmp = (1.0 - (1.0 - col2.g)/tmp)) < 0.0)
        col2.g = 0.0;
    else if(tmp > 1.0)
        col2.g = 1.0;
    else
        col2.g = tmp;

    tmp = facm + factor *col1.b;
    if(tmp <= 0.0)
        col2.b = 0.0;
    else if((tmp = (1.0 - (1.0 - col2.b)/tmp)) < 0.0)
        col2.b = 0.0;
    else if(tmp > 1.0)
        col2.b = 1.0;
    else
        col2.b = tmp;
}

-- hueBlender
#include blender.rgbToHsv
#include blender.hsvToRgb
void hueBlender(vec4 col1, inout vec4 col2, float factor)
{
    float facm = 1.0 - factor ;

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

-- satBlender
#include blender.rgbToHsv
#include blender.hsvToRgb
void satBlender( vec4 col1, inout vec4 col2, float factor)
{
    float facm = 1.0 - factor ;

    vec4 hsv = vec4(0.0);
    vec4 hsv2 = vec4(0.0);
    rgbToHsv(col2, hsv);

    if(hsv.y != 0.0) {
        rgbToHsv(col1, hsv2);
        hsv.y = facm*hsv.y + factor *hsv2.y;
        hsvToRgb(hsv, col2);
    }
}

-- valBlender
#include blender.rgbToHsv
#include blender.hsvToRgb
void valBlender(vec4 col1, inout vec4 col2, float factor)
{
    float facm = 1.0 - factor ;

    vec4 hsv, hsv2;
    rgbToHsv(col2, hsv);
    rgbToHsv(col1, hsv2);

    hsv.z = facm*hsv.z + factor *hsv2.z;
    hsvToRgb(hsv, col2);
}

-- colBlender
#include blender.rgbToHsv
#include blender.hsvToRgb
void colBlender(vec4 col1, inout vec4 col2, float factor)
{
    float facm = 1.0 - factor ;

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

-- softBlender
void softBlender(vec4 col1, inout vec4 col2, float factor)
{
    float facm = 1.0 - factor ;

    vec4 one = vec4(1.0);
    vec4 scr = one - (one - col1)*(one - col2);
    col2 = facm*col2 + factor *((one - col2)*col1*col2 + col2*scr);
}

-- linearBlender
void linearBlender(vec4 col1, inout vec4 col2, float factor)
{
    if(col1.r > 0.5)
        col2.r= col2.r + factor *(2.0*(col1.r - 0.5));
    else
        col2.r= col2.r + factor *(2.0*(col1.r) - 1.0);

    if(col1.g > 0.5)
        col2.g= col2.g + factor *(2.0*(col1.g - 0.5));
    else
        col2.g= col2.g + factor *(2.0*(col1.g) - 1.0);

    if(col1.b > 0.5)
        col2.b= col2.b + factor *(2.0*(col1.b - 0.5));
    else
        col2.b= col2.b + factor *(2.0*(col1.b) - 1.0);
}


