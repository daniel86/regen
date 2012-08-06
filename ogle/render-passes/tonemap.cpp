/*
 * tonemap.cpp
 *
 *  Created on: 31.07.2012
 *      Author: daniel
 */

#include <tonemap.h>
#include <filter-shader.h>

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

////////////////

class TonemapShader : public TextureShader {
public:
  TonemapShader(const vector<string> &args, const Texture &tex);
  virtual string code() const;
};

TonemapPass::TonemapPass(Scene *scene,
    ref_ptr<Texture2D> blurredTex)
: UnitOrthoRenderPass(scene),
  blurredTex_(blurredTex)
{
  { // create shader
    vector<string> args;
    args.push_back("sceneTexture");
    args.push_back("blurredTexture");
    args.push_back("gl_FragCoord.xy");
    args.push_back("fragmentColor_");
    TonemapShader f(args, scene_->lastPostPassTexture());
    f.addUniform( GLSLUniform("sampler2D", "blurredTexture") );
    tonemapShader_ = initShader(f);
    tonemapShader_->addTexture("blurredTexture", 5);
  }
}

void TonemapPass::render()
{
  // bind vertex data
  glBindBuffer(GL_ARRAY_BUFFER, unitOrthoQuad_->buffer());
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unitOrthoQuad_->indexBuffer());
  // bind textures
  glActiveTexture(GL_TEXTURE5); blurredTex_->bind();
  // enable the shader
  tonemapShader_->enableShader();
  // make the draw call
  unitOrthoQuad_->drawMesh();
}

TonemapShader::TonemapShader(
    const vector<string> &args,
    const Texture &tex)
: TextureShader("tonemap", args, tex)
{
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
  s << "void tonemap(sampler2D unblurredTex, sampler2D blurredTex, " << endl;
  s << "             vec2 texco, inout vec4 col)" << endl;
  s << "{" << endl;
  s << "    // sum original and blurred image" << endl;
  s << "    vec4 c = mix(texture(unblurredTex, texco), " << endl;
  s << "                 texture(blurredTex, texco), blurAmount);" << endl;
  s << "    c += radialBlur(blurredTex, texco, 30, 1.0, 0.95)*effectAmount;" << endl;
  s << "    // exposure and vignette effect" << endl;
  s << "    c *= exposure * vignette(uv*2.0-vec2(1.0), 0.7, 1.5);" << endl;
  s << "    // gamma correction" << endl;
  s << "    col.rgb = pow(c.rgb, vec3(gamma));" << endl;
  s << "}" << endl;
  return s.str();
}
