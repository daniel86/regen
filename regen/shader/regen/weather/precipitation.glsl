
-- spawnParticle
void spawnParticle(uint idx, inout uint seed)
{
    in_pos[idx] = variance(in_emitterCone.xyx, seed) + in_cameraPosition.xyz;
    in_velocity[idx] = variance(in_initialVelocity, seed);
    in_velocity[idx].y = -abs(in_velocity[idx].y);
#ifdef HAS_brightness
    in_brightness[idx] = in_initialBrightness.x + variance(in_initialBrightness.y, seed);
#endif
    in_lifetime[idx] = 1.0;
}

-- updateParticle
void updateParticle(uint idx, inout uint seed)
{
    float dt = in_timeDeltaMS*0.01;
    vec3 force = in_gravity;
#ifdef HAS_wind || HAS_windFlow
    force.xz += windAtPosition(in_pos[idx]) * in_windFactor;
#endif
    in_pos[idx] += in_velocity[idx]*dt;
    in_velocity[idx] += force*dt/in_mass;
    in_lifetime[idx] += dt;
}

-- isRespawnRequired
bool isRespawnRequired(uint idx) {
    bool isDead = (in_lifetime[idx]<0.01) ||
        (in_pos[idx].y<in_cameraPosition.y-in_emitterCone.y);
#ifdef HAS_mapCenter
    vec2 mapUV = (in_pos[idx].xz - in_mapCenter) / in_mapSize + 0.5;
    float heightAtPos = texture(in_heightMap, mapUV).r * in_mapFactor - in_mapFactor*0.5;
    isDead = isDead || (in_pos[idx].y < heightAtPos);
#endif
#ifdef HAS_surfaceHeight
    isDead = isDead || (in_pos[idx].y < in_surfaceHeight);
#endif
    return isDead;
}

-- update.includes
#include regen.states.camera.defines
#include regen.states.camera.input
#include regen.particles.emitter.inputs
#include regen.particles.emitter.defines

const vec3 in_gravity = vec3(-0.05,-0.5,0);
const vec3 in_initialVelocity = vec3(0.001,0.001,0.01);
// x: cone radius, y: cone height
const vec2 in_emitterCone = vec3(50.0,20.0);
#ifdef HAS_brightness
const vec2 in_initialBrightness = vec2(0.9,0.2);
#endif
const float in_mass = 1.0;

#include regen.noise.variance
#ifdef HAS_wind || HAS_windFlow
const float in_windFactor = 10.0;
    #include regen.weather.wind.windAtPosition
#endif

-- update.cs
#include regen.stages.compute.defines
#include regen.weather.precipitation.update.includes
#include regen.weather.precipitation.isRespawnRequired
#include regen.weather.precipitation.spawnParticle
#include regen.weather.precipitation.updateParticle

void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= NUM_PARTICLES) return;

    uint seed = in_randomSeed[gid];
    if(isRespawnRequired(gid)) {
        spawnParticle(gid, seed);
    }
    else {
        updateParticle(gid, seed);
    }
    in_randomSeed[gid] = seed;
}

-- draw.vs
in vec3 in_pos;
in vec3 in_velocity;

out vec3 out_pos;
out vec3 out_velocity;

#define HANDLE_IO(i)

void main() {
    out_pos = in_pos;
    out_velocity = in_velocity;
    HANDLE_IO(gl_VertexID);
}

-- draw.gs
layout(points) in;
layout(triangle_strip, max_vertices=4) out;

in vec3 in_pos[1];
in vec3 in_velocity[1];

out vec3 out_posEye;
out vec3 out_posWorld;
out vec2 out_spriteTexco;
flat out int out_layer;

#include regen.states.camera.input
uniform vec2 in_viewport;
uniform vec3 in_gravity;
uniform vec2 in_particleSize;

#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen
#include regen.math.computeSpritePoints2

#define HANDLE_IO(i)

void emitVertex(vec2 texco, vec3 posWorld, int layer, int vertexID)
{
    out_layer = 0;
    out_spriteTexco = texco;
    out_posWorld = posWorld;
    vec4 posEye = transformWorldToEye(out_posWorld,layer);
    out_posEye = posEye.xyz;
    gl_Position = transformEyeToScreen(posEye,layer);
    HANDLE_IO(vertexID);
    EmitVertex();
}
void emitSprite(vec3 quadPos[4], int layer)
{
    emitVertex(vec2(1.0,0.0),quadPos[0],layer,0);
    emitVertex(vec2(1.0,1.0),quadPos[1],layer,0);
    emitVertex(vec2(0.0,0.0),quadPos[2],layer,0);
    emitVertex(vec2(0.0,1.0),quadPos[3],layer,0);
    EndPrimitive();
}

void main() {
    vec3 zAxis = normalize(in_cameraPosition.xyz-in_pos[0]);
    vec3 yAxis = normalize(in_velocity[0]+in_gravity);
    vec3 quadPos[4] = computeSpritePoints(in_pos[0], in_particleSize, zAxis, yAxis);
    emitSprite(quadPos,out_layer);
}
