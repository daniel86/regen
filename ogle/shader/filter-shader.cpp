/*
 * textures.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include <sstream>
#include <ogle/shader/filter-shader.h>

TextureShader::TextureShader(
    const string &name,
    const vector<string> &args,
    const Texture &tex)
: ShaderFunctions(name, args),
  samplerType_(tex.samplerType())
{
}

Downsample::Downsample(const vector<string> &args, const Texture &tex)
: TextureShader("downsample", args, tex)
{
}
string Downsample::code() const
{
  stringstream s;
  s << "void downsample(vec2 uv, "<<samplerType_<<" tex, out vec4 col)" << endl;
  s << "{" << endl;
  s << "    col = texture(tex, uv);" << endl;
  s << "}" << endl;
  return s.str();
}

/////////////

Sepia::Sepia(const vector<string> &args, const Texture &tex)
: TextureShader("sepia", args, tex)
{
  addConstant( (GLSLConstant) { "float", "sepiaDesaturate", "0.0" } );
  addConstant( (GLSLConstant) { "float", "sepiaToning", "1.0" } );
  addConstant( (GLSLConstant) { "vec3", "lightColor", "vec3( 1.0, 0.9,  0.5  )" } );
  addConstant( (GLSLConstant) { "vec3", "darkColor", "vec3( 0.2, 0.05, 0.0  )" } );
  addConstant( (GLSLConstant) { "vec3", "grayXfer", "vec3( 0.3, 0.59, 0.11  )" } );
}
string Sepia::code() const
{
  stringstream s;
  s << "void sepia(vec2 uv, "<<samplerType_<<" tex, inout vec4 col)" << endl;
  s << "{" << endl;
  s << "    vec3 scnColor = lightColor * texel(tex, uv).rgb;" << endl;
  s << "    float gray = dot( grayXfer, scnColor );" << endl;
  s << "    vec3 muted = mix( scnColor, vec3(gray), sepiaDesaturate );" << endl;
  s << "    vec3 sepia = mix( darkColor, lightColor, gray );" << endl;
  s << "    col.xyz = mix( muted, sepia, sepiaToning );" << endl;
  s << "}" << endl;
  return s.str();
}

/////////////

GreyScaleFilter::GreyScaleFilter(const vector<string> &args, const Texture &tex)
: TextureShader("greyScale", args, tex)
{
}
string GreyScaleFilter::code() const
{
  stringstream s;
  s << "void greyScale(vec2 uv, "<<samplerType_<<" tex, inout vec4 col)" << endl;
  s << "{" << endl;
  s << "    vec4 texcolor = texel(tex, uv);" << endl;
  s << "    float gray = dot( texcolor.rgb, vec3(0.299, 0.587, 0.114) );" << endl;
  s << "    col = vec4(gray, gray, gray, texcolor.a);" << endl;
  s << "}" << endl;
  return s.str();
}

//////////

Tiles::Tiles(
    const vector<string> &args,
    const Texture &tex)
: TextureShader("tiles", args, tex)
{
  addConstant( (GLSLConstant) { "float", "tilesVal", "20.0" } );
}
string Tiles::code() const
{
  stringstream s;
  s << "void tiles(vec2 uv, "<<samplerType_<<" tex, inout vec4 col)" << endl;
  s << "{" << endl;
  s << "    vec2 tiledUV = uv - mod(uv, vec2(tilesVal)) + 0.5*vec2(tilesVal);" << endl;
  s << "    col = texel(tex, tiledUV);" << endl;
  s << "}" << endl;
  return s.str();
}

/////////////

ConvolutionKernel::ConvolutionKernel(
    GLuint size,
    GLfloat *weigths,
    Vec2f *offsets)
: size_(size), weights_(weigths), offsets_(offsets)
{
}

ConvolutionShader::ConvolutionShader(
    const string &name,
    const vector<string> &args,
    const ConvolutionKernel &k,
    const Texture &tex)
: TextureShader(name, args, tex),
  name_(name),
  convolutionCode_(""),
  kernelSize_(k.size_)
{
  stringstream s;
  s << "   { // convolution kernel" << endl;
  for(unsigned int i=0; i<k.size_; ++i) {
    s << "       col += " << k.weights_[i] << "*texture( tex, uv " <<
        "+ vec2("<<k.offsets_[i].x<<", "<<k.offsets_[i].y<<"));" << endl;
  }
  s << "   }" << endl;
  convolutionCode_ = s.str();
}
string ConvolutionShader::code() const
{
  stringstream s;
  s << "void " << name_ << "(vec2 uv, "<<samplerType_<<" tex, out vec4 col)" << endl;
  s << "{" << endl;
  s << "    col = vec4(0.0);" << endl;
  s << convolutionCode_ << endl;
  s << "}" << endl;
  return s.str();
}

BloomShader::BloomShader(
    const vector<string> &args,
    const ConvolutionKernel &k,
    const Texture &tex)
: ConvolutionShader("bloomFilter", args, k, tex)
{
}
string BloomShader::code() const
{
  stringstream s;
  s << "void " << name_ << "(vec2 uv, "<<samplerType_<<" tex, out vec4 col)" << endl;
  s << "{" << endl;
  s << "    vec4 center = texel(tex, uv);" << endl;
  s << "    col = vec4(0.0);" << endl;
  s << convolutionCode_ << endl;
  s << "    if (col.r < 0.3) {" << endl;
  s << "        col = col*col*0.06 + center;" << endl;
  s << "    } else if (col.r < 0.5) {" << endl;
  s << "        col = col*col*0.045 + center;" << endl;
  s << "    } else {" << endl;
  s << "        col = col*col*0.037 + center;" << endl;
  s << "    }" << endl;
  s << "}" << endl;
  return s.str();
}

////////////////

ConvolutionKernel sharpenKernel(
    Texture &tex,
    GLuint pixelsPerSide,
    float stepFactor)
{
  const GLfloat dx = (tex.targetType()==GL_TEXTURE_RECTANGLE ?
      stepFactor : tex.texelSizeX()*stepFactor);
  const GLfloat dy = (tex.targetType()==GL_TEXTURE_RECTANGLE ?
      stepFactor : tex.texelSizeY()*stepFactor);
  GLuint sizeSide = 2*pixelsPerSide+1;
  GLuint center = pixelsPerSide;
  GLuint i = 0;
  GLfloat *weights = new GLfloat[sizeSide*sizeSide];
  Vec2f *offsets = new Vec2f[sizeSide*sizeSide];
  for(GLuint x=0; x<sizeSide; ++x)
  {
    for(GLuint y=0; y<sizeSide; ++y)
    {
      offsets[i] = Vec2f(
          ((float)x-pixelsPerSide)*dx,
          ((float)y-pixelsPerSide)*dy
          );
      if(x==center && y==center) {
        weights[i] = (float) sizeSide*sizeSide;
      } else {
        weights[i] = -1.0f;
      }
      i += 1;
    }
  }
  return ConvolutionKernel(sizeSide*sizeSide, weights, offsets);
}
ConvolutionKernel edgeDetectKernel(
    Texture &tex,
    GLuint pixelsPerSide,
    float stepFactor)
{
  const GLfloat dx = (tex.targetType()==GL_TEXTURE_RECTANGLE ?
      stepFactor : tex.texelSizeX()*stepFactor);
  const GLfloat dy = (tex.targetType()==GL_TEXTURE_RECTANGLE ?
      stepFactor : tex.texelSizeY()*stepFactor);
  GLuint sizeSide = 2*pixelsPerSide+1;
  GLuint center = pixelsPerSide;
  GLuint i = 0;
  GLfloat *weights = new GLfloat[sizeSide*sizeSide];
  Vec2f *offsets = new Vec2f[sizeSide*sizeSide];
  for(GLuint x=0; x<sizeSide; ++x)
  {
    for(GLuint y=0; y<sizeSide; ++y)
    {
      offsets[i] = Vec2f(
          ((float)x-pixelsPerSide)*dx,
          ((float)y-pixelsPerSide)*dy
          );
      if(x==center && y==center) {
        weights[i] = -((float)sizeSide*2);
      } else if(x==center || y==center) {
        weights[i] = 1.0f;
      } else {
        weights[i] = 0.0f;
      }
      i += 1;
    }
  }
  return ConvolutionKernel(sizeSide*sizeSide, weights, offsets);
}
ConvolutionKernel meanKernel(
    Texture &tex,
    GLuint pixelsPerSide,
    float stepFactor)
{
  const GLfloat dx = (tex.targetType()==GL_TEXTURE_RECTANGLE ?
      stepFactor : tex.texelSizeX()*stepFactor);
  const GLfloat dy = (tex.targetType()==GL_TEXTURE_RECTANGLE ?
      stepFactor : tex.texelSizeY()*stepFactor);
  GLuint sizeSide = 2*pixelsPerSide+1;
  GLuint i = 0;
  GLfloat *weights = new GLfloat[sizeSide*sizeSide];
  Vec2f *offsets = new Vec2f[sizeSide*sizeSide];
  const float meanWeigth = 1.0/((float)sizeSide*sizeSide);
  for(GLuint x=0; x<sizeSide; ++x)
  {
    for(GLuint y=0; y<sizeSide; ++y)
    {
      offsets[i] = Vec2f(
          ((float)x-pixelsPerSide)*dx,
          ((float)y-pixelsPerSide)*dy
          );
      weights[i] = meanWeigth;
      i += 1;
    }
  }
  return ConvolutionKernel(sizeSide*sizeSide, weights, offsets);
}
ConvolutionKernel bloomKernel(
    Texture &tex,
    GLuint pixelsPerSide,
    float stepFactor)
{
  const GLfloat dx = (tex.targetType()==GL_TEXTURE_RECTANGLE ?
      stepFactor : tex.texelSizeX()*stepFactor);
  const GLfloat dy = (tex.targetType()==GL_TEXTURE_RECTANGLE ?
      stepFactor : tex.texelSizeY()*stepFactor);
  GLuint sizeSide = 2*pixelsPerSide+1;
  GLuint i = 0;
  GLfloat *weights = new GLfloat[sizeSide*sizeSide];
  Vec2f *offsets = new Vec2f[sizeSide*sizeSide];
  for(GLuint x=0; x<sizeSide; ++x)
  {
    for(GLuint y=0; y<sizeSide; ++y)
    {
      offsets[i] = Vec2f(
          ((float)x-pixelsPerSide)*dx,
          ((float)y-pixelsPerSide)*dy
          );
      weights[i] = 0.15f;
      i += 1;
    }
  }
  return ConvolutionKernel(sizeSide*sizeSide, weights, offsets);
}

/////////////////

static ConvolutionKernel blurSeparable(
    GLfloat blurSize,
    const Vec2f &blurMultiplyVec,
    const BlurConfig &cfg)
{
  GLuint size = cfg.pixelsPerSide*2+1;
  GLuint center = cfg.pixelsPerSide;
  GLfloat *weigths = new GLfloat[size];
  Vec2f *offsets = new Vec2f[size];

  // Incremental Gaussian Coefficent Calculation (See GPU Gems 3 pp. 877 - 889)
  Vec3f incrementalGaussian;
  incrementalGaussian.x = 1.0f / (sqrt(2.0f * M_PI) * cfg.sigma);
  incrementalGaussian.y = exp(-0.5f / (cfg.sigma * cfg.sigma));
  incrementalGaussian.z = incrementalGaussian.y * incrementalGaussian.y;

  float coefficientSum = 0.0f;

  // Take the central sample first...
  weigths[center] = incrementalGaussian.x;
  offsets[center] = Vec2f(0.0f, 0.0f);

  coefficientSum += incrementalGaussian.x;
  incrementalGaussian.x *= incrementalGaussian.y;
  incrementalGaussian.y *= incrementalGaussian.z;

  // go through the remaining samples
  for (GLuint i = 1; i <= cfg.pixelsPerSide; ++i)
  {
    weigths[center-i] = incrementalGaussian.x;
    offsets[center-i] = blurMultiplyVec * blurSize * (-(float)i);

    weigths[center+i] = incrementalGaussian.x;
    offsets[center+i] = blurMultiplyVec * blurSize * (+(float)i);

    coefficientSum += 2.0f * incrementalGaussian.x;
    incrementalGaussian.x *= incrementalGaussian.y;
    incrementalGaussian.y *= incrementalGaussian.z;
  }

  for (GLuint i = 0; i<size; ++i)
  {
    weigths[i] /= coefficientSum;
  }

  return ConvolutionKernel(size, weigths, offsets);
}

ConvolutionKernel blurHorizontalKernel(Texture &tex, const BlurConfig &cfg)
{
  const GLfloat dx = (tex.targetType()==GL_TEXTURE_RECTANGLE ?
      cfg.stepFactor : tex.texelSizeX()*cfg.stepFactor);
  return blurSeparable(dx, Vec2f(1.0f, 0.0f), cfg);
}
ConvolutionKernel blurVerticalKernel(Texture &tex, const BlurConfig &cfg)
{
  const GLfloat dy = (tex.targetType()==GL_TEXTURE_RECTANGLE ?
      cfg.stepFactor : tex.texelSizeY()*cfg.stepFactor);
  return blurSeparable(dy, Vec2f(0.0f, 1.0f), cfg);
}

/////////////////

static const string vignette =
"// vignetting effect (makes corners of image darker)\n"
"float vignette(vec2 pos, float inner, float outer)\n"
"{\n"
"  float r = length(pos);\n"
"  r = 1.0 - smoothstep(inner, outer, r);\n"
"  return r;\n"
"}\n";

static const string radialBlur =
"// radial blur\n"
"vec4 radialBlur(sampler2D tex, vec2 texcoord, int samples,\n"
"        float startScale = 1.0, float scaleMul = 0.9)\n"
"{\n"
"    vec4 c = vec4(0);\n"
"    float scale = startScale;\n"
"    for(int i=0; i<samples; i++) {\n"
"        vec2 uv = ((texcoord-0.5)*scale)+0.5;\n"
"        vec4 s = texture(tex, uv);\n"
"        c += s;\n"
"        scale *= scaleMul;\n"
"    }\n"
"    c /= samples;\n"
"    return c;\n"
"}\n";

TonemapShader::TonemapShader(
    const vector<string> &args,
    const Texture &tex)
: TextureShader("tonemap", args, tex)
{
  addUniform( GLSLUniform("sampler2D", "blurTexture") );

  addConstant( GLSLConstant("float", "blurAmount", "0.5") );
  addConstant( GLSLConstant("float", "effectAmount", "0.2") );
  addConstant( GLSLConstant("float", "exposure", "16.0") );
  addConstant( GLSLConstant("float", "gamma", "0.5") );

  addDependencyCode("vignette", vignette);
  addDependencyCode("radialBlur", radialBlur);
}
string TonemapShader::code() const
{
  stringstream s;
  s << "void tonemap(vec2 uv, inout vec4 col)" << endl;
  s << "{" << endl;
  s << "    // sum original and blurred image" << endl;
  s << "    vec4 c = mix( texture(sceneTexture, uv), texture(blurTexture, uv), blurAmount );" << endl;
  s << "    c += radialBlur(blurTexture, uv, 30, 1.0, 0.95)*effectAmount;" << endl;
  s << "    // exposure and vignette effect" << endl;
  s << "    c *= exposure * vignette(uv*2.0-vec2(1.0), 0.7, 1.5);" << endl;
  s << "    // gamma correction" << endl;
  s << "    col.rgb = pow(c.rgb, vec3(gamma));" << endl;
  s << "}" << endl;
  return s.str();
}

