
-- incrementalGaussian
const float sqrt2pi = 2.506628274631;
const float in_blurSigma = 4.0;

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
#include regen.defines.all
#include regen.states.camera.defines

in vec3 in_pos;
#ifdef RENDER_TARGET == 2D
flat out vec2 out_blurStep;
#else
flat out vec3 out_blurStep;
#endif
flat out vec3 out_incrementalGaussian;
#ifdef VS_LAYER_SELECTION
flat out int out_layer;
#endif

#include regen.layered.VS_SelectLayer
#include regen.filter.blur.incrementalGaussian

uniform vec2 in_inverseViewport;

void main() {
    incrementalGaussian();

#if RENDER_TARGET == CUBE
    #ifdef BLUR_HORIZONTAL
    float dx = in_inverseViewport.x;
    vec3 blurStepArray[6] = vec3[](
        vec3(0.0, 0.0, -dx), // +X
        vec3(0.0, 0.0,  dx), // -X
        vec3( dx, 0.0, 0.0), // +Y
        vec3( dx, 0.0, 0.0), // -Y
        vec3( dx, 0.0, 0.0), // +Z
        vec3(-dx, 0.0, 0.0)  // -Z
    );
    #else
    float dy = in_inverseViewport.y;
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
#elif  RENDER_TARGET == 2D
    #ifdef BLUR_HORIZONTAL
    out_blurStep = vec2(in_inverseViewport.x, 0.0);
    #else
    out_blurStep = vec2(0.0, in_inverseViewport.y);
    #endif
#else // RENDER_TARGET != CUBE && RENDER_TARGET != 2D
    #ifdef BLUR_HORIZONTAL
    out_blurStep = vec3(in_inverseViewport.x, 0.0, 0.0);
    #else
    out_blurStep = vec3(0.0, in_inverseViewport.y, 0.0);
    #endif
#endif // RENDER_TARGET != CUBE

    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
    VS_SelectLayer(regen_RenderLayer());
}

-- gs
// pass-through geometry shader, e.g. in case it is needed
// for layer selection.
#include regen.models.mesh.gs

-- fs
#include regen.states.camera.defines

uniform vec2 in_inverseViewport;
#if RENDER_TARGET == 2D_ARRAY
uniform sampler2DArray in_inputTexture;
#elif RENDER_TARGET == CUBE
uniform samplerCube in_inputTexture;
#else // RENDER_TARGET == 2D
uniform sampler2D in_inputTexture;
#endif
out vec4 out_color;

#include regen.filter.sampling.computeTexco

const int in_numBlurPixels = 4;
flat in vecTexco in_blurStep;
flat in vec3 in_incrementalGaussian;

#define SAMPLE(texco) texture(in_inputTexture,texco)*incrementalGaussian.x

void main()
{
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    vecTexco texco = computeTexco(texco_2D);
    
    float coefficientSum = 0.0;
    vec3 incrementalGaussian = in_incrementalGaussian;

    // Take the central sample first...
    out_color = SAMPLE(texco);
    coefficientSum += incrementalGaussian.x;
    incrementalGaussian.xy *= incrementalGaussian.yz;

    for(float i=1.0; i<float(in_numBlurPixels); i++) {
        vecTexco offset = i*in_blurStep;
        out_color += SAMPLE( texco - offset );         
        out_color += SAMPLE( texco + offset );    
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
#include regen.filter.blur.vs
-- horizontal.gs
#define BLUR_HORIZONTAL
#include regen.filter.blur.gs
-- horizontal.fs
#define BLUR_HORIZONTAL
#include regen.filter.blur.fs

--------------------------------------
--------------------------------------
---- Vertical blur pass. Input mesh should be a unit-quad.
---- Supports blurring cube textures, texture arrays and regular 2D textures.
--------------------------------------
--------------------------------------
-- vertical.vs
#define BLUR_VERTICAL
#include regen.filter.blur.vs
-- vertical.gs
#define BLUR_VERTICAL
#include regen.filter.blur.gs
-- vertical.fs
#define BLUR_VERTICAL
#include regen.filter.blur.fs
