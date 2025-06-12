
-- spawnParticle
void spawnParticle(inout uint seed)
{
    out_pos = variance(in_emitterCone.xyx, seed) + in_cameraPosition.xyz;
    out_velocity = variance(in_initialVelocity, seed);
    out_velocity.y = -abs(out_velocity.y);
#ifdef HAS_brightness
    out_brightness = in_initialBrightness.x + variance(in_initialBrightness.y, seed);
#endif
    out_lifetime = 1.0;
}

-- updateParticle
void updateParticle(inout uint seed)
{
    float dt = in_timeDeltaMS*0.01;
    vec3 force = in_gravity;
#ifdef HAS_wind || HAS_windFlow
    force.xz += windAtPosition(in_pos) * in_windFactor;
#endif
    out_pos = in_pos + in_velocity*dt;
    out_velocity = in_velocity + force*dt/in_mass;
#ifdef HAS_brightness
    out_brightness = in_brightness;
#endif
    out_lifetime = in_lifetime+dt;
}

-- isRespawnRequired
bool isRespawnRequired()
{
    return (in_lifetime<0.01)
#ifdef HAS_surfaceHeight
        || (in_pos.y<in_surfaceHeight)
#endif
        || (in_pos.y<in_cameraPosition.y-in_emitterCone.y);
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

-- update.vs
#include regen.weather.precipitation.update.includes
#include regen.weather.precipitation.isRespawnRequired
#include regen.weather.precipitation.spawnParticle
#include regen.weather.precipitation.updateParticle

void main() {
    uint seed = in_randomSeed;
    if(isRespawnRequired()) {
        spawnParticle(seed);
    }
    else {
        updateParticle(seed);
    }
    out_randomSeed = seed;
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

out vec4 out_posEye;
out vec4 out_posWorld;
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
    out_posWorld = vec4(posWorld,1.0);
    out_posEye = transformWorldToEye(out_posWorld,layer);
    gl_Position = transformEyeToScreen(out_posEye,layer);
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
