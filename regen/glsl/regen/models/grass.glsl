
-- sprite.gs
//#define SEPERATE_VIEW_PROJ
#include regen.states.camera.defines
#include regen.defines.all
#ifdef HAS_PRIMITIVE_POINTS
layout(points) in;
#else
layout(triangles) in;
#endif
layout(triangle_strip, max_vertices=12) out;

#ifdef HAS_PRIMITIVE_POINTS
in vec3 in_posWorld[1];
#else
in vec3 in_posWorld[3];
#endif
out vec3 out_posEye;
out vec3 out_posWorld;
out vec3 out_norWorld;
out vec4 out_col;
out vec2 out_texco0;

#include regen.states.camera.input
uniform vec2 in_viewport;
const vec2 in_quadSize = vec2(2.0, 0.3);
const float in_posVariation = 0.2;

#ifdef HAS_windFlow || HAS_colliderRadius
    #define USE_FORCE
#endif
//#define HAS_UPWARDS_NORMAL
#define HAS_UV_FADED_COLOR
//#define USE_SPRITE_LOD

#include regen.noise.random2D
#include regen.models.sprite.emitSpriteCross
#ifdef USE_SPRITE_LOD
    #include regen.models.sprite.emitBillboard
#endif
#ifdef HAS_wind || HAS_windFlow
    #include regen.states.wind.windAtPosition
#endif
#ifdef HAS_colliderRadius
    #include regen.states.collision.collisionAtPosition
#endif

void main() {
#ifdef HAS_PRIMITIVE_POINTS
    // we can center the sprite at the input point
    vec3 base = in_posWorld[0];
#else
    // we can center the sprite at the triangle center
    // FIXME: I think this can cause artifacts at edges of tesselation patches.
    vec3 base = (in_posWorld[0] + in_posWorld[1] + in_posWorld[2]) / 3.0;
#endif
    // use xz position as seed to get smooth transition over the plane.
    // note that we need to have a constant seed for each sprite as we vary its properties
    // based on the seed, e.g. size, orientation, color. these should be constant for each sprite.
    vec2 seed = in_posWorld[0].xz/100.0;
    // random orientation
    float orientation = random(seed) * 6.283185;
    // max size - variation
    float size = in_quadSize.x + 2.0 * (random(seed)-0.5) * in_quadSize.y;
    // align at the bottom - variation
    vec3 center = vec3(base.x, base.y + 0.5*size, base.z);
#ifdef HAS_posVariation
    center.x += (random(seed) - 0.5) * in_posVariation;
    center.z += (random(seed) - 0.5) * in_posVariation;
#endif
#ifdef HAS_offset
    center += in_offset;
#endif

    // set intial output values
    out_col = vec4(vec3(random(seed)*0.3 + 0.7), 1.0);
#ifndef HAS_UPWARDS_NORMAL
    out_norWorld = vec3(0,1,0);
#endif

#ifdef USE_FORCE
    vec2 force = vec2(0.0, 0.0);
#ifdef HAS_wind || HAS_windFlow
    force += windAtPosition(center);
#endif
#ifdef HAS_colliderRadius
    // TODO: support 2d collision map too.
    //  - if rendering as post-effect, the depth buffer could be used! but then
    //     no shadows in usual pipeline.
    //  - maybe ping-pong depth buffer could get desired effect
    //  - one could render a 2d collision map in a separate pass.
    vec4 collision = collisionAtPosition(center);
    force = mix(force,
        normalize(collision.xyz).xz * in_colliderStrength,
        collision.w);
#endif
#endif

#ifdef USE_SPRITE_LOD
    const float LOD1 = 80.0f;
    const float LOD2 = 260.0f;
    // FIXME: emitSpriteCross is darker than emitBillboard due to three quads with some black at edges.
    float cameraDistance = length(in_cameraPosition - center);
    if (cameraDistance < LOD1) {
    #ifdef USE_FORCE
        emitSpriteCross(center, vec2(size), orientation, force);
    #else
        emitSpriteCross(center, vec2(size), orientation);
    #endif
    }
    else if (cameraDistance < LOD2) {
    #ifdef USE_FORCE
        emitBillboard(center, vec2(size), force);
    #else
        emitBillboard(center, vec2(size));
    #endif
    }
#else
    #ifdef USE_FORCE
    emitSpriteCross(center, vec2(size), orientation, force);
    #else
    emitSpriteCross(center, vec2(size), orientation);
    #endif
#endif
}

-------------------
------ A patch creates a sprite at the center of each triangle face of the input geometry.
------ Not that tessellation can be used to create more grass sprites e.g. for LOD.
-------------------
-- patch.vs
#include regen.models.mesh.vs
-- patch.tcs
#include regen.models.mesh.tcs
-- patch.tes
#include regen.models.mesh.tes
-- patch.gs
#define HAS_PRIMITIVE_TRIANGLES
#include regen.models.grass.sprite.gs
-- patch.fs
#define HAS_nor
#define HAS_col
#define DISCARD_ALPHA_THRESHOLD 0.25
in vec4 in_col;
#include regen.models.mesh.fs

-------------------
------ Grass where individual sprites are represented as points in the vertex buffer.
-------------------
-- vs
#include regen.models.sprite.vs
-- tcs
#include regen.models.mesh.tcs
-- tes
#include regen.models.mesh.tes
-- gs
#define HAS_PRIMITIVE_POINTS
#include regen.models.grass.sprite.gs
-- fs
#define HAS_nor
#define HAS_col
#define DISCARD_ALPHA_THRESHOLD 0.25
in vec4 in_col;
#include regen.models.sprite.fs
