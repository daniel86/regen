
-- collisionAtPosition
#ifndef REGEN_collisionAtPosition_Included_
#define REGEN_collisionAtPosition_Included_

vec4 collisionAtPosition(vec3 posWorld)
{
    vec3 colliderCenter = (in_modelMatrix_Collider * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    vec3 collisionVector = colliderCenter - posWorld;
    return vec4(collisionVector,
        1.0 - clamp(0.0, 1.0, length(collisionVector) / in_colliderRadius));
}
#endif // REGEN_windAtPosition_Included_

-- getCollisionVector
#ifndef REGEN_getCollisionVector_Included_
#define REGEN_getCollisionVector_Included_

// collision map area in world space x and z
const vec2 in_collisionMapArea = vec2(90.0,90.0);
// the center of the collision map in world space
const vec2 in_collisionMapCenter = vec2(0.0,0.0);

vec4 getCollisionVector(vec3 posWorld)
{
    vec3 dir = vec3(0.0);
    float collision = 0.0;
    float count = 0.0;
#ifdef HAS_modelMatrix_Collider
    vec3 colliderCenter = (in_modelMatrix_Collider * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    vec3 collisionVector = colliderCenter - posWorld;
    dir += normalize(collisionVector);
    collision += 1.0 - clamp(0.0, 1.0, length(collisionVector) / in_colliderRadius);
    count += 1.0;
#endif
#ifdef HAS_collisionMap
    #define2 COLLISION_ID ${TEX_ID_collisionMap}
    #define2 COLLISION_MAP_TX ${TEX_TEXEL_X${COLLISION_ID}}
    #define2 COLLISION_MAP_TY ${TEX_TEXEL_Y${COLLISION_ID}}
    // a collision map "in_collisionMap" is given. Finding the collision at the point is trivial.
    // however, the reflection vector mustbe computed. Since we only have collision
    // information, best we can do is to sample the neighboring points and compute
    // the reflection vector pointing towards the weakest collision.
    // the collision map UV coordinate for posWorld
    vec2 uv = (posWorld.zx - in_collisionMapCenter) / in_collisionMapArea + vec2(0.5);
    vec2 step = vec2(${COLLISION_MAP_TX},${COLLISION_MAP_TY}) * 3.0;
    float ct = texture(in_collisionMap, uv + vec2(0.0, step.y)).r;
    float cb = texture(in_collisionMap, uv - vec2(0.0, step.y)).r;
    float cl = texture(in_collisionMap, uv - vec2(step.x, 0.0)).r;
    float cr = texture(in_collisionMap, uv + vec2(step.x, 0.0)).r;
    collision += ct + cb + cl + cr + texture(in_collisionMap, uv).r;
    dir += vec3((ct - cb), 0.25, (cr - cl));
    count += 5.0;
#endif
    return vec4(dir,collision) / count;
}
#endif // REGEN_getCollisionVector_Included_
