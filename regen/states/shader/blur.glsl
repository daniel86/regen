
-- incrementalGaussian
const float sqrt2pi = 2.506628274631;

void incrementalGaussian() {
    // Incremental Gaussian Coefficent Calculation (See GPU Gems 3 pp. 877 - 889)
    float x_incrementalGaussian = 1.0/ (sqrt2pi * in_blurSigma);
    float y_incrementalGaussian = exp(-0.5 / (in_blurSigma * in_blurSigma));
    out_incrementalGaussian = vec3(
        x_incrementalGaussian,
        y_incrementalGaussian,
        y_incrementalGaussian * y_incrementalGaussian
    );
}

--------------------------------------
--------------------------------------
---- Separable blur pass. Input mesh should be a unit-quad.
---- Supports blurring cube textures, texture arrays and regular 2D textures.
--------------------------------------
--------------------------------------
-- vs
#include sampling.vsHeader

uniform vec2 in_viewport;

#ifdef IS_2D_TEXTURE
flat out vec2 out_blurStep;
flat out vec3 out_incrementalGaussian;
#endif
const float in_blurSigma = 4.0;

#ifdef IS_2D_TEXTURE
#include blur.incrementalGaussian
#endif

void main() {
#ifdef IS_2D_TEXTURE
    incrementalGaussian();
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
#ifdef BLUR_HORIZONTAL
    out_blurStep = vec2(1.0/in_viewport.x, 0.0);
#else
    out_blurStep = vec2(0.0, 1.0/in_viewport.y);
#endif
#endif
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- gs
#ifndef IS_2D_TEXTURE
#include sampling.gsHeader
#include sampling.gsEmit

uniform vec2 in_viewport;

flat out vec3 out_incrementalGaussian;
flat out vec3 out_blurStep;
const float in_blurSigma = 4.0;

#include blur.incrementalGaussian

void main(void) {
    int layer = gl_InvocationID;
    // select framebuffer layer
    gl_Layer = layer;

    incrementalGaussian();
#ifdef IS_CUBE_TEXTURE
#ifdef BLUR_HORIZONTAL
    // TODO: texel size uniform
    float dx = 1.0/in_viewport.x;
    vec3 blurStepArray[6] = vec3[](
        vec3(0.0, 0.0, -dx), // +X
        vec3(0.0, 0.0,  dx), // -X
        vec3( dx, 0.0, 0.0), // +Y
        vec3( dx, 0.0, 0.0), // -Y
        vec3( dx, 0.0, 0.0), // +Z
        vec3(-dx, 0.0, 0.0)  // -Z
    );
#else
    float dy = 1.0/in_viewport.y;
    vec3 blurStepArray[6] = vec3[](
        vec3(0.0,  dy, 0.0), // +X
        vec3(0.0,  dy, 0.0), // -X
        vec3(0.0, 0.0, -dy), // +Y
        vec3(0.0, 0.0,  dy), // -Y
        vec3(0.0,  dy, 0.0), // +Z
        vec3(0.0,  dy, 0.0)  // -Z
    );
#endif
    out_blurStep = blurStepArray[layer];
#else
#ifdef BLUR_HORIZONTAL
    out_blurStep = vec3(1.0/in_viewport.x, 0.0, 0.0);
#else
    out_blurStep = vec3(0.0, 1.0/in_viewport.y, 0.0);
#endif
#endif

    emitVertex(gl_PositionIn[0], layer);
    emitVertex(gl_PositionIn[1], layer);
    emitVertex(gl_PositionIn[2], layer);
    EndPrimitive();
}
#endif

-- fs
#include sampling.fsHeader
out vec4 out_color;

const float in_numBlurPixels = 4.0;
const float in_blurSigma = 4.0;
#ifdef IS_2D_TEXTURE
flat in vec2 in_blurStep;
#else
flat in vec3 in_blurStep;
#endif
flat in vec3 in_incrementalGaussian;

#define SAMPLE(texco) texture(in_inputTexture,texco)*incrementalGaussian.x

void main()
{
    float coefficientSum = 0.0;
    vec3 incrementalGaussian = in_incrementalGaussian;

    // Take the central sample first...
    out_color = SAMPLE(in_texco);
    coefficientSum += incrementalGaussian.x;
    incrementalGaussian.xy *= incrementalGaussian.yz;

    for(float i=1.0; i<in_numBlurPixels; i++) {
#ifdef IS_2D_TEXTURE
        vec2 offset = i*in_blurStep;
#else
        vec3 offset = i*in_blurStep;
#endif
        out_color += SAMPLE( in_texco - offset );         
        out_color += SAMPLE( in_texco + offset );    
        coefficientSum += 2.0 * incrementalGaussian.x;
        incrementalGaussian.xy *= incrementalGaussian.yz;
    }

    out_color /= coefficientSum;
}

--------------------------------------
--------------------------------------
---- Horizontal blur pass. Input mesh should be a unit-quad.
---- Supports blurring cube textures, texture arrays and regular 2D textures.
--------------------------------------
--------------------------------------
-- horizontal.vs
#define BLUR_HORIZONTAL
#include blur.vs
-- horizontal.gs
#define BLUR_HORIZONTAL
#include blur.gs
-- horizontal.fs
#define BLUR_HORIZONTAL
#include blur.fs

--------------------------------------
--------------------------------------
---- Vertical blur pass. Input mesh should be a unit-quad.
---- Supports blurring cube textures, texture arrays and regular 2D textures.
--------------------------------------
--------------------------------------
-- vertical.vs
#define BLUR_VERTICAL
#include blur.vs
-- vertical.gs
#define BLUR_VERTICAL
#include blur.gs
-- vertical.fs
#define BLUR_VERTICAL
#include blur.fs
