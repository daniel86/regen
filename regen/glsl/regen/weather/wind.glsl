
-- windAtPosition
#ifndef REGEN_windAtPosition_Included_
#define REGEN_windAtPosition_Included_
const float in_windFlowScale = 100.0;
const float in_windFlowTime = 0.2;
#ifdef HAS_windNoise
const float in_windNoiseScale = 100.0;
const float in_windNoiseSpeed = 1.0;
const float in_windNoiseStrength = 0.1;
#endif
const vec2 in_wind = vec2(1.0, 0.0);

uniform sampler2D in_windFlow;

vec2 windAtPosition(vec3 posWorld) {
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
    vec2 wind = (2.0*windSample - vec2(1.0)) * length(in_wind);
#else
    vec2 wind = in_wind;
    //wind.x -= 0.1*sin(in_time * in_windFlowTime * 10.0 + posWorld.x/in_windFlowScale);
    //wind.y -= 0.1*cos(in_time * in_windFlowTime * 10.0 + posWorld.z/in_windFlowScale);
#endif
#ifdef HAS_windNoise
    float scaledTime = in_time*0.01*in_windNoiseSpeed;
    // add some noise to the wind
    float windNoise_x = texture(in_windNoise,
        (posWorld.xz/in_windNoiseScale + scaledTime)).x;
    float windNoise_y = texture(in_windNoise,
        (posWorld.zx/in_windNoiseScale + scaledTime)).x;
    wind.x += (2.0*windNoise_x - 1.0) * in_windNoiseStrength;
    wind.y += (2.0*windNoise_y - 1.0) * in_windNoiseStrength;
#endif
    return wind;
}
#endif // REGEN_windAtPosition_Included_

-- wavingBaseTransfer
#ifndef REGEN_wavingBaseTransfer_Included_
#define REGEN_wavingBaseTransfer_Included_
#define POS_MODEL_TRANSFER_NAME wavingBaseTransfer
#include regen.models.sprite.applyForceBase
#include regen.weather.wind.windAtPosition
void wavingBaseTransfer(inout vec3 posWorld) {
    vec2 wind = windAtPosition(posWorld);
    applyForceBase(posWorld, in_basePos, wind);
}
#endif
