
-- vs
in vec3 v_pos;
#ifndef IS_2D_SIMULATION
out vec4 g_pos;
out int g_instanceID;
#endif

void main()
{
    // we assume here that v_pos is a vertex position
    // of a unit quad
    // projectionUnitMatrixOrtho * vec4(v_pos,1.0);
    // TODO: optimize calculation below
    vec4 pos_ = vec4(2.0*v_pos.xy, 0.0, 1.0-v_pos.x-v_pos.y);
#ifndef IS_2D_SIMULATION
    g_pos = pos_;
    g_instanceID = gl_InstanceID;
#endif
    gl_Position = pos_;
}

-- gs
in vec4 g_pos[3];
in int g_instanceID[3];
out float f_layer;

void main()
{
    gl_Layer = g_instanceID[0];
    f_layer = float(g_instanceID[0]) + 0.5;
    gl_Position = g_pos[0]; EmitVertex();
    gl_Position = g_pos[1]; EmitVertex();
    gl_Position = g_pos[2]; EmitVertex();
    EndPrimitive();
}

-- fs.header
#define NORTH 0
#define SOUTH 1
#define EAST 2
#define WEST 3
#define FRONT 4
#define BACK 5

#ifdef IS_2D_SIMULATION
    #define vecFluid vec2
    #define ivecFluid ivec2
    #define toVecFluid(v) v.xy
    #define samplerFluid sampler2D
    #define fragCoord() gl_FragCoord.xy
    #define ifragCoord() ivec2(fragCoord())
#else
    #define vecFluid vec3
    #define ivecFluid ivec3
    #define toVecFluid(v) v.xyz
    #define samplerFluid sampler3D
    #define fragCoord() vec3(gl_FragCoord.xy,f_layer)
    #define ifragCoord() ivec3(fragCoord())
#endif
#define fragCoordNormalized() inverseGridSize*fragCoord()

uniform vecFluid inverseGridSize;
#ifndef IS_2D_SIMULATION
in float f_layer;
#endif

-------------------------------------
------------ Advection --------------
-------------------------------------

-- advect
uniform float quantityLoss;
uniform float decayAmount;
uniform samplerFluid velocityBuffer;
#ifdef USE_OBSTACLES
uniform samplerFluid obstaclesBuffer;
#endif
#ifdef IS_LIQUID
uniform int treatAsLiquid;
uniform samplerFluid levelSetBuffer;
#endif
uniform samplerFluid quantityBuffer;

out vec4 output;

#include fluid.isOutsideSimulationDomain
#include fluid.isNonEmptyCell

void main() {
    vecFluid pos0 = fragCoordNormalized();

#ifdef IS_LIQUID
    if( treatAsLiquid==1 && isOutsideSimulationDomain(pos0) ) {
        output = texture(velocityBuffer, pos0);
        return;
    }
#endif
#ifdef USE_OBSTACLES
    if( isNonEmptyCell(pos0) ) {
        output = vec4(0);
        return;
    }
#endif

    // follow the velocity field 'back in time'
    vecFluid pos1 = fragCoord() - toVecFluid(TIMESTEP*texture(velocityBuffer, pos0));

    vec4 output_ = decayAmount * texture(quantityBuffer, inverseGridSize*pos1);
    if(quantityLoss>0.0) {
        output_ -= quantityLoss*TIMESTEP;
    }
    output = output_;
}

-- advectMacCormack
uniform float quantityLoss;
uniform float decayAmount;
uniform samplerFluid velocityBuffer;
#ifdef USE_OBSTACLES
uniform samplerFluid obstaclesBuffer;
#endif
#ifdef IS_LIQUID
uniform int treatAsLiquid;
uniform samplerFluid levelSetBuffer;
#endif
uniform samplerFluid quantityBuffer;
uniform samplerFluid quantityBufferHat;

out vec4 output;

#include fluid.isOutsideSimulationDomain
#include fluid.isNonEmptyCell

