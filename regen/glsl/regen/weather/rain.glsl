
-- update.vs
#include regen.states.camera.defines
#include regen.states.camera.input
#include regen.particles.emitter.inputs
#include regen.particles.emitter.defines

const vec3 in_wind = vec3(-0.05,-0.5,0);
const vec3 in_rainVelocity = vec3(0.001,0.001,0.01);
const vec2 in_rainCone = vec3(50.0,20.0);
const vec2 in_rainBrightness = vec2(0.9,0.2);

#include regen.noise.variance

bool isRespawnRequired()
{
  return (in_lifetime<0.01) || (in_pos.y<in_cameraPosition.y-in_rainCone.y);
}

void main() {
    uint seed = in_randomSeed;
    if(isRespawnRequired()) {
      out_pos = variance(in_rainCone.xyx, seed) + in_cameraPosition;
      out_velocity = variance(in_rainVelocity, seed);
#ifdef USE_RAIN_DB_RAND
      out_type = int(random(seed)*369.0);
#else
      out_type = int(floor(random(seed)*8.0));
#endif
      out_brightness = in_rainBrightness.x + variance(in_rainBrightness.y, seed);
      out_lifetime = 1.0;
    }
    else {
      float dt = in_timeDeltaMS*0.01;
      out_pos = in_pos + (in_velocity+in_wind)*dt;
      out_velocity = in_velocity;
      out_type = in_type;
      out_brightness = in_brightness;
      out_lifetime = in_lifetime+dt;
    }
    out_randomSeed = seed;
}

-------------------------------
-------------------------------
-------------------------------
-------------------------------

-- draw.vs
#include regen.weather.precipitation.draw.vs
-- draw.gs
#include regen.weather.precipitation.draw.gs
-- draw.fs
#include regen.models.mesh.defines

layout(location = 0) out vec4 out_color;
in float in_brightness;
in vec4 in_posWorld;
in vec2 in_spriteTexco;

#ifdef USE_RAIN_DB
#include regen.weather.rain.draw.fs-database
#elif USE_RAIN_DB_RAND
#include regen.weather.rain.draw.fs-database-random
#else
#include regen.weather.rain.draw.fs-single-texture
#endif

-- draw.fs-single-texture
#include regen.shading.direct.diffuse
#include regen.states.camera.linearizeDepth
#include regen.particles.sprite.softParticleScale

uniform sampler2D in_rainTexture;

void main() {
  vec3 P = in_posWorld.xyz;
  float density = in_brightness*texture(in_rainTexture, in_spriteTexco).x;
  vec3 diffuseColor = getDiffuseLight(P, gl_FragCoord.z);
  out_color = density*vec4(diffuseColor,1.0);
}

-- draw.fs-database.input
flat in int in_type;
in vec3 in_velocity;

