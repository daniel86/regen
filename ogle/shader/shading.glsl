-- types
struct Shading {
  vec4 ambient;
  vec4 diffuse;
  vec4 specular;
  vec4 emission;
  float shininess;
};
struct LightProperties {
    vec3 lightVec[NUM_LIGHTS];
    float attenuation[NUM_LIGHTS];
};

-- fs.header
#include shading.type
in vec3 in_lightVec[NUM_LIGHTS];
in float in_attenFacs[NUM_LIGHTS];

-- vs.header
#include shading.type
out vec3 in_lightVec[NUM_LIGHTS];
out float in_attenFacs[NUM_LIGHTS];

-- init
void shadeDirectionalProperties(inout LightProperties p, vec3 P, int i) {
    prop.lightVec[i] = in_lightPosition[i].xyz;
    prop.attenuation[i] = 0.0;
}
void shadePointProperties(inout LightProperties prop[NUM_LIGHTS], vec3 P, int i) {
    prop.lightVec[i] = vec3( in_lightPosition[i].xyz - P );

    float dist = length(prop.lightVec[i]);
    prop.attenuation[i] = 1.0/(
        in_lightConstantAttenuation[i] +
        in_lightLinearAttenuation[i] * dist +
        in_lightQuadricAttenuation[i] * dist * dist );
}
void shadeSpotProperties(inout LightProperties prop[NUM_LIGHTS], vec3 P, int i) {
    prop.lightVec[i] = vec3( in_lightPosition[i].xyz - P );

    float spotEffect = dot( normalize(in_lightSpotDirection[i]), normalize(-prop.lightVec[i]));
    if (spotEffect > in_lightInnerConeAngle[i]) {
        float dist = length(prop.lightVec[i]);
        prop.attenuation[i] = pow(spotEffect, in_lightSpotExponent[i])/(
            in_lightConstantAttenuation[i] +
            in_lightLinearAttenuation[i] * dist +
            in_lightQuadricAttenuation[i] * dist * dist);
    } else {
        prop.attenuation[i] = 0.0;
    }
}

-- shade
#ifdef HAS_PHONG_SHADING
#include shading.phong
#else HAS_GOURAD_SHADING
#include shading.gourad
#else HAS_BLINN_SHADING
#include shading.blinn
#else HAS_OREN_NAYER_SHADING
#include shading.oren-nayer
#else HAS_MINNAERT_SHADING
#include shading.minnaert
#else HAS_TOON_SHADING
#include shading.toon
#endif

void shadeDirectionalLight(LightProperties props, inout Shading s, vec3 N, int i) {
    s.ambient += in_lightAmbient[i];
    vec3 L = normalize(props.lightVec[i]);
    float nDotL = max( dot( N, L ), 0.0 );
    if (nDotL <= 0.0) return;
    shadeDirectionalDiffuse(props,s,i,N,L,nDotL);
    if(s.shininess > 0.0) shadeDirectionalSpecular(props,s,i,N,L);
}
void shadePointLight(LightProperties props, inout Shading s, vec3 N, int i) {
    s.ambient += in_lightAmbient[i];
    vec3 L = normalize(props.lightVec[i]);
    float nDotL = max( dot( N, L ), 0.0 );
    if (nDotL <= 0.0) return;
    shadePointDiffuse(props,s,i,N,L,nDotL);
    if(s.shininess > 0.0) shadePointSpecular(props,s,i,N,L);
}
void shadeSpotLight(LightProperties props, inout Shading s, vec3 N, int i) {
    s.ambient += in_lightAmbient[i];
    vec3 L = normalize(props.lightVec[i]);
    float nDotL = max( dot( N, L ), 0.0 );
    if (nDotL <= 0.0) return;
    shadeSpotDiffuse(props,s,i,N,L,nDotL);
    if(s.shininess > 0.0) shadeSpotSpecular(props,s,i,N,L);
}

-- directional.diffuse
void shadeDirectionalDiffuse(LightProperties props, inout Shading s, int i, vec3 N, vec3 L, float nDotL) {
    s.diffuse += in_lightDiffuse[i] * nDotL;
}
-- directional.specular
void shadeDirectionalSpecular(LightProperties props, inout Shading s, int i, vec3 N, vec3 L) {
    vec3 P = normalize( -in_pos.xyz );
    vec3 reflected = normalize( reflect( -L, N ) );
    float rDotE = max( dot( reflected, P ), 0.0);
    specular += props.attenuation[i] * in_lightSpecular[i] * pow(rDotE, s.shininess);
}

-- point.diffuse
void shadePointDiffuse(LightProperties props, inout Shading s, int i, vec3 N, vec3 L, float nDotL) {
    s.diffuse += props.attenuation[i] * in_lightDiffuse[i] * nDotL;
}
-- point.specular
void shadePointSpecular(LightProperties props, inout Shading s, int i, vec3 N, vec3 L) {
    vec3 P = normalize( -in_pos.xyz );
    vec3 reflected = normalize( reflect( -L, N ) );
    float rDotE = max( dot( reflected, P ), 0.0);
    specular += props.attenuation[i] * in_lightSpecular[i] * pow(rDotE, s.shininess);
}

