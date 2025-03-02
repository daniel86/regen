
-- update.vs
#include regen.states.camera.defines
#include regen.states.camera.input
#include regen.particles.emitter.inputs
#include regen.particles.emitter.defines

const vec3 in_precipitationWind = vec3(-0.05,-0.5,0);
const vec3 in_precipitationVelocity = vec3(0.001,0.001,0.01);
// x: cone radius, y: cone height
const vec2 in_precipitationCone = vec3(50.0,20.0);
#ifdef HAS_brightness
const vec2 in_precipitationBrightness = vec2(0.9,0.2);
#endif

#include regen.noise.variance

bool isRespawnRequired()
{
    return (in_lifetime<0.01)
#ifdef HAS_surfaceHeight
        || (in_pos.y<in_surfaceHeight)
#endif
        || (in_pos.y<in_cameraPosition.y-in_precipitationCone.y);
}

void main() {
    // TODO: proper wind handling
    // TODO: allow to apply some rotational force to the particles, but maybe changing wind is enough
    uint seed = in_randomSeed;
    if(isRespawnRequired()) {
        out_pos = variance(in_precipitationCone.xyx, seed) + in_cameraPosition;
        out_velocity = variance(in_precipitationVelocity, seed);
#ifdef HAS_type
        out_type = int(floor(random(seed)*8.0));
#endif
#ifdef HAS_brightness
        out_brightness = in_precipitationBrightness.x + variance(in_precipitationBrightness.y, seed);
#endif
        out_lifetime = 1.0;
    }
    else {
        float dt = in_timeDeltaMS*0.01;
        out_pos = in_pos + (in_velocity+in_precipitationWind)*dt;
        out_velocity = in_velocity;
#ifdef HAS_type
        out_type = in_type;
#endif
#ifdef HAS_brightness
        out_brightness = in_brightness;
#endif
        out_lifetime = in_lifetime+dt;
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
uniform vec3 in_precipitationWind;
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
    emitVertex(vec2(1.0,1.0),quadPos[1],layer,1);
    emitVertex(vec2(0.0,0.0),quadPos[2],layer,2);
    emitVertex(vec2(0.0,1.0),quadPos[3],layer,3);
    EndPrimitive();
}

void main() {
    vec3 zAxis = normalize(in_cameraPosition-in_pos[0]);
    vec3 yAxis = normalize(in_velocity[0]+in_precipitationWind);
    vec3 quadPos[4] = computeSpritePoints(in_pos[0], in_particleSize, zAxis, yAxis);
    emitSprite(quadPos,out_layer);
}