// [10,30,50,70,90 vangle #5][10,30,50,70,90,110,130,150,170 hangle #9][0-9 Types #10]
// Note: for vangle=90° only hangle=170° sampled.
uniform sampler2DArray in_rainDB;
//normalization factors for the rain textures, one per texture
const float in_rainNormalization[370] = float[](
  0.004535 , 0.014777 , 0.012512 , 0.130630 , 0.013893 , 0.125165 , 0.011809 , 0.244907 , 0.010722 , 0.218252,
  0.011450 , 0.016406 , 0.015855 , 0.055476 , 0.015024 , 0.067772 , 0.021120 , 0.118653 , 0.018705 , 0.142495, 
  0.004249 , 0.017267 , 0.042737 , 0.036384 , 0.043433 , 0.039413 , 0.058746 , 0.038396 , 0.065664 , 0.054761, 
  0.002484 , 0.003707 , 0.004456 , 0.006006 , 0.004805 , 0.006021 , 0.004263 , 0.007299 , 0.004665 , 0.007037, 
  0.002403 , 0.004809 , 0.004978 , 0.005211 , 0.004855 , 0.004936 , 0.006266 , 0.007787 , 0.006973 , 0.007911, 
  0.004843 , 0.007565 , 0.007675 , 0.011109 , 0.007726 , 0.012165 , 0.013179 , 0.021546 , 0.013247 , 0.012964, 
  0.105644 , 0.126661 , 0.128746 , 0.101296 , 0.123779 , 0.106198 , 0.123470 , 0.129170 , 0.116610 , 0.137528, 
  0.302834 , 0.379777 , 0.392745 , 0.339152 , 0.395508 , 0.334227 , 0.374641 , 0.503066 , 0.387906 , 0.519618, 
  0.414521 , 0.521799 , 0.521648 , 0.498219 , 0.511921 , 0.490866 , 0.523137 , 0.713744 , 0.516829 , 0.743649, 
  0.009892 , 0.013868 , 0.034567 , 0.025788 , 0.034729 , 0.036399 , 0.030606 , 0.017303 , 0.051809 , 0.030852, 
  0.018874 , 0.027152 , 0.031625 , 0.023033 , 0.038150 , 0.024483 , 0.029034 , 0.021801 , 0.037730 , 0.016639, 
  0.002868 , 0.004127 , 0.133022 , 0.013847 , 0.123368 , 0.012993 , 0.122183 , 0.015031 , 0.126043 , 0.015916, 
  0.002030 , 0.002807 , 0.065443 , 0.002752 , 0.069440 , 0.002810 , 0.081357 , 0.002721 , 0.076409 , 0.002990, 
  0.002425 , 0.003250 , 0.003180 , 0.011331 , 0.002957 , 0.011551 , 0.003387 , 0.006086 , 0.002928 , 0.005548, 
  0.003664 , 0.004258 , 0.004269 , 0.009404 , 0.003925 , 0.009233 , 0.004224 , 0.009405 , 0.004014 , 0.008435, 
  0.038058 , 0.040362 , 0.035946 , 0.072104 , 0.038315 , 0.078789 , 0.037069 , 0.077795 , 0.042554 , 0.073945, 
  0.124160 , 0.122589 , 0.121798 , 0.201886 , 0.122283 , 0.214549 , 0.118196 , 0.192104 , 0.122268 , 0.209397, 
  0.185212 , 0.181729 , 0.194527 , 0.420721 , 0.191558 , 0.437096 , 0.199995 , 0.373842 , 0.192217 , 0.386263, 
  0.003520 , 0.053502 , 0.060764 , 0.035197 , 0.055078 , 0.036764 , 0.048231 , 0.052671 , 0.050826 , 0.044863, 
  0.002254 , 0.023290 , 0.082858 , 0.043008 , 0.073780 , 0.035838 , 0.080650 , 0.071433 , 0.073493 , 0.026725, 
  0.002181 , 0.002203 , 0.112864 , 0.060140 , 0.115635 , 0.065531 , 0.093277 , 0.094123 , 0.093125 , 0.144290, 
  0.002397 , 0.002369 , 0.043241 , 0.002518 , 0.040455 , 0.002656 , 0.002540 , 0.090915 , 0.002443 , 0.101604, 
  0.002598 , 0.002547 , 0.002748 , 0.002939 , 0.002599 , 0.003395 , 0.002733 , 0.003774 , 0.002659 , 0.004583, 
  0.003277 , 0.003176 , 0.003265 , 0.004301 , 0.003160 , 0.004517 , 0.003833 , 0.008354 , 0.003140 , 0.009214, 
  0.008558 , 0.007646 , 0.007622 , 0.026437 , 0.007633 , 0.021560 , 0.007622 , 0.017570 , 0.007632 , 0.018037, 
  0.031062 , 0.028428 , 0.028428 , 0.108300 , 0.028751 , 0.111013 , 0.028428 , 0.048661 , 0.028699 , 0.061490, 
  0.051063 , 0.047597 , 0.048824 , 0.129541 , 0.045247 , 0.124975 , 0.047804 , 0.128904 , 0.045053 , 0.119087, 
  0.002197 , 0.002552 , 0.002098 , 0.200688 , 0.002073 , 0.102060 , 0.002111 , 0.163116 , 0.002125 , 0.165419, 
  0.002060 , 0.002504 , 0.002105 , 0.166820 , 0.002117 , 0.144274 , 0.005074 , 0.143881 , 0.004875 , 0.205333, 
  0.001852 , 0.002184 , 0.002167 , 0.163804 , 0.002132 , 0.212644 , 0.003431 , 0.244546 , 0.004205 , 0.315848, 
  0.002450 , 0.002360 , 0.002243 , 0.154635 , 0.002246 , 0.148259 , 0.002239 , 0.348694 , 0.002265 , 0.368426, 
  0.002321 , 0.002393 , 0.002376 , 0.074124 , 0.002439 , 0.126918 , 0.002453 , 0.439270 , 0.002416 , 0.489812, 
  0.002484 , 0.002629 , 0.002559 , 0.150246 , 0.002579 , 0.140103 , 0.002548 , 0.493103 , 0.002637 , 0.509481, 
  0.002960 , 0.002952 , 0.002880 , 0.294884 , 0.002758 , 0.332805 , 0.002727 , 0.455842 , 0.002816 , 0.431807, 
  0.003099 , 0.003028 , 0.002927 , 0.387154 , 0.002899 , 0.397946 , 0.002957 , 0.261333 , 0.002909 , 0.148548, 
  0.004887 , 0.004884 , 0.006581 , 0.414647 , 0.003735 , 0.431317 , 0.006426 , 0.148997 , 0.003736 , 0.080715, 
  0.001969 , 0.002159 , 0.002325 , 0.200211 , 0.002288 , 0.202137 , 0.002289 , 0.595331 , 0.002311 , 0.636097 
);

-- draw.fs-database-random
#include regen.weather.rain.draw.fs-database.input
#include regen.shading.direct.diffuse
#include regen.states.camera.linearizeDepth
#include regen.particles.sprite.softParticleScale

void main() {
  int index = in_type;
  float scale = 100.0;
  
  float density = in_brightness*in_rainNormalization[index]*
    texture(in_rainDB, vec3(in_spriteTexco,index)).x;
  if(density<0.0001) discard;
  
  vec3 diffuseColor = getDiffuseLight(in_posWorld.xyz, gl_FragCoord.z);
  out_color = density*vec4(diffuseColor,1.0);
}


