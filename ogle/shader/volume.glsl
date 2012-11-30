// Shader for Volume rendering

--------------------------------------------
------------- Vertex Shader ----------------
--------------------------------------------

-- vs
#undef HAS_LIGHT
#undef HAS_MATERIAL

out vec3 out_posWorld;

in vec3 in_pos;

#ifdef HAS_MODELMAT
uniform mat4 in_modelMatrix;
#endif
uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;

#include mesh.transformation

#define HANDLE_IO()

void main() {
    vec4 posWorld = posWorldSpace(in_pos);
    out_posWorld = posWorld.xyz;
    vec4 posEye = posEyeSpace(posWorld);

    gl_Position = in_projectionMatrix * posEye;

    HANDLE_IO(gl_VertexID);
}

--------------------------------------------
--------- Tesselation Control --------------
--------------------------------------------

-- tcs

void main() {}

--------------------------------------------
--------- Tesselation Evaluation -----------
--------------------------------------------

-- tes

void main() {}

--------------------------------------------
--------- Geometry Shader ------------------
--------------------------------------------

-- gs

void main() {}

--------------------------------------------
--------- Fragment Shader ------------------
--------------------------------------------

-- fs
#undef HAS_LIGHT
#undef HAS_MATERIAL

#define DRAW_RAY_LENGTH 0
#define DRAW_RAY_START 0
#define DRAW_RAY_STOP 0

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_specular;
layout(location = 2) out vec4 out_norWorld;
layout(location = 3) out vec3 out_posWorld;

in vec3 in_posWorld;

uniform sampler3D in_volumeTexture;
uniform mat4 in_inverseViewMatrix;

const float in_rayStep=(1.0/50.0);

vec4 volumeTransfer(vec4 color) {
    float val = color.r;
    return vec4(0.0066,0.0666,0.6666,0.5*val);
}

bool intersectBox(vec3 origin, vec3 dir,
           vec3 minBound, vec3 maxBound,
           out float t0, out float t1)
{
    vec3 invR = 1.0 / dir;
    vec3 tbot = invR * (minBound-origin);
    vec3 ttop = invR * (maxBound-origin);
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);
    vec2 t = max(tmin.xx, tmin.yz);
    t0 = max(t.x, t.y);
    t = min(tmax.xx, tmax.yz);
    t1 = min(t.x, t.y);
    return t0 < t1;
}

void main() {
    vec3 rayOrigin_ = in_inverseViewMatrix[3].xyz;
    vec3 rayDirection = normalize(in_posWorld.xyz - in_inverseViewMatrix[3].xyz);

    float tnear, tfar;
    if(!intersectBox( rayOrigin_, rayDirection, 
           vec3(-1.0), vec3(+1.0), tnear, tfar))
    {
        discard;
    }
    if (tnear < 0.0) tnear = 0.0;
    

    vec3 rayStart = rayOrigin_ + rayDirection * tnear;
    vec3 rayStop = rayOrigin_ + rayDirection * tfar;
    rayStart.y *= -1; rayStop.y *= -1;
    
    // Transform from object space to texture coordinate space:
    rayStart = 0.5 * (rayStart + 1.0);
    rayStop = 0.5 * (rayStop + 1.0);
    
    vec3 ray = rayStop - rayStart;
    vec3 stepVector = normalize(ray) * in_rayStep;
    
    vec3 pos = rayStart;
    vec4 dst = vec4(0);
    for(float rayLength=length(ray); rayLength>0.0; rayLength-=in_rayStep)
    {
        vec4 src = volumeTransfer(texture(in_volumeTexture, pos));
        // opacity weighted color
        src.rgb *= src.a;
        // front-to-back blending
        dst = (1.0 - dst.a) * src + dst;
        pos += stepVector;
        // break out of the loop if alpha reached 1.0
        if(dst.a > 0.999) break;
    }

    out_norWorld = vec4(0.0);
    out_posWorld = in_posWorld;
    out_specular = vec4(0.0);
#if DRAW_RAY_LENGTH==1
    out_color = vec4(vec3(length(ray)), 1.0);
#elif DRAW_RAY_START==1
    out_color = vec4(rayStart, 1.0);
#elif DRAW_RAY_STOP==1
    out_color = vec4(rayStop, 1.0);
#else
    out_color = dst;
#endif
}

