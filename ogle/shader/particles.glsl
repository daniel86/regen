
-- vs

#for NUM_PARTICLE_ATTRIBUTES
#define2 _TYPE ${PARTICLE_ATTRIBUTE${FOR_INDEX}_TYPE}
#define2 _NAME ${PARTICLE_ATTRIBUTE${FOR_INDEX}_NAME}
in ${_TYPE} in_${_NAME};
out ${_TYPE} out_${_NAME};
#endfor

void main() {
#for NUM_PARTICLE_ATTRIBUTES
#define2 _NAME ${PARTICLE_ATTRIBUTE${FOR_INDEX}_NAME}
    out_${_NAME} = in_${_NAME};
#endfor
}

-- gs
layout(points) in;
layout(points, max_vertices=1) out;

#for NUM_PARTICLE_ATTRIBUTES
#define2 _TYPE ${PARTICLE_ATTRIBUTE${FOR_INDEX}_TYPE}
#define2 _NAME ${PARTICLE_ATTRIBUTE${FOR_INDEX}_NAME}
in ${_TYPE} in_${_NAME}[1];
out ${_TYPE} out_${_NAME};
#endfor

uniform float in_deltaT;
uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;

uniform float in_startPointSize;
uniform float in_stopPointSize;

/////////

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

/////////

// include particle updater
#for NUM_PARTICLE_UPDATER
#include ${PARTICLE_UPDATER${FOR_INDEX}_NAME}
#endfor

// include particle emitter
#for NUM_PARTICLE_EMITTER
#include ${PARTICLE_EMITTER${FOR_INDEX}_NAME}
#endfor

//////////

void emitParticle(float dt, inout uint randomSeed)
{
#for NUM_PARTICLE_EMITTER
    if(gl_PrimitiveIDIn < ${PARTICLE_EMITTER${FOR_INDEX}_STOP}) {
        emit${FOR_INDEX} (dt,randomSeed);
        return;
    }
#endfor
}

void main() {
    float dt = in_deltaT*0.001;

    // init outputs to input values
#for NUM_PARTICLE_ATTRIBUTES
#define2 _NAME ${PARTICLE_ATTRIBUTE${FOR_INDEX}_NAME}
    out_${_NAME} = in_${_NAME}[0];
#endfor
    
    if(out_lifetime <= 0) { // emit particle
        gl_PointSize = in_startPointSize;
        emitParticle(dt,out_randomSeed);
    }
    else { // update particle
        gl_PointSize = mix(in_stopPointSize, in_startPointSize, out_lifetime);
        // let custom updater functions change attributes
#for NUM_PARTICLE_UPDATER
        ${PARTICLE_UPDATER${FOR_INDEX}_NAME}(dt,out_randomSeed);
#endfor
        // modify position by current velocity
        out_pos += out_velocity*dt;
    }
    
    // project the particle position onto the screen
    vec4 posEye = in_viewMatrix * vec4(out_pos,1.0);
    gl_Position = in_projectionMatrix * posEye;
    // calculate point size depending on distance to camera
    float cameraDistance = length(posEye.xyz);
    gl_PointSize = clamp(
        gl_PointSize*sqrt(1.0/cameraDistance),
        0.1, in_stopPointSize);

    EmitVertex();
    EndPrimitive();
}

-- fs

layout(location = 0) out vec4 out_color;
layout(location = 2) out vec4 out_norWorld;

in float in_lifetime;
#ifdef HAS_COLOR
in vec3 in_col;
#endif

#ifdef HAS_DENSITY_TEXTURE
uniform sampler2D in_densityTexture;
#endif

void main() {
    vec2 texco = gl_PointCoord.xy;
#ifdef HAS_LIFETIME_ALPHA
  #ifdef INVERT_LIFETIME_ALPHA
    float density = 1.0 - in_lifetime;
  #elif PARABOL_LIFETIME_ALPHA
    float density = -pow(2.0*in_lifetime - 1.0,2) + 1.0;
  #else
    float density = in_lifetime;
  #endif
#else
    float density = 1.0;
#endif
#ifdef HAS_DENSITY_TEXTURE
    density *= texture(in_densityTexture, texco).r;
#else
    density *= 1.0 - 2.0*length(texco - vec2(0.5));
#endif
    // discard fragment when density smaller than 1/255
    if(density < 0.0039) { discard; }
#ifdef HAS_COLOR
    out_color = vec4(in_col,density);
#else
    out_color = vec4(1,1,1,density);
#endif
    out_norWorld = vec4(0.0);
}

