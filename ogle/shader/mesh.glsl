-- material
#ifdef HAS_MATERIAL
uniform vec4 in_matEmission;
uniform vec4 in_matSpecular;
uniform vec4 in_matAmbient;
uniform vec4 in_matDiffuse;
uniform float in_matShininess;
uniform float in_matShininessStrength;
uniform float in_matRefractionIndex;
uniform float in_matRoughness;
uniform float in_matAlpha;
uniform float in_matReflection;

void materialShading(inout Shading sh) {
    sh.ambient *= in_matAmbient;
    sh.diffuse *= in_matDiffuse;
    sh.specular *= in_matShininessStrength * in_matSpecular;
    sh.emission *= in_matEmission;
}
#endif

-- transformation
#ifdef HAS_TANGENT_SPACE
vec3 getTangent(v) {
    return normalize( in_tan.xyz );
}
vec3 getBinormal(vec3 tangent, vec3 nor) {
    return cross(nor, tangent) * in_tan.w;
}
#endif
#ifdef HAS_BONES
void boneTransformation(inout vec4 v) {
    vec4 bone = vec4(0.0);
    for(int i=0; i<NUM_BONE_WEIGTHS; ++i) {
        bone += in_boneWeights[i] * in_boneMatrices[in_boneIndices[i]] * v;
    }
    v = bone;
}
#endif
vec4 posWorldSpace(vec3 pos) {
    vec4 pos_ws = vec4(pos.xyz,1.0);
#ifdef HAS_BONES
    boneTransformation(pos_ws);
#endif
#ifdef HAS_MODELMAT
    pos_ws = in_modelMat * pos_ws;
#endif
    return pos_ws;
}
vec4 posEyeSpace(vec4 ws) {
#ifdef IGNORE_VIEW_ROTATION
    return vec4(in_viewMatrix[3].xyz,0.0) + ws;
#elif IGNORE_VIEW_TRANSLATION
    return mat4(in_viewMatrix[0], in_viewMatrix[1], in_viewMatrix[2], vec3(0.0), 1.0) * ws;
#else
    return in_viewMatrix * ws;
#endif
}
#ifdef HAS_TANGENT_SPACE
vec3 posTanSpace(vec3 t, vec3 b, vec3 n, vec3 v) {
    vec3 tanSpace;
    tanSpace.x = dot( v, t );
    tanSpace.y = dot( v, b );
    tanSpace.z = dot( v, n );
    return tanSpace;
}
#endif

vec3 norWorldSpace(vec3 nor) {
    // FIXME normal transform is wrong for scaled objects
    vec4 ws = vec4(nor.xyz,0.0);
#if HAS_BONES
    boneTransformation(ws);
#endif
#ifdef HAS_MODELMAT
    ws = in_modelMat * ws;
#endif
    return normalize(ws);
}

-- vs.header
//      shadeProperties(inout LightProperties props, vec4 posWorld)
//      shade(LightProperties props, inout Shading shading, vec3 norWorld)
//      modifyTransformation(inout vec4 posWorld, inout vec3 norWorld)
//      FINALIZE()
in vec3 in_pos;
#ifndef HAS_TESSELATION
out vec3 out_posWorld;
out vec3 out_posEye;
  #ifdef HAS_TANGENT_SPACE
out vec3 out_posTan;
  #endif
#endif // !HAS_TESSELATION

#ifdef HAS_INSTANCES
flat out int out_instanceID;
#endif

#ifdef HAS_NORMAL
in vec3 in_nor;
out vec3 out_norWorld;
#endif

#ifdef HAS_COLOR
in vec3 in_col;
out vec3 out_col;
#endif

#ifdef HAS_FRAGMENT_SHADING
out LightProperties out_lightProperties;
#elif HAS_VERTEX_SHADING
out Shading out_shading;
#endif

#include mesh.transformation

#include shading.types
#ifdef HAS_VERTEX_SHADING
#include shading.shade
#endif
#include shading.shadeInit

#include mesh.material

