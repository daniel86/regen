
-- vs
in vec3 in_pos;
out vec4 out_eye;
out vec4 out_ray;

uniform mat4 in_equToHorMatrix;

#include regen.states.camera.input

void main() {
    vec4 p = vec4(in_pos.xy, 0.0, 1.0);
    out_eye = in_inverseProjectionMatrix * p * in_viewMatrix;
    out_ray = in_equToHorMatrix * out_eye;
    gl_Position = p;
}

-- fs
out vec4 out_color;
in vec4 in_eye;
in vec4 in_ray;

uniform vec3 in_sunPosition;
uniform float in_q;
uniform vec4 in_cmn;
uniform vec2 in_inverseViewport;

const float in_deltaM = 1.0;
const float surfaceHeight = 0.99;

uniform samplerCube in_starmapCube;

#include regen.sky.utility.scatter
#include regen.sky.utility.computeEyeExtinction

void main(void) {
  vec3 eye = normalize(in_eye.xyz);
  float ext = computeEyeExtinction(eye);
  if(ext <= 0.0) discard;
  
  vec3 stu = normalize(in_ray.xyz);
  vec4 fc = texture(in_starmapCube, stu);
  fc *= 3e-2 / sqrt(in_q) * in_deltaM;
  
  float omega = acos(eye.y * 0.9998);
  // Day-Twilight-Night-Intensity Mapping (Butterworth-Filter)
  float b = 1.0 / sqrt(1 + pow(in_sunPosition.z + 1.14, 32));
  
  ext = smoothstep(0.0, 0.05, ext);
    
  out_color = vec4(b * (fc.rgb - scatter(omega)), 1.0) * ext;
}
