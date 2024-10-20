
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