-- vs.main
void main() {
    vec4 posWorld = posWorldSpace(in_pos);
#ifdef HAS_NORMAL
    out_nor = norWorldSpace(in_nor);
#endif

    // position transformation
#ifndef HAS_TESSELATION
    out_posWorld = posWorld;
    // allow textures to modify texture/normal
  #ifdef HAS_NORMAL
    modifyTransformation(out_posWorld,out_nor);
  #else
    modifyTransformation(out_posWorld,vec3(0,1,0));
  #endif
    out_posEye = posEyeSpace(out_posWorld);
  #ifdef HAS_TANGENT_SPACE
    out_posTan = posTanSpace(getTangent(), getBinormal(), out_nor, posWorld);
  #endif
    gl_Position = in_projectionMatrix * out_posEye;
#else // !HAS_TESSELATION
    gl_Position = posWorld; // let TES do the transformations
#endif // HAS_TESSELATION

#ifdef HAS_INSTANCES
    out_instanceID = gl_InstanceID;
#endif // HAS_INSTANCES

#ifdef HAS_COLOR
    out_col = in_col;
#endif // HAS_COLOR

#ifdef HAS_VERTEX_SHADING
    // calculate shading for FS
    LightProperties lightProperties;
    shadeProperties(lightProperties, posWorld);
  #ifdef HAS_TANGENT_SPACE
    // TODO: lightVec to tan space...
  #endif
    // gourad shading
    out_shading = newShading();
  #ifdef HAS_MATERIAL
    out_shading.shininess = in_matShininess;
  #endif
    shade(lightProperties, out_shading, out_nor);
  #ifdef HAS_MATERIAL
    materialShading(out_shading);
  #endif
#elif HAS_FRAGMENT_SHADING
    // calculate light properties for FS
    shadeProperties(out_lightProperties, posWorld);
  #ifdef HAS_TANGENT_SPACE
    // TODO: lightVec to tan space...
  #endif
#endif // HAS_VERTEX_SHADING
    FINALIZE();
}

-- tcs
//      FINALIZE()

#ifdef IS_QUAD
layout(num_vertices=4) out;
#else
layout(num_vertices=3) out;
#endif

#ifdef HAS_INSTANCES
flat in int in_instanceID[];
flat out int out_instanceID[];
#endif
#ifdef HAS_NORMAL
in vec3 in_norWorld[];
in vec3 out_norWorld[];
#endif
#ifdef HAS_COLOR
in vec3 in_col[];
in vec3 out_col[];
#endif
#ifdef HAS_FRAGMENT_SHADING
in LightProperties in_lightProperties[];
out LightProperties out_lightProperties[];
#elif HAS_VERTEX_SHADING
in Shading in_shading[];
out Shading out_shading[];
#endif

#include tesselation.tesselationControl

void main() {
    tesselationControl();
    gl_out[ID].gl_Position = gl_in[ID].gl_Position;
#ifdef HAS_INSTANCES
    out_instanceID[ID] = in_instanceID[ID];
#endif
#ifdef HAS_NORMAL
    out_norWorld[ID] = in_norWorld[ID];
#endif
#ifdef HAS_COLOR
    out_col[ID] = in_col[ID];
#endif
#ifdef HAS_FRAGMENT_SHADING
    out_lightProperties[ID] = in_lightProperties[ID];
#elif HAS_VERTEX_SHADING
    out_shading[ID] = in_shading[ID];
#endif
    FINALIZE();
}

-- tes
//      applyTransformationMaps(inout vec4 posWorld, inout vec3 norWorld)
//      FINALIZE()
layout(
#ifdef IS_QUAD
      quads
#else
      triangles
#endif
#ifdef FRACTIONAL_ODD_SPACING
    , fractional_odd_spacing
#else
    , equal_spacing
#endif
#ifdef IS_CCW
    , ccw
#else
    , cw
#endif
) in;

out vec3 out_posWorld;
out vec3 out_posEye;
#ifdef HAS_TANGENT_SPACE
out vec3 out_posTan;
#endif
#ifdef HAS_INSTANCES
flat in int in_instanceID[];
flat out int out_instanceID;
#endif
#ifdef HAS_NORMAL
in vec3 in_norWorld[];
in vec3 out_norWorld;
#endif
#ifdef HAS_COLOR
in vec3 in_col[];
in vec3 out_col;
#endif
#ifdef HAS_FRAGMENT_SHADING
in LightProperties in_lightProperties[];
out LightProperties out_lightProperties;
#elif HAS_VERTEX_SHADING
in Shading in_shading[];
out Shading out_shading;
#endif

