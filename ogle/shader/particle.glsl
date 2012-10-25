
-- vs.header
in vec3 in_pos;
out vec3 out_pos;

in float in_lifetime;
out float out_lifetime;

in vec3 in_velocity;
out vec3 out_velocity;

uniform float in_dt;

float getParticleAge() {
    return in_lifetime+in_dt;
}
vec3 getParticlePosition(vec3 pos, vec3 velocity) {
    return pos + in_dt*velocity;
}

-- seed.point
uniform vec3 in_seedPoint;

vec3 seed() {
    out_pos = in_seedPoint;
    out_lifetime = 0.0;
    out_velocity = vec3(0.0);
}

-- gravity
uniform vec3 in_gravity;

void gravity(inout vec3 velocity) {
    velocity += in_gravity * in_dt;
}

-- advection.vs
#include particle.vs.header

void main() {
    out_lifetime = getParticleAge();
    vec3 p = in_pos;
    if(out_lifetime > 100.0) {
        p = seed();
    }
    vec3 v = in_velocity;

    gravity(v);

    out_velocity = v;
    out_pos = getParticlePosition(p,v);
}

