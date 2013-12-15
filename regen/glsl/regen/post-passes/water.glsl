--------------------------------------
--------------------------------------
---- Deferred water.
---- @see http://www.gamedev.net/page/reference/index.html/_/technical/graphics-programming-and-theory/rendering-water-as-a-post-process-effect-r2642
--------------------------------------
--------------------------------------
-- vs
#include regen.post-passes.fullscreen.vs
-- fs

#define NUM_HEIGHT_SAMPLES 10

in vec2 in_texco;
out vec4 out_color;

uniform sampler2D in_depthTexture;
uniform sampler2D in_refractionTexture;
uniform sampler2D in_reflectionTexture;
uniform sampler2D in_heightTexture;
uniform sampler2D in_normalTexture;
uniform sampler2D in_foamTexture;
// camera input
#include regen.states.camera.input
uniform mat4 in_reflectionMatrix;

const float in_heightTextureSize = 256.0;
const float in_time = 0.0;

const float in_surfaceHeight = 0.0;
const float in_normalScale = 3.0;
// A constant related to the index of refraction (IOR).
// It should be computed on the CPU and passed to the shader.
const float in_refractionConstant = 0.25;

// The smaller this value is, the more soft the transition between
// shore and water. If you want hard edges use very big value.
// Default is 1.0f.
const float in_shoreHardness = 0.1;

// Wind force in x and z axes.
const vec2 in_windDirection = vec2(0.0,-1.0);
const vec2 in_waveScale = vec2(0.005);
// Maximum waves amplitude
const float in_maxAmplitude = 1.5;

// This value modifies current fresnel term. If you want to weaken
// reflections use bigger value. If you want to empasize them use
// value smaller then 0. Default is 0.
const float in_refractionStrength = 0.0;
// Strength of displacement along normal.
const float in_reflectionDisplace = 30.0;

// Sun configuration
const vec3 in_sunColor = vec3(1.0,1.0,1.0);
const vec3 in_sunDirection = vec3(0.0,-1.0,0.0);
const float in_sunScale = 3.0;
// Color of the water surface
const vec3 in_waterColor = vec3(0.0078,0.5176,0.7);
// Water shininess factor
const float in_shininess = 0.7;
// Color of the water depth
const vec3 in_deepWaterColor = vec3(0.0039,0.00196,0.145);
const vec3 in_colorExtinction = vec3(7.0,30.0,40.0);
// How fast will colours fade out. You can also think about this
// values as how clear water is. Therefore use smaller values (eg. 0.05)
// to have crystal clear water and bigger to achieve "muddy" water.
const float in_fadeSpeed = 0.1;
// Water transparency along eye vector.
const float in_visibility = 3.0;

// Describes at what depth foam starts to fade out and
// at what it is completely invisible. The fird value is at
// what height foam for waves appear (+ waterLevel).
const vec3 in_foamExistence = vec3(0.45,4.35,1.5);
const float in_foamIntensity = 0.5;
const float in_foamHardness = 1.0;

// Modifies 4 sampled normals. Increase first values to have more
// smaller "waves" or last to have more bigger "waves"
const vec4 in_normalModifier = vec4(1.0,2.0,4.0,8.0);

#include regen.utility.utility.texcoToWorldSpace
#include regen.utility.textures.texco_planar_reflection

#ifdef USE_RIPPLES
mat3 MatrixInverse(in mat3 inMatrix){  
    float det = dot(cross(inMatrix[0], inMatrix[1]), inMatrix[2]);
    mat3 T = transpose(inMatrix);
    return mat3(
	cross(T[1], T[2]),
        cross(T[2], T[0]),
        cross(T[0], T[1])) / det;
}

mat3 computeTangentFrame(in vec3 N, in vec3 P, in vec2 UV) {
    vec3 dp1 = dFdx(P);
    vec3 dp2 = dFdy(P);
    vec2 duv1 = dFdx(UV);
    vec2 duv2 = dFdy(UV);

    // solve the linear system
    mat3 M = mat3(dp1, dp2, cross(dp1, dp2));
    mat3 inverseM = MatrixInverse(M);

    vec3 T = inverseM * vec3(duv1.x, duv2.x, 0.0);
    vec3 B = inverseM * vec3(duv1.y, duv2.y, 0.0);

    // construct tangent frame
    float maxLength = max(length(T), length(B));
    T = T / maxLength;
    B = B / maxLength;

    return mat3(T, B, N);
}

