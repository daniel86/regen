
--------------------------------
--------------------------------
----- Computes atmosphere cube map.
----- Code based on: http://codeflow.org/entries/2011/apr/13/advanced-webgl-part-2-sky-rendering/
--------------------------------
--------------------------------
-- vs
in vec3 in_pos;
out vec2 out_pos;

void main(void) {
    out_pos = vec2(in_pos.x, -in_pos.y);
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- gs
layout(triangles) in;
layout(triangle_strip, max_vertices=15) out;

out vec3 out_pos;
in vec2 in_pos[3];

vec3 getCubePoint(vec2 p, int i)
{
    vec3 cubePoints[5] = vec3[](
        vec3( 1.0, p.y,-p.x), // +X
        vec3(-1.0, p.y, p.x), // -X
        vec3( p.x, 1.0,-p.y), // +Y
        vec3( p.x, p.y, 1.0), // +Z
        vec3(-p.x, p.y,-1.0)  // -Z
    );
    return cubePoints[i];
}

void main(void) {
    for(int i=0; i<5; ++i) {
        // select framebuffer layer
        gl_Layer = i + int(i>2);
        
        out_pos = getCubePoint(in_pos[0],i);
        gl_Position = gl_in[0].gl_Position;
        EmitVertex();
        out_pos = getCubePoint(in_pos[1],i);
        gl_Position = gl_in[1].gl_Position;
        EmitVertex();
        out_pos = getCubePoint(in_pos[2],i);
        gl_Position = gl_in[2].gl_Position;
        EmitVertex();
        EndPrimitive();
    }
}

-- fs
out vec4 out_color;
in vec3 in_pos;

uniform vec3 in_sunDir;
// brightness, strength, collection power
uniform vec3 in_rayleigh;
// brightness, strength, collection power, distribution
uniform vec4 in_mie;
uniform float in_spotBrightness;
uniform float in_scatterStrength;

const int stepCount = 16;

uniform vec3 in_skyAbsorption;
const float surfaceHeight = 0.99;
const float intensity = 1.8;

#ifdef HAS_cirrus || HAS_cumulus
#define USE_CLOUDS
#endif
#ifdef HAS_cirrus || HAS_cumulus || HAS_dithering
#define USE_NOISE
#endif
#ifdef HAS_dithering
const float in_ditheringScale = 100.0;
#endif

#ifdef USE_NOISE
uniform float in_worldTime;
const mat3 m = mat3(0.0, 1.60,  1.20, -1.6, 0.72, -0.96, -1.2, -0.96, 1.28);

float hash(float n) {
    return fract(sin(n) * 43758.5453123);
}

float noise(vec3 x) {
    vec3 f = fract(x);
    float n = dot(floor(x), vec3(1.0, 157.0, 113.0));
    return mix(mix(mix(hash(n +   0.0), hash(n +   1.0), f.x),
                   mix(hash(n + 157.0), hash(n + 158.0), f.x), f.y),
               mix(mix(hash(n + 113.0), hash(n + 114.0), f.x),
                   mix(hash(n + 270.0), hash(n + 271.0), f.x), f.y), f.z);
}
#endif
#ifdef USE_CLOUDS
float fbm(vec3 p) {
    float f = 0.0;
    f += noise(p) / 2; p = m * p * 1.1;
    f += noise(p) / 4; p = m * p * 1.2;
    f += noise(p) / 6; p = m * p * 1.3;
    f += noise(p) / 12; p = m * p * 1.4;
    f += noise(p) / 24;
    return f;
}
const float in_cloudTimeFactor = 0.2;
#endif

#include regen.sky.utility.computeHorizonExtinction
#include regen.sky.utility.computeEyeExtinction
#include regen.sky.utility.computeAtmosphericDepth
#include regen.sky.utility.computeEyeDepth
#include regen.sky.utility.phase
#include regen.sky.utility.absorb

void main(void)
{
    vec3 eyedir = normalize(in_pos);
    vec3 eyePosition = vec3(0.0, surfaceHeight, 0.0);
    float eyeExtinction = computeEyeExtinction(eyedir);
    float eyeDepth = computeEyeDepth(eyedir);

    float stepLength = eyeDepth/float(stepCount);

    vec3 rayleighCollected = vec3(0.0, 0.0, 0.0);
    vec3 mieCollected = vec3(0.0, 0.0, 0.0);

    for(int i=0; i<stepCount; i++)
    {
        float sampleDistance = stepLength*float(i);
        vec3 position = eyePosition + eyedir*sampleDistance;
        float sampleExtinction = computeHorizonExtinction(position, in_sunDir, surfaceHeight-0.35);
        float sampleDepth = computeAtmosphericDepth(position, in_sunDir);
        
        vec3 influx = absorb(sampleDepth, vec3(intensity), in_scatterStrength)*sampleExtinction;
        rayleighCollected += absorb(sampleDistance, in_skyAbsorption*influx, in_rayleigh.y);
        mieCollected += absorb(sampleDistance, influx, in_mie.y);
    }

    float collectedFactor = eyeExtinction/float(stepCount);
    rayleighCollected *= pow(eyeDepth, in_rayleigh.z)*collectedFactor;
    mieCollected      *= pow(eyeDepth, in_mie.z)*collectedFactor;

    float alpha = dot(eyedir, in_sunDir);
    out_color.rgb  = smoothstep(0.0, 15.0, phase(alpha, 0.9995))*in_spotBrightness* mieCollected;
    out_color.rgb += phase(alpha, in_mie.w)*in_mie.x* mieCollected;
    out_color.rgb += phase(alpha, -0.01)*in_rayleigh.x * rayleighCollected;

#ifdef USE_CLOUDS
    float cloud_t = in_worldTime * 0.0001 * in_cloudTimeFactor;
    float density;
    #ifdef HAS_cirrus
    density = smoothstep(
        1.0 - in_cirrus, 1.0,
        fbm(in_pos.xyz / in_pos.y * 2.0 + cloud_t)) * 0.3;
    out_color.rgb = mix(out_color.rgb, vec3(collectedFactor) * 4.0, density * max(in_pos.y, 0.0));
    #endif
    #ifdef HAS_cumulus
    for (int i = 0; i < 3; i++) {
        density = smoothstep(
            1.0 - in_cumulus, 1.0,
            fbm((0.7 + float(i) * 0.01) * in_pos.xyz / in_pos.y + cloud_t));
        out_color.rgb = mix(out_color.rgb, vec3(collectedFactor) * density * 5.0, min(density, 1.0) * max(in_pos.y, 0.0));
    }
    #endif
#endif
#ifdef HAS_dithering && USE_NOISE
    out_color.rgb += noise(in_pos * in_ditheringScale) * 0.01 * in_dithering;
#endif

    out_color.a = smoothstep(0.0,0.05,length(out_color.rgb));
}
