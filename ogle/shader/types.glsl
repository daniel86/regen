
-- declaration
#ifndef HAS_MATERIAL
  #ifdef HAS_LIGHT
#define SHADING GOURAD
  #endif
#endif

#if SHADING != NONE
#ifdef HAS_LIGHT
  #ifdef SHADING == GOURAD
#define HAS_VERTEX_SHADING
  #else
#define HAS_FRAGMENT_SHADING
  #endif
#endif
#define HAS_SHADING
#else
// #undef HAS_SHADING
#endif

#ifdef HAS_SHADING
struct Shading {
  vec4 ambient;
  vec4 diffuse;
  vec4 specular;
  vec4 emission;
  float shininess;
};
#endif
#ifdef HAS_LIGHT
struct LightProperties {
    vec3 lightVec[NUM_LIGHTS];
    float attenuation[NUM_LIGHTS];
};
#endif

-- tes.interpolate
Shading interpolate(Shading shading[]) {
    Shading ret;
    ret.ambient = INTERPOLATE_STRUCT(shading,ambient);
    ret.diffuse = INTERPOLATE_STRUCT(shading,diffuse);
    ret.specular = INTERPOLATE_STRUCT(shading,specular);
    ret.emission = INTERPOLATE_STRUCT(shading,emission);
    ret.shininess = INTERPOLATE_STRUCT(shading,shininess);
    return ret;
}
LightProperties interpolate(LightProperties props[]) {
    LightProperties ret;
    for(int i=0; i<NUM_LIGHTS; ++i) {
        ret.lightVec[i] = INTERPOLATE_STRUCT(props,lightVec,i);
        ret.attenuation[i] = INTERPOLATE_STRUCT(props,attenuation,i);
    }
    return ret;
}