-- spot.diffuse
void shadeSpotDiffuse(LightProperties props, inout Shading s, int i, vec3 N, vec3 L, float nDotL) {
    vec3 normalizedSpotDir = normalize(in_lightSpotDirection[i]);
    float spotEffect = dot( -L, normalizedSpotDir );
    float coneDiff = (in_lightInnerConeAngle[i] - in_lightOuterConeAngle[i]);
    float falloff = clamp((spotEffect - in_lightOuterConeAngle[i]) / coneDiff, 0.0, 1.0);
    s.diffuse += props.attenuation[i] * in_lightDiffuse[i] * nDotL * falloff;
}
-- spot.specular
void shadeSpotSpecular(LightProperties props, inout Shading s, int i, vec3 N, vec3 L) {
    vec3 P = normalize( -in_pos.xyz );
    vec3 reflected = normalize( reflect( -L, N ) );
    float rDotE = max( dot( reflected, P ), 0.0);
    specular += in_lightSpecular[i] * pow(rDotE, s.shininess);
}

-- phong
#include shading.point.diffuse
#include shading.point.specular
#include shading.spot.diffuse
#include shading.spot.specular
#include shading.directional.diffuse
#include shading.directional.specular

-- gourad
#include shading.point.diffuse
#include shading.point.specular
#include shading.spot.diffuse
#include shading.spot.specular
#include shading.directional.diffuse
#include shading.directional.specular

-- blinn
#include shading.point.diffuse
#include shading.spot.diffuse
#include shading.directional.diffuse
void shadePointSpecular(LightProperties props, inout Shading s, int i, vec3 N, vec3 L) {
    vec3 P = normalize( -in_pos.xyz );
    vec3 halfVecNormalized = normalize( L + P );
    s.specular += in_lightSpecular[i] * pow( max(0.0,dot(N,halfVecNormalized)), s.shininess);
}
#define shadeSpotSpecular shadePointSpecular
#define shadeDirectionalSpecular shadePointSpecular

-- oren-nayer
#include shading.point.specular
#include shading.spot.specular
#include shading.directional.specular
void shadePointDiffuse(LightProperties props, inout Shading s, int i, vec3 N, vec3 L, float nDotL) {
    vec3 V = normalize( -in_pos.xyz );
    float vDotN = dot(V, N);
    float cos_theta_i = nDotL;
    float theta_r = acos(vDotN);
    float theta_i = acos(cos_theta_i);
    float cos_phi_diff = dot(normalize(V-N*vDotN), normalize(L-N*nDotL));
    float alpha = max(theta_i,theta_r);
    float beta = min(theta_i,theta_r);
    float r = in_materialRoughness*in_materialRoughness;
    float a = 1.0-0.5*r/(r+0.33);
    float b = 0.45*r/(r+0.09);
    if (cos_phi_diff>=0) {
        b*=sin(alpha)*tan(beta);
    } else {
        b=0.0;
    }
    s.diffuse += in_lightDiffuse[i] * cos_theta_i * (a+b);
}
#define shadeSpotDiffuse shadePointDiffuse
#define shadeDirectionalDiffuse shadePointDiffuse

-- minnaert
#include shading.point.specular
#include shading.spot.specular
#include shading.directional.specular
void shadePointDiffuse(LightProperties props, inout Shading s, int i, vec3 N, vec3 L, float nDotL) {
    s.diffuseVar += in_lightDiffuse[i] * pow(nDotL, in_materialDarkness);
}
#define shadeSpotDiffuse shadePointDiffuse
#define shadeDirectionalDiffuse shadePointDiffuse

-- cook-torrance
#include shading.point.diffuse
#include shading.spot.diffuse
#include shading.directional.diffuse
void shadePointSpecular(LightProperties props, inout Shading s, int i, vec3 N, vec3 L) {
    vec3 P = normalize( -in_pos.xyz );
    vec3 halfVecNormalized = normalize( L + P );
    float nDotH = dot(N, halfVecNormalized);
    if(nDotH >= 0.0) {
        float nDotV = max(dot(N, P), 0.0);
        float specularFactor = pow(nDotH, s.shininess);
        specularFactor = specularFactor/(0.1+nDotV);
        s.specular += in_lightSpecular[i] * specularFactor;
    }
}
#define shadeSpotSpecular shadePointSpecular
#define shadeDirectionalSpecular shadePointSpecular

-- toon
void shadePointDiffuse(LightProperties props, inout Shading s, int i, vec3 N, vec3 L, float nDotL) {
    float diffuseFactor;
    float intensity = dot( L , N );
    if( intensity > 0.95 ) diffuseFactor = 1.0;
    else if( intensity > 0.5  ) diffuseFactor = 0.7;
    else if( intensity > 0.25 ) diffuseFactor = 0.4;
    else diffuseFactor = 0.0;
    s.diffuse = in_lightDiffuse[i] * vec4(vec3(diffuseFactor), 1.0);
}
#define shadeSpotDiffuse shadePointDiffuse
#define shadeDirectionalDiffuse shadePointDiffuse

void shadePointSpecular(LightProperties props, inout Shading s, int i, vec3 N, vec3 L) {
    const float size=1.0;
    const float tsmooth=0.25;

    vec3 h = normalize(L + in_pos.xyz);
    float specfac = dot(h, N);
    float ang = acos(specfac);
    if(ang < size) specfac = 1.0;
    else if(ang >= (size + tsmooth) || tsmooth == 0.0) specfac = 0.0;
    else specfac = 1.0 - ((ang - size)/ tsmooth);
    s.specular += in_lightSpecular[i] * specfac;
}
#define shadeSpotSpecular shadePointSpecular
#define shadeDirectionalSpecular shadePointSpecular

