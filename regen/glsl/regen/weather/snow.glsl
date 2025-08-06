
-- update.vs
#include regen.weather.precipitation.update.vs

-- draw.vs
#include regen.weather.precipitation.draw.vs
-- draw.gs
#include regen.weather.precipitation.draw.gs
-- draw.fs
#include regen.models.mesh.defines

layout(location = 0) out vec4 out_color;
in float in_brightness;
in vec3 in_posWorld;
in vec2 in_spriteTexco;

#include regen.shading.direct.diffuse
#include regen.states.camera.linearizeDepth
#include regen.particles.sprite.softParticleScale

uniform sampler2D in_snowTexture;

void main() {
    vec3 P = in_posWorld.xyz;
    float density = in_brightness*texture(in_snowTexture, in_spriteTexco).x;
    vec3 diffuseColor = getDiffuseLight(P, gl_FragCoord.z);
    out_color = density*vec4(diffuseColor,1.0);
}
