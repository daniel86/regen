--------------------------------------
--------------------------------------
---- Motion blur implementation.
---- Two modes are supported:
----    * Fullscreen-Motion-Blur: computed by view-projection and previous view-projection matrix.
----    * Per-Pixel-Motion-Blur: computed by velocity input texture.
--------------------------------------
--------------------------------------
-- vs
#include regen.filter.sampling.vs
-- gs
#include regen.filter.sampling.gs
-- fs
out vec4 out_color;

uniform sampler2D in_inputTexture;
uniform sampler2D in_depthTexture;

const int in_numMotionBlurSamples = 10;
const float in_velocityScale = 0.25;

#include regen.filter.sampling.computeTexco

#ifndef USE_VELOCITY_TEXTURE
void worldPosFromDepth(float depth, vec2 texco, out vec4 pos0, out vec4 posWorld) {
    pos0 = vec4(texco.x*2 - 1, (1-texco.y)*2 - 1, depth, 1);
    // Transform viewport position by the view-projection inverse.
    vec4 D = in_inverseViewProjectionMatrix*pos0;
    // Divide by w to get the world position.
    posWorld = D/D.w;
}
#endif

void main() {
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    vecTexco texco = computeTexco(texco_2D);
    
    // Get the initial color at this pixel.
    out_color = texture(in_inputTexture, texco);
#ifdef USE_VELOCITY_TEXTURE
    // get velocity from texture
    vec2 velocity = texture(in_velocityTexture, texco).xy;
    // Early exit
    if(length(velocity) == 0.0) { return; }
#else
    float depth = texture(in_depthTexture, texco).x;
    vec4 pos0, posWorld;
    worldPosFromDepth(depth, texco, pos0, posWorld);
    // transform by previous frame view-projection matrix
    vec4 pos1 = in_lastViewProjectionMatrix*vec4(posWorld.xyz,1.0);
    // Convert to nonhomogeneous points [-1,1] by dividing by w.
    pos1 /= pos1.w;
    // Use this frame's position and last frame's to compute the pixel velocity.
    vec2 velocity = (in_velocityScale/in_timeDeltaMS)*(pos0.xy-pos1.xy);
#endif
    vec2 texCoord = texco_2D + velocity;
    for(int i = 1; i < in_numMotionBlurSamples; ++i, texCoord+=velocity) {
      // Sample the color buffer along the velocity vector.
      vec4 currentColor = texture(in_inputTexture, computeTexco(texCoord));
      // Add the current color to our color sum.
      out_color += currentColor;
    }
    // Average all of the samples to get the final blur color.
    out_color /= float(in_numMotionBlurSamples);
}

--------------------------------------
--------------------------------------
---- Computes per pixel velocity by comparing this and last frames
---- vertex positions.
---- Velocity can be computed in world space or eye space.
--------------------------------------
--------------------------------------
-- velocity.vs
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

#include regen.states.camera.input
#include regen.states.camera.transformWorldToScreen
#include regen.states.camera.transformEyeToScreen

void main() {
#ifdef WORLD_SPACE
    out_pos0 = in_posWorld.xyz;
    out_pos1 = in_lastPosWorld.xyz;
    gl_Position = transformWorldToScreen(in_posWorld,0);
#else
#ifdef EYE_SPACE
    out_pos0 = in_posEye.xyz;
    out_pos1 = in_lastPosEye.xyz;
    gl_Position = transformEyeToScreen(in_posWorld,0);
#else
    out_pos0 = in_Position.xyz;
    out_pos1 = in_lastPosition.xyz;
    gl_Position = in_Position;
#endif
}

-- velocity.fs
// bias used for scene depth buffer access
#define DEPTH_BIAS 0.1

out float out_color;
// this and last frame model position
in vec3 in_pos0;
in vec3 in_pos1;

uniform vec2 in_inverseViewport;
uniform float in_timeDeltaMS;
#ifdef USE_DEPTH_TEST
uniform sampler2D in_depthTexture;
#include regen.filter.sampling.computeTexco
#endif

void main() {
#ifdef USE_DEPTH_TEST
    // use the fragment coordinate to find the texture coordinates of
    // this fragment in the scene depth buffer.
    // gl_FragCoord.xy is in window space, divided by the buffer size
    // we get the range [0,1] that can be used for texture access.
    vec2 depthTexco = gl_FragCoord.xy*in_inverseViewport.xy;
    // depth at this pixel obtained in main pass.
    // this is non linear depth in the range [0,1].
    float sceneDepth = texture(in_depthTexture, computeTexco(depthTexco)).r;
    // do the depth test against gl_FragCoord.z using a bias.
    // bias is used to avoid flickering
    if( gl_FragCoord.z > sceneDepth+DEPTH_BIAS ) { discard; };
#endif
    out_color = length( (in_pos0 - in_pos1)/in_timeDeltaMS );
}