void main() {
    vecFluid pos0 = fragCoordNormalized();

#ifdef IS_LIQUID
    if( treatAsLiquid==1 && isOutsideSimulationDomain(pos0) ) {
        output = vec4(0);
        return;
    }
#endif
#ifdef USE_OBSTACLES
    if( isNonEmptyCell(pos0) ) {
        output = vec4(0);
        return;
    }
#endif

    // follow the velocity field 'back in time'
    vecFluid pos1 = toVecFluid((fragCoord() - TIMESTEP*texture(velocityBuffer, pos0));
    // convert new position to texture coordinates
    vecFluid nposTC = inverseGridSize * pos1;
    // find the texel corner closest to the semi-Lagrangian 'particle'
    vecFluid nposTexel = floor( pos1 + vecFluid( 0.5f ) );
    vecFluid nposTexelTC = inverseGridSize*nposTexel;
    // ht (half-texel)
    vecFluid ht = 0.5*inverseGridSize;


    // get the values of nodes that contribute to the interpolated value
    // (texel centers are at half-integer locations)
#ifdef IS_2D_SIMULATION
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
    vec4 output_ = texture( quantityTexture, nposTC, 0) + 0.5*(
        texture( quantityTexture, fragCoord ) -
        texture( quantityTextureHat, fragCoord ) );

    // clamp result to the desired range
    output_ = max( min( outCol, phiMax ), phiMin ) * decayAmount;
    if(quantityLoss>0.0) {
        output_ -= quantityLoss*TIMESTEP;
    }
    output = output_;
}

-------------------------------------
------------ Pressure Solve ---------
-------------------------------------

-- pressure
uniform float alpha;
uniform float inverseBeta;
uniform samplerFluid pressureBuffer;
uniform samplerFluid divergenceBuffer;
#ifdef USE_OBSTACLES
uniform samplerFluid obstaclesBuffer;
#endif
#ifdef IS_LIQUID
uniform samplerFluid levelSetBuffer;
#endif

out vec4 output;

#include fluid.isOutsideSimulationDomain
#include fluid.fetchNeighbors

void main() {
    ivecFluid pos = ifragCoord();
#ifdef IS_LIQUID
    // air pressure
    if( isOutsideSimulationDomain(inverseGridSize*gl_FragCoord.xy) ) {
        output = vec4(0.0,0.0,0.0,1.0);
        return;
    }
#endif

    vec4 p[] = fetchNeighbors(pressureBuffer, pos);
#ifdef USE_OBSTACLES
    // Make sure that the pressure in solid cells is effectively ignored.
    vec4 pC = texelFetch(pressureBuffer, pos, 0);
    vec4 o[] = fetchNeighbors(obstaclesBuffer, pos);
    if (o[NORTH].x > 0) p[NORTH] = pC;
    if (o[SOUTH].x > 0) p[SOUTH] = pC;
    if (o[EAST].x > 0) p[EAST] = pC;
    if (o[WEST].x > 0) p[WEST] = pC;
#ifndef IS_2D_SIMULATION
    if (o[FRONT].x > 0) p[FRONT] = pC;
    if (o[BACK].x > 0) p[BACK] = pC;
#endif
#endif
    
    vec4 dC = texelFetch(divergenceBuffer, pos, 0);
#ifdef IS_2D_SIMULATION
    output = (p[WEST] + p[EAST] +
        p[SOUTH] + p[NORTH] - alpha * dC) * inverseBeta;
#else
    output = (p[WEST] + p[EAST] +
        p[SOUTH] + p[NORTH] +
        p[FRONT] + p[BACK] - alpha * dC) * inverseBeta;
#endif
}

-- substractGradient
uniform float densityInverse;
uniform samplerFluid velocityBuffer;
uniform samplerFluid pressureBuffer;
#ifdef USE_OBSTACLES
uniform samplerFluid obstaclesBuffer;
#endif
#ifdef IS_LIQUID
uniform samplerFluid levelSetBuffer;
#endif

out vecFluid output;

#include fluid.isOutsideSimulationDomain
#include fluid.fetchNeighbors

void main() {
#ifdef IS_LIQUID
    if( isOutsideSimulationDomain(fragCoordNormalized()) ) {
        output = vec2(0);
        return;
    }
#endif
    
    ivecFluid pos = ifragCoord();
#ifdef USE_OBSTACLES
    vec4 oC = texelFetch(obstaclesBuffer, pos, 0);
    if (oC.x > 0) {
        output = toVecFluid(oC);
        return;
    }
#endif
    
    vec4 oldV = texelFetch(velocityBuffer, pos, 0);

    vec4 p[] = fetchNeighbors(pressureBuffer, pos);
    vec2 obstV = vec2(0);
    vec2 vMask = vec2(1);
    
#ifdef USE_OBSTACLES
    // Use center pressure for solid cells
    float pC = texelFetch(pressureBuffer, pos, 0).r;
    vec4 o[] = fetchNeighbors(obstaclesBuffer, pos);
    if (o[NORTH].x > 0) { p[NORTH].x = pC; obstV.y = o[NORTH].z; }
    if (o[SOUTH].x > 0) { p[SOUTH].x = pC; obstV.y = o[SOUTH].z; }
    if (o[EAST].x > 0) { p[EAST].x = pC; obstV.x = o[EAST].y; }
    if (o[WEST].x > 0) { p[WEST].x = pC; obstV.x = o[WEST].y; }
#ifndef IS_2D_SIMULATION
    if (o[FRONT].x > 0) { p[FRONT].x = pC; obstV.z = 0.0f; vMask.z = 0; }
    if (o[BACK].x > 0) { p[BACK].x = pC; obstV.z = 0.0f; vMask.z = 0; }
#endif
#endif

#ifdef IS_2D_SIMULATION
    vec2 v = oldV.xy - vec2(p[EAST].x - p[WEST].x, p[NORTH].x - p[SOUTH].x) * 0.5f * densityInverse;
#else
    vec3 v = oldV.xyz - vec3(p[EAST].x - p[WEST].x, p[NORTH].x - p[SOUTH].x, p[FRONT].x - p[BACK].x) * 0.5f * densityInverse;
#endif

    // there are some artifacts with fluid sticking to obstacles with that code...
    //if ( (oN.x > 0 && v.y > 0.0) || (oS.x > 0 && v.y < 0.0) ) { vMask.y = 0; }
    //if ( (oE.x > 0 && v.x > 0.0) || (oW.x > 0 && v.x < 0.0) ) { vMask.x = 0; }

    output = ((vMask * v) + obstV);
}

-- divergence
uniform float halfInverseCellSize;
uniform samplerFluid velocityBuffer;
#ifdef USE_OBSTACLES
uniform samplerFluid obstaclesBuffer;
#endif

out float output;

#include fluid.fetchNeighbors

void main() {
    ivecFluid pos = ifragCoord();

    vec4 velocity[] = fetchNeighbors(velocityBuffer, pos);

#ifdef USE_OBSTACLES
    // Use obstacle velocities for solid cells
    vec4 obstacle[] = fetchNeighbors(obstaclesBuffer, pos);
#ifdef IS_2D_SIMULATION
    if (obstacle[NORTH].x > 0) velocity[NORTH].xy = obstacle[NORTH].yz;
    if (obstacle[SOUTH].x > 0) velocity[SOUTH].xy = obstacle[SOUTH].yz;
    if (obstacle[EAST].x > 0)  velocity[EAST].xy  = obstacle[EAST].yz;
    if (obstacle[WEST].x > 0)  velocity[WEST].xy  = obstacle[WEST].yz;
#else
    if (obstacle[NORTH].x > 0) velocity[NORTH].xyz = obstacle[NORTH].yzw;
    if (obstacle[SOUTH].x > 0) velocity[SOUTH].xyz = obstacle[SOUTH].yzw;
    if (obstacle[EAST].x > 0)  velocity[EAST].xyz  = obstacle[EAST].yzw;
    if (obstacle[WEST].x > 0)  velocity[WEST].xyz  = obstacle[WEST].yzw;
    if (obstacle[FRONT].x > 0) velocity[FRONT].xyz = obstacle[FRONT].yzw;
    if (obstacle[BACK].x > 0)  velocity[BACK].xyz  = obstacle[BACK].yzw;
#endif
#endif

#ifdef IS_2D_SIMULATION
    output = halfInverseCellSize * (
                velocity[EAST].x - velocity[WEST].x +
                velocity[NORTH].y - velocity[SOUTH].y);
#else
    outCol = halfInverseCellSize * (
            velocity[EAST].x - velocity[WEST].x +
            velocity[NORTH].y - velocity[SOUTH].y +
            velocity[FRONT].z - velocity[BACK].z);
#endif
}

-------------------------------------
---------- External forces ----------
-------------------------------------

-- gravity
uniform vec4 gravityValue;
#ifdef USE_OBSTACLES
uniform samplerFluid obstaclesBuffer;
#endif
#ifdef IS_LIQUID
uniform samplerFluid levelSetBuffer;
#endif
out vec4 output;

#include fluid.isOutsideSimulationDomain
#include fluid.isNonEmptyCell

void main() {
    vecFluid pos = fragCoordNormalized();
#ifdef IS_LIQUID
    if( isOutsideSimulationDomain(pos) ) discard;
#endif
#ifdef USE_OBSTACLES
    if( isNonEmptyCell(pos) ) discard;
#endif
    output = TIMESTEP * gravityValue;
}

-- splat.circle
uniform float splatRadius;
uniform vec3 splatValue;
uniform vecFluid splatPoint;
#ifdef USE_OBSTACLES
uniform samplerFluid obstaclesBuffer;
#endif

out vec4 output;

#include fluid.isNonEmptyCell

void main() {
#ifdef USE_OBSTACLES
    if( isNonEmptyCell(fragCoordNormalized()) ) discard;
#endif

    float dist = distance(splatPoint, fragCoord());
    if (dist > splatRadius) discard;
    output = vec4(TIMESTEP * splatValue, exp( -dist*dist ) );
}

-- splat.rect
uniform vecFluid splatSize;
uniform vec3 splatValue;
uniform vecFluid splatPoint;
#ifdef USE_OBSTACLES
uniform samplerFluid obstaclesBuffer;
#endif

out vec4 output;

#include fluid.isNonEmptyCell

void main() {
#ifdef USE_OBSTACLES
    if( isNonEmptyCell(fragCoordNormalized()) ) discard;
#endif

    vecFluid pos = fragCoord();
    if (abs(pos.x-splatPoint.x) > 0.5*splatSize.x) discard;
    if (abs(pos.y-splatPoint.y) > 0.5*splatSize.y) discard;
#ifndef IS_2D_SIMULATION
    if (abs(pos.z-splatPoint.z) > 0.5*splatSize.z) discard;
#endif
    output = vec4(TIMESTEP * splatValue, 1.0f );
}

-- splat.tex
uniform samplerFluid splatTexture;
#ifdef USE_OBSTACLES
uniform samplerFluid obstaclesBuffer;
#endif
uniform float texelFactor;

out vec4 output;

#include fluid.isNonEmptyCell

void main() {
    vecFluid pos = fragCoordNormalized();
#ifdef USE_OBSTACLES
    if( isNonEmptyCell(pos) ) discard;
#endif
    vec4 val = texture(splatTexture, pos);
    if (val.a <= 0.00001) discard;
    output = texelFactor*val;
}

-------------------------------------
---------- Vorticity ----------------
-------------------------------------

-- vorticity.compute
uniform samplerFluid velocityBuffer;
out vecFluid output;

#include fluid.fetchNeighbors

void main() {
    ivecFluid pos = ifragCoord();
    vec4 v[] = fetchNeighbors(velocityBuffer, pos);
    // using central differences: D0_x = (D+_x - D-_x) / 2;
#ifdef IS_2D_SIMULATION
    output = 0.5*vec2(v[NORTH].y - v[SOUTH].y, v[EAST].x - v[WEST].x);
#else
    output = 0.5*vec3(
        (( v[NORTH].z - v[SOUTH].z ) - ( v[FRONT].y - v[BACK].y )),
        (( v[FRONT].x - v[BACK].x ) - ( v[EAST].z - v[WEST].z )),
        (( v[EAST].y - v[WEST].y ) - ( v[NORTH].x - v[SOUTH].x )));
#endif
}

-- vorticity.confinement
uniform samplerFluid vorticityBuffer;
#ifdef USE_OBSTACLES
uniform samplerFluid obstaclesBuffer;
#endif
uniform float confinementScale;

out vecFluid output;

#include fluid.fetchNeighbors

void main() {
    ivecFluid pos = ifragCoord();
#ifdef USE_OBSTACLES
    // discard obstacle fragments
    if (texelFetch(obstaclesBuffer, pos, 0).x > 0) discard;
#endif

    vec4 omegaC = texelFetch(vorticityBuffer, pos, 0);
    vec4 omega[] = fetchNeighbors(vorticityBuffer, pos);

#ifdef IS_2D_SIMULATION
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
------- Other Stages ----------------
-------------------------------------

-- buoyancy
uniform samplerFluid temperatureBuffer;
uniform float ambientTemperature;
uniform vecFluid buoyancy;

out vecFluid output;

void main() {
    float deltaTemperature = texelFetch(temperatureBuffer, ifragCoord(), 0).r - ambientTemperature;
    output = (TIMESTEP * deltaTemperature) * buoyancy;
}

-- diffusion
uniform samplerFluid currentBuffer;
uniform samplerFluid initialBuffer;
uniform float viscosity;

out vec4 output;

#include fluid.fetchNeighbors

void main() {
    ivecFluid pos = ifragCoord();
    
    vec4 phin[] = fetchNeighbors(currentBuffer, pos);
#ifdef IS_2D_SIMULATION
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

-- extrapolate
uniform samplerFluid velocityBuffer;
#ifdef USE_OBSTACLES
uniform samplerFluid obstaclesBuffer;
#endif
#ifdef IS_LIQUID
uniform samplerFluid levelSetBuffer;
#endif
out vec4 output;

#include fluid.isOutsideSimulationDomain
#include fluid.isNonEmptyCell
#include fluid.fetchNeighbors

vecFluid gradient(samplerFluid tex, ivecFluid pos)
{
    vec4 gradient[] = fetchNeighbors(tex,pos);
    return 0.5f * vecFluid(
        gradient[EAST].x - gradient[WEST].x,
        gradient[NORTH].x - gradient[SOUTH].x
#ifdef IS_2D_SIMULATION
    );
#else
        gradient[FRONT].x - gradient[BACK].x
    );
#endif
}

void main() {
    // FIXME: stupid factor
    const float DUMP_FACTOR = 61.25;
    //const float DUMP_FACTOR = 50.0 * 0.25;

    vecFluid pos = fragCoordNormalized();
#ifdef USE_OBSTACLES
    if( isNonEmptyCell(pos) ) {
        output = vec4(0);
        return;
    }
#endif
#ifdef IS_LIQUID
    if( !isOutsideSimulationDomain(pos) ) {
        output = texture(velocityBuffer, pos);
        return;
    }
#endif
    ivecFluid ipos = ifragCoord();
    vecFluid normalizedGradLS = normalize(gradient(levelSetBuffer, ipos) );
    vecFluid backwardsPos = fragCoord() - normalizedGradLS * TIMESTEP * DUMP_FACTOR;

    output = texture(velocityBuffer, inverseGridSize*backwardsPos);
}

-------------------------------------
---------- Liquids ------------------
-------------------------------------

-- liquid.redistance
uniform samplerFluid levelSetBuffer;
uniform samplerFluid initialLevelSetBuffer;
out float output;

#include fluid.gradient.liquid

void main() {
    vecFluid pos = fragCoordNormalized();
    float phiC = texture( levelSetTexture, pos, 0).r;
    //const float dt = 0.1f;
    const float dt = 0.1f;
    const float dt2 = 0.3f;

    // avoid redistancing near boundaries, where gradients are ill-defined
    if( (fragCoord().x < 3) ||
        (fragCoord().x > (1.0/inverseGridSize.x-4)) )
    {
        output = phiC;
        return;
    }
    if( (gl_FragCoord.y < 3) ||
        (gl_FragCoord.y > (1.0/inverseGridSize.y-4)) )
    {
        output = phiC;
        return;
    }
#ifndef IS_2D_SIMULATION
    if( (fragCoord().z < 3) ||
        (fragCoord().z > (1.0/inverseGridSize.z-4)) )
    {
        output = phiC;
        return;
    };
#endif

    bool isBoundary;
    bool hasHighSlope;
    vec2 gradPhi = gradientLiquid(levelSetBuffer, ivec2(gl_FragCoord.xy),
            1.01f, isBoundary, hasHighSlope);
    float normGradPhi = length(gradPhi);
    if( isBoundary || !hasHighSlope || ( normGradPhi < 0.01f ) ) {
        outCol = phiC;
        return;
    }

    float phiC0 = texture( initialLevelSetBuffer, fragCoord ).r;
    float signPhi = phiC0 / sqrt( (phiC0*phiC0) + 1);
    vec2 backwardsPos = gl_FragCoord.xy - (gradPhi.xy/normGradPhi) * signPhi * TIMESTEP * 50.0 * 0.05 ;
    vec2 npos = inverseGridSize*vec2(backwardsPos.x, backwardsPos.y);
    outCol = texture( levelSetBuffer, npos ).r + (signPhi * TIMESTEP * 50.0 * 0.075);
}

-- liquid.stream
uniform vecFluid streamCenter;
uniform float streamRadius;
uniform vec3 streamValue;
uniform bool streamUseValue;
#ifdef USE_OBSTACLES
uniform samplerFluid obstaclesBuffer;
#endif

out vec4 output;

#include fluid.isNonEmptyCell

void main() {
#ifdef USE_OBSTACLES
    if( isNonEmptyCell(fragCoordNormalized()) ) discard;
#endif
    float dist = length(fragCoord() - streamCenter);

    if( dist > streamRadius ) discard;

    if( streamUseValue ) output.rgb = streamValue;
    else output.r = (dist - streamRadius);
}

-- liquid.distanceToHeight
uniform float liquidHeight;

float liquidHeightDistance()
{
     return gl_FragCoord.y - liquidHeight;
}


-------------------------------------
---------- Helper functions ---------
-------------------------------------

-- isOutsideSimulationDomain
#ifdef IS_LIQUID
bool isOutsideSimulationDomain(vecFluid pos) {
    return texture(levelSetBuffer, pos).r > 0.0;
}
#endif
-- isNonEmptyCell
#ifdef USE_OBSTACLES
bool isNonEmptyCell(vecFluid pos) {
    return texture(obstaclesBuffer, pos).r > 0;
}
#endif

-- fetchNeighbors
#ifndef fetchNeighbors_INCLUDED
#define fetchNeighbors_INCLUDED
#ifdef IS_2D_SIMULATION
vec4[4] fetchNeighbors(samplerFluid tex, ivecFluid pos) {
    vec4[4] neighbors;
    neighbors[NORTH] = texelFetchOffset(tex, pos, 0, ivec2( 0,  1));
    neighbors[SOUTH] = texelFetchOffset(tex, pos, 0, ivec2( 0, -1));
    neighbors[EAST]  = texelFetchOffset(tex, pos, 0, ivec2( 1,  0));
    neighbors[WEST]  = texelFetchOffset(tex, pos, 0, ivec2(-1,  0));
    return neighbors;
}
#else
vec4[6] fetchNeighbors(samplerFluid tex, ivecFluid pos) {
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

-- liquid.gradient
#ifndef liquid.gradient_INCLUDED
#define liquid.gradient_INCLUDED
vecFluid liquidGradient(
        samplerFluid tex,
        ivecFluid pos,
        float minSlope,
        out bool isBoundary,
        out bool highEnoughSlope)
{
#ifdef IS_2D_SIMULATION
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
#endif // liquid.gradient_INCLUDED


-------------------------------------
---------- Visualizing --------------
-------------------------------------

-- visualize.scalar
uniform samplerFluid quantity;
uniform float texelFactor;
uniform vec3 colorPositive;
uniform vec3 colorNegative;

out vec4 output;

void main() {
    float x = texelFactor*texture(quantity, fragCoordNormalized()).r;
    if(x>0.0) {
        output = vec4(colorPositive, x);
    } else {
        output = vec4(colorNegative, -x);
    }
}

-- visualize.rgb
uniform samplerFluid quantity;
uniform float texelFactor;

out vec4 output;

void main() {
    output.rgb = texelFactor*texture(quantity, fragCoordNormalized()).rgb;
    output.a = min(1.0, length(output.rgb));
    output.b = 0.0;
    if(output.r < 0.0) output.b += -0.5*output.r;
    if(output.g < 0.0) output.b += -0.5*output.g;
}

-- visualize.levelSet
uniform samplerFluid quantity;
uniform float texelFactor;

out vec4 output;

void main() {
    float levelSet = texelFactor*texture(quantity, fragCoordNormalized()).r;
    if( levelSet < 0 ) {
        output = vec4(0.0f, 0.0f, 1.0f, 1.0f);
    } else {
        output = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    }
}

-- visualize.fire
uniform samplerFluid quantity;
uniform sampler2D pattern;
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

    float s = texture(quantity, fragCoordNormalized()).r * texelFactor;
    s = clamp(s,0,maxValue);

    if( s > threshold ) { //render fire
        float lookUpVal = ( (s-threshold)/(maxValue-threshold) );
        lookUpVal = 1.0 - pow(lookUpVal, rednessFactor);
        lookUpVal = clamp(lookUpVal,0,1);
        vec3 interpColor = texture(pattern, vec2(1.0-lookUpVal,0)).rgb;
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