-- @see http://www1.cs.columbia.edu/CAVE/publications/pdfs/Garg_TOG06.pdf
-- draw.fs-database
#include regen.weather.rain.draw.fs-database.input
#include regen.states.camera.linearizeDepth
#include regen.particles.sprite.softParticleScale

#define MAX_VIDX 4
#define MAX_HIDX 8

const vec3 in_wind = vec3(-0.05,-0.5,0);

const float RAD_TO_DEG = 57.2957795;

vec2 computeRainAngles(vec3 eyeDir, vec3 lightDir, vec3 upVector, float dotUpInv) {
  vec3 x0 = length(upVector*eyeDir)*dotUpInv*upVector + eyeDir;
  vec3 x1 = length(upVector*lightDir)*dotUpInv*upVector + lightDir;
  float l_x1 = length(x1);
  float vangle = RAD_TO_DEG*acos(dot(lightDir,x1)/(length(lightDir)*l_x1));
  float hangle = RAD_TO_DEG*acos(dot(x0,x1)/(length(x0)*l_x1));
  return vec2(vangle,hangle);
}

int computeIndex(int vIndex, int hIndex, int typeIndex) {
  return typeIndex + 10*hIndex + 90*vIndex;
}

float computeRainDensity(vec2 angles, int typeIndex) {
  // Compute index for the vetical angle between light and rain plane.
  // Vetical angles sampled from 10° to 90° in 20° steps (5 samples)
  float vangle = max(abs(angles.x-90.0)-10.0,0.0);
  vangle = 5.0*vangle/80.0;
  float vangle_0 = floor(vangle);
  int v_i0 = min(4,int(vangle_0));
  int v_i1 = min(4,v_i0+1);
  float v_blend = (vangle-vangle_0); // blend*c1 + (1.0-blend)*c0
  
  // Compute index for the horizontal angle between light and camera on rain plane.
  // Horizontal angles sampled from 10° to 170° in 20° steps (8 samples)
  float hangle = max(angles.y-10.0,0.0);
  hangle = 9.0*hangle/160.0;
  float hangle_0 = floor(hangle);
  int h_i0 = min(8,int(hangle_0));
  if(v_i0==4) { h_i0 = 0; }
  int h_i1 = min(8,h_i0+1);
  if(v_i1==4) { h_i1 = 0; }
  float h_blend = (hangle-hangle_0); // blend*c1 + (1.0-blend)*c0
  
  // Compute index for 2DArray texture access
  int i0 = computeIndex(v_i0,h_i0,typeIndex);
  int i1 = computeIndex(v_i0,h_i1,typeIndex);
  int i2 = computeIndex(v_i1,h_i0,typeIndex);
  int i3 = computeIndex(v_i1,h_i1,typeIndex);
  // Sample 4 nearest texels
  float r0 = in_rainNormalization[i0]*texture(in_rainDB,vec3(in_spriteTexco,i0)).x;
  float r1 = in_rainNormalization[i1]*texture(in_rainDB,vec3(in_spriteTexco,i1)).x;
  float r2 = in_rainNormalization[i2]*texture(in_rainDB,vec3(in_spriteTexco,i2)).x;
  float r3 = in_rainNormalization[i3]*texture(in_rainDB,vec3(in_spriteTexco,i3)).x;
  // Linear blend results based on distance of actual angle to indexed one
  float r01 = h_blend*r1 + (1.0-h_blend)*r0;
  float r23 = h_blend*r3 + (1.0-h_blend)*r2;
  float r = v_blend*r23 + (1.0-v_blend)*r01;
  
  return r;
}

void main() {
  vec3 P = in_posWorld.xyz;
  // camera to raindrop
  vec3 eyeDir = normalize(in_cameraPosition - P);
  // up vector of rain drop space
  vec3 upVector = normalize(in_velocity + in_wind);
  vec3 lightDir;
  vec2 angles;
  
  float dotUpInv = 1.0/dot(upVector,upVector);
  float density;
  
  out_color = vec4(0.0);
#for INDEX to NUM_LIGHTS
#define2 __ID ${LIGHT${INDEX}_ID}
#if LIGHT_TYPE${__ID} == DIRECTIONAL
  lightDir = normalize(-in_lightDirection${__ID});
#else
  lightDir = normalize(in_lightPosition${__ID} - P);
#endif
  // Compute angles in range [0.0,180.0]
  angles = computeRainAngles(eyeDir,lightDir,upVector,dotUpInv);
  // Compute rain density at this fragment
  density = in_brightness*computeRainDensity(angles,in_type);
#ifdef LIGHT_IS_ATTENUATED${__ID}
  density *= radiusAttenuation(length(P - in_lightPosition${__ID}), in_lightRadius${__ID}.x, in_lightRadius${__ID}.y);
#endif
#if LIGHT_TYPE${__ID} == SPOT
  density *= spotConeAttenuation(lightDir, in_lightDirection${__ID}, in_lightConeAngles${__ID});
#endif

  out_color += density*vec4(in_lightDiffuse${__ID},1.0);
#endfor
}
