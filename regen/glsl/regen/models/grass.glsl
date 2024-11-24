
-------------------
------
-------------------
-- patch.vs
#include regen.models.mesh.vs
-- patch.tcs
#include regen.models.mesh.tcs
-- patch.tes
#include regen.models.mesh.tes
-- patch.gs
#include regen.states.camera.defines
#include regen.defines.all
layout(triangles) in;
layout(triangle_strip, max_vertices=12) out;

in vec3 in_posWorld[3];
out vec3 out_posEye;
out vec3 out_posWorld;
out vec3 out_norWorld;
out vec4 out_col;
out vec2 out_texco0;

#include regen.states.camera.input
uniform vec2 in_viewport;
const vec2 in_quadSize = vec2(2.0, 0.3);

#ifdef HAS_windFlow || HAS_colliderRadius
    #define USE_FORCE
#endif
#define HAS_UPWARDS_NORMAL
#define HAS_UV_FADED_COLOR
//#define USE_SPRITE_LOD

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

float random (vec2 st) {
    return fract(sin(dot(st.xy,vec2(12.9898,78.233)))*43758.5453123);
}

void main() {
    vec3 triangleCenter = (in_posWorld[0] + in_posWorld[1] + in_posWorld[2]) / 3.0;
    float orientation = random(triangleCenter.zx) * 3.14159;
    float size = in_quadSize.x*(random(triangleCenter.xz) * in_quadSize.y + (1.0 - in_quadSize.y));
    // align at the bottom
    vec3 center = vec3(triangleCenter.x, triangleCenter.y + 0.5*size, triangleCenter.z);

    vec3 vColor = vec3(random(triangleCenter.xz)*0.3 + 0.7);
    out_col = vec4(vColor, 1.0);
    //out_norWorld = vec3(0,1,0);

#ifdef USE_FORCE
    vec2 force = vec2(0.0, 0.0);
#ifdef HAS_wind || HAS_windFlow
    force += windAtPosition(center);
#endif
#ifdef HAS_colliderRadius
    // TODO: support 2d collision map too.
    vec4 collision = collisionAtPosition(center);
    force = mix(force,
        normalize(collision.xyz).xz * in_colliderStrength,
        collision.w);
#endif
#endif

#ifdef USE_SPRITE_LOD
    const float LOD1 = 40.0f;
    const float LOD2 = 160.0f;
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
-- patch.fs
#define HAS_nor
#define HAS_col
#define DISCARD_ALPHA_THRESHOLD 0.25
in vec4 in_col;
#include regen.models.mesh.fs

-------------------
------
-------------------
-- vs
#include regen.models.sprite.vs

-- tcs
#include regen.models.mesh.tcs

-- tes
#include regen.models.mesh.tes

-- gs
layout(points) in;
layout(triangle_strip, max_vertices=12) out;

in vec3 in_pos[1];
out vec3 out_posEye;
out vec3 out_posWorld;
out vec3 out_norWorld;
out vec4 out_col;
out vec2 out_texco0;

#include regen.states.camera.input
uniform vec2 in_viewport;
const vec2 in_quadSize = vec2(2.0, 0.3);

#ifdef HAS_windFlow || HAS_colliderRadius
    #define USE_FORCE
#endif
#define HAS_UPWARDS_NORMAL
#define HAS_UV_FADED_COLOR
//#define USE_SPRITE_LOD

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

float random (vec2 st) {
    return fract(sin(dot(st.xy,vec2(12.9898,78.233)))*43758.5453123);
}

void main() {
    float orientation = random(in_pos[0].zx) * 3.14159;
    float size = in_quadSize.x*(random(in_pos[0].xz) * in_quadSize.y + (1.0 - in_quadSize.y));
    // align at the bottom
    vec3 center = vec3(in_pos[0].x, in_pos[0].y + 0.5*size, in_pos[0].z);

    vec3 vColor = vec3(random(in_pos[0].xz)*0.3 + 0.7);
    out_col = vec4(vColor, 1.0);
    //out_norWorld = vec3(0,1,0);

#ifdef USE_FORCE
    vec2 force = vec2(0.0, 0.0);
#endif
#ifdef HAS_wind || HAS_windFlow
    force += windAtPosition(center);
#endif
#ifdef HAS_colliderRadius
    // TODO: support 2d collision map too.
    vec4 collision = collisionAtPosition(center);
    force = mix(force,
        normalize(collision.xyz).xz * in_colliderStrength,
        collision.w);
#endif

#ifdef USE_SPRITE_LOD
    const float LOD1 = 40.0f;
    const float LOD2 = 160.0f;
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

-- fs
#define HAS_nor
#define HAS_col
#define DISCARD_ALPHA_THRESHOLD 0.25
in vec4 in_col;
#include regen.models.sprite.fs