vec3 computeRippledNormal(vec3 pos, vec3 nor, vec3 eyeVecNorm, float t0, float t1) {
  vec2 uv = pos.xz * t0 + in_windDirection * in_time * t1;
  mat3 tangentFrame = computeTangentFrame(nor, eyeVecNorm, uv);
  return normalize(tangentFrame*(2.0 * texture(in_normalTexture,uv).xyz - 1.0));
}
#endif

vec3 computeNormal(vec2 uv) {
  float duv = 1.0/in_heightTextureSize;
  float normal1 = texture(in_heightTexture, uv + vec2(-duv,  0.0)).r;
  float normal2 = texture(in_heightTexture, uv + vec2( duv,  0.0)).r;
  float normal3 = texture(in_heightTexture, uv + vec2( 0.0, -duv)).r;
  float normal4 = texture(in_heightTexture, uv + vec2( 0.0,  duv)).r;
  return normalize(vec3(
      (normal1 - normal2) * in_maxAmplitude,
      in_normalScale,
      (normal3 - normal4) * in_maxAmplitude));
}

// Function calculating fresnel term.
// - normal - normalized normal vector
// - eyeVec - normalized eye vector
float fresnelTerm(vec3 normal, vec3 eyeVec)
{
  float angle = 1.0 - clamp(dot(normal, eyeVec), 0.0, 1.0);
  float fresnel = angle * angle;
  fresnel = fresnel * fresnel;
  fresnel = fresnel * angle;
  return clamp(fresnel * (1.0 - clamp(in_refractionConstant,0.0,1.0)) +
               in_refractionConstant - in_refractionStrength, 0.0, 1.0);
}

