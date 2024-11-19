
--------------
----- Twist the mesh around the Y axis
--------------
-- twist
#ifndef REGEN_TRANSFER_TWIST_
#define2 REGEN_TRANSFER_TWIST_

const float in_twistSpeed = 0.3;
const float in_twistStrength = 1.0;
const float in_twistHeight = 0.5;
const float in_twistAngle = 3.1415;

void twist(inout vec3 pos)
{
	float angle = (pos.y/in_twistHeight) * in_twistAngle * sin(in_time);
	float st = sin(angle * in_twistSpeed) * in_twistStrength;
	float ct = cos(angle * in_twistSpeed) * in_twistStrength;
    pos.xz = vec2(
	    pos.x*ct - pos.z*st,
        pos.x*st + pos.z*ct);
}
#endif // REGEN_TRANSFER_TWIST_

--------------
-----
--------------
-- posWavingTransfer
#ifndef REGEN_POS_TRANSFER_WAVING_
#define2 REGEN_POS_TRANSFER_WAVING_

const float in_wavingSpeed = 14.0;
const float in_wavingShakeStrength = 0.05;
const vec2 in_waveBase = vec2(0.5, 1.0);
const vec3 in_wind = vec3(1.0, 0.0, 0.0);

void posWavingTransfer(inout vec3 pos)
{
    float time = in_time * in_wavingSpeed + float(gl_VertexID) * 0.1;
    vec2 texco = in_texco0;
    float wave = sin(texco.y + time);
    float attenuation = pow(abs(texco.y - in_waveBase.y), 2.0);
    pos += in_wind*attenuation + normalize(in_wind)*wave*in_wavingShakeStrength*attenuation;
}
#endif // REGEN_POS_TRANSFER_WAVING_

--------------
----- Simpe mesh animation shader that takes two key frames as input
----- and interpolates between these values with an user defined
----- function.
----- The interpolated value can be captured using trnasform feedback.
--------------
-- interpolate
#define M_PI 3.141592653589

uniform float in_frameTimeNormalized;
uniform float in_friction;
uniform float in_frequency;

#for INDEX to NUM_ATTRIBUTES
#define2 _NAME ${ATTRIBUTE${INDEX}_NAME}
#define2 _TYPE ${ATTRIBUTE${INDEX}_TYPE}
in ${_TYPE} in_last_${_NAME};
in ${_TYPE} in_next_${_NAME};
out ${_TYPE} out_${_NAME};
#endfor

// flat
#define interpolate_flat(X,Y,T) (X)
// linear
#define interpolate_linear(X,Y,T) (T*X + (1.0-T)*Y)
// nearest
#define interpolate_nearest(X,Y,T) (T<0.5 ? X : Y)
// elastic
#define REGEN_ELASTIC(T) abs( exp(-in_friction*T)*cos(in_frequency*T*2.5*M_PI) )
#define interpolate_elastic(X,Y,T) interpolate_linear(Y,X,REGEN_ELASTIC(T))

// include interpolation functions
#for INDEX to NUM_ATTRIBUTES
  #ifdef ${ATTRIBUTE${INDEX}_INTERPOLATION_KEY}
#include ${ATTRIBUTE${INDEX}_INTERPOLATION_KEY}
  #endif
#endfor

void main() {
#for INDEX to NUM_ATTRIBUTES
#define2 _NAME ${ATTRIBUTE${INDEX}_NAME}
#define2 _TYPE ${ATTRIBUTE${INDEX}_NAME}
    out_${_NAME} = ${ATTRIBUTE${INDEX}_INTERPOLATION_NAME}(
        in_last_${_NAME},
        in_next_${_NAME},
        in_frameTimeNormalized);
#endfor
}

