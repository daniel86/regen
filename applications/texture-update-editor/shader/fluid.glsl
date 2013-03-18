
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
    // TODO: volume texco
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
#define IS_CELL_OCCUPIED(pos) (texelFetch(obstaclesBuffer, pos, 0).r > OCCUPIED_THRESHOLD)
#define IS_OUTSIDE_SIMULATION(pos) (texelFetch(levelSetBuffer, pos, 0).r > 0.0)

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

out vec4 output;

void main() {
    ivecTex itexco = ifragCoord();
#ifdef IS_LIQUID
    if( treatAsLiquid==1 && IS_OUTSIDE_SIMULATION(itexco) ) {
        output = texelFetch(velocityBuffer, itexco, 0);
        return;
    }
#endif
#ifndef IGNORE_OBSTACLES
    if( IS_CELL_OCCUPIED(itexco) ) {
        output = vec4(0);
        return;
    }
#endif

    // follow the velocity field 'back in time'
    vecTex pos1 = fragCoord() - TIMESTEP*toVecTex(texelFetch(velocityBuffer,itexco,0));

    output = decayAmount * texture(quantityBuffer, in_inverseViewport*pos1);
    if(quantityLoss>0.0) {
        output -= quantityLoss*TIMESTEP;
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

out vec4 output;

void main() {
    ivecTex itexco = ifragCoord();

#ifdef IS_LIQUID
    if( treatAsLiquid==1 && IS_OUTSIDE_SIMULATION(itexco) ) {
        output = vec4(0);
        return;
    }
#endif
#ifndef IGNORE_OBSTACLES
    if( IS_CELL_OCCUPIED(itexco) ) {
        output = vec4(0);
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
    vec4 output = texture( quantityTexture, nposTC, 0) + 0.5*(
        texture( quantityTexture, fragCoord ) -
        texture( quantityTextureHat, fragCoord ) );

    // clamp result to the desired range
    output = max( min( output, phiMax ), phiMin ) * decayAmount;
    if(quantityLoss>0.0) {
        output -= quantityLoss*TIMESTEP;
    }
}

-------------------------------------
------------ Pressure Solve ---------
---------------------------------------
-- pressure.vs
#include fluid.vs
-- pressure.fs
#include fluid.fs.header
uniform float alpha;
uniform float inverseBeta;
uniform samplerTex pressureBuffer;
uniform samplerTex divergenceBuffer;
#ifndef IGNORE_OBSTACLES
uniform samplerTex obstaclesBuffer;
#endif
#ifdef IS_LIQUID
uniform samplerTex levelSetBuffer;
#endif

out vec4 output;

#include fluid.fetchNeighbors

void main() {
    ivecTex pos = ifragCoord();
#ifdef IS_LIQUID
    // air pressure
    if( IS_OUTSIDE_SIMULATION(pos) ) {
        output = vec4(0.0,0.0,0.0,1.0);
        return;
    }
#endif

    vec4 p[] = fetchNeighbors(pressureBuffer, pos);
#ifndef IGNORE_OBSTACLES
    // Make sure that the pressure in solid cells is effectively ignored.
    vec4 pC = texelFetch(pressureBuffer, pos, 0);
    vec4 o[] = fetchNeighbors(obstaclesBuffer, pos);
    if (o[NORTH].x > OCCUPIED_THRESHOLD) p[NORTH] = pC;
    if (o[SOUTH].x > OCCUPIED_THRESHOLD) p[SOUTH] = pC;
    if (o[EAST].x > OCCUPIED_THRESHOLD) p[EAST] = pC;
    if (o[WEST].x > OCCUPIED_THRESHOLD) p[WEST] = pC;
#ifdef IS_VOLUME
    if (o[FRONT].x > OCCUPIED_THRESHOLD) p[FRONT] = pC;
    if (o[BACK].x > OCCUPIED_THRESHOLD) p[BACK] = pC;
#endif
#endif
    
    vec4 dC = texelFetch(divergenceBuffer, pos, 0);
#ifndef IS_VOLUME
    output = (p[WEST] + p[EAST] + p[SOUTH] + p[NORTH] + alpha * dC) * inverseBeta;
#else
    output = (p[WEST] + p[EAST] + p[SOUTH] + p[NORTH] +
        p[FRONT] + p[BACK] + alpha * dC) * inverseBeta;
#endif
}

-- substractGradient.vs
#include fluid.vs
-- substractGradient.fs
#include fluid.fs.header
uniform float gradientScale;
uniform samplerTex velocityBuffer;
uniform samplerTex pressureBuffer;
#ifndef IGNORE_OBSTACLES
uniform samplerTex obstaclesBuffer;
#endif
#ifdef IS_LIQUID
uniform samplerTex levelSetBuffer;
#endif

out vecTex output;

#include fluid.fetchNeighbors

void main() {
    ivecTex pos = ifragCoord();

#ifdef IS_LIQUID
    if( IS_OUTSIDE_SIMULATION(pos) ) {
        output = vecTex(0);
        return;
    }
#endif
#ifndef IGNORE_OBSTACLES
    vec4 oC = texelFetch(obstaclesBuffer, pos, 0);
    if (oC.x > OCCUPIED_THRESHOLD) {
        output = toVecTex(oC.yzw);
        return;
    }
#endif
    
    vec4 oldV = texelFetch(velocityBuffer, pos, 0);

    vec4 p[] = fetchNeighbors(pressureBuffer, pos);
    vecTex obstV = vecTex(0);
    vecTex vMask = vecTex(1);
   
#ifndef IGNORE_OBSTACLES
    // Use center pressure for solid cells
    float pC = texelFetch(pressureBuffer, pos, 0).r;
    vec4 o[] = fetchNeighbors(obstaclesBuffer, pos);
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
    vec2 v = oldV.xy - vec2(p[EAST].x - p[WEST].x, p[NORTH].x - p[SOUTH].x) * gradientScale;
#else
    vec3 v = oldV.xyz - vec3(p[EAST].x - p[WEST].x, p[NORTH].x - p[SOUTH].x, p[FRONT].x - p[BACK].x) * gradientScale;
#endif

    // there are some artifacts with fluid sticking to obstacles with that code...
    //if ( (oN.x > 0 && v.y > 0.0) || (oS.x > 0 && v.y < 0.0) ) { vMask.y = 0; }
    //if ( (oE.x > 0 && v.x > 0.0) || (oW.x > 0 && v.x < 0.0) ) { vMask.x = 0; }

    output = (vMask * v) + obstV;
}

-- divergence.vs
#include fluid.vs
-- divergence.fs
#include fluid.fs.header
uniform float halfInverseCellSize;
uniform samplerTex velocityBuffer;
#ifndef IGNORE_OBSTACLES
uniform samplerTex obstaclesBuffer;
#endif

out float output;

#include fluid.fetchNeighbors

void main() {
    ivecTex pos = ifragCoord();

    vec4 velocity[] = fetchNeighbors(velocityBuffer, pos);
#ifndef IGNORE_OBSTACLES
    // Use obstacle velocities for solid cells
    vec4 obstacle[] = fetchNeighbors(obstaclesBuffer, pos);
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
    output = halfInverseCellSize * (
                velocity[EAST].x - velocity[WEST].x +
                velocity[NORTH].y - velocity[SOUTH].y);
#else
    output = halfInverseCellSize * (
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
uniform samplerTex velocityBuffer;
out vecTex output;

#include fluid.fetchNeighbors

void main() {
    ivecTex pos = ifragCoord();
    vec4 v[] = fetchNeighbors(velocityBuffer, pos);
    // using central differences: D0_x = (D+_x - D-_x) / 2;
#ifndef IS_VOLUME
    output = 0.5*vec2(v[NORTH].y - v[SOUTH].y, v[EAST].x - v[WEST].x);
#else
    output = 0.5*vec3(
        (( v[NORTH].z - v[SOUTH].z ) - ( v[FRONT].y - v[BACK].y )),
        (( v[FRONT].x - v[BACK].x ) - ( v[EAST].z - v[WEST].z )),
        (( v[EAST].y - v[WEST].y ) - ( v[NORTH].x - v[SOUTH].x )));
#endif
}

-- vorticity.confinement.vs
#include fluid.vs
-- vorticity.confinement.fs
#include fluid.fs.header
uniform samplerTex vorticityBuffer;
#ifndef IGNORE_OBSTACLES
uniform samplerTex obstaclesBuffer;
#endif
uniform float confinementScale;

out vecTex output;

#include fluid.fetchNeighbors

void main() {
    ivecTex pos = ifragCoord();
#ifndef IGNORE_OBSTACLES
    // discard obstacle fragments
    if (texelFetch(obstaclesBuffer, pos, 0).x > OCCUPIED_THRESHOLD) discard;
#endif

    vec4 omegaC = texelFetch(vorticityBuffer, pos, 0);
    vec4 omega[] = fetchNeighbors(vorticityBuffer, pos);

#ifndef IS_VOLUME
    // compute normalized vorticity vector field psi:
    //     psi = eta / |eta| , with eta = NABLA omega
    vec2 eta = 0.5*vec2(
            length(omega[EAST].xy) - length(omega[WEST].xy),
            length(omega[NORTH].xy) - length(omega[SOUTH].xy));
    eta = normalize( eta + vec2(0.001) );
    
    // compute the vorticity force by:
    //     f = epsilon * cross( psi, omega ) * delta_x
    output = confinementScale  * eta.yx * omegaC.xy;
#else
    // compute normalized vorticity vector field psi:
    //     psi = eta / |eta| , with eta = NABLA omega
    vec3 eta = 0.5 * vec3(length(omega[EAST].xyz) - length(omega[WEST].xyz),
                          length(omega[NORTH].xyz) - length(omega[SOUTH].xyz),
                          length(omega[FRONT].xyz) - length(omega[BACK].xyz));
    eta = normalize( eta + vec3(0.001) );

    // compute the vorticity force by:
    //     f = epsilon * cross( psi, omega ) * delta_x
    output = vortConfinementScale * vec3(
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

uniform samplerTex temperatureBuffer;
uniform samplerTex densityBuffer;
uniform float ambientTemperature;
uniform float weight;
uniform float buoyancy;

out vecTex output;

void main() {
    ivecTex TC = ifragCoord();
    float t = texelFetch(temperatureBuffer, TC, 0).r;

    if (t > ambientTemperature) {
        float d = texelFetch(densityBuffer, TC, 0).x;
#ifndef IS_VOLUME
        vec2 v = vec2(0,1);
#else
        vec3 v = vec3(0,1,0);
#endif
        output = (TIMESTEP * (t - ambientTemperature) * buoyancy - d * weight ) * v;
    } else {
        discard;
    }
}

-- diffusion
#include fluid.fs.header

uniform samplerTex currentBuffer;
uniform samplerTex initialBuffer;
uniform float viscosity;

out vec4 output;

#include fluid.fetchNeighbors

void main() {
    ivecTex pos = ifragCoord();
    
    vec4 phin[] = fetchNeighbors(currentBuffer, pos);
#ifndef IS_VOLUME
    vec4 sum = phin[NORTH] + phin[EAST] + phin[SOUTH] + phin[WEST];
#else
    vec4 sum = phin[NORTH] + phin[EAST] + phin[SOUTH] + phin[WEST] + phin[FRONT] + phin[BACK];
#endif
    
    vec4 phiC = texelFetch(initialBuffer, pos, 0);
    float dX = 1;
    output =
        ( (phiC * dX*dX) - (dT * viscosity * sum) ) /
        ((6 * TIMESTEP * viscosity) + (dX*dX));
}

-------------------------------------
---------- Liquids ------------------
-------------------------------------

-- liquid.redistance.vs
#include fluid.vs
-- liquid.redistance.fs
#include fluid.fs.header

uniform samplerTex levelSetBuffer;
uniform samplerTex initialLevelSetBuffer;

uniform float gradientScale;

out float output;

#include fluid.liquid.gradient

void main() {
    float phiC = texelFetch( levelSetBuffer, ifragCoord(), 0).r;

    // avoid redistancing near boundaries, where gradients are ill-defined
    if( (fragCoord().x < 3) ||
        (fragCoord().x > (1.0/in_inverseViewport.x-4)) )
    {
        output = phiC;
        return;
    }
    if( (gl_FragCoord.y < 3) ||
        (gl_FragCoord.y > (1.0/in_inverseViewport.y-4)) )
    {
        output = phiC;
        return;
    }
#ifdef IS_VOLUME
    if( (fragCoord().z < 3) ||
        (fragCoord().z > (1.0/in_inverseViewport.z-4)) )
    {
        output = phiC;
        return;
    };
#endif

    bool isBoundary;
    bool hasHighSlope;
    vec2 gradPhi = liquidGradient(
            levelSetBuffer,
            ivec2(gl_FragCoord.xy),
            1.01f,
            isBoundary,
            hasHighSlope);
    float normGradPhi = length(gradPhi);
    if( isBoundary || !hasHighSlope || ( normGradPhi < 0.01f ) ) {
        output = phiC;
        return;
    }

    float phiC0 = texture( initialLevelSetBuffer, pos ).r;
    float signPhi = phiC0 / sqrt( (phiC0*phiC0) + 1);
    vec2 backwardsPos = gl_FragCoord.xy -
        TIMESTEP * gradientScale * (gradPhi.xy/normGradPhi) * signPhi;
    vec2 npos = in_inverseViewport*vec2(backwardsPos.x, backwardsPos.y);
    output = texture( levelSetBuffer, npos ).r + signPhi;
}

-- extrapolate.vs
#include fluid.vs
-- extrapolate.fs
#include fluid.fs.header

uniform float gradientScale;
uniform samplerTex velocityBuffer;
#ifndef IGNORE_OBSTACLES
uniform samplerTex obstaclesBuffer;
#endif
#ifdef IS_LIQUID
uniform samplerTex levelSetBuffer;
#endif
out vecTex output;

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
        output = vecTex(0);
        return;
    }
#endif
#ifdef IS_LIQUID
    if( !IS_OUTSIDE_SIMULATION(ipos) ) {
        output = toVecTex(texelFetch(velocityBuffer,ipos,0));
        return;
    }
#endif
    vecTex normalizedGradLS = normalize(gradient(levelSetBuffer, ipos) );
    vecTex backwardsPos = fragCoord() - TIMESTEP * gradientScale * normalizedGradLS;

    output = toVecTex(texture(velocityBuffer, in_inverseViewport*backwardsPos));
}

-- liquid.stream.vs
#include fluid.vs
-- liquid.stream.fs
#include fluid.fs.header

uniform vecTex streamCenter;
uniform float streamRadius;
uniform vec3 streamValue;
uniform bool streamUseValue;
#ifndef IGNORE_OBSTACLES
uniform samplerTex obstaclesBuffer;
#endif

out vec4 output;

void main() {
    ivecTex ipos = ifragCoord();
#ifndef IGNORE_OBSTACLES
    if( IS_CELL_OCCUPIED(ipos) ) discard;
#endif
    float dist = length(fragCoord() - streamCenter);

    if( dist > streamRadius ) discard;

    if( streamUseValue ) output.rgb = streamValue;
    else output.r = (dist - streamRadius);
}

-- liquid.distanceToHeight.vs
#include fluid.vs
-- liquid.distanceToHeight.fs
uniform float liquidHeight;

out float output;

void main() {
     output = gl_FragCoord.y - liquidHeight;
}

-------------------------------------
---------- External forces ----------
-------------------------------------
-- gravity.vs
#include fluid.vs
-- gravity.fs
#include fluid.fs.header

uniform vec4 gravityValue;
#ifndef IGNORE_OBSTACLES
uniform samplerTex obstaclesBuffer;
#endif
#ifdef IS_LIQUID
uniform samplerTex levelSetBuffer;
#endif
out vec4 output;

void main() {
    ivecTex ipos = ifragCoord();
#ifdef IS_LIQUID
    if( IS_OUTSIDE_SIMULATION(ipos) ) discard;
#endif
#ifndef IGNORE_OBSTACLES
    if( IS_CELL_OCCUPIED(ipos) ) discard;
#endif
    output = TIMESTEP * gravityValue;
}

-- splat.circle.vs
#include fluid.vs
-- splat.circle.fs
#include fluid.fs.header

#ifndef IGNORE_OBSTACLES
uniform samplerTex obstaclesBuffer;
#endif

uniform float splatRadius;
uniform vec4 splatValue;
uniform vecTex splatPoint;

out vec4 output;

#define AA_PIXELS 2.0
#define USE_AA

void main() {
    ivecTex ipos = ifragCoord();
#ifndef IGNORE_OBSTACLES
    if( IS_CELL_OCCUPIED(ipos) ) discard;
#endif

    float dist = distance(toVecTex(splatPoint), fragCoord());
    if (dist > splatRadius) {
        output = vec4(0);
    } else {
#ifdef USE_AA
        float threshold = splatRadius - AA_PIXELS;
        if(dist<threshold) {
            output = splatValue;
        } else {
            // anti aliasing
            output = splatValue * (1.0f - (dist-threshold)/(splatRadius-threshold));
        }
#else
        output = splatValue;
#endif
    }
}

-- splat.border.vs
#include fluid.vs
-- splat.border.fs
#include fluid.fs.header

uniform float splatBorder;
uniform vec4 splatValue;
#ifndef IGNORE_OBSTACLES
uniform samplerTex obstaclesBuffer;
#endif

out vec4 output;

void main() {
    ivecTex ipos = ifragCoord();

#ifndef IGNORE_OBSTACLES
    if( IS_CELL_OCCUPIED(ipos) ) discard;
#endif

    vecTex splatBorderNormalized = splatBorder*in_inverseViewport;
    if(in_texco.x < splatBorderNormalized.x
       || in_texco.x > 1.0-splatBorderNormalized.x
       || in_texco.y < splatBorderNormalized.y
       || in_texco.y > 1.0-splatBorderNormalized.y
#ifdef IS_VOLUME
       || in_texco.z < splatBorderNormalized.z
       || in_texco.z > 1.0-splatBorderNormalized.z
#endif
    ){
        output = splatValue;
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

uniform vecTex splatSize;
uniform vec4 splatValue;
uniform vecTex splatPoint;
#ifndef IGNORE_OBSTACLES
uniform samplerTex obstaclesBuffer;
#endif

out vec4 output;

void main() {
    ivecTex ipos = ifragCoord();
#ifndef IGNORE_OBSTACLES
    if( IS_CELL_OCCUPIED(ipos) ) discard;
#endif

    vecTex pos = fragCoord();
    if (abs(pos.x-splatPoint.x) > 0.5*splatSize.x) discard;
    if (abs(pos.y-splatPoint.y) > 0.5*splatSize.y) discard;
#ifdef IS_VOLUME
    if (abs(pos.z-splatPoint.z) > 0.5*splatSize.z) discard;
#endif
    output = splatValue;
}

-- splat.tex.vs
#include fluid.vs
-- splat.tex.fs
#include fluid.fs.header

uniform samplerTex splatTexture;
#ifndef IGNORE_OBSTACLES
uniform samplerTex obstaclesBuffer;
#endif
uniform float texelFactor;

out vec4 output;

void main() {
    ivecTex ipos = ifragCoord();
#ifndef IGNORE_OBSTACLES
    if( IS_CELL_OCCUPIED(ipos) ) discard;
#endif
    vec4 val = texture(splatTexture, vec2(in_texco.x,-in_texco.y));
    if (val.a <= 0.00001) discard;
    output = texelFactor*val;
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

out float output;

void main() {
    ivecTex ipos = ifragCoord();
#ifndef IGNORE_OBSTACLES
    if( IS_CELL_OCCUPIED(ipos) ) discard;
#endif
    vec4 val = texture(splatTexture, vec2(in_texco.x,-in_texco.y));
    if (val.a <= 0.00001) discard;
    output = texelFactor*(val.r+val.g+val.b)/3.0;
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
uniform samplerTex quantity;

out vec4 output;

void main() {
    output = texture(quantity, in_texco);
}

-- sample.coarseToFine.vs
#include fluid.vs
-- sample.coarseToFine.fs
#include fluid.fs.header

uniform samplerTex quantityCoarse;
uniform samplerTex quantityCoarse0;
vecTex quantitySizeCoarse;

uniform samplerTex quantityFine0;
vecTex quantitySizeFine;

uniform float alpha;

out vec4 output;

void main() {
    vec4 valCoarse = texture(quantityCoarse, in_texco);
    vec4 valCoarse0 = texture(quantityCoarse0, in_texco);
    vec4 valFine0 = texture(quantityFine0, in_texco);
    output = alpha*valCoarse +
        (1.0-alpha)*(valFine0 + valCoarse - valCoarse0);
}

-- sample.removeLines.vs
#include fluid.vs
-- sample.removeLines.fs
#include fluid.fs.header

uniform samplerTex quantity;
uniform vec4 removeColor;

out vec4 output;

void main() {
    ivecTex pos = ifragCoord();
    vec4 col = texelFetchOffset(quantity, pos, 0, ivec2(0,0));
    int count = 0;
    if(distance(texelFetchOffset(quantity, pos, 0, ivec2(0,1)).rgb,col.rgb)<0.0001) {
        count += 1;
    }
    if(distance(texelFetchOffset(quantity, pos, 0, ivec2(0,-1)).rgb,col.rgb)<0.0001) {
        count += 1;
    }
    if(distance(texelFetchOffset(quantity, pos, 0, ivec2(1,0)).rgb,col.rgb)<0.0001) {
        count += 1;
    }
    if(distance(texelFetchOffset(quantity, pos, 0, ivec2(-1,0)).rgb,col.rgb)<0.0001) {
        count += 1;
    }
    if(count>2) {
        output = col;
    } else {
        output = removeColor;
    }
}

-- sample.smooth.vs
#include fluid.vs
-- sample.smooth.fs
#include fluid.fs.header

uniform samplerTex quantity;

out vec4 output;

void main() {
    ivecTex pos = ifragCoord();
    output = texelFetchOffset(quantity, pos, 0, ivec2( 0,  1));
    output += texelFetchOffset(quantity, pos, 0, ivec2( 0, -1));
    output += texelFetchOffset(quantity, pos, 0, ivec2( 1,  0));
    output += texelFetchOffset(quantity, pos, 0, ivec2(-1,  0));
    output /= 4.0;
}

-- sample.scalar.vs
#include fluid.vs
-- sample.scalar.fs
#include fluid.fs.header

uniform samplerTex quantity;
uniform float texelFactor;
uniform vec3 colorPositive;
uniform vec3 colorNegative;

out vec4 output;

void main() {
    float x = texelFactor*texture(quantity,in_texco).r;
    if(x>0.0) {
        output = vec4(colorPositive, x);
    } else {
        output = vec4(colorNegative, -x);
    }
}

-- sample.levelSet.vs
#include fluid.vs
-- sample.levelSet.fs
#include fluid.fs.header

uniform samplerTex quantity;
uniform float texelFactor;
uniform vec3 colorPositive;
uniform vec3 colorNegative;

out vec4 output;

void main() {
    float x = texelFactor*texture(quantity,in_texco).r;
    if(x>0.0) {
        output = vec4(colorNegative, 1.0f);
    } else {
        output = vec4(colorPositive, 0.0f);
    }
}

-- sample.rgb.vs
#include fluid.vs
-- sample.rgb.fs
#include fluid.fs.header

uniform samplerTex quantity;
uniform float texelFactor;

out vec4 output;

void main() {
    output.rgb = texelFactor*texture(quantity,in_texco).rgb;
    output.a = min(1.0, length(output.rgb));
}

-- sample.velocity.vs
#include fluid.vs
-- sample.velocity.fs
#include fluid.fs.header

uniform samplerTex quantity;
uniform float texelFactor;

out vec4 output;

void main() {
    output.rgb = texelFactor*texture(quantity,in_texco).rgb;
    output.a = min(1.0, length(output.rgb));
    output.b = 0.0;
    if(output.r < 0.0) output.b += -0.5*output.r;
    if(output.g < 0.0) output.b += -0.5*output.g;
}

-- sample.fire.vs
#include fluid.vs
-- sample.fire.fs
#include fluid.fs.header

uniform samplerTex quantity;
uniform sampler1D pattern;
uniform float texelFactor;
uniform float fireAlphaMultiplier;
uniform float fireWeight;
uniform float smokeColorMultiplier;
uniform float smokeAlphaMultiplier;
uniform int rednessFactor;
uniform vec3 smokeColor;

out vec4 output;

void main() {
    const float threshold = 1.4;
    const float maxValue = 5;

    float s = texture(quantity,in_texco).r * texelFactor;
    s = clamp(s,0,maxValue);

    if( s > threshold ) { //render fire
        float lookUpVal = ( (s-threshold)/(maxValue-threshold) );
        lookUpVal = 1.0 - pow(lookUpVal, rednessFactor);
        lookUpVal = clamp(lookUpVal,0,1);
        vec3 interpColor = texture(pattern, (1.0-lookUpVal)).rgb;
        vec4 tmp = vec4(interpColor,1);
        float mult = (s-threshold);
        output.rgb = fireWeight*tmp.rgb;
        output.a = min(1.0, fireWeight*mult*mult*fireAlphaMultiplier + 0.5);
    } else { // render smoke
        output.rgb = vec3(fireWeight*s);
        output.a = min(1.0, output.r*smokeAlphaMultiplier);
        output.rgb = output.a * output.rrr * smokeColor * smokeColorMultiplier;
    }
}

