
-- interpolate
#version 150
#define M_PI 3.141592653589

uniform float frameTimeNormalized;

#for NUM_ATTRIBUTES
#define2 _NAME ${ATTRIBUTE${FOR_INDEX}_NAME}
#define2 _TYPE ${ATTRIBUTE${FOR_INDEX}_TYPE}
in ${_TYPE} in_last_${_NAME};
in ${_TYPE} in_next_${_NAME};
out ${_TYPE} out_${_NAME};
#endfor

// flat
#define INTERPOLATE_FLAT(X,Y,T) (X)
// linear
#define INTERPOLATE_LINEAR(X,Y,T) (T*X + (1.0-T)*Y)
// nearest
#define INTERPOLATE_NEAREST(X,Y,T) (T<0.5 ? X : Y)
// elastic
const float in_friction = 6.0;
const float in_frequency = 3.0;
#define ELASTIC(T) abs( exp(-in_friction*T)*cos(in_frequency*T*2.5*M_PI) )
#define INTERPOLATE_ELASTIC(X,Y,T) INTERPOLATE_LINEAR(X,Y,ELASTIC(T))

void main() {
#for NUM_ATTRIBUTES
#define2 _NAME ${ATTRIBUTE${FOR_INDEX}_NAME}
#define2 _TYPE ${ATTRIBUTE${FOR_INDEX}_NAME}
    out_${_NAME} = INTERPOLATE_LINEAR(
        in_last_${_NAME},
        in_next_${_NAME},
        frameTimeNormalized);
#endfor
}

