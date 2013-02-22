

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
#define M_PI 3.14159265

in vec3 in_pos;
out vec3 out_incrementalGaussian;

#ifdef IS_2D_TEXTURE
out vec2 out_texco;
out vec2 out_blurStep;

uniform vec2 in_viewport;
#endif

const float in_blurSigma = 4.0;

const float sqrt2pi = 2.506628274631;

void main() {
    // Incremental Gaussian Coefficent Calculation (See GPU Gems 3 pp. 877 - 889)
    float x_incrementalGaussian = 1.0/ (sqrt2pi * in_blurSigma);
    float y_incrementalGaussian = exp(-0.5 / (in_blurSigma * in_blurSigma));
    out_incrementalGaussian = vec3(
        x_incrementalGaussian,
        y_incrementalGaussian,
        y_incrementalGaussian * y_incrementalGaussian
    );
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

in vec3 in_incrementalGaussian[3];
out vec3 out_incrementalGaussian;
out vec3 out_texco;
out vec3 out_blurStep;

#ifdef IS_CUBE_TEXTURE
#include utility.computeCubeDirection
void emitVertex(vec4 P, int layer)
{
    gl_Position = P;
    
    out_texco = computeCubeDirection(vec2(P.x, -P.y), layer);

#ifdef BLUR_HORIZONTAL
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
    
    EmitVertex();
}
#else
void emitVertex(vec4 P, int layer)
{
    gl_Position = P;
    out_texco = vec3(0.5*(P.xy+vec2(1.0)), layer);
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
    // TODO: allow to skip cube layers
    out_incrementalGaussian = in_incrementalGaussian[0];
    emitVertex(gl_PositionIn[0], layer);
    out_incrementalGaussian = in_incrementalGaussian[1];
    emitVertex(gl_PositionIn[1], layer);
    out_incrementalGaussian = in_incrementalGaussian[2];
    emitVertex(gl_PositionIn[2], layer);
    EndPrimitive();
}

-- fs
#include utility.sample_texture.fsHeader
out vec4 output;

const float in_numBlurPixels = 4.0;
const float in_blurSigma = 4.0;

#ifdef IS_2D_TEXTURE
in vec2 in_blurStep;
#else
in vec3 in_blurStep;
#endif
in vec3 in_incrementalGaussian;

#define SAMPLE(texco) texture(in_inputTexture,texco)*incrementalGaussian.x

void main() {

    float coefficientSum = 0.0;
    vec3 incrementalGaussian = in_incrementalGaussian;

    // Take the central sample first...
    output = SAMPLE(in_texco);
    coefficientSum += incrementalGaussian.x;
    incrementalGaussian.xy *= incrementalGaussian.yz;

    for(float i=1.0; i<in_numBlurPixels; i++) {
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

