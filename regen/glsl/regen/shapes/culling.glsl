-- isSphereVisible
#ifndef isSphereVisible_included_
#define isSphereVisible_included_
bool isSphereVisible(vec3 center, float radius) {
    for (int i = 0; i < 6; ++i) {
        if (dot(in_frustumPlanes[i].xyz, center) + in_frustumPlanes[i].w < -radius)
            return false;
    }
    return true;
}
#endif // isSphereVisible_included_
