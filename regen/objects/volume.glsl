
-- intersectBox
bool intersectBox(vec3 origin, vec3 dir, out float t0, out float t1) {
    vec3 invR = 1.0 / dir;
    vec3 tbot = invR * (-in_halfVolumeSize-origin);
    vec3 ttop = invR * (+in_halfVolumeSize-origin);
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);
    t0 = max(max(tmin.x, tmin.y), tmin.z);
    t1 = min(min(tmax.x, tmax.y), tmax.z);
    return t0 < t1;
}

-- computeNormal
vec3 computeNormal(vec3 pos) {
    float dx =
        texture(in_volumeTexture, pos + vec3(in_rayStep, 0.0, 0.0)).x -
        texture(in_volumeTexture, pos - vec3(in_rayStep, 0.0, 0.0)).x;
    float dy =
        texture(in_volumeTexture, pos + vec3(0.0, in_rayStep, 0.0)).x -
        texture(in_volumeTexture, pos - vec3(0.0, in_rayStep, 0.0)).x;
    float dz =
        texture(in_volumeTexture, pos + vec3(0.0, 0.0, in_rayStep)).x -
        texture(in_volumeTexture, pos - vec3(0.0, 0.0, in_rayStep)).x;
    return vec3(-dx, -dy, -dz);
}

-- vs
#include regen.objects.mesh.defines

in vec3 in_pos;
#ifdef VS_CAMERA_TRANSFORM
out vec3 out_posWorld;
out vec3 out_posEye;
#endif

out vec3 out_rayOrigin;
out vec3 out_rayDirection;

#ifdef HAS_INSTANCES
flat out int out_instanceID;
#endif
#ifdef VS_LAYER_SELECTION
flat out int out_layer;
#define in_layer regen_RenderLayer()
#endif
#include regen.regen.VS_SelectLayer

#include regen.camera.camera.input

#include regen.objects.tf.transformModel
#ifdef VS_CAMERA_TRANSFORM
    #include regen.camera.camera.transformWorldToEye
    #include regen.camera.camera.transformEyeToScreen
#endif

#define HANDLE_IO()

void main() {
    int layer = regen_RenderLayer();
    vec4 posWorld = transformModel(vec4(in_pos,1.0));
#ifdef HAS_modelMatrix
    out_rayOrigin = (
        inverse(in_modelMatrix) *
        vec4(in_cameraPosition.xyz,1.0)).xyz;
#else
    out_rayOrigin = in_cameraPosition.xyz;
#endif
    out_rayDirection = in_pos.xyz - out_rayOrigin;

#ifdef VS_CAMERA_TRANSFORM
    vec4 posEye  = transformWorldToEye(posWorld,layer);
    gl_Position  = transformEyeToScreen(posEye,layer);
    out_posWorld = posWorld.xyz;
    out_posEye   = posEye.xyz;
#else
    gl_Position = posWorld;
#endif
#ifdef HAS_INSTANCES
    out_instanceID = gl_InstanceID + gl_BaseInstance;
#endif // HAS_INSTANCES
    VS_SelectLayer(layer);

    HANDLE_IO(gl_VertexID);
}
-- gs
#include regen.objects.mesh.gs
-- fs
#define DRAW_RAY_LENGTH 0
#define DRAW_RAY_START 0
#define DRAW_RAY_STOP 0
#ifndef RAY_CASTING_MODE
    #define RAY_CASTING_MODE FIRST_MAXIMUM
#endif
#include regen.objects.mesh.defines
#include regen.objects.mesh.fs-outputs
#include regen.regen.layer-defines

#if OUTPUT_TYPE == DEPTH
    #define FS_NO_OUTPUT
#endif

in vec3 in_rayOrigin;
in vec3 in_rayDirection;

#ifdef HAS_modelMatrix
uniform mat4 in_modelMatrix;
#endif
uniform sampler3D in_volumeTexture;
uniform sampler2D in_transferTexture;
uniform vec4 in_cameraPosition;

