-- scattering.vs
in vec3 in_pos;
out vec2 out_pos;

void main(void) {
    out_pos = vec2(in_pos.x, -in_pos.y);
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- scattering.gs
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

-- scattering.fs
out vec3 output;
in vec3 in_pos;

uniform mat4 in_planetRotation;

uniform float in_brightStarVisibility;
uniform samplerCube brightStarMap;

uniform float in_milkyWayVisibility;
uniform samplerCube milkyWayMap;

uniform vec3 in_lightDir;
uniform vec3 in_skyAbsorbtion;
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
    return color-color*pow(in_skyAbsorbtion, vec3(factor/dist));
}

void main(void)
{
    vec3 eyedir = normalize(in_pos);

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
        rayleighCollected += absorb(sampleDistance, in_skyAbsorbtion*influx, in_rayleigh.y);
        mieCollected += absorb(sampleDistance, influx, in_mie.y);
    }

    float collectedFactor = eyeExtinction/float(stepCount);
    rayleighCollected *= pow(eyeDepth, in_rayleigh.z)*collectedFactor;
    mieCollected      *= pow(eyeDepth, in_mie.z)*collectedFactor;

    float alpha = dot(eyedir, in_lightDir);
    output  = smoothstep(0.0, 15.0, phase(alpha, 0.9995))*in_spotBrightness* mieCollected;
    output += phase(alpha, in_mie.w)*in_mie.x* mieCollected;
    output += phase(alpha, -0.01)*in_rayleigh.x * rayleighCollected;
    
    vec4 rotatedPos = in_planetRotation * vec4(in_pos,1.0);
    float nightBlend = eyeExtinction * (1.0-alpha);
    if(in_milkyWayVisibility>0.0) {
        vec4 mwColor = texture(milkyWayMap, rotatedPos.xyz);
        output += mwColor.rgb * nightBlend * in_milkyWayVisibility;
    }
    if(in_brightStarVisibility>0.0) {
        vec4 starColor = texture(brightStarMap, rotatedPos.xyz);
        output += starColor.rgb * nightBlend * in_brightStarVisibility * starColor.a;
    }
}

--------------------------------
--------------------------------
--------------------------------

-- stars.vs
layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec4 in_color;

out vec4 out_color;

void main(void) {
    out_color = in_color;
    gl_Position = vec4(in_pos, 1.0);
}

-- stars.gs
#extension GL_EXT_geometry_shader4 : enable

layout(points) in;
layout(triangle_strip, max_vertices=36) out;

in vec4 in_color[1];

flat out vec4 out_color;

out vec2 out_texco;
out vec3 out_pos;

uniform mat4 in_mvpMatrices[6];

const vec4 quadPoints[4] = vec4[](
    vec4(-1.0, 1.0,0.0,1.0),
    vec4(-1.0,-1.0,0.0,1.0),
    vec4( 1.0,-1.0,0.0,1.0),
    vec4( 1.0, 1.0,0.0,1.0)
);
const vec2 quadTexco[4] = vec2[](
    vec2(0.0,1.0), vec2(0.0,0.0),
    vec2(1.0,0.0), vec2(1.0,1.0) );

// lookAt(pos=(0,0,0), dir=f, up=(0,1,0))
mat4 getLookAtMatrixXZ(vec3 f) {
  return mat4(
        -f.z,               0.0,      f.x,  0.0,
    -f.x*f.y, f.x*f.x + f.z*f.z, -f.z*f.y,  0.0,
        -f.x,              -f.y,     -f.z, 0.0,
         0.0,               0.0,      0.0, 1.0
  );
}
mat4 getLookAtMatrixY(vec3 f) {
  return mat4(
       -f.y,     f.x,               0.0, 0.0,
    f.x*f.z, f.y*f.z, -f.y*f.y -f.x*f.x, 0.0,
       -f.x,    -f.y,              -f.z, 0.0,
        0.0,     0.0,               0.0, 1.0
  );
}

void main()
{
    vec3 pos = gl_PositionIn[0].xyz;
    float starSize = length(pos);
    pos = normalize(pos);

    mat4 spriteRotation;
    if(pos.y>abs(pos.x) && pos.y>abs(pos.z)) {
      spriteRotation = getLookAtMatrixY(pos);
    }
    else {
      spriteRotation = getLookAtMatrixXZ(pos);
    }
    // find sprite positions
    vec3 quadPos[] = vec3[](
        pos + (spriteRotation*(quadPoints[0]*starSize)).xyz,
        pos + (spriteRotation*(quadPoints[1]*starSize)).xyz,
        pos + (spriteRotation*(quadPoints[2]*starSize)).xyz,
        pos + (spriteRotation*(quadPoints[3]*starSize)).xyz
    );
    
    // color does not change for emitted vertices
    out_color = in_color[0];
    
    // emit geometry to cube layers
    // TODO: skip unneeded layers.
    for(int layer=0; layer<6; ++layer)
    {
        gl_Layer = layer;

        vec4 eyePos[] = vec4[](
            in_mvpMatrices[layer]*vec4(quadPos[0],1.0),
            in_mvpMatrices[layer]*vec4(quadPos[1],1.0),
            in_mvpMatrices[layer]*vec4(quadPos[2],1.0),
            in_mvpMatrices[layer]*vec4(quadPos[3],1.0)
        );
        
        gl_Position = eyePos[0];
        out_texco = quadTexco[0];
        EmitVertex();
        gl_Position = eyePos[1];
        out_texco = quadTexco[1];
        EmitVertex();
        gl_Position = eyePos[3];
        out_texco = quadTexco[3];
        EmitVertex();
        EndPrimitive();
        
        gl_Position = eyePos[1];
        out_texco = quadTexco[1];
        EmitVertex();
        gl_Position = eyePos[2];
        out_texco = quadTexco[2];
        EmitVertex();
        gl_Position = eyePos[3];
        out_texco = quadTexco[3];
        EmitVertex();
        EndPrimitive();
    }
}

-- stars.fs
flat in vec4 in_color;
in vec2 in_texco;

out vec4 output;

void main()
{
    float density = 1.0 - 2.0*length(in_texco - vec2(0.5));
    output = vec4(in_color.rgb, density*in_color.a);
}

