-- isSphereVisible
#ifndef isSphereVisible_included_
#define isSphereVisible_included_
bool isSphereVisible(vec3 center, float radius) {
#for PLANE_I to 6
    if (in_frustumPlanes[${PLANE_I}].w + radius <
        dot(in_frustumPlanes[${PLANE_I}].xyz, center)) return false;
#endfor
    return true;
}
#endif // isSphereVisible_included_