const float in_rayStep=0.02;
const float in_densityThreshold=0.125;
const float in_densityScale=2.0;
const vec3 in_halfVolumeSize = vec3(1.0);

#ifndef FS_NO_OUTPUT
    #include regen.textures.textures.mapToFragment
    #include regen.textures.textures.mapToLight
    #include regen.objects.mesh.writeOutput
#endif
#include regen.camera.camera.transformWorldToScreen
#include regen.objects.volume.intersectBox
#include regen.objects.volume.computeNormal

vec4 volumeTransfer(float val) {
    vec3 col = texture(in_transferTexture, vec2(val)).rgb;
#ifdef DENSITY_ALPHA
    return vec4(col, min(1.0,in_densityScale*val));
#else
    return vec4(col*1.6, 1.0);
#endif
}

void main() {
    vec3 rayDirection = normalize(in_rayDirection);
    // find start/end intersection of ray with volume box
    float tnear, tfar;
    intersectBox( in_rayOrigin, rayDirection, tnear, tfar);
    tnear = max(tnear,0.0);
    vec3 rayStart = in_rayOrigin + rayDirection * tnear;
    vec3 rayStop = in_rayOrigin + rayDirection * tfar;
#ifdef SWITCH_VOLUME_Y
    // switch y-axis
    rayStart.y *= -1; rayStop.y *= -1;
#endif
    // Transform from object space to texture coordinate space:
    rayStart = 0.5 * (rayStart + vec3(1.0));
    rayStop = 0.5 * (rayStop + vec3(1.0));
    // do the ray casting from start to stop
    vec3 ray = rayStop - rayStart;

    float rayLength = length(ray);
    int numSteps = int(rayLength / in_rayStep);
    vec3 stepVector = ray / float(numSteps);
    //vec3 stepVector = normalize(ray) * max(in_rayStep, 0.001);

    vec3 pos = rayStart;
    vec4 dst = vec4(0);
#if RAY_CASTING_MODE==AVERAGE_INTENSITY
    float counter = 0.0;
#endif
#if RAY_CASTING_MODE==MAX_INTENSITY
    float maxIntensity = 0.0;
#endif
    for(int i=0; i<numSteps; i++) {
#ifdef RAY_CAST_JITTER
        vec3 jitter = vec3(
            fract(sin(dot(pos.xy, vec2(12.9898, 78.233))) * 43758.5453),
            fract(sin(dot(pos.yz, vec2(12.9898, 78.233))) * 43758.5453),
            fract(sin(dot(pos.zx, vec2(12.9898, 78.233))) * 43758.5453)
        ) * in_rayStep * 0.5;
        float value = texture(in_volumeTexture, pos + jitter).x;
#else
        float value = texture(in_volumeTexture, pos).x;
#endif

#if RAY_CASTING_MODE==MAX_INTENSITY
        if(value > max(in_densityThreshold,maxIntensity)) {
            dst = volumeTransfer(value);
            maxIntensity = value;
        }
#elif RAY_CASTING_MODE==AVERAGE_INTENSITY
        if(value > in_densityThreshold) {
            dst += volumeTransfer(value);
            counter += 1.0;
        }
#elif RAY_CASTING_MODE==FIRST_MAXIMUM
        if(value > in_densityThreshold) {
            dst = volumeTransfer(value);
            break;
        }
#else // emission/absorbtion
        if(value > in_densityThreshold) {
            vec4 src = volumeTransfer(value);
            // opacity weighted color
            src.rgb *= src.a;
            // front-to-back blending
            dst = (1.0 - dst.a) * src + dst;
            // break out of the loop if alpha reached 1.0
            if(dst.a > 0.95) break;
        }
#endif
        pos += stepVector;
    }
#if RAY_CASTING_MODE==AVERAGE_INTENSITY
    dst /= counter;
#endif
#ifdef HAS_alphaDiscardThreshold
    if (dst.a < in_alphaDiscardThreshold) discard;
#endif
#ifdef DEPTH_CORRECT
    vec3 po = 2.0 * pos - vec3(1.0);
    #ifdef HAS_modelMatrix
    vec3 pw = (in_modelMatrix * vec4(po,1.0)).xyz;
    #else
    vec3 pw = po.z;
    #endif
    vec4 ps = transformWorldToScreen(vec4(pw,1.0),0);
    gl_FragDepth = (ps.z/ps.w)*0.5 + 0.5;
#endif
    vec3 normal = normalize(mat3(in_modelMatrix) * computeNormal(pos));

#ifndef FS_NO_OUTPUT
    #if DRAW_RAY_LENGTH==1
    writeOutput(pos,normal,vec4(vec3(length(ray)), 1.0));
    #elif DRAW_RAY_START==1
    writeOutput(pos,normal,vec4(rayStart, 1.0));
    #elif DRAW_RAY_STOP==1
    writeOutput(pos,normal,vec4(rayStop, 1.0));
    #else
    writeOutput(pos,normal,dst);
    #endif
#endif
}

