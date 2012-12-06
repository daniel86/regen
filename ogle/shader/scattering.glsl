-- vs
in vec3 in_pos;
out vec2 out_pos;

void main(void) {
    out_pos = vec2(in_pos.x, -in_pos.y);
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- gs
#extension GL_EXT_geometry_shader4 : enable

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;
layout(invocations = 5) in;

out vec2 out_texco;
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

-- fs

out vec3 output;
in vec3 in_pos;

uniform vec3 in_lightDir;
uniform vec3 in_skyColor;
// brightness, strength, collection power
uniform vec3 in_rayleigh;
// brightness, strength, collection power, distribution
uniform vec4 in_mie;
uniform float in_spotBrightness;
uniform float in_scatterStrength;

const float surfaceHeight = 0.99;
const float intensity = 1.8;
const int stepCount = 16;

float atmosphericDepth(vec3 position, vec3 dir)
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

float phase(float alpha, float g)
{
    float a = 3.0*(1.0-g*g);
    float b = 2.0*(2.0+g*g);
    float c = 1.0+alpha*alpha;
    float d = pow(1.0+g*g-2.0*g*alpha, 1.5);
    return (a/b)*(c/d);
}

float horizonExtinction(vec3 position, vec3 dir, float radius)
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

vec3 absorb(float dist, vec3 color, float factor)
{
    return color-color*pow(in_skyColor, vec3(factor/dist));
}

void main(void)
{
    vec3 eyedir = normalize(in_pos);
    float alpha = dot(eyedir, in_lightDir);

    float rayleighFactor = phase(alpha, -0.01)*in_rayleigh.x;
    float mieFactor = phase(alpha, in_mie.w)*in_mie.x;
    float spot = smoothstep(0.0, 15.0, phase(alpha, 0.9995))*in_spotBrightness;

    vec3 eyePosition = vec3(0.0, surfaceHeight, 0.0);
    float eyeDepth = atmosphericDepth(eyePosition, eyedir);
    float stepLength = eyeDepth/float(stepCount);
    float eyeExtinction = horizonExtinction(eyePosition, eyedir, surfaceHeight-0.15);

    vec3 rayleighCollected = vec3(0.0, 0.0, 0.0);
    vec3 mieCollected = vec3(0.0, 0.0, 0.0);

    for(int i=0; i<stepCount; i++)
    {
        float sampleDistance = stepLength*float(i);
        vec3 position = eyePosition + eyedir*sampleDistance;
        float extinction = horizonExtinction(position, in_lightDir, surfaceHeight-0.35);
        float sampleDepth = atmosphericDepth(position, in_lightDir);
        vec3 influx = absorb(sampleDepth, vec3(intensity), in_scatterStrength)*extinction;
        
        rayleighCollected += absorb(sampleDistance, in_skyColor*influx, in_rayleigh.y);
        mieCollected += absorb(sampleDistance, influx, in_mie.y);
    }

    rayleighCollected = (rayleighCollected*eyeExtinction*pow(eyeDepth, in_rayleigh.z))/float(stepCount);
    mieCollected = (mieCollected*eyeExtinction*pow(eyeDepth, in_mie.z))/float(stepCount);

    output = vec3(
        spot*mieCollected +
        mieFactor*mieCollected +
        rayleighFactor*rayleighCollected);
}

