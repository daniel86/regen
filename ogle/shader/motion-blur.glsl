-- vs
in vec3 in_pos;
out vec2 out_texco;

void main()
{
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- fs
out vec4 output;
in vec2 in_texco;

uniform sampler2D in_inputTexture;
uniform sampler2D in_depthTexture;
#ifdef USE_VELOCITY_TEXTURE
uniform sampler2D in_velocityTexture;
#else
uniform mat4 in_lastViewProjectionMatrix;
uniform mat4 in_inverseViewProjectionMatrix;
#endif

uniform float in_deltaT;

const int in_numMotionBlurSamples = 10;
const float in_velocityScale = 0.25;

#ifndef USE_VELOCITY_TEXTURE
void worldPosFromDepth(out vec4 pos0, out vec4 posWorld)
{
    // get the depth value at this pixel
    float depth = texture(in_depthTexture, in_texco).r;
    pos0 = vec4(in_texco.x*2 - 1, (1-in_texco.y)*2 - 1, depth, 1);
    // Transform viewport position by the view-projection inverse.
    vec4 D = in_inverseViewProjectionMatrix*pos0;
    // Divide by w to get the world position.
    posWorld = D/D.w;
}
#endif

void main()
{
    // Get the initial color at this pixel.
    output = texture(in_inputTexture, in_texco);
#ifdef USE_VELOCITY_TEXTURE
    // get velocity from texture
    vec2 velocity = texture(in_velocityTexture, in_texco).xy;
    // Early exit
    if(length(velocity) == 0.0) { return; }
#else
    vec4 pos0, posWorld;
    worldPosFromDepth(pos0, posWorld);
    // transform by previous frame view-projection matrix
    vec4 pos1 = in_lastViewProjectionMatrix*vec4(posWorld.xyz,1.0);
    // Convert to nonhomogeneous points [-1,1] by dividing by w.
    pos1 /= pos1.w;
    // Use this frame's position and last frame's to compute the pixel velocity.
    vec2 velocity = (in_velocityScale/in_deltaT)*(pos0.xy-pos1.xy);
#endif
    vec2 texCoord = in_texco + velocity;
    for(int i = 1; i < in_numMotionBlurSamples; ++i, texCoord+=velocity) {
      // Sample the color buffer along the velocity vector.
      vec4 currentColor = texture(in_inputTexture, texCoord);
      // Add the current color to our color sum.
      output += currentColor;
    }
    // Average all of the samples to get the final blur color.
    output /= float(in_numMotionBlurSamples);
}

