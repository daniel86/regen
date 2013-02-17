-- cube.vs
in vec3 in_pos;
out vec2 out_pos;

void main(void) {
    out_pos = vec2(in_pos.x, -in_pos.y);
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- cube.gs
#extension GL_EXT_geometry_shader4 : enable

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;
layout(invocations = 5) in;

out vec3 out_pos;
in vec2 in_pos[3];

vec3 getCubePoint(vec2 p)
{
    vec3 cubePoints[5] = vec3[](
        vec3( 1.0, p.y,-p.x), // +X
        vec3(-1.0, p.y, p.x), // -X
        vec3( p.x, 1.0,-p.y), // +Y
        vec3( p.x, p.y, 1.0), // +Z
        vec3(-p.x, p.y,-1.0)  // -Z
    );
    return cubePoints[gl_InvocationID];
}

void main(void) {
    // select framebuffer layer
    gl_Layer = gl_InvocationID + int(gl_InvocationID>2);
    // emit vertices
    out_pos = getCubePoint(in_pos[0]);
    gl_Position = gl_PositionIn[0];
    EmitVertex();
    out_pos = getCubePoint(in_pos[1]);
    gl_Position = gl_PositionIn[1];
    EmitVertex();
    out_pos = getCubePoint(in_pos[2]);
    gl_Position = gl_PositionIn[2];
    EmitVertex();
    EndPrimitive();
}

--------------------------------
--------------------------------
--------------------------------

-- utility
uniform vec3 in_skyAbsorbtion;
const float surfaceHeight = 0.99;
const float intensity = 1.8;

float getHorizonExtinction(vec3 position, vec3 dir, float radius)
{
    float u = dot(dir, -position);
    if(u<0.0){
        return 1.0;
    }
    vec3 near = position + u*dir;
    if(length(near) < radius){
        return 0.0;
    }
    else{
        vec3 v2 = normalize(near)*radius - position;
        float diff = acos(dot(normalize(v2), dir));
        return smoothstep(0.0, 1.0, pow(diff*2.0, 3.0));
    }
}
float getEyeExtinction(vec3 eyedir)
{
    vec3 eyePosition = vec3(0.0, surfaceHeight, 0.0);
    return getHorizonExtinction(eyePosition, eyedir, surfaceHeight-0.15);
}

float getAtmosphericDepth(vec3 position, vec3 dir)
{
    float a = dot(dir, dir);
    float b = 2.0*dot(dir, position);
    float c = dot(position, position)-1.0;
    float det = b*b-4.0*a*c;
    float detSqrt = sqrt(det);
    float q = (-b - detSqrt)/2.0;
    float t1 = c/q;
    return t1;
}
float getEyeDepth(vec3 eyedir)
{
    vec3 eyePosition = vec3(0.0, surfaceHeight, 0.0);
    return getAtmosphericDepth(eyePosition, eyedir);
}

float phase(float alpha, float g)
{
    float a = 3.0*(1.0-g*g);
    float b = 2.0*(2.0+g*g);
    float c = 1.0+alpha*alpha;
    float d = pow(1.0+g*g-2.0*g*alpha, 1.5);
    return (a/b)*(c/d);
}

vec3 absorb(float dist, vec3 color, float factor)
{
    return color-color*pow(in_skyAbsorbtion, vec3(factor/dist));
}

--------------------------------
--------------------------------
--------------------------------

-- skyBox.vs
#extension GL_EXT_gpu_shader4 : enable
#include mesh.defines

in vec3 in_pos;

uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;

#include mesh.transformation

#define HANDLE_IO(i)

void main() {
    gl_Position = in_projectionMatrix * posEyeSpace(vec4(in_pos.xyz,1.0) );
    HANDLE_IO(gl_VertexID);
}

-- skyBox.fs
#include mesh.defines
#include textures.defines

out vec4 out_color;

#include textures.input
#include textures.mapToFragment

void main() {
    vec3 P = vec3(0.0);
    vec3 N = vec3(0.0);
    vec4 C = vec4(1.0);
    float A = 1.0;
    textureMappingFragment(P,N,C,A);
    
    out_color = C;
    gl_FragDepth = 1.0; // needs less or equal check
}

--------------------------------
--------------------------------
--------------------------------

-- scattering.vs
#include sky.cube.vs

-- scattering.gs
#include sky.cube.gs

-- scattering.fs
out vec4 output;
in vec3 in_pos;

uniform vec3 in_sunDir;
// brightness, strength, collection power
uniform vec3 in_rayleigh;
// brightness, strength, collection power, distribution
uniform vec4 in_mie;
uniform float in_spotBrightness;
uniform float in_scatterStrength;

const int stepCount = 16;

#include sky.utility

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
    output.rgb  = smoothstep(0.0, 15.0, phase(alpha, 0.9995))*in_spotBrightness* mieCollected;
    output.rgb += phase(alpha, in_mie.w)*in_mie.x* mieCollected;
    output.rgb += phase(alpha, -0.01)*in_rayleigh.x * rayleighCollected;

    output.a = smoothstep(0.0,0.05,length(output.rgb));
}

