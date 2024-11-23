
-- collisionAtPosition
#ifndef REGEN_collisionAtPosition_Included_
#define REGEN_collisionAtPosition_Included_

vec4 collisionAtPosition(vec3 posWorld)
{
    // FIXME: why HAS_colliderModelMat not defined?
    vec3 colliderCenter = (in_colliderModelMat * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    vec3 collisionVector = colliderCenter - posWorld;
    return vec4(collisionVector,
        1.0 - clamp(0.0, 1.0, length(collisionVector) / in_colliderRadius));
}
#endif // REGEN_windAtPosition_Included_
