
-- vs
out vec2 out_texco;
#ifdef IS_VOLUME
out vec4 out_pos;
out int out_instanceID;
#endif

in vec3 in_pos;

void main()
{
#ifdef IS_VOLUME
    out_pos = vec4(in_pos.xy, 0.0, 1.0);
    out_instanceID = gl_InstanceID;
#endif
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- gs
layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;

in vec4 in_texco[3];
in vec4 in_pos[3];
in int in_instanceID[3];
out float out_layer;
out float out_texco;

void main()
{
    gl_Layer = in_instanceID[0];
    out_layer = float(g_instanceID[0]) + 0.5;
    gl_Position = in_pos[0]; EmitVertex();
    gl_Position = in_pos[1]; EmitVertex();
    gl_Position = in_pos[2]; EmitVertex();
    EndPrimitive();
}

-- fs.header
#ifndef IS_VOLUME
    #define vecTex vec2
    #define ivecTex ivec2
    #define toVecTex(v) v.xy
    #define samplerTex sampler2D
    #define fragCoord() gl_FragCoord.xy
    #define ifragCoord() ivec2(fragCoord())
#else
    #define vecTex vec3
    #define ivecTex ivec3
    #define toVecTex(v) v.xyz
    #define samplerTex sampler3D
    #define fragCoord() vec3(gl_FragCoord.xy,in_layer)
    #define ifragCoord() ivec3(fragCoord())
#endif
in vecTex in_texco;

#define NORTH 0
#define SOUTH 1
#define EAST 2
#define WEST 3
#define FRONT 4
#define BACK 5

#define OCCUPIED_THRESHOLD 0.9
#define IS_CELL_OCCUPIED(pos) (texelFetch(in_obstaclesBuffer, pos, 0).r > OCCUPIED_THRESHOLD)
#define IS_OUTSIDE_SIMULATION(pos) (texelFetch(in_levelSetBuffer, pos, 0).r > 0.0)

uniform vecTex in_inverseViewport;
#ifdef IS_VOLUME
in float in_layer;
#endif

-------------------------------------
------------ Advection --------------
-------------------------------------

-- advect.vs
#include fluid.vs
-- advect.fs
#include fluid.fs.header

uniform float in_quantityLoss;
uniform float in_decayAmount;
uniform samplerTex in_velocityBuffer;
#ifndef IGNORE_OBSTACLES
uniform samplerTex in_obstaclesBuffer;
#endif
#ifdef IS_LIQUID
uniform int in_treatAsLiquid;
uniform samplerTex in_levelSetBuffer;
#endif
uniform samplerTex in_quantityBuffer;

out vec4 out_color;

void main() {
    ivecTex itexco = ifragCoord();
#ifdef IS_LIQUID
    if( in_treatAsLiquid==1 && IS_OUTSIDE_SIMULATION(itexco) ) {
        out_color = texelFetch(in_velocityBuffer, itexco, 0);
        return;
    }
#endif
#ifndef IGNORE_OBSTACLES
    if( IS_CELL_OCCUPIED(itexco) ) {
        out_color = vec4(0);
        return;
    }
#endif

    // follow the velocity field 'back in time'
    vecTex pos1 = fragCoord() - TIMESTEP*toVecTex(texelFetch(in_velocityBuffer,itexco,0));

    out_color = in_decayAmount * texture(in_quantityBuffer, in_inverseViewport*pos1);
    if(in_quantityLoss>0.0) {
        out_color -= in_quantityLoss*TIMESTEP;
    }
}

-- advectMacCormack.vs
#include fluid.vs
-- advectMacCormack.fs
#include fluid.fs.header
uniform float quantityLoss;
uniform float decayAmount;
uniform samplerTex velocityBuffer;
#ifndef IGNORE_OBSTACLES
uniform samplerTex obstaclesBuffer;
#endif
#ifdef IS_LIQUID
uniform int treatAsLiquid;
uniform samplerTex levelSetBuffer;
#endif
uniform samplerTex quantityBuffer;
uniform samplerTex quantityBufferHat;

out vec4 out_color;

void main() {
    ivecTex itexco = ifragCoord();

#ifdef IS_LIQUID
    if( treatAsLiquid==1 && IS_OUTSIDE_SIMULATION(itexco) ) {
        out_color = vec4(0);
        return;
    }
#endif
#ifndef IGNORE_OBSTACLES
    if( IS_CELL_OCCUPIED(itexco) ) {
        out_color = vec4(0);
        return;
    }
#endif

    // follow the velocity field 'back in time'
    vecTex pos1 = toVecTex((fragCoord() - TIMESTEP*texelFetch(velocityBuffer,itexco,0));
    // convert new position to texture coordinates
    vecTex nposTC = in_inverseViewport * pos1;
    // find the texel corner closest to the semi-Lagrangian 'particle'
    vecTex nposTexel = floor( pos1 + vecTex( 0.5f ) );
    vecTex nposTexelTC = in_inverseViewport*nposTexel;
    // ht (half-texel)
    vecTex ht = 0.5*in_inverseViewport;


    // get the values of nodes that contribute to the interpolated value
    // (texel centers are at half-integer locations)
#ifndef IS_VOLUME
    vec4 nodeValues[4];
    nodeValues[0] = texture( quantityBuffer, nposTexelTC + vec2(-ht.x, -ht.y) );
    nodeValues[1] = texture( quantityBuffer, nposTexelTC + vec2(-ht.x,  ht.y) );
    nodeValues[2] = texture( quantityBuffer, nposTexelTC + vec2( ht.x, -ht.y) );
    nodeValues[3] = texture( quantityBuffer, nposTexelTC + vec2( ht.x,  ht.y) );

    // determine a valid range for the result
    vec4 phiMin = min(min(min(nodeValues[0], nodeValues [1]), nodeValues [2]), nodeValues [3]);
    vec4 phiMax = max(max(max(nodeValues[0], nodeValues [1]), nodeValues [2]), nodeValues [3]);
#else
    vec4 nodeValues[8];
    nodeValues[0] = texture( quantityBuffer, nposTexelTC + vec3(-ht.x, -ht.y, -ht.z) );
    nodeValues[1] = texture( quantityBuffer, nposTexelTC + vec3(-ht.x, -ht.y,  ht.z) );
    nodeValues[2] = texture( quantityBuffer, nposTexelTC + vec3(-ht.x,  ht.y, -ht.z) );
    nodeValues[3] = texture( quantityBuffer, nposTexelTC + vec3(-ht.x,  ht.y,  ht.z) );
    nodeValues[4] = texture( quantityBuffer, nposTexelTC + vec3( ht.x, -ht.y, -ht.z) );
    nodeValues[5] = texture( quantityBuffer, nposTexelTC + vec3( ht.x, -ht.y,  ht.z) );
    nodeValues[6] = texture( quantityBuffer, nposTexelTC + vec3( ht.x,  ht.y, -ht.z) );
    nodeValues[7] = texture( quantityBuffer, nposTexelTC + vec3( ht.x, ht.y,  ht.z) );

    // determine a valid range for the result
    vec4 phiMin = min(min(min(nodeValues[0], nodeValues [1]), nodeValues [2]), nodeValues [3]);
    phiMin = min(min(min(min(phiMin, nodeValues [4]), nodeValues [5]), nodeValues [6]), nodeValues [7]);
    vec4 phiMax = max(max(max(nodeValues[0], nodeValues [1]), nodeValues [2]), nodeValues [3]);
    phiMax = max(max(max(max(phiMax, nodeValues [4]), nodeValues [5]), nodeValues [6]), nodeValues [7]);
#endif

    // Perform final MACCORMACK advection step
    out_color = texture( quantityTexture, nposTC, 0) + 0.5*(
        texture( quantityTexture, fragCoord ) -
        texture( quantityTextureHat, fragCoord ) );

    // clamp result to the desired range
    out_color = max( min( out_color, phiMax ), phiMin ) * decayAmount;
    if(quantityLoss>0.0) {
        out_color -= quantityLoss*TIMESTEP;
    }
}

-------------------------------------
------------ Pressure Solve ---------
---------------------------------------
-- pressure.vs
#include fluid.vs
-- pressure.fs
#include fluid.fs.header
uniform float in_alpha;
uniform float in_inverseBeta;
uniform samplerTex in_pressureBuffer;
uniform samplerTex in_divergenceBuffer;
#ifndef IGNORE_OBSTACLES
uniform samplerTex in_obstaclesBuffer;
#endif
#ifdef IS_LIQUID
uniform samplerTex in_levelSetBuffer;
#endif

out vec4 out_color;

#include fluid.fetchNeighbors

void main() {
    ivecTex pos = ifragCoord();
#ifdef IS_LIQUID
    // air pressure
    if( IS_OUTSIDE_SIMULATION(pos) ) {
        out_color = vec4(0.0,0.0,0.0,1.0);
        return;
    }
#endif

    vec4 p[] = fetchNeighbors(in_pressureBuffer, pos);
#ifndef IGNORE_OBSTACLES
    // Make sure that the pressure in solid cells is effectively ignored.
    vec4 pC = texelFetch(in_pressureBuffer, pos, 0);
    vec4 o[] = fetchNeighbors(in_obstaclesBuffer, pos);
    if (o[NORTH].x > OCCUPIED_THRESHOLD) p[NORTH] = pC;
    if (o[SOUTH].x > OCCUPIED_THRESHOLD) p[SOUTH] = pC;
    if (o[EAST].x > OCCUPIED_THRESHOLD) p[EAST] = pC;
    if (o[WEST].x > OCCUPIED_THRESHOLD) p[WEST] = pC;
#ifdef IS_VOLUME
    if (o[FRONT].x > OCCUPIED_THRESHOLD) p[FRONT] = pC;
    if (o[BACK].x > OCCUPIED_THRESHOLD) p[BACK] = pC;
#endif
#endif
    
    vec4 dC = texelFetch(in_divergenceBuffer, pos, 0);
#ifndef IS_VOLUME
    out_color = (p[WEST] + p[EAST] + p[SOUTH] + p[NORTH] + in_alpha * dC) * in_inverseBeta;
#else
    out_color = (p[WEST] + p[EAST] + p[SOUTH] + p[NORTH] +
        p[FRONT] + p[BACK] + in_alpha * dC) * in_inverseBeta;
#endif
}

-- substractGradient.vs
#include fluid.vs
-- substractGradient.fs
#include fluid.fs.header
uniform float in_gradientScale;
uniform samplerTex in_velocityBuffer;
uniform samplerTex in_pressureBuffer;
#ifndef IGNORE_OBSTACLES
uniform samplerTex in_obstaclesBuffer;
#endif
#ifdef IS_LIQUID
uniform samplerTex in_levelSetBuffer;
#endif

out vecTex out_color;

#include fluid.fetchNeighbors

void main() {
    ivecTex pos = ifragCoord();

#ifdef IS_LIQUID
    if( IS_OUTSIDE_SIMULATION(pos) ) {
        out_color = vecTex(0);
        return;
    }
#endif
#ifndef IGNORE_OBSTACLES
    vec4 oC = texelFetch(in_obstaclesBuffer, pos, 0);
    if (oC.x > OCCUPIED_THRESHOLD) {
        out_color = toVecTex(oC.yzw);
        return;
    }
#endif
    
    vec4 oldV = texelFetch(in_velocityBuffer, pos, 0);

    vec4 p[] = fetchNeighbors(in_pressureBuffer, pos);
    vecTex obstV = vecTex(0);
    vecTex vMask = vecTex(1);
   
#ifndef IGNORE_OBSTACLES
    // Use center pressure for solid cells
    float pC = texelFetch(in_pressureBuffer, pos, 0).r;
    vec4 o[] = fetchNeighbors(in_obstaclesBuffer, pos);
    if (o[NORTH].x > OCCUPIED_THRESHOLD) { p[NORTH].x = pC; obstV.y = o[NORTH].z; }
    if (o[SOUTH].x > OCCUPIED_THRESHOLD) { p[SOUTH].x = pC; obstV.y = o[SOUTH].z; }
    if (o[EAST].x > OCCUPIED_THRESHOLD) { p[EAST].x = pC; obstV.x = o[EAST].y; }
    if (o[WEST].x > OCCUPIED_THRESHOLD) { p[WEST].x = pC; obstV.x = o[WEST].y; }
#ifdef IS_VOLUME
    if (o[FRONT].x > OCCUPIED_THRESHOLD) { p[FRONT].x = pC; obstV.z = 0.0f; vMask.z = 0; }
    if (o[BACK].x > OCCUPIED_THRESHOLD) { p[BACK].x = pC; obstV.z = 0.0f; vMask.z = 0; }
#endif
#endif

#ifndef IS_VOLUME
    vec2 v = oldV.xy - vec2(p[EAST].x - p[WEST].x, p[NORTH].x - p[SOUTH].x) * in_gradientScale;
#else
    vec3 v = oldV.xyz - vec3(p[EAST].x - p[WEST].x, p[NORTH].x - p[SOUTH].x, p[FRONT].x - p[BACK].x) * in_gradientScale;
#endif

    // there are some artifacts with fluid sticking to obstacles with that code...
    //if ( (oN.x > 0 && v.y > 0.0) || (oS.x > 0 && v.y < 0.0) ) { vMask.y = 0; }
    //if ( (oE.x > 0 && v.x > 0.0) || (oW.x > 0 && v.x < 0.0) ) { vMask.x = 0; }

    out_color = (vMask * v) + obstV;
}

-- divergence.vs
#include fluid.vs
-- divergence.fs
#include fluid.fs.header
uniform float in_halfInverseCellSize;
uniform samplerTex in_velocityBuffer;
#ifndef IGNORE_OBSTACLES
uniform samplerTex in_obstaclesBuffer;
#endif

out float out_color;

#include fluid.fetchNeighbors

void main() {
    ivecTex pos = ifragCoord();

    vec4 velocity[] = fetchNeighbors(in_velocityBuffer, pos);
#ifndef IGNORE_OBSTACLES
    // Use obstacle velocities for solid cells
    vec4 obstacle[] = fetchNeighbors(in_obstaclesBuffer, pos);
#ifndef IS_VOLUME
    if (obstacle[NORTH].x > OCCUPIED_THRESHOLD) velocity[NORTH].xy = obstacle[NORTH].yz;
    if (obstacle[SOUTH].x > OCCUPIED_THRESHOLD) velocity[SOUTH].xy = obstacle[SOUTH].yz;
    if (obstacle[EAST].x > OCCUPIED_THRESHOLD)  velocity[EAST].xy  = obstacle[EAST].yz;
    if (obstacle[WEST].x > OCCUPIED_THRESHOLD)  velocity[WEST].xy  = obstacle[WEST].yz;
#else
    if (obstacle[NORTH].x > OCCUPIED_THRESHOLD) velocity[NORTH].xyz = obstacle[NORTH].yzw;
    if (obstacle[SOUTH].x > OCCUPIED_THRESHOLD) velocity[SOUTH].xyz = obstacle[SOUTH].yzw;
    if (obstacle[EAST].x > OCCUPIED_THRESHOLD)  velocity[EAST].xyz  = obstacle[EAST].yzw;
    if (obstacle[WEST].x > OCCUPIED_THRESHOLD)  velocity[WEST].xyz  = obstacle[WEST].yzw;
    if (obstacle[FRONT].x > OCCUPIED_THRESHOLD) velocity[FRONT].xyz = obstacle[FRONT].yzw;
    if (obstacle[BACK].x > OCCUPIED_THRESHOLD)  velocity[BACK].xyz  = obstacle[BACK].yzw;
#endif
#endif
#ifndef IS_VOLUME
    out_color = in_halfInverseCellSize * (
                velocity[EAST].x - velocity[WEST].x +
                velocity[NORTH].y - velocity[SOUTH].y);
#else
    out_color = in_halfInverseCellSize * (
            velocity[EAST].x - velocity[WEST].x +
            velocity[NORTH].y - velocity[SOUTH].y +
            velocity[FRONT].z - velocity[BACK].z);
#endif
}

-------------------------------------
---------- Vorticity ----------------
-------------------------------------

-- vorticity.compute.vs
#include fluid.vs
-- vorticity.compute.fs
#include fluid.fs.header
uniform samplerTex in_velocityBuffer;
out vecTex out_color;

#include fluid.fetchNeighbors

void main() {
    ivecTex pos = ifragCoord();
    vec4 v[] = fetchNeighbors(in_velocityBuffer, pos);
    // using central differences: D0_x = (D+_x - D-_x) / 2;
#ifndef IS_VOLUME
    out_color = 0.5*vec2(v[NORTH].y - v[SOUTH].y, v[EAST].x - v[WEST].x);
#else
    out_color = 0.5*vec3(
        (( v[NORTH].z - v[SOUTH].z ) - ( v[FRONT].y - v[BACK].y )),
        (( v[FRONT].x - v[BACK].x ) - ( v[EAST].z - v[WEST].z )),
        (( v[EAST].y - v[WEST].y ) - ( v[NORTH].x - v[SOUTH].x )));
#endif
}

-- vorticity.confinement.vs
#include fluid.vs
-- vorticity.confinement.fs
#include fluid.fs.header
uniform samplerTex in_vorticityBuffer;
#ifndef IGNORE_OBSTACLES
uniform samplerTex in_obstaclesBuffer;
#endif
uniform float in_confinementScale;

out vecTex out_color;

#include fluid.fetchNeighbors

void main() {
    ivecTex pos = ifragCoord();
#ifndef IGNORE_OBSTACLES
    // discard obstacle fragments
    if (texelFetch(in_obstaclesBuffer, pos, 0).x > OCCUPIED_THRESHOLD) discard;
#endif

    vec4 omegaC = texelFetch(in_vorticityBuffer, pos, 0);
    vec4 omega[] = fetchNeighbors(in_vorticityBuffer, pos);

#ifndef IS_VOLUME
    // compute normalized vorticity vector field psi:
    //     psi = eta / |eta| , with eta = NABLA omega
    vec2 eta = 0.5*vec2(
            length(omega[EAST].xy) - length(omega[WEST].xy),
            length(omega[NORTH].xy) - length(omega[SOUTH].xy));
    eta = normalize( eta + vec2(0.001) );
    
    // compute the vorticity force by:
    //     f = epsilon * cross( psi, omega ) * delta_x
    out_color = in_confinementScale  * eta.yx * omegaC.xy;
#else
    // compute normalized vorticity vector field psi:
    //     psi = eta / |eta| , with eta = NABLA omega
    vec3 eta = 0.5 * vec3(length(omega[EAST].xyz) - length(omega[WEST].xyz),
                          length(omega[NORTH].xyz) - length(omega[SOUTH].xyz),
                          length(omega[FRONT].xyz) - length(omega[BACK].xyz));
    eta = normalize( eta + vec3(0.001) );

    // compute the vorticity force by:
    //     f = epsilon * cross( psi, omega ) * delta_x
    out_color = vortConfinementScale * vec3(
                   eta.y * omega.z - eta.z * omegaC.y,
                   eta.z * omega.x - eta.x * omegaC.z,
                   eta.x * omega.y - eta.y * omegaC.x);
#endif
}

-------------------------------------
----------- buoyancy ----------------
-------------------------------------

-- buoyancy.vs
#include fluid.vs
-- buoyancy.fs
#include fluid.fs.header

uniform samplerTex in_temperatureBuffer;
uniform samplerTex in_densityBuffer;
uniform float in_ambientTemperature;
uniform float in_weight;
uniform float in_buoyancy;

out vecTex out_color;

void main() {
    ivecTex TC = ifragCoord();
    float t = texelFetch(in_temperatureBuffer, TC, 0).r;

    if (t > in_ambientTemperature) {
        float d = texelFetch(in_densityBuffer, TC, 0).x;
#ifndef IS_VOLUME
        vec2 v = vec2(0,1);
#else
        vec3 v = vec3(0,1,0);
#endif
        out_color = (TIMESTEP * (t - in_ambientTemperature) * in_buoyancy - d * in_weight ) * v;
    } else {
        discard;
    }
}

-- diffusion
#include fluid.fs.header

uniform samplerTex in_currentBuffer;
uniform samplerTex in_initialBuffer;
uniform float in_viscosity;

out vec4 out_color;

#include fluid.fetchNeighbors

void main() {
    ivecTex pos = ifragCoord();
    
    vec4 phin[] = fetchNeighbors(in_currentBuffer, pos);
#ifndef IS_VOLUME
    vec4 sum = phin[NORTH] + phin[EAST] + phin[SOUTH] + phin[WEST];
#else
    vec4 sum = phin[NORTH] + phin[EAST] + phin[SOUTH] + phin[WEST] + phin[FRONT] + phin[BACK];
#endif
    
    vec4 phiC = texelFetch(in_initialBuffer, pos, 0);
    float dX = 1;
    out_color =
        ( (phiC * dX*dX) - (dT * in_viscosity * sum) ) /
        ((6 * TIMESTEP * in_viscosity) + (dX*dX));
}

-------------------------------------
---------- Liquids ------------------
-------------------------------------

-- liquid.redistance.vs
#include fluid.vs
-- liquid.redistance.fs
#include fluid.fs.header

uniform samplerTex in_levelSetBuffer;
uniform samplerTex in_initialLevelSetBuffer;

uniform float in_gradientScale;

out float out_color;

#include fluid.liquid.gradient

void main() {
    float phiC = texelFetch( in_levelSetBuffer, ifragCoord(), 0).r;

    // avoid redistancing near boundaries, where gradients are ill-defined
    if( (fragCoord().x < 3) ||
        (fragCoord().x > (1.0/in_inverseViewport.x-4)) )
    {
        out_color = phiC;
        return;
    }
    if( (gl_FragCoord.y < 3) ||
        (gl_FragCoord.y > (1.0/in_inverseViewport.y-4)) )
    {
        out_color = phiC;
        return;
    }
#ifdef IS_VOLUME
    if( (fragCoord().z < 3) ||
        (fragCoord().z > (1.0/in_inverseViewport.z-4)) )
    {
        out_color = phiC;
        return;
    };
#endif

    bool isBoundary;
    bool hasHighSlope;
    vec2 gradPhi = liquidGradient(
            in_levelSetBuffer,
            ivec2(gl_FragCoord.xy),
            1.01f,
            isBoundary,
            hasHighSlope);
    float normGradPhi = length(gradPhi);
    if( isBoundary || !hasHighSlope || ( normGradPhi < 0.01f ) ) {
        out_color = phiC;
        return;
    }

    float phiC0 = texture( in_initialLevelSetBuffer, pos ).r;
    float signPhi = phiC0 / sqrt( (phiC0*phiC0) + 1);
    vec2 backwardsPos = gl_FragCoord.xy -
        TIMESTEP * in_gradientScale * (gradPhi.xy/normGradPhi) * signPhi;
    vec2 npos = in_inverseViewport*vec2(backwardsPos.x, backwardsPos.y);
    out_color = texture( in_levelSetBuffer, npos ).r + signPhi;
}

-- extrapolate.vs
#include fluid.vs
-- extrapolate.fs
#include fluid.fs.header

uniform float in_gradientScale;
uniform samplerTex in_velocityBuffer;
#ifndef IGNORE_OBSTACLES
uniform samplerTex in_obstaclesBuffer;
#endif
#ifdef IS_LIQUID
uniform samplerTex in_levelSetBuffer;
#endif
out vecTex out_color;

#include fluid.fetchNeighbors

vecTex gradient(samplerTex tex, ivecTex pos)
{
    vec4 gradient[] = fetchNeighbors(tex,pos);
    return 0.5f * vecTex(
        gradient[EAST].x - gradient[WEST].x,
        gradient[NORTH].x - gradient[SOUTH].x
#ifndef IS_VOLUME
    );
#else
        gradient[FRONT].x - gradient[BACK].x
    );
#endif
}

void main() {
    ivecTex ipos = ifragCoord();
#ifndef IGNORE_OBSTACLES
    if( IS_CELL_OCCUPIED(ipos) ) {
        out_color = vecTex(0);
        return;
    }
#endif
#ifdef IS_LIQUID
    if( !IS_OUTSIDE_SIMULATION(ipos) ) {
        out_color = toVecTex(texelFetch(in_velocityBuffer,ipos,0));
        return;
    }
#endif
    vecTex normalizedGradLS = normalize(gradient(in_levelSetBuffer, ipos) );
    vecTex backwardsPos = fragCoord() - TIMESTEP * in_gradientScale * normalizedGradLS;

    out_color = toVecTex(texture(in_velocityBuffer, in_inverseViewport*backwardsPos));
}

-- liquid.stream.vs
#include fluid.vs
-- liquid.stream.fs
#include fluid.fs.header

uniform vecTex in_streamCenter;
uniform float in_streamRadius;
uniform vec3 in_streamValue;
uniform int in_streamUseValue;
#ifndef IGNORE_OBSTACLES
uniform samplerTex in_obstaclesBuffer;
#endif

out vec4 out_color;

void main() {
    ivecTex ipos = ifragCoord();
#ifndef IGNORE_OBSTACLES
    if( IS_CELL_OCCUPIED(ipos) ) discard;
#endif
    float dist = length(fragCoord() - in_streamCenter);

    if( dist > in_streamRadius ) discard;

    if( in_streamUseValue==1 ) out_color.rgb = in_streamValue;
    else out_color.r = (dist - in_streamRadius);
}

-- liquid.distanceToHeight.vs
#include fluid.vs
-- liquid.distanceToHeight.fs
uniform float in_liquidHeight;

out float out_color;

void main() {
     out_color = gl_FragCoord.y - in_liquidHeight;
}

-------------------------------------
---------- External forces ----------
-------------------------------------
-- gravity.vs
#include fluid.vs
-- gravity.fs
#include fluid.fs.header

uniform vec4 in_gravityValue;
#ifndef IGNORE_OBSTACLES
uniform samplerTex in_obstaclesBuffer;
#endif
#ifdef IS_LIQUID
uniform samplerTex in_levelSetBuffer;
#endif
out vec4 out_color;

void main() {
    ivecTex ipos = ifragCoord();
#ifdef IS_LIQUID
    if( IS_OUTSIDE_SIMULATION(ipos) ) discard;
#endif
#ifndef IGNORE_OBSTACLES
    if( IS_CELL_OCCUPIED(ipos) ) discard;
#endif
    out_color = TIMESTEP * in_gravityValue;
}

-- splat.circle.vs
#include fluid.vs
-- splat.circle.fs
#include fluid.fs.header

#ifndef IGNORE_OBSTACLES
uniform samplerTex in_obstaclesBuffer;
#endif

uniform float in_splatRadius;
uniform vec4 in_splatValue;
uniform vecTex in_splatPoint;

out vec4 out_color;

#define AA_PIXELS 2.0
#define USE_AA

void main() {
    ivecTex ipos = ifragCoord();
#ifndef IGNORE_OBSTACLES
    if( IS_CELL_OCCUPIED(ipos) ) discard;
#endif

    float dist = distance(toVecTex(in_splatPoint), fragCoord());
    if (dist > in_splatRadius) discard;
#ifdef USE_AA
    float threshold = in_splatRadius - AA_PIXELS;
    if(dist<threshold) {
        out_color = in_splatValue;
    } else {
        // anti aliasing
        out_color = in_splatValue * (1.0f - (dist-threshold)/(in_splatRadius-threshold));
    }
#else
    out_color = in_splatValue;
#endif
}

-- splat.mouse.vs
#include fluid.vs
-- splat.mouse.fs
#include fluid.fs.header
#include regen.states.camera.defines
#include regen.states.camera.transformTexcoToWorld
#include regen.filter.sampling.computeTexco

#ifndef IGNORE_OBSTACLES
uniform samplerTex in_obstaclesBuffer;
#endif

uniform float in_splatRadius;
uniform vec4 in_splatValue;

uniform mat4 in_modelMatrix;
// Normalized Device Coordinates of the mouse
uniform vec2 in_mouseTexco;
// Depth value at the mouse position in view space
uniform float in_mouseDepthVS;
uniform vec2 in_objectSize;

uniform texture2D in_gDepthTexture;

out vec4 out_color;

#define AA_PIXELS 2.0
#define USE_AA

void main() {
    ivecTex ipos = ifragCoord();
#ifndef IGNORE_OBSTACLES
    if( IS_CELL_OCCUPIED(ipos) ) discard;
#endif

    vec2 fragTexco = gl_FragCoord.xy*in_inverseViewport;
    vec2 texco = computeTexco(fragTexco);

    vec2 mouseUV = in_mouseTexco;
    //mouseUV.x = 1.0 - mouseUV.x;

    vec3 mouseWS = transformTexcoToWorld(mouseUV, texture(in_gDepthTexture, mouseUV).x, in_layer);
    vec4 mouseWorldSpace = vec4(mouseWS, 1.0);

    //float z_ndc = depthBufferValue * 2.0 - 1.0;
    //float z_view = (2.0 * in_near * in_far) / (in_far + in_near - z_ndc * (in_far - in_near));

/*
    // in_mouseTexco is in [0,1]x[0,1] with (0,0) in the top left corner
    // todo: maybe y is switched
    // Convert mouse UV to NDC, where (0,0) is in the center of the screen
    // and the screen is in the range [-1,1]x[-1,1]
    vec2 mouseNDC = mouseUV * 2.0 - vec2(1.0);
    // in_mouseDepthVS is the intersection depth of the mouse in view space.
    // We need to convert this to NDC space, i.e. in the range [-1,1].
    float ndcDepth = (2.0 * in_mouseDepthVS - in_near - in_far) / (in_far - in_near);
    // mouseClipSpace is the mouse position in clip space
    vec4 mouseClipSpace = vec4(mouseUV, ndcDepth, 1.0);
    // Transform from clip space to world space.
    // This coordinate is the point of intersection of the mouse with the
    // object in world space.
    vec4 mouseWorldSpace = REGEN_VIEW_PROJ_INV_(0) * mouseClipSpace;
    mouseWorldSpace /= mouseWorldSpace.w;
    */
    // Transform from world space to local space using the inverse model matrix.
    // TODO: rather compute model inverse on cpu and hand in
    vec4 mouseLocalSpace = inverse(in_modelMatrix) * mouseWorldSpace;
    // Normalize the mouse position to the object size
    vec2 normalizedMouseLocalSpace = ((mouseLocalSpace.xz + 0.5*in_objectSize) / in_objectSize);
    normalizedMouseLocalSpace.x = clamp(normalizedMouseLocalSpace.x, 0.0, 1.0);
    normalizedMouseLocalSpace.y = clamp(normalizedMouseLocalSpace.y, 0.0, 1.0);
    // Flip y coordinate
    normalizedMouseLocalSpace.y = 1.0 - normalizedMouseLocalSpace.y;
    // Compute the distance from the current fragment to the splat point
    float dist = length(gl_FragCoord.xy - normalizedMouseLocalSpace*in_viewport);

    if (dist > in_splatRadius) discard;
#ifdef USE_AA
    float threshold = in_splatRadius - AA_PIXELS;
    if(dist<threshold) {
        out_color = in_splatValue;
    } else {
        // anti aliasing
        out_color = in_splatValue * (1.0f - (dist-threshold)/(in_splatRadius-threshold));
    }
#else
    out_color = in_splatValue;
#endif
}

-- splat.border.vs
#include fluid.vs
-- splat.border.fs
#include fluid.fs.header

uniform float in_splatBorder;
uniform vec4 in_splatValue;
#ifndef IGNORE_OBSTACLES
uniform samplerTex in_obstaclesBuffer;
#endif

out vec4 out_color;

void main() {
    ivecTex ipos = ifragCoord();

#ifndef IGNORE_OBSTACLES
    if( IS_CELL_OCCUPIED(ipos) ) discard;
#endif

    vecTex splatBorderNormalized = in_splatBorder*in_inverseViewport;
    if(in_texco.x < splatBorderNormalized.x
       || in_texco.x > 1.0-splatBorderNormalized.x
       || in_texco.y < splatBorderNormalized.y
       || in_texco.y > 1.0-splatBorderNormalized.y
#ifdef IS_VOLUME
       || in_texco.z < splatBorderNormalized.z
       || in_texco.z > 1.0-splatBorderNormalized.z
#endif
    ){
        out_color = in_splatValue;
    }
    else
    {
        discard;
    }
}

-- splat.rect.vs
#include fluid.vs
-- splat.rect.fs
#include fluid.fs.header

uniform vecTex in_splatSize;
uniform vec4 in_splatValue;
uniform vecTex in_splatPoint;
#ifndef IGNORE_OBSTACLES
uniform samplerTex in_obstaclesBuffer;
#endif

out vec4 out_color;

void main() {
    ivecTex ipos = ifragCoord();
#ifndef IGNORE_OBSTACLES
    if( IS_CELL_OCCUPIED(ipos) ) discard;
#endif

    vecTex pos = fragCoord();
    if (abs(pos.x-in_splatPoint.x) > 0.5*in_splatSize.x) discard;
    if (abs(pos.y-in_splatPoint.y) > 0.5*in_splatSize.y) discard;
#ifdef IS_VOLUME
    if (abs(pos.z-in_splatPoint.z) > 0.5*in_splatSize.z) discard;
#endif
    out_color = in_splatValue;
}

-- splat.tex.vs
#include fluid.vs
-- splat.tex.fs
#include fluid.fs.header

uniform samplerTex in_splatTexture;
#ifndef IGNORE_OBSTACLES
uniform samplerTex in_obstaclesBuffer;
#endif
uniform float in_texelFactor;

out vec4 out_color;

void main() {
    ivecTex ipos = ifragCoord();
#ifndef IGNORE_OBSTACLES
    if( IS_CELL_OCCUPIED(ipos) ) discard;
#endif
    vec4 val = texture(in_splatTexture, vec2(in_texco.x,in_texco.y));
    if (val.a <= 0.00001) discard;
    out_color = in_texelFactor*val;
}

-- splat.texToScalar.vs
#include fluid.vs
-- splat.texToScalar.fs
#include fluid.fs.header

uniform samplerTex splatTexture;
#ifndef IGNORE_OBSTACLES
uniform samplerTex obstaclesBuffer;
#endif
uniform float texelFactor;

out float out_color;

void main() {
    ivecTex ipos = ifragCoord();
#ifndef IGNORE_OBSTACLES
    if( IS_CELL_OCCUPIED(ipos) ) discard;
#endif
    vec4 val = texture(splatTexture, vec2(in_texco.x,-in_texco.y));
    if (val.a <= 0.00001) discard;
    out_color = texelFactor*(val.r+val.g+val.b)/3.0;
}

-- liquid.gradient.vs
#include fluid.vs
-- liquid.gradient.fs
vecTex liquidGradient(
        samplerTex tex,
        ivecTex pos,
        float minSlope,
        out bool isBoundary,
        out bool highEnoughSlope)
{
#ifndef IS_VOLUME
    float CC = texelFetchOffset( tex, pos, 0, ivec2( 0,  0 ) ).r;
    float CT = texelFetchOffset( tex, pos, 0, ivec2( 0,  1 ) ).r;
    float CB = texelFetchOffset( tex, pos, 0, ivec2( 0, -1 ) ).r;
    float RC = texelFetchOffset( tex, pos, 0, ivec2( 1,  0 ) ).r;
    float RT = texelFetchOffset( tex, pos, 0, ivec2( 1,  1 ) ).r;
    float RB = texelFetchOffset( tex, pos, 0, ivec2( 1, -1 ) ).r;
    float LC = texelFetchOffset( tex, pos, 0, ivec2(-1,  0 ) ).r;
    float LT = texelFetchOffset( tex, pos, 0, ivec2(-1,  1 ) ).r;
    float LB = texelFetchOffset( tex, pos, 0, ivec2(-1, -1 ) ).r;
    
    // is this cell next to the LevelSet boundary
    float product = LC * RC * CB * CT * LB * LT * RB * RT;
    isBoundary = (product < 0 ? true : false);
    // is the slope high enough
    highEnoughSlope = (
            (abs(RC - CC) > minSlope) ||
            (abs(LC - CC) > minSlope) ||
            (abs(CT - CC) > minSlope) ||
            (abs(CB - CC) > minSlope) );

    return 0.5 * vec2(RC - LC,  CT - CB);
#else
    float LCC = texelFetchOffset( tex, pos, 0, ivec3(-1,  0,  0) ).r;
    float RCC = texelFetchOffset( tex, pos, 0, ivec3( 1,  0,  0) ).r;
    float CBC = texelFetchOffset( tex, pos, 0, ivec3( 0, -1,  0) ).r;
    float CTC = texelFetchOffset( tex, pos, 0, ivec3( 0,  1,  0) ).r;
    float CCD = texelFetchOffset( tex, pos, 0, ivec3( 0,  0, -1) ).r;
    float CCU = texelFetchOffset( tex, pos, 0, ivec3( 0,  0,  1) ).r;
    float LBD = texelFetchOffset( tex, pos, 0, ivec3(-1, -1, -1) ).r;
    float LBC = texelFetchOffset( tex, pos, 0, ivec3(-1, -1,  0) ).r;
    float LBU = texelFetchOffset( tex, pos, 0, ivec3(-1, -1,  1) ).r;
    float LCD = texelFetchOffset( tex, pos, 0, ivec3(-1,  0, -1) ).r;
    float LCU = texelFetchOffset( tex, pos, 0, ivec3(-1,  0,  1) ).r;
    float LTD = texelFetchOffset( tex, pos, 0, ivec3(-1,  1, -1) ).r;
    float LTC = texelFetchOffset( tex, pos, 0, ivec3(-1,  1,  0) ).r;
    float LTU = texelFetchOffset( tex, pos, 0, ivec3(-1,  1,  1) ).r;
    float CBD = texelFetchOffset( tex, pos, 0, ivec3( 0, -1, -1) ).r;
    float CBU = texelFetchOffset( tex, pos, 0, ivec3( 0, -1,  1) ).r;
    float CCC = texelFetchOffset( tex, pos, 0, ivec3( 0,  0,  0) ).r;
    float CTD = texelFetchOffset( tex, pos, 0, ivec3( 0,  1, -1) ).r;
    float CTU = texelFetchOffset( tex, pos, 0, ivec3( 0,  1,  1) ).r;
    float RBD = texelFetchOffset( tex, pos, 0, ivec3( 1, -1, -1) ).r;
    float RBC = texelFetchOffset( tex, pos, 0, ivec3( 1, -1,  0) ).r;
    float RBU = texelFetchOffset( tex, pos, 0, ivec3( 1, -1,  1) ).r;
    float RCD = texelFetchOffset( tex, pos, 0, ivec3( 1,  0, -1) ).r;
    float RCU = texelFetchOffset( tex, pos, 0, ivec3( 1,  0,  1) ).r;
    float RTD = texelFetchOffset( tex, pos, 0, ivec3( 1,  1, -1) ).r;
    float RTC = texelFetchOffset( tex, pos, 0, ivec3( 1,  1,  0) ).r;
    float RTU = texelFetchOffset( tex, pos, 0, ivec3( 1,  1,  1) ).r;
    
    // is this cell next to the LevelSet boundary
    float product = LCC * RCC * CBC * CTC * CCD * CCU;
    product *= LBD * LBC * LBU * LCD * LCU * LTD * LTC * LTU
             * CBD * CBU * CTD * CTU
             * RBD * RBC * RBU * RCD * RCU * RTD * RTC * RTU;
    isBoundary = product < 0;
    // is the slope high enough
    highEnoughSlope =
            (abs(R - CCC) > minSlope) ||
            (abs(L - CCC) > minSlope) ||
            (abs(T - CCC) > minSlope) ||
            (abs(B - CCC) > minSlope) ||
            (abs(U - CCC) > minSlope) ||
            (abs(D - CCC) > minSlope);

    return 0.5 * vec3(RCC - LCC,  CTC - CBC,  CCU - CCD);
#endif
}

-- fetchNeighbors
#ifndef fetchNeighbors_INCLUDED
#define fetchNeighbors_INCLUDED
#ifndef IS_VOLUME
vec4[4] fetchNeighbors(samplerTex tex, ivecTex pos) {
    vec4[4] neighbors;
    neighbors[NORTH] = texelFetchOffset(tex, pos, 0, ivec2( 0,  1));
    neighbors[SOUTH] = texelFetchOffset(tex, pos, 0, ivec2( 0, -1));
    neighbors[EAST]  = texelFetchOffset(tex, pos, 0, ivec2( 1,  0));
    neighbors[WEST]  = texelFetchOffset(tex, pos, 0, ivec2(-1,  0));
    return neighbors;
}
#else
vec4[6] fetchNeighbors(samplerTex tex, ivecTex pos) {
    vec4[6] neighbors;
    neighbors[NORTH] = texelFetchOffset(tex, pos, 0, ivec3( 0,  1,  0));
    neighbors[SOUTH] = texelFetchOffset(tex, pos, 0, ivec3( 0, -1,  0));
    neighbors[EAST]  = texelFetchOffset(tex, pos, 0, ivec3( 1,  0,  0));
    neighbors[WEST]  = texelFetchOffset(tex, pos, 0, ivec3(-1,  0,  0));
    neighbors[FRONT] = texelFetchOffset(tex, pos, 0, ivec3( 0,  0,  1));
    neighbors[BACK]  = texelFetchOffset(tex, pos, 0, ivec3( 0,  0, -1));
    return neighbors;
}
#endif
#endif // fetchNeighbors_INCLUDED


-- sample.identity.vs
#include fluid.vs
-- sample.identity.fs
#include fluid.fs.header
uniform samplerTex in_quantity;

out vec4 out_color;

void main() {
    out_color = texture(in_quantity, in_texco);
}

-- sample.coarseToFine.vs
#include fluid.vs
-- sample.coarseToFine.fs
#include fluid.fs.header

uniform samplerTex in_quantityCoarse;
uniform samplerTex in_quantityCoarse0;
vecTex quantitySizeCoarse;

uniform samplerTex in_quantityFine0;
vecTex quantitySizeFine;

uniform float alpha;

out vec4 out_color;

void main() {
    vec4 valCoarse = texture(in_quantityCoarse, in_texco);
    vec4 valCoarse0 = texture(in_quantityCoarse0, in_texco);
    vec4 valFine0 = texture(in_quantityFine0, in_texco);
    out_color = alpha*valCoarse +
        (1.0-alpha)*(valFine0 + valCoarse - valCoarse0);
}

-- sample.removeLines.vs
#include fluid.vs
-- sample.removeLines.fs
#include fluid.fs.header

uniform samplerTex quantity;
uniform vec4 in_removeColor;

out vec4 out_color;

void main() {
    ivecTex pos = ifragCoord();
    vec4 col = texelFetchOffset(in_quantity, pos, 0, ivec2(0,0));
    int count = 0;
    if(distance(texelFetchOffset(in_quantity, pos, 0, ivec2(0,1)).rgb,col.rgb)<0.0001) {
        count += 1;
    }
    if(distance(texelFetchOffset(in_quantity, pos, 0, ivec2(0,-1)).rgb,col.rgb)<0.0001) {
        count += 1;
    }
    if(distance(texelFetchOffset(in_quantity, pos, 0, ivec2(1,0)).rgb,col.rgb)<0.0001) {
        count += 1;
    }
    if(distance(texelFetchOffset(in_quantity, pos, 0, ivec2(-1,0)).rgb,col.rgb)<0.0001) {
        count += 1;
    }
    if(count>2) {
        out_color = col;
    } else {
        out_color = in_removeColor;
    }
}

-- sample.smooth.vs
#include fluid.vs
-- sample.smooth.fs
#include fluid.fs.header

uniform samplerTex in_quantity;

out vec4 out_color;

void main() {
    ivecTex pos = ifragCoord();
    out_color = texelFetchOffset(in_quantity, pos, 0, ivec2( 0,  1));
    out_color += texelFetchOffset(in_quantity, pos, 0, ivec2( 0, -1));
    out_color += texelFetchOffset(in_quantity, pos, 0, ivec2( 1,  0));
    out_color += texelFetchOffset(in_quantity, pos, 0, ivec2(-1,  0));
    out_color /= 4.0;
}

-- sample.scalar.vs
#include fluid.vs
-- sample.scalar.fs
#include fluid.fs.header

uniform samplerTex in_quantity;
uniform float in_texelFactor;
uniform vec3 in_colorPositive;
uniform vec3 in_colorNegative;

out vec4 out_color;

void main() {
    vec2 texco = vec2(in_texco.x, 1.0-in_texco.y);
    float x = in_texelFactor*texture(in_quantity,in_texco).r;
    if(x>0.0) {
        out_color = vec4(in_colorPositive, x);
    } else {
        out_color = vec4(in_colorNegative, -x);
    }
}

-- sample.levelSet.vs
#include fluid.vs
-- sample.levelSet.fs
#include fluid.fs.header

uniform samplerTex in_quantity;
uniform float in_texelFactor;
uniform vec3 in_colorPositive;
uniform vec3 in_colorNegative;

out vec4 out_color;

void main() {
    float x = in_texelFactor*texture(in_quantity,in_texco).r;
    if(x>0.0) {
        out_color = vec4(in_colorNegative, 1.0f);
    } else {
        out_color = vec4(in_colorPositive, 0.0f);
    }
}

-- sample.rgb.vs
#include fluid.vs
-- sample.rgb.fs
#include fluid.fs.header

uniform samplerTex in_quantity;
uniform float in_texelFactor;

out vec4 out_color;

void main() {
    vec2 texco = vec2(in_texco.x, 1.0-in_texco.y);
    out_color.rgb = in_texelFactor*texture(in_quantity,in_texco).rgb;
    out_color.a = min(1.0, length(out_color.rgb));
}

-- sample.velocity.vs
#include fluid.vs
-- sample.velocity.fs
#include fluid.fs.header

uniform samplerTex in_quantity;
uniform float in_texelFactor;

out vec4 out_color;

void main() {
    out_color.rgb = in_texelFactor*texture(in_quantity,in_texco).rgb;
    out_color.a = min(1.0, length(out_color.rgb));
    out_color.b = 0.0;
    if(out_color.r < 0.0) out_color.b += -0.5*out_color.r;
    if(out_color.g < 0.0) out_color.b += -0.5*out_color.g;
}

-- sample.fire.vs
#include fluid.vs
-- sample.fire.fs
#include fluid.fs.header

uniform samplerTex in_quantity;
uniform sampler1D in_pattern;
uniform float in_texelFactor;
uniform float in_fireAlphaMultiplier;
uniform float in_fireWeight;
uniform float in_smokeColorMultiplier;
uniform float in_smokeAlphaMultiplier;
uniform int in_rednessFactor;
uniform vec3 in_smokeColor;

out vec4 out_color;

void main() {
    const float threshold = 1.4;
    const float maxValue = 5;

    float s = texture(in_quantity,in_texco).r * in_texelFactor;
    s = clamp(s,0,maxValue);

    if( s > threshold ) { //render fire
        float lookUpVal = ( (s-threshold)/(maxValue-threshold) );
        lookUpVal = 1.0 - pow(lookUpVal, in_rednessFactor);
        lookUpVal = clamp(lookUpVal,0,1);
        vec3 interpColor = texture(in_pattern, (1.0-lookUpVal)).rgb;
        vec4 tmp = vec4(interpColor,1);
        float mult = (s-threshold);
        out_color.rgb = in_fireWeight*tmp.rgb;
        out_color.a = min(1.0, in_fireWeight*mult*mult*in_fireAlphaMultiplier + 0.5);
    } else { // render smoke
        out_color.rgb = vec3(in_fireWeight*s);
        out_color.a = min(1.0, out_color.r*in_smokeAlphaMultiplier);
        out_color.rgb = out_color.a * out_color.rrr * in_smokeColor * in_smokeColorMultiplier;
    }
}

