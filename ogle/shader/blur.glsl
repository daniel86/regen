
-- downsample.vs
in vec3 in_pos;
out vec2 out_texco;
void main()
{
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- downsample.fs
in vec2 in_texco;
out vec4 output;

uniform sampler2D in_blurTexture;

void main() {
  output = texture(in_blurTexture, in_texco);
}

-- vs
in vec3 in_pos;
out vec2 out_texco;
void main()
{
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- fs
#define M_PI 3.14159265

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

// BLUR_HORIZONTAL / BLUR_VERTICAL

in vec2 in_texco;
out vec4 output;

uniform sampler2D in_blurTexture;
uniform vec2 in_viewport;

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
    output = texture(in_blurTexture, in_texco) * incrementalGaussian.x;
    coefficientSum += incrementalGaussian.x;
    incrementalGaussian.xy *= incrementalGaussian.yz;

    for(float i=1.0; i <= in_numBlurPixels; i++) {
#ifdef BLUR_HORIZONTAL
        vec2 offset = vec2(i/in_viewport.x, 0);
#else
        vec2 offset = vec2(0, i/in_viewport.y);
#endif
        output += texture2D(in_blurTexture, in_texco - offset) * incrementalGaussian.x;         
        output += texture2D(in_blurTexture, in_texco + offset) * incrementalGaussian.x;         
        coefficientSum += 2.0 * incrementalGaussian.x;
        incrementalGaussian.xy *= incrementalGaussian.yz;
    }

    output /= coefficientSum;
}

