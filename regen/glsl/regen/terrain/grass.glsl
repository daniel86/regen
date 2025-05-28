
-- sprite.gs
//#define SEPERATE_VIEW_PROJ
#include regen.states.camera.defines
#include regen.defines.all
#include regen.states.textures.defines
#ifdef HAS_PRIMITIVE_POINTS
layout(points) in;
#else
layout(triangles) in;
#endif
layout(triangle_strip, max_vertices=12) out;

#ifdef HAS_PRIMITIVE_POINTS
in vec3 in_posWorld[1];
    #ifdef HAS_VERTEX_MASK_MAP
in float in_mask[1];
    #endif
#else
in vec3 in_posWorld[3];
    #ifdef HAS_VERTEX_MASK_MAP
in float in_mask[3];
    #endif
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
#ifdef USE_SPRITE_LOD
const float in_lodGeomLevel0 = 60.0;
const float in_lodGeomLevel1 = 120.0;
const float in_lodGeomBrightness0 = 1.7;
const float in_lodGeomVariance = 0.2;
#endif

#ifdef HAS_windFlow || HAS_colliderRadius
    #define USE_FORCE
#endif
#ifdef HAS_colliderRadius || HAS_collisionMap
    #define USE_COLLISION
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
    #include regen.weather.wind.windAtPosition
#endif
#ifdef USE_COLLISION
    #include regen.shapes.collision.getCollisionVector
#endif
#ifdef HAS_VERTEX_MASK_MAP
const float in_maskThreshold = 0.1;
#endif

void main() {
#ifdef HAS_VERTEX_MASK_MAP
    #ifdef HAS_PRIMITIVE_POINTS
    float mask = in_mask[0];
    #else
    float mask = (in_mask[0] + in_mask[1] + in_mask[2]) / 3.0;
    #endif
    if (mask < in_maskThreshold) {
        return;
    }
#endif
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
#ifdef HAS_VERTEX_MASK_MAP
    size *= clamp(mask + 0.5, 0.0, 1.0);
#endif
    // align at the bottom - variation
    vec3 center = vec3(base.x, base.y + 0.5*size, base.z);
#ifdef HAS_posVariation
    center.x += (random(seed) - 0.5) * in_posVariation;
    center.z += (random(seed) - 0.5) * in_posVariation;
#endif
#ifdef HAS_offset
    center += in_offset;
#endif
#ifdef USE_COLLISION
    vec4 collision = getCollisionVector(center);
    const float collisionThreshold = 0.75;
    if (collision.w > collisionThreshold) { return; }
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
#ifdef USE_COLLISION
    force = mix(force,
        collision.xz * in_colliderStrength,
        collision.w / collisionThreshold);
#endif
#endif

#ifdef USE_SPRITE_LOD
    vec3 lodPos = center;
    lodPos.xz += in_lodGeomVariance*
        vec2(2.0*random(seed)-1.0, 2.0*random(seed)-1.0);
    float cameraDistance = length(in_cameraPosition - lodPos);

    if (cameraDistance < in_lodGeomLevel0) {
        out_col.rgb *= in_lodGeomBrightness0;
    #ifdef USE_FORCE
        emitSpriteCross(center, vec2(size), orientation, force);
    #else
        emitSpriteCross(center, vec2(size), orientation);
    #endif
    }
    else if (cameraDistance < in_lodGeomLevel1) {
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
// no culling needed as grass comes in small patches
#define NO_TESS_CULL
#include regen.models.mesh.tcs
-- patch.tes
#include regen.models.mesh.tes
-- patch.gs
#define HAS_PRIMITIVE_TRIANGLES
#include regen.terrain.grass.sprite.gs
-- patch.fs
#define HAS_nor
#define HAS_col
//#define DISCARD_ALPHA_THRESHOLD 0.25
in vec4 in_col;
#include regen.models.mesh.fs

-------------------
------ Grass where individual sprites are represented as points in the vertex buffer.
-------------------
-- vs
#include regen.models.sprite.vs
-- tcs
// no culling needed as grass comes in small patches
#define NO_TESS_CULL
#include regen.models.mesh.tcs
-- tes
#include regen.models.mesh.tes
-- gs
#define HAS_PRIMITIVE_POINTS
#include regen.terrain.grass.sprite.gs
-- fs
#define HAS_nor
#define HAS_col
//#define DISCARD_ALPHA_THRESHOLD 0.25
in vec4 in_col;
#include regen.models.sprite.fs
