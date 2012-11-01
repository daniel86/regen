
-- fs

#ifdef USE_VELOCITY_TEXTURE
uniform sampler2D in_velocityTexture;
#else
uniform sampler2D in_depthTexture;
uniform mat4 in_previousViewProjectionMatrix;
#endif
uniform vec2 in_viewport;

const int in_numMotionBlurSamples = 20;

void main()
{
    // Get the initial color at this pixel.
    vec4 color = texture(tex, uv);
#ifdef USE_VELOCITY_TEXTURE
    // get velocity from texture
    velocity = texture(velocityTexture, uv.xy/viewport.xy);
    // Early exit
    if(length(velocity) == 0.0) {
        col=color;
        return;
    }
#else
    vec2 depthTexco = uv.xy/viewport.xy;
    vec4 pos0, posWorld;
    worldPositionFromDepth(depthTexture,
        depthTexco, inverseViewProjectionMatrix,
        pos0, posWorld);
    // transform by previous frame view-projection matrix
    vec4 pos1 = previousViewProjectionMatrix*posWorld;
    // Convert to nonhomogeneous points [-1,1] by dividing by w.
    pos1 /= pos1.w;
    // Use this frame's position and last frame's to compute the pixel velocity.
    velocity = (pos0-pos1)/deltaT;
#endif
    vec2 texCoord = uv + velocity;
    for(int i = 1; i < numMotionBlurSamples; ++i, texCoord+=velocity) {
      // Sample the color buffer along the velocity vector.
      vec4 currentColor = texture(tex, texCoord);
      // Add the current color to our color sum.
      color += currentColor;
    }
    // Average all of the samples to get the final blur color.
    col = color/float(numMotionBlurSamples);
}

