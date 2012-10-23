
-- mandelbrot

in vec2 in_texco;
out vec3 output;

uniform float mouseZoom;
uniform vec2 mouseOffset;

uniform float maxIterations;
uniform vec2 center;
uniform vec3 innerColor;
uniform vec3 outerColor1;
uniform vec3 outerColor2;

#ifdef JULIA_SET
uniform vec2 juliaConstants;
#endif

void main()
{
    vec2 pos = 2.5*(in_texco-vec2(0.5));
    vec2 z = pos*mouseZoom + center + mouseOffset;
#ifdef JULIA_SET
    vec2 c = juliaConstants;
#else
    vec2 c = z;
#endif
    float r2=0.0;
    float i=0.0;
    for(; i<maxIterations && r2<4.0; ++i) {
        z = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + c;
        r2 = dot(z, z);
    }
    if (r2 < 4.0) {
        output = innerColor;
    } else {
        output = mix(outerColor1, outerColor2, fract(i * 0.05));
    }
}

