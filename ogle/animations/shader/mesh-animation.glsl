
-- interpolate
#version 150
#define M_PI 3.141592653589

uniform float frameTimeNormalized;
uniform float in_friction;
uniform float in_frequency;

#for NUM_ATTRIBUTES
#define2 _NAME ${ATTRIBUTE${FOR_INDEX}_NAME}
#define2 _TYPE ${ATTRIBUTE${FOR_INDEX}_TYPE}
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
#define __ELASTIC(T) abs( exp(-in_friction*T)*cos(in_frequency*T*2.5*M_PI) )
#define interpolate_elastic(X,Y,T) interpolate_linear(Y,X,__ELASTIC(T))

// include interpolation functions
#for NUM_ATTRIBUTES
  #ifdef ${ATTRIBUTE${FOR_INDEX}_INTERPOLATION_KEY}
#include ${ATTRIBUTE${FOR_INDEX}_INTERPOLATION_KEY}
  #endif
#endfor

void main() {
#for NUM_ATTRIBUTES
#define2 _NAME ${ATTRIBUTE${FOR_INDEX}_NAME}
#define2 _TYPE ${ATTRIBUTE${FOR_INDEX}_NAME}
    out_${_NAME} = ${ATTRIBUTE${FOR_INDEX}_INTERPOLATION_NAME}(
        in_last_${_NAME},
        in_next_${_NAME},
        frameTimeNormalized);
#endfor
}

