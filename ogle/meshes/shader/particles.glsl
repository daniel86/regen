
-- randomize
// return pseudorandom float with values between 0 and 1.
float random(inout uint seed) {
    seed = (seed * 1103515245u + 12345u);
    return float(seed) / 4294967296.0;
}

// return pseudorandom vec3 with values between -1 and 1.
vec3 random3(inout uint seed) {
    vec3 result;
    seed = (seed * 1103515245u + 12345u); result.x = float(seed);
    seed = (seed * 1103515245u + 12345u); result.y = float(seed);
    seed = (seed * 1103515245u + 12345u); result.z = float(seed);
    return (result / 2147483648.0) - vec3(1,1,1);
}

/////////

float variance(float v, inout uint seed) {
    return v*2.0*(random(seed)-0.5);
}
vec2 variance(vec2 v, inout uint seed) {
    return vec2(
        variance(v.x,seed),
        variance(v.y,seed));
}
vec3 variance(vec3 v, inout uint seed) {
    return vec3(
        variance(v.x,seed),
        variance(v.y,seed),
        variance(v.z,seed));
}
vec4 variance(vec4 v, inout uint seed) {
    return vec4(
        variance(v.x,seed),
        variance(v.y,seed),
        variance(v.z,seed),
        variance(v.w,seed));
}

-- attributes
#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _TYPE ${PARTICLE_ATTRIBUTE${INDEX}_TYPE}
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
in ${_TYPE} in_${_NAME};
out ${_TYPE} out_${_NAME};
#endfor

void setOutputAttributes() {
    // init outputs to input values
#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
    out_${_NAME} = in_${_NAME};
#endfor
}
