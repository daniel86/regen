

-- downsample.vs
#include utility.sample_texture.vs

-- downsample.gs
#include utility.sample_texture.gs

-- downsample.fs
#include utility.sample_texture.fsHeader
out vec4 output;
void main() {
  output = texture(in_inputTexture, in_texco);
}

-- vs
in vec3 in_pos;

#ifdef IS_2D_TEXTURE
out vec2 out_texco;
out vec2 out_blurStep;

uniform vec2 in_viewport;
#endif

void main() {
#ifdef IS_2D_TEXTURE
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
#extension GL_EXT_geometry_shader4 : enable
#extension GL_ARB_gpu_shader5 : enable

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;
#ifdef IS_CUBE_TEXTURE
layout(invocations = 6) in;
#else
layout(invocations = NUM_TEXTURE_LAYERS) in;
#endif

uniform vec2 in_viewport;

out vec3 out_texco;
out vec3 out_blurStep;

#ifdef IS_CUBE_TEXTURE
#include utility.computeCubeMapDirection
void emitVertex(vec4 P, int layer)
{
    gl_Position = P;
    
    out_texco = computeCubeMapDirection(vec2(P.x, -P.y), layer);

#ifdef BLUR_HORIZONTAL
    float dx = 1.0/in_viewport.x;
    float dy = 0.0;
#else
    float dx = 0.0;
    float dy = 1.0/in_viewport.y;
#endif
    vec3 blurStepArray[6] = vec3[](
        vec3( 1.0,  dy, -dx), // +X
        vec3(-1.0,  dy,  dx), // -X
        vec3(  dx, 1.0, -dy), // +Y
        vec3(  dx,-1.0,  dy), // -Y
        vec3(  dx,  dy, 1.0), // +Z
        vec3( -dx,  dy,-1.0)  // -Z
    );
    out_blurStep = blurStepArray[layer];
    
    EmitVertex();
}
#else
void emitVertex(vec4 P, int layer)
{
    gl_Position = P;
    out_texco = vec3(0.5*(gl_Position.xy+vec2(1.0)), layer);
#ifdef BLUR_HORIZONTAL
    out_blurStep = vec3(1.0/in_viewport.x, 0.0, 0.0);
#else
    out_blurStep = vec3(0.0, 1.0/in_viewport.y, 0.0);
#endif
    EmitVertex();
}
#endif

void main(void) {
    int layer = gl_InvocationID;
    // select framebuffer layer
    gl_Layer = layer;
    // TODO: allow to skip layers
    emitVertex(gl_PositionIn[0], layer);
    emitVertex(gl_PositionIn[1], layer);
    emitVertex(gl_PositionIn[2], layer);
    EndPrimitive();
}

-- fs
#define M_PI 3.14159265

#include utility.sample_texture.fsHeader
out vec4 output;

// Half number of texels to consider.
//   With 4.0 you get 9x9 kernel
//   With 3.0 you get 7x7 kernel
//   With 2.0 you get 5x5 kernel
const float in_numBlurPixels = 4.0;
// The sigma value for the gaussian function: higher value means more blur
//   A good value for 9x9 is around 3 to 5
//   A good value for 7x7 is around 2.5 to 4
//   A good value for 5x5 is around 2 to 3.5
const float in_blurSigma = 4.0;

#define SAMPLE(texco) texture(in_inputTexture,texco)*incrementalGaussian.x

#ifdef IS_2D_TEXTURE
in vec2 in_blurStep;
#else
in vec3 in_blurStep;
#endif

void main() {
    // Incremental Gaussian Coefficent Calculation (See GPU Gems 3 pp. 877 - 889)
    float x_incrementalGaussian = 1.0/ (sqrt(2.0 * M_PI) * in_blurSigma);
    float y_incrementalGaussian = exp(-0.5 / (in_blurSigma * in_blurSigma));
    vec3 c_incrementalGaussian = vec3(
        x_incrementalGaussian,
        y_incrementalGaussian,
        y_incrementalGaussian * y_incrementalGaussian
    );

    float coefficientSum = 0.0;
    vec3 incrementalGaussian = c_incrementalGaussian;

    // Take the central sample first...
    output = SAMPLE(in_texco);
    coefficientSum += incrementalGaussian.x;
    incrementalGaussian.xy *= incrementalGaussian.yz;

    for(float i=1.0; i <= in_numBlurPixels; i++) {
#ifdef IS_2D_TEXTURE
        vec2 offset = i*in_blurStep;
#else
        vec3 offset = i*in_blurStep;
#endif
        output += SAMPLE( in_texco - offset );         
        output += SAMPLE( in_texco + offset );    
        coefficientSum += 2.0 * incrementalGaussian.x;
        incrementalGaussian.xy *= incrementalGaussian.yz;
    }

    output /= coefficientSum;
}

