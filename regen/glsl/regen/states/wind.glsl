
-- windAtPosition
#ifndef REGEN_windAtPosition_Included_
#define REGEN_windAtPosition_Included_
const float in_windFlowScale = 100.0;
const float in_windFlowTime = 0.2;

vec2 windAtPosition(vec3 posWorld)
{
#ifdef HAS_windFlow
    vec2 windFlow_uv =
        // map position to "wind flow space"
        posWorld.xz/in_windFlowScale +
        // translate the wind flow with the time in the direction of the wind
        normalize(in_wind) * in_time * in_windFlowTime;
    // wrap around the wind flow texture
    windFlow_uv.x = mod(windFlow_uv.x, 1.0);
    windFlow_uv.y = mod(windFlow_uv.y, 1.0);
    // sample the wind flow texture, scale it to -1,1, and scale it by the wind strength
    vec2 windSample = texture(in_windFlow, windFlow_uv).xy;
    return (2.0*windSample - vec2(1.0)) * length(in_wind);
#else
    return in_wind;
#endif
}
#endif // REGEN_windAtPosition_Included_