-- neural.training-data.cs
#include regen.compute.compute.defines

buffer vec3 in_rayOrigin[];
buffer vec3 in_rayDirection[];

buffer vec3 in_position[];
buffer vec3 in_normal[];
buffer float in_density[];

uniform sampler3D in_volumeTexture;

const float in_rayStep=0.02;
const float in_densityThreshold=0.125;
const float in_densityScale=2.0;
const vec3 in_halfVolumeSize = vec3(1.0);

#include regen.objects.volume.intersectBox
#include regen.objects.volume.computeNormal

void main() {
    const uint id = gl_GlobalInvocationID.x;
    const uint numElements = CS_WORK_UNITS_X;
    if (id >= numElements) return;

    const vec3 origin = in_rayOrigin[id].xyz;
    vec3 dir = in_rayDirection[id].xyz;
    float len = length(dir);
    if (len > 0.0) {
        dir /= len;
    } else {
        // invalid ray
        in_position[id] = vec3(9999.0, 9999.0, 9999.0);
        in_normal[id] = vec3(0.0, 0.0, 0.0);
        in_density[id] = 0.0;
        return;
    }

    // We use mode=FIRST_MAXIMUM for generating training data
    float tnear, tfar;
    intersectBox( origin, dir, tnear, tfar);
    tnear = max(tnear,0.0);
    vec3 rayStart = origin + dir * tnear;
    vec3 rayStop = origin + dir * tfar;
#ifdef SWITCH_VOLUME_Y
    // switch y-axis
    rayStart.y *= -1; rayStop.y *= -1;
#endif

    // Transform from object space to texture coordinate space:
    rayStart = 0.5 * (rayStart + vec3(1.0));
    rayStop = 0.5 * (rayStop + vec3(1.0));

    // do the ray casting from start to stop
    vec3 ray = rayStop - rayStart;
    float rayLength = length(ray);
    int numSteps = int(rayLength / in_rayStep);
    vec3 stepVector = ray / float(numSteps);
    vec3 pos = rayStart;
    float density = 0.0;
    bool hit = false;

    for(int i=0; i<numSteps; i++) {
        float value = texture(in_volumeTexture, pos).x;
        if(value > in_densityThreshold) {
            density = value;
            hit = true;
            break;
        }
        pos += stepVector;
    }

    if(hit) {
        // Store position in object space
        vec3 po = 2.0 * pos - vec3(1.0);
        in_position[id] = po;
        in_normal[id] = normalize(computeNormal(pos));
        in_density[id] = density;
    } else {
        // No hit: write invalid values
        in_position[id] = vec3(9999.0, 9999.0, 9999.0);
        in_normal[id] = vec3(0.0, 0.0, 0.0);
        in_density[id] = 0.0;
    }
}
