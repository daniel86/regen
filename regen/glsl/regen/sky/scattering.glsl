
--------------------------------
--------------------------------
----- Computes a sky scattering cube map.
----- Code based on: http://codeflow.org/entries/2011/apr/13/advanced-webgl-part-2-sky-rendering/
--------------------------------
--------------------------------
-- vs
#include regen.sky.cube.vs

-- gs
#include regen.sky.cube.gs

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

#include regen.sky.utility.all

void main(void)
{
    vec3 eyedir = normalize(in_pos);
    vec3 eyePosition = vec3(0.0, surfaceHeight, 0.0);
    float eyeExtinction = getEyeExtinction(eyedir);
    float eyeDepth = getEyeDepth(eyedir);

    float stepLength = eyeDepth/float(stepCount);

    vec3 rayleighCollected = vec3(0.0, 0.0, 0.0);
    vec3 mieCollected = vec3(0.0, 0.0, 0.0);

    for(int i=0; i<stepCount; i++)
    {
        float sampleDistance = stepLength*float(i);
        vec3 position = eyePosition + eyedir*sampleDistance;
        float sampleExtinction = getHorizonExtinction(position, in_sunDir, surfaceHeight-0.35);
        float sampleDepth = getAtmosphericDepth(position, in_sunDir);
        
        vec3 influx = absorb(sampleDepth, vec3(intensity), in_scatterStrength)*sampleExtinction;
        rayleighCollected += absorb(sampleDistance, in_skyAbsorbtion*influx, in_rayleigh.y);
        mieCollected += absorb(sampleDistance, influx, in_mie.y);
    }

    float collectedFactor = eyeExtinction/float(stepCount);
    rayleighCollected *= pow(eyeDepth, in_rayleigh.z)*collectedFactor;
    mieCollected      *= pow(eyeDepth, in_mie.z)*collectedFactor;

    float alpha = dot(eyedir, in_sunDir);
    out_color.rgb  = smoothstep(0.0, 15.0, phase(alpha, 0.9995))*in_spotBrightness* mieCollected;
    out_color.rgb += phase(alpha, in_mie.w)*in_mie.x* mieCollected;
    out_color.rgb += phase(alpha, -0.01)*in_rayleigh.x * rayleighCollected;

    out_color.a = smoothstep(0.0,0.05,length(out_color.rgb));
}
