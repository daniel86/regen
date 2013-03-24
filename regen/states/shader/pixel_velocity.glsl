/**
 * Supposed to calculate velocity using vertex data
 * captured with transform feedback.
 **/

-- vs
out vec3 out_pos0;
out vec3 out_pos1;

#ifdef WORLD_SPACE
in vec4 in_posWorld;
in vec4 in_lastPosWorld;
#elif EYE_SPACE
in vec4 in_posEye;
in vec4 in_lastPosEye;
#else
in vec4 in_Position;
in vec4 in_lastPosition;
#endif

#ifdef WORLD_SPACE
uniform mat4 in_viewProjectionMatrix;
#else
#ifdef EYE_SPACE
uniform mat4 in_projectionMatrix;
#endif

void main() {
#ifdef WORLD_SPACE
    out_pos0 = in_posWorld.xyz;
    out_pos1 = in_lastPosWorld.xyz;
    gl_Position = in_viewProjectionMatrix * in_posWorld;
#else
#ifdef EYE_SPACE
    out_pos0 = in_posEye.xyz;
    out_pos1 = in_lastPosEye.xyz;
    gl_Position = in_projectionMatrix * in_posEye;
#else
    out_pos0 = in_Position.xyz;
    out_pos1 = in_lastPosition.xyz;
    gl_Position = in_Position;
#endif
}

-- fs
out float out_color;

in vec3 in_pos0;
in vec3 in_pos1;

uniform vec2 in_viewport;
uniform float in_deltaT;
#ifdef USE_DEPTH_TEST
uniform sampler2D in_depthTexture;
#endif

#ifndef DEPTH_BIAS
// bias used for scene depth buffer access
#define DEPTH_BIAS 0.1
#endif

void main() {
#ifdef USE_DEPTH_TEST
    // use the fragment coordinate to find the texture coordinates of
    // this fragment in the scene depth buffer.
    // gl_FragCoord.xy is in window space, divided by the buffer size
    // we get the range [0,1] that can be used for texture access.
    vec2 depthTexco = gl_FragCoord.xy/in_viewport.xy;
    // depth at this pixel obtained in main pass.
    // this is non linear depth in the range [0,1].
    float sceneDepth = texture(in_depthTexture, depthTexco).r;
    // do the depth test against gl_FragCoord.z using a bias.
    // bias is used to avoid flickering
    if( gl_FragCoord.z > sceneDepth+DEPTH_BIAS ) { discard; };
#endif
    out_color = length( (in_pos0 - in_pos1)/in_deltaT );
}

