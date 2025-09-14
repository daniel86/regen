
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
    vec4 color = vec4(vec3(random(seed)*0.3 + 0.7), 1.0);
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
    // TODO: support layered rendering here!
    float cameraDistance = length(REGEN_CAM_POS_(0) - lodPos);

    if (cameraDistance < in_lodGeomLevel0) {
        color.rgb *= in_lodGeomBrightness0;
    #ifdef USE_FORCE
        emitSpriteCross(center, vec2(size), color, orientation, force);
    #else
        emitSpriteCross(center, vec2(size), color, orientation);
    #endif
    }
    else if (cameraDistance < in_lodGeomLevel1) {
        out_col = color;
    #ifdef USE_FORCE
        emitBillboard(center, vec2(size), force);
    #else
        emitBillboard(center, vec2(size));
    #endif
    }
#else
    #ifdef USE_FORCE
    emitSpriteCross(center, vec2(size), color, orientation, force);
    #else
    emitSpriteCross(center, vec2(size), color, orientation);
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

-- vs
#include regen.models.mesh.defines
#ifdef HAS_windFlow || HAS_colliderRadius
    #define USE_FORCE
#endif
#ifdef HAS_colliderRadius || HAS_collisionMap
    #define USE_COLLISION
#endif

in vec3 in_pos;
out vec2 out_texco0;
out vec3 out_col;

#ifdef VS_LAYER_SELECTION
flat out int out_layer;
#define in_layer regen_RenderLayer()
#endif
#include regen.layered.VS_SelectLayer

#include regen.states.textures.input
#include regen.states.textures.texco_xz_plane

#define HANDLE_IO(i)

#include regen.states.camera.input

#include regen.models.tf.transformModel
#include regen.states.camera.transformWorldToScreen
#include regen.noise.random2D
#ifdef USE_FORCE
    #include regen.models.sprite.applyForceBase
#endif
#ifdef HAS_wind || HAS_windFlow
    #include regen.weather.wind.windAtPosition
#endif
#ifdef USE_COLLISION
    #include regen.shapes.collision.getCollisionVector
#endif

const float in_maskThreshold = 0.1; // threshold for the mask texture
const float in_uvDarken = 0.5;
const float in_collisionThreshold = 0.75;

#ifdef HAS_HEIGHT_MAP
ivec2 getHeightCoords(vec2 uv_xz) {
    // nearest texel fetch for height
    vec2 hSize = vec2(
        TEX_WIDTH${TEX_ID_heightTexture},
        TEX_HEIGHT${TEX_ID_heightTexture});
    return ivec2(clamp(uv_xz * hSize,
        vec2(0), hSize - vec2(1)));
}
#endif

void main() {
    int layer = regen_RenderLayer();

    vec4 baseWorld = transformModel(vec4(in_basePos,1.0));
    vec2 uv_xz = clamp(baseWorld.xz / in_planeSize + vec2(0.5), 0.0, 1.0);
#ifdef HAS_maskTexture
    float mask = texture(in_maskTexture, uv_xz)[VERTEX_MASK_INDEX];
#else
    float mask = 1.0;
#endif

    // use xz position as seed to get smooth transition over the plane.
    // note that we need to have a constant seed for each sprite as we vary its properties
    // based on the seed, e.g. size, orientation, color. these should be constant for each sprite.
    vec2 seed = baseWorld.xz*0.01;
    // randomized size
    float size = clamp(mask + 0.5, 0.0, 1.0) * (
        in_quadSize.x + (random(seed)-0.5) * in_quadSize.y);
    baseWorld.y += 0.5 * size;

    // Build this corner in world space around the center
    vec3 posModel = in_pos.xyz - in_basePos;
    vec3 up = REGEN_VIEW_INV_(layer)[1].xyz * (posModel.y * size); // camera +Y in world
    vec3 posWorld = baseWorld.xyz + up +
#ifdef HAS_offset
        in_offset +
#endif
        REGEN_VIEW_INV_(layer)[0].xyz * (posModel.x * size) + // camera +X in world
        REGEN_VIEW_INV_(layer)[2].xyz * (posModel.z * size);  // camera +Z in world
#ifdef HAS_HEIGHT_MAP
    // Apply height texture
    float height = texelFetch(in_heightTexture, getHeightCoords(uv_xz), 0).r
            * TEX_BLEND_FACTOR${TEX_ID_heightTexture};
    posWorld.y += height;
#endif
#ifdef HAS_posVariation
    // Randomize position
    posWorld.x += (random(seed) - 0.5) * in_posVariation;
    posWorld.z += (random(seed) - 0.5) * in_posVariation;
#endif

#ifdef USE_FORCE
    vec3 fixedPoint = posWorld - up;
    vec2 force = vec2(0.0, 0.0);
#ifdef HAS_wind || HAS_windFlow
    force += windAtPosition(fixedPoint.xyz);
#endif
#ifdef USE_COLLISION
    vec4 collision = getCollisionVector(fixedPoint);
    float collisionW = collision.w / in_collisionThreshold;
    mask *= step(clamp(collisionW, 0.0, 1.0), 0.8);
    force = mix(force, collision.xz * in_colliderStrength, collisionW);
#endif
    applyForceBase(posWorld, fixedPoint.xyz, force);
#endif

    gl_Position = transformWorldToScreen(vec4(posWorld.xyz,1.0),layer);
    out_texco0 = vec2(posModel.x + 0.5, posModel.y);
    // Randomized color
    out_col = vec3(random(seed)*0.5 + 0.5); // * in_uvDarken;
    gl_ClipDistance[0] = mask - in_maskThreshold;
    VS_SelectLayer(layer);
    HANDLE_IO(gl_VertexID);
}

-- gs
#define HAS_nor
#define HAS_texco0
//#define HAS_PRIMITIVE_POINTS
//#include regen.terrain.grass.sprite.gs
#include regen.models.mesh.gs
-- fs
#define HAS_nor
#define HAS_texco0
//#define HAS_col
//#include regen.models.mesh.fs

#include regen.models.mesh.defines
#include regen.models.mesh.fs-outputs
in vec3 in_col;

#include regen.states.camera.input
#include regen.states.material.input
#include regen.states.textures.input

#include regen.states.textures.mapToFragment

void main() {
    vec3 posWorld = vec3(0.0, 0.0, 0.0); // not used
    vec3 norWorld = vec3(0.0, 1.0, 0.0);
    vec4 color = vec4(1.0);
    textureMappingFragment(posWorld, color, norWorld);
#ifdef HAS_alphaDiscardThreshold
    if (color.a < in_alphaDiscardThreshold) discard;
#endif
#if OUTPUT_TYPE != DEPTH
    color.rgb *= in_col * in_matDiffuse.rgb;
    out_normal = vec4(0.5, 1.0, 0.5, 1.0);
    out_color = color;
    out_specular = vec4(0.0, 0.0, 0.0, 0.0);
#endif // OUTPUT_TYPE != DEPTH
}