vec4 computeWaterColor(vec3 position, float sceneDepth)
{
  vec2 uv;
  // Height of water intersecting with view ray.
  float level = in_surfaceHeight;
  // Vector from camera to sampled scene point
  vec3 eyeVec = position - in_cameraPosition;
  float diff = level - position.y;
  float cameraDepth = in_cameraPosition.y - position.y;

  // Find intersection with water surface
  vec3 eyeVecNorm = normalize(eyeVec);
  float t = (level - in_cameraPosition.y) / eyeVecNorm.y;
  vec3 surfacePoint = in_cameraPosition + eyeVecNorm * t;
  
  // Sample height map a few times to find wave amplitude
  float biasFactor = 1.0/float(${NUM_HEIGHT_SAMPLES});
  vec2 windOffset = 0.03*in_time*in_windDirection;
#for INDEX to ${NUM_HEIGHT_SAMPLES}
  uv = biasFactor * in_waveScale * (surfacePoint.xz + eyeVecNorm.xz) + windOffset;
  level += biasFactor*in_maxAmplitude * texture(in_heightTexture, uv).r;
  t = (level - in_cameraPosition.y) / eyeVecNorm.y;
  surfacePoint = in_cameraPosition + eyeVecNorm * t;
#endfor
  //surfacePoint.y -= (level - in_surfaceHeight);
  surfacePoint.y -= in_maxAmplitude;

  // Compute eye vector with corrected surface point
  eyeVecNorm = normalize(in_cameraPosition - surfacePoint);
  float depth = length(position - surfacePoint);
  float depth2 = surfacePoint.y - position.y;
#ifdef USE_FOAM
  // XXX: HACK ALERT: Increase water depth to infinity if at far plane
  // Prevents "foam on horizon" issue
  // For best results, replace the "100.0" below with the
  // highest value in the m_ColorExtinction vec3
  float isAtFarPlane = step(0.99998, sceneDepth);
  depth  += isAtFarPlane * 100.0;
  depth2 += isAtFarPlane * 100.0;
#endif
  float depthN = depth * in_fadeSpeed;
  
#ifdef USE_RIPPLES
  vec3 surfaceNormal = computeNormal(uv);
  vec3 n0 = computeRippledNormal(surfacePoint,surfaceNormal,eyeVecNorm,3.2,1.6);
  vec3 n1 = computeRippledNormal(surfacePoint,surfaceNormal,eyeVecNorm,1.6,0.8);
  vec3 n2 = computeRippledNormal(surfacePoint,surfaceNormal,eyeVecNorm,0.8,0.4);
  vec3 n3 = computeRippledNormal(surfacePoint,surfaceNormal,eyeVecNorm,0.4,0.2);
  surfaceNormal = normalize(
      n0 * in_normalModifier.x +
      n1 * in_normalModifier.y +
      n2 * in_normalModifier.z +
      n3 * in_normalModifier.w);
#else
  vec3 surfaceNormal = computeNormal(uv);
#endif
  
  // Compute reflection color
  vec3 waterPosition = surfacePoint.xyz;
  //waterPosition.y -= (level - in_surfaceHeight);
  vec4 proj = in_reflectionMatrix * vec4(waterPosition,1.0);
  proj.x = proj.x + in_reflectionDisplace * surfaceNormal.x;
  proj.z = proj.z + in_reflectionDisplace * surfaceNormal.z;
  vec2 reflectionTexco = (proj.xy/proj.w + vec2(1.0))*0.5;
  vec3 reflection = texture(in_reflectionTexture, reflectionTexco).rgb;

  // Compute refraction color
#ifdef USE_REFRACTION
  uv = in_texco.xy;
  uv += vec2( sin(in_time*0.2 + 3.0 * abs(position.y)) * (in_waveScale.x * min(depth2, 1.0)) );
  vec3 refraction = texture(in_refractionTexture, uv).rgb;
#else
  vec3 refraction = texture(in_refractionTexture, in_texco).rgb;;
#endif
  // compute the water color based on depth and color extinction
  float k = clamp(length(in_sunColor) / in_sunScale, 0.0, 1.0);
  vec3 c0 =    mix(refraction,     k*in_waterColor, clamp(depthN/in_visibility, 0.0, 1.0));
  refraction = mix(        c0, k*in_deepWaterColor, clamp(depth2/in_colorExtinction, 0.0, 1.0));
  
#ifdef USE_FOAM
  float foam = 0.0;
  vec2 foam0 = 0.05*(surfacePoint.xz + 0.1*eyeVecNorm.xz);
  vec2 foam1 = in_time*in_windDirection;
  uv       = foam0 + 0.05*foam1 + 0.005*sin(0.001*in_time + position.x);
  vec2 uv2 = foam0 +  0.1*foam1 + 0.005*sin(0.001*in_time + position.z);
  if(depth2 < in_foamExistence.x){
    foam = in_foamIntensity*(texture(in_foamTexture,uv).r + texture(in_foamTexture,uv2).r);
  }
  else if(depth2 < in_foamExistence.y){
    foam = mix(
	in_foamIntensity*(texture(in_foamTexture,uv).x + texture(in_foamTexture,uv2).x),
	0.0,
	(depth2 - in_foamExistence.x) / (in_foamExistence.y - in_foamExistence.x));
  }
  
  if(in_maxAmplitude - in_foamExistence.z > 0.0001){
    float p = clamp((level - (in_surfaceHeight + in_foamExistence.z))/
                    (in_maxAmplitude - in_foamExistence.z), 0.0, 1.0);
    foam += in_foamIntensity*in_foamIntensity*p*0.3*
	(texture(in_foamTexture,uv).r + texture(in_foamTexture,uv2).r);
  }
#endif
  float fresnel = fresnelTerm(surfaceNormal, eyeVecNorm);

#ifdef USE_SPECULAR
  vec3 mirrorEye = 2.0*dot(eyeVecNorm, surfaceNormal)*surfaceNormal - eyeVecNorm;
  float dotSpec = clamp(dot(mirrorEye.xyz, in_sunDirection) * 0.5 + 0.5, 0.0, 1.0);
  float spec = (1.0-fresnel)*(in_shininess*1.8 + 0.2)*
    clamp(in_sunDirection.y,0.0,1.0)*pow(dotSpec,512.0);
  spec += 25.0*spec*clamp(in_shininess - 0.05, 0.0, 1.0);
#endif
  
  vec3 color = mix(refraction,reflection,fresnel);
#ifdef USE_FOAM && USE_SPECULAR
  color += max(spec,foam)*in_sunColor.rgb;
#elif USE_FOAM
  color += foam*in_sunColor.rgb;
#elif USE_SPECULAR
  color += spec*in_sunColor.rgb;
#endif
#ifdef USE_FOAM
  //color = mix(refraction, color, clamp(depth * in_foamHardness, 0.0, 1.0));
#endif
  color = mix(refraction, color, clamp(depth * in_shoreHardness, 0.0, 1.0));
  
  return vec4(color,1.0);
}

vec4 computeUnderWaterColor(vec3 position)
{
  // TODO
  return texture(in_refractionTexture, in_texco);
}

void main()
{
  // sample depth at pixel location and compute the position in world space
  float depth = texture(in_depthTexture, in_texco).x;
  vec3 posWorldSpace = texcoToWorldSpace(in_texco, depth);
  
  // check if camera is below surface
  if(in_cameraPosition.y<in_surfaceHeight) {
    out_color = computeUnderWaterColor(posWorldSpace);
  }
  // check if point is above water surface
  else if(posWorldSpace.y>=in_surfaceHeight) {
    out_color = texture(in_refractionTexture, in_texco);
  }
  else {
    out_color = computeWaterColor(posWorldSpace,depth);
  }
}
