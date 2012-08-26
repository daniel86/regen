/*
 * textures.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include <sstream>
#include <ogle/shader/filter-shader.h>
#include <ogle/utility/string-util.h>

TextureShader::TextureShader(
    const string &name,
    const vector<string> &args)
: ShaderFunctions(name, args)
{
}
TextureShader::TextureShader(
    const string &name)
: ShaderFunctions(name)
{
}

/////////////

Sepia::Sepia(
    const vector<string> &args)
: TextureShader("sepia", args)
{
  addConstant( GLSLConstant( "float", "in_sepiaDesaturate", "0.0" ) );
  addConstant( GLSLConstant( "float", "in_sepiaToning", "1.0" ) );
  addConstant( GLSLConstant( "vec3", "in_lightColor", "vec3( 1.0, 0.9,  0.5  )" ) );
  addConstant( GLSLConstant( "vec3", "in_darkColor", "vec3( 0.2, 0.05, 0.0  )" ) );
  addConstant( GLSLConstant( "vec3", "in_grayXfer", "vec3( 0.3, 0.59, 0.11  )" ) );
}
string Sepia::code() const
{
  stringstream s;
  s << "vec4 sepia(sampler2D tex, vec2 texco)" << endl;
  s << "{" << endl;
  s << "    vec3 scnColor = in_lightColor * texel(tex, texco).rgb;" << endl;
  s << "    float gray = dot( in_grayXfer, scnColor );" << endl;
  s << "    vec3 muted = mix( scnColor, vec3(gray), in_sepiaDesaturate );" << endl;
  s << "    vec3 sepia = mix( in_darkColor, in_lightColor, gray );" << endl;
  s << "    return vec4( mix( muted, sepia, in_sepiaToning ), 1.0 );" << endl;
  s << "}" << endl;
  return s.str();
}

/////////////

GreyScaleFilter::GreyScaleFilter(
    const vector<string> &args)
: TextureShader("greyScale", args)
{
}
string GreyScaleFilter::code() const
{
  stringstream s;
  s << "void greyScale(sampler2D tex, vec2 texco)" << endl;
  s << "{" << endl;
  s << "    vec4 texcolor = texel(tex, texco);" << endl;
  s << "    float gray = dot( texcolor.rgb, vec3(0.299, 0.587, 0.114) );" << endl;
  s << "    vec4 col = vec4(gray, gray, gray, texcolor.a);" << endl;
  s << "    return col;" << endl;
  s << "}" << endl;
  return s.str();
}

//////////

Tiles::Tiles(
    const vector<string> &args)
: TextureShader("tiles", args)
{
  addConstant( GLSLConstant( "float", "in_tilesVal", "20.0" ) );
}
string Tiles::code() const
{
  stringstream s;
  s << "void tiles(sampler2D tex, vec2 texco)" << endl;
  s << "{" << endl;
  s << "    vec2 tiledUV = texco - mod(texco, vec2(in_tilesVal)) + 0.5*vec2(in_tilesVal);" << endl;
  s << "    return texel(tex, tiledUV);" << endl;
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
    const ConvolutionKernel &k)
: TextureShader(name),
  name_(name),
  convolutionCode_(""),
  kernelSize_(k.size_)
{
  stringstream s;
  s << "   { // convolution kernel" << endl;
  for(int i=0; i<k.size_; ++i) {
    s << "       col += " << k.weights_[i] << "*texture( tex, texco " <<
        "+ vec2("<<k.offsets_[i].x<<", "<<k.offsets_[i].y<<"));" << endl;
  }
  s << "   }" << endl;
  convolutionCode_ = s.str();
}
ConvolutionShader::ConvolutionShader(
    const string &name,
    const ConvolutionKernel &k,
    const vector<string> &args)
: TextureShader(name, args),
  name_(name),
  convolutionCode_(""),
  kernelSize_(k.size_)
{
  stringstream s;
  s << "   { // convolution kernel" << endl;
  for(int i=0; i<k.size_; ++i) {
    s << "       col += " << k.weights_[i] << "*texture( tex, texco " <<
        "+ vec2("<<k.offsets_[i].x<<", "<<k.offsets_[i].y<<"));" << endl;
  }
  s << "   }" << endl;
  convolutionCode_ = s.str();
}
string ConvolutionShader::code() const
{
  stringstream s;
  s << "vec4 " << name_ << "(sampler2D tex, vec2 texco)" << endl;
  s << "{" << endl;
  s << "    vec4 col = vec4(0.0);" << endl;
  s << convolutionCode_ << endl;
  s << "    return col;" << endl;
  s << "}" << endl;
  return s.str();
}

BloomShader::BloomShader(
    const ConvolutionKernel &k,
    const vector<string> &args)
: ConvolutionShader("bloomFilter", k, args)
{
}
string BloomShader::code() const
{
  stringstream s;
  s << "vec4 " << name_ << "(sampler2D tex, vec2 texco)" << endl;
  s << "{" << endl;
  s << "    vec4 center = texel(tex, texco);" << endl;
  s << "    vec4 col = vec4(0.0);" << endl;
  s << convolutionCode_ << endl;
  s << "    if (col.r < 0.3) {" << endl;
  s << "        col = col*col*0.06 + center;" << endl;
  s << "    } else if (col.r < 0.5) {" << endl;
  s << "        col = col*col*0.045 + center;" << endl;
  s << "    } else {" << endl;
  s << "        col = col*col*0.037 + center;" << endl;
  s << "    }" << endl;
  s << "    return col;" << endl;
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
"vec4 radialBlur(sampler2D tex, vec2 texcoord, int samples,\n"
"        float startScale = 1.0, float scaleMul = 0.9)\n"
"{\n"
"    vec4 c = vec4(0);\n"
"    float scale = startScale;\n"
"    for(int i=0; i<samples; i++) {\n"
"        vec2 texco = ((texcoord-vec2(0.5))*scale)+vec2(0.5);\n"
"        vec4 s = texture(tex, texco);\n"
"        c += s;\n"
"        scale *= scaleMul;\n"
"    }\n"
"    c /= samples;\n"
"    return c;\n"
"}\n";

TonemapShader::TonemapShader(
    const vector<string> &args)
: TextureShader("tonemap", args)
{
  addUniform( GLSLUniform("sampler2D", "in_blurTexture") );
  addUniform( GLSLUniform("sampler2D", "in_sceneTexture") );

  addConstant( GLSLConstant("float", "in_blurAmount", "0.5") );
  addConstant( GLSLConstant("float", "in_effectAmount", "0.2") );
  addConstant( GLSLConstant("float", "in_exposure", "16.0") );
  addConstant( GLSLConstant("float", "in_gamma", "0.5") );

  addDependencyCode("vignette", vignette);
  addDependencyCode("radialBlur", radialBlur);
}
string TonemapShader::code() const
{
  stringstream s;
  s << "void tonemap(sampler2D originalTex, sampler2D blurTex, out vec4 color)" << endl;
  s << "{" << endl;
  s << "    // sum original and blurred image" << endl;
  s << "    vec4 c = mix( texture(originalTex, in_texco), texture(blurTex, in_texco), in_blurAmount );" << endl;
  s << "    c += radialBlur(blurTex, in_texco, 30, 1.0, 0.9)*in_effectAmount;" << endl;
  s << "    // exposure and vignette effect" << endl;
  s << "    c *= in_exposure * vignette(in_texco*2.0-vec2(1.0), 0.7, 1.5);" << endl;
  s << "    // gamma correction" << endl;
  s << "    c.rgb = pow(c.rgb, vec3(in_gamma));" << endl;
  s << "    color = c;" << endl;
  s << "}" << endl;
  return s.str();
}

