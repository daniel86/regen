
--------------------------------
--------------------------------
----- Visualization of the moon.
----- Code based on: https://code.google.com/p/osghimmel/
--------------------------------
--------------------------------
-- vs
#include regen.models.mesh.defines

in vec3 in_pos;

out mat3 out_tangent;
out vec3 out_eye;
out vec2 out_texco;
#ifdef VS_LAYER_SELECTION
flat out int out_layer;
#endif

const float in_scale = 0.1;

#include regen.states.camera.transformWorldToScreen
#include regen.layered.VS_SelectLayer

#define HANDLE_IO(i)

void main(void) {
    vec3 m = in_lightDirection_Moon.xzy;
    vec3 u = normalize(cross(vec3(0, 1, 0), m));
    vec3 v = normalize(cross(u,m));
    out_eye = m - (in_pos.x*u + in_pos.y*v)*in_scale;
    out_tangent = mat3(u, v, m);
    out_texco = in_pos.xy;

    gl_Position = transformWorldToScreen(vec4(out_eye,0.0),0);
    gl_Position.z = gl_Position.w;
    VS_SelectLayer(regen_RenderLayer());
    HANDLE_IO(0);
}

-- gs
#include regen.models.mesh.gs

-- fs
#include regen.models.mesh.defines
#define PI_05 1.5707963267948966
#define TWO_OVER_THREEPI 0.2122065907891938

out vec4 out_color;
in mat3 in_tangent;
in vec3 in_eye;
in vec2 in_texco;

uniform samplerCube in_moonmapCube;

uniform float in_q;
uniform mat4 in_moonOrientationMatrix;
uniform vec4 in_cmn;

const vec4 in_sunShine = vec4(0.923,0.786,0.636,56.0);
const vec3 in_earthShine = vec3(0.88,0.96,1.00);
const float in_surface = 1.0;
const float in_scale = 0.1;
const float surfaceHeight = 0.99;

#include regen.weather.utility.computeEyeExtinction
#include regen.weather.utility.sunIntensity

// Hapke-Lommel-Seeliger approximation of the moons reflectance function.
// i between incident  (+sun) and surface
// r between reflected (-eye) and surface
// p as i + r
float brdf(float cos_r, float cos_i, float cos_p)
{
    // surface densitiy parameter which determines the sharpness of the peak at the full Moon
    float g = 0.6;
    // small amount of forward scattering
    float t = 0.1;
    // The following two params have various fixes applied
    float p = cos_p < 1.0 ? acos(cos_p) : 0.0;
    float p05 = abs(p + step(0, -p) * 0.000001) * 0.5;
    float tan_p = tan(p);
    // Retrodirective. - Formular (Hapke66.3)
    float s = 1 - step(PI_05, p);
    float e = - exp(-g / tan_p);
    float B = s * (2 - tan_p / (2 * g) * (1 + e) * (3 + e))  + (1 - s);
    // Scattering
    float S = (1 - sin(p05) * tan(p05) * log(tan(PI_05 - p05 * 0.5)))
	    + t * (1 - cos_p) * (1 - cos_p);
    // BRDF
    float F = TWO_OVER_THREEPI * B * S / (1.0 + cos_r / cos_i);
    // return 0.0 if cos_i>0
    return float(cos_i<=0) * (1.0 - step(0, cos_i)) * F;
}

#ifndef SKIP_SCATTER
const float in_scattering = 4.0;
#include regen.weather.utility.scatter
#endif

void main(void)
{
    vec3 eye = normalize(in_eye.xyz);
    float zz = 1.0 - in_texco.x * in_texco.x - in_texco.y * in_texco.y;
    
    // fov and size indepentent antialiasing
    float w  = smoothstep(0.0, 2.828 * in_q / in_scale, zz);
    float ext = computeEyeExtinction(eye);
    if(ext <= 0.0) discard;
    w *= smoothstep(0.0, 0.05, ext);

    vec3 v = vec3(in_texco, sqrt(zz));
    // horizontal space
    vec3 hn = in_tangent * v;
    // apply orientation for correct "FrontFacing" with optical librations in selenocentric space
    vec3 sn = normalize((vec4(v, 1.0) * in_moonOrientationMatrix).xyz);
    // fetch albedo and normals
    vec4 c  = texture(in_moonmapCube, sn);
    vec3 cn = vec3(c.r * 2 - 1, c.g * 2 - 1, c.b);
    // convert normals to selenocentric space
    vec3 s_n = mat3(v.zxy, v.yzx, v) * cn;
    // convert normals to horizontal space
    vec3 h_n = mix(hn, in_tangent * s_n, in_surface);
    // brdf
    float cos_p = dot(-eye, in_lightDirection_Sun.xyz);
    float cos_i = dot( in_lightDirection_Sun.xyz, h_n);
    float cos_r = dot(-eye, h_n);
    float f = brdf(cos_r, cos_i, cos_p);
    
    vec3 diffuse = (in_earthShine.rgb + in_sunShine.rgb * in_sunShine.w * f * sunIntensity())*c.a;

#ifndef SKIP_SCATTER
    diffuse *= (1 - in_scattering * scatter(acos(eye.y)));
#endif

    out_color = w * vec4(diffuse, 1.0);
}