--------------------------------
--------------------------------
--------------------------------

-- starMap.vs
#include sky.cube.vs

-- starMap.gs
#include sky.cube.gs

-- starMap.fs
out vec4 output;
in vec3 in_pos;

uniform float in_starMapBrightness;
uniform mat4 in_starMapRotation;
uniform samplerCube starMap;

uniform vec3 in_sunDir;

#include sky.utility

void main() {
    vec4 eyedir = in_starMapRotation * vec4(normalize(in_pos),1.0);
    float eyeExtinction = getEyeExtinction(eyedir.xyz);
    vec4 starMapColor = texture(starMap,eyedir.xyz);
    output.rgb = starMapColor.rgb*in_starMapBrightness*eyeExtinction;
    output.a = 1.0;
}

--------------------------------
--------------------------------
--------------------------------

-- moon.vs
layout(location = 0) in vec4 in_moonPosition;
layout(location = 1) in vec3 in_moonColor;

out vec3 out_sunToMoon;
out vec3 out_moonColor;
out float out_moonIndex;
out float out_moonSize;

uniform vec3 in_sunDir;
uniform float in_sunDistance;

void main() {
    vec3 moonDirection = normalize(in_moonPosition.xyz);
    out_moonColor = in_moonColor;
    out_moonIndex = float(gl_VertexID);
    out_moonSize = in_moonPosition.w;
    out_sunToMoon = normalize(
        in_sunDir*in_sunDistance + in_moonPosition.xyz);
    gl_Position = vec4(moonDirection, 1.0);
}

-- moon.gs
#extension GL_EXT_geometry_shader4 : enable

layout(points) in;
layout(triangle_strip, max_vertices=12) out;

in float in_moonSize[1];
in float in_moonIndex[1];
in vec3 in_sunToMoon[1];
in vec3 in_moonColor[1];

out vec3 out_pos;
out vec2 out_spriteTexco;
flat out float out_moonIndex;
flat out vec3 out_sunToMoon;
flat out vec3 out_moonColor;

// look at matrices for each cube face
uniform mat4 in_mvpMatrices[6];

#include sprite.getSpritePoints
#include sprite.getSpriteLayer

void main() {
    vec3 pos = gl_PositionIn[0].xyz;
    vec3 quadPos[4] = getSpritePoints(pos, vec2(in_moonSize[0]));
    int quadLayer[3] = getSpriteLayer(pos);

    out_moonIndex = in_moonIndex[0];
    out_sunToMoon = in_sunToMoon[0];
    out_moonColor = in_moonColor[0];
    // emit geometry to cube layers
    for(int i=0; i<3; ++i) {
        gl_Layer = quadLayer[i];
        mat4 mvp = in_mvpMatrices[gl_Layer];

        out_spriteTexco = vec2(1.0,0.0);
        out_pos = quadPos[0];
        gl_Position = mvp*vec4(out_pos,1.0);
        EmitVertex();
        
        out_spriteTexco = vec2(1.0,1.0);
        out_pos = quadPos[1];
        gl_Position = mvp*vec4(out_pos,1.0);
        EmitVertex();
        
        out_spriteTexco = vec2(0.0,0.0);
        out_pos = quadPos[2];
        gl_Position = mvp*vec4(out_pos,1.0);
        EmitVertex();
        
        out_spriteTexco = vec2(0.0,1.0);
        out_pos = quadPos[3];
        gl_Position = mvp*vec4(out_pos,1.0);
        EmitVertex();
        EndPrimitive();
    }
}

-- moon.fs
out vec4 output;

in vec2 in_spriteTexco;
in vec3 in_pos;
flat in float in_moonIndex;
flat in vec3 in_sunToMoon;
flat in vec3 in_moonColor;

uniform sampler2DArray moonColorTexture;

#include sky.utility

vec3 fakeSphereNormal(vec2 texco) {
    vec2 x = texco*2.0 - vec2(1.0);
    return vec3(x, sqrt(1.0 - dot(x,x)));
}

void main() {
    vec3 eyedir = normalize(in_pos);
    float eyeExtinction = getEyeExtinction(eyedir.xyz);
    
    vec2 texco = vec2(in_spriteTexco.x, 1.0 - in_spriteTexco.y);
    // TODO: use normal map
    vec3 moonNormal = fakeSphereNormal(texco);
    float nDotL = dot(-moonNormal, in_sunToMoon);
    float blendFactor = nDotL;
    blendFactor *= eyeExtinction;

    vec4 moonColor = texture(moonColorTexture, vec3(texco,in_moonIndex));
    output.rgb = in_moonColor*moonColor.rgb*blendFactor;
    output.a = moonColor.a;
}

