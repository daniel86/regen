
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
#include regen.models.mesh.gs

-- fs
#define HAS_nor
#define HAS_texco0
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