void main() {
    out_posWorld = INTERPOLATE_TES(gl_in, gl_Position);
    // allow textures to modify texture/normal
  #ifdef HAS_NORMAL
    out_norWorld = INTERPOLATE_TES(in_norWorld);
    modifyTransformation(out_posWorld,out_norWorld);
  #else
    modifyTransformation(out_posWorld,vec3(0,1,0));
  #endif
    out_posEye = posEyeSpace(out_posWorld);
  #ifdef HAS_TANGENT_SPACE
    out_posTan = posTanSpace(getTangent(), getBinormal(), out_norWorld, posWorld);
  #endif
    gl_Position = in_projectionMatrix * out_posEye;
    
#ifdef HAS_INSTANCES
    out_instanceID = in_instanceID[0];
#endif
#ifdef HAS_COLOR
    out_col = INTERPOLATE_TES(in_col);
#endif
#ifdef HAS_FRAGMENT_SHADING
    out_lightProperties.lightVec = INTERPOLATE_TES(in_lightProperties, lightVec);
    out_lightProperties.attenuation = INTERPOLATE_TES(in_lightProperties, attenuation);
#elif HAS_VERTEX_SHADING
    out_shading.ambient = INTERPOLATE_TES(in_shading, ambient);
    out_shading.diffuse = INTERPOLATE_TES(in_shading, diffuse);
    out_shading.specular = INTERPOLATE_TES(in_shading, specular);
    out_shading.emission = INTERPOLATE_TES(in_shading, emission);
    out_shading.shininess = INTERPOLATE_TES(in_shading, shininess);
#endif
    FINALIZE();
}

-- gs
//      FINALIZE()
void main() {
    FINALIZE();
}

-- fs.header
//      modifyLight(inout Shading shading)
//      modifyColor(inout vec4 color)
//      modifyAlpha(inout float color)
//      modifyNormal(inout vec3 nor)
//      shade(LightProperties props, inout Shading shading, vec3 norWorld)
//      FINALIZE()
in vec3 in_posWorld;
in vec3 in_posEye;
#ifdef HAS_TANGENT_SPACE
in vec3 in_posTan;
#endif
#ifdef HAS_INSTANCES
flat in int in_instanceID;
#endif
#ifdef HAS_NORMAL
in vec3 in_norWorld;
#endif
#ifdef HAS_COLOR
in vec3 in_col;
#endif
#ifdef HAS_FRAGMENT_SHADING
in LightProperties in_lightProperties;
#elif HAS_VERTEX_SHADING
in Shading in_shading;
#endif

#include mesh.material

#ifdef HAS_FOG
uniform vec4 in_fogColor;
uniform float in_fogEnd;
uniform float in_fogScale;
#endif

-- fs.main
void main() {
#ifdef HAS_FRAGMENT_SHADING
  #ifdef HAS_TWO_SIDES
    vec3 nor = (gl_FrontFacing ? in_norWorld : -in_norWorld);
  #else
    vec3 nor = in_norWorld;
  #endif
    modifyNormal(nor);
#endif // HAS_FRAGMENT_SHADING

#ifdef HAS_ALPHA
  #ifdef HAS_MATERIAL
    float alpha = in_matAlpha;
  #else
    float alpha = 1.0;
  #endif
    modifyAlpha(alpha);
#endif // HAS_ALPHA
    
#ifdef HAS_COL
    output = in_col;
#else
    output = vec4(1);
#endif
    modifyColor(output);

#ifdef HAS_FRAGMENT_SHADING
    Shading sh = newShading();
  #ifdef HAS_MATERIAL
    sh.shininess = in_matShininess;
  #endif
    modifyLight(sh);
    shade(in_lightProperties,sh,nor);
  #ifdef HAS_MATERIAL
    materialShading(sh);
  #endif
#elif HAS_VERTEX_SHADING
    Shading sh = in_shading;
    modifyLight(sh);
#elif HAS_LIGHT_MAPS
    Shading sh = newShading();
    modifyLight(sh);
#endif

#ifdef HAS_LIGHT || HAS_LIGHT_MAPS
    output = output*(sh.emission + sh.ambient + sh.diffuse) + sh.specular;
#endif

#ifdef HAS_ALPHA
    output.a = output.a * alpha;
#endif

#ifdef HAS_FOG
    float fogVar = clamp(in_fogScale*(in_fogEnd + gl_FragCoord.z), 0.0, 1.0);
    output = mix(in_fogColor, output, fogVar);
#endif
    
    FINALIZE();
}

