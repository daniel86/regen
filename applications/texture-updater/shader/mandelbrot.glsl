
-- vs
out vec2 out_texco;
#ifdef IS_VOLUME
out vec4 out_pos;
out int out_instanceID;
#endif

in vec3 in_pos;

void main()
{
#ifdef IS_VOLUME
    out_pos = vec4(in_pos.xy, 0.0, 1.0);
    out_instanceID = gl_InstanceID;
#endif
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- fs
in vec2 in_texco;
out vec3 out_color;

uniform float in_maxIterations;
uniform vec2 in_center;
uniform vec3 in_innerColor;
uniform vec3 in_outerColor1;
uniform vec3 in_outerColor2;

#ifdef JULIA_SET
uniform vec2 in_juliaConstants;
#endif

void main()
{
    vec2 pos = 2.5*(in_texco-vec2(0.5));
    //vec2 z = pos*in_mouseZoom + in_center + in_mouseOffset;
    vec2 z = pos + in_center;
#ifdef JULIA_SET
    vec2 c = in_juliaConstants;
#else
    vec2 c = z;
#endif
    float r2=0.0;
    float i=0.0;
    for(; i<in_maxIterations && r2<4.0; ++i) {
        z = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + c;
        r2 = dot(z, z);
    }
    if (r2 < 4.0) {
        out_color = in_innerColor;
    } else {
        out_color = mix(in_outerColor1, in_outerColor2, fract(i * 0.05));
    }
}

