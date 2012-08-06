/*
 * motion-blur.cpp
 *
 *  Created on: 29.07.2012
 *      Author: daniel
 */

#include <motion-blur.h>

MotionBlurShader::MotionBlurShader(
    const vector<string> &args,
    Texture &tex,
    ref_ptr<TexelTransfer> &velocityTransfer,
    GLuint numSamples,
    bool useVelocityTexture)
: TextureShader("motionBlur", args, tex),
  velocityTransfer_(velocityTransfer),
  useVelocityTexture_(useVelocityTexture)
{
  addConstant(GLSLConstant("int", "numMotionBlurSamples",
      FORMAT_STRING(numSamples) ));
  addUniform(GLSLUniform( "vec2", "viewport" ));
  if(velocityTransfer_.get() != NULL) {
    addDependencyCode(
        velocityTransfer_->name(),
        velocityTransfer_->transfer() );
  }
  if(useVelocityTexture_) {
    addUniform( GLSLUniform( "sampler2D", "velocityTexture" ) );
  } else {
    addUniform( GLSLUniform( "sampler2D", "depthTexture" ) );
    addUniform( GLSLUniform( "mat4", "previousViewProjectionMatrix" ) );
    addDependencyCode("worldPositionFromDepth",
        ShaderFunctions::worldPositionFromDepth);
  }
}
string MotionBlurShader::code() const
{
  stringstream s;
  s << "void motionBlur(vec2 uv, "<<samplerType_<<" tex, inout vec4 col)" << endl;
  s << "{" << endl;
  s << "    // Get the initial color at this pixel." << endl;
  s << "    vec4 color = texture(tex, uv);" << endl;
  if(useVelocityTexture_) {
    s << "    // get velocity from texture" << endl;
    string velocity = "texture(velocityTexture, uv.xy/viewport.xy)";
    if(velocityTransfer_.get() != NULL) { // use a transfer function for texel
      velocity = FORMAT_STRING(
          velocityTransfer_->name() << "(" << velocity << ")");
    }
    s << "    vec2 velocity = " << velocity << ";" << endl;
    s << "    // Early exit" << endl;
    s << "    if(length(velocity) == 0.0) { col=color; return; }" << endl;
  } else {
    s << "    vec2 depthTexco = uv.xy/viewport.xy;" << endl;
    s << "    vec4 pos0, posWorld;" << endl;
    s << "    worldPositionFromDepth(depthTexture, " << endl;
    s << "              depthTexco, inverseViewProjectionMatrix, " << endl;
    s << "              pos0, posWorld);" << endl;
    s << "    // transform by previous frame view-projection matrix" << endl;
    s << "    vec4 pos1 = previousViewProjectionMatrix*posWorld;" << endl;
    s << "    // Convert to nonhomogeneous points [-1,1] by dividing by w." << endl;
    s << "    pos1 /= pos1.w;" << endl;
    s << "    // Use this frame's position and last frame's to compute the pixel velocity." << endl;
    string velocity = "(pos0-pos1)/deltaT";
    if(velocityTransfer_.get() != NULL) {
      velocity = FORMAT_STRING(
          velocityTransfer_->name() << "(" << velocity << ")");
    }
    s << "    vec2 velocity = " << velocity << ";" << endl;
  }
  s << "    vec2 texCoord = uv + velocity;" << endl;
  s << "    for(int i = 1; i < numMotionBlurSamples; ++i, texCoord+=velocity) {" << endl;
  s << "      // Sample the color buffer along the velocity vector." << endl;
  s << "      vec4 currentColor = texture(tex, texCoord);" << endl;
  s << "      // Add the current color to our color sum." << endl;
  s << "      color += currentColor;" << endl;
  s << "    }" << endl;
  s << "    // Average all of the samples to get the final blur color." << endl;
  s << "    col = color/float(numMotionBlurSamples);" << endl;
  s << "}" << endl;
  return s.str();
}

static ShaderFunctions makeMotionBlurShader(Scene *scene,
    ref_ptr<TexelTransfer> &velocityTransfer,
    GLuint numSamples,
    bool useVelocityTexture)
{
  Texture2D &tex = scene->lastPostPassTexture();
  vector<string> args;
  args.push_back("gl_FragCoord.xy");
  args.push_back("sceneTexture");
  args.push_back("fragmentColor_");
  return MotionBlurShader(args,tex,
      velocityTransfer,numSamples,useVelocityTexture);
}
MotionBlurPass::MotionBlurPass(
    Scene *scene,
    ref_ptr<Texture2D> &velocityTexture,
    ref_ptr<TexelTransfer> &velocityTransfer,
    GLuint numSamples)
: UnitOrthoRenderPass(scene, makeMotionBlurShader(
    scene,velocityTransfer,numSamples,true)),
  velocityTexture_(velocityTexture)
{
  textureUniform_ = glGetUniformLocation(shader_->id(), "velocityTexture");
  shader_->addUniform(scene_->viewportUniform());
}
MotionBlurPass::MotionBlurPass(
    Scene *scene,
    ref_ptr<TexelTransfer> &velocityTransfer,
    GLuint numSamples)
: UnitOrthoRenderPass(scene, makeMotionBlurShader(
    scene,velocityTransfer,numSamples,false))
{
  previousViewProjectionUniform_ = ref_ptr<UniformMat4>::manage(
      new UniformMat4("previousViewProjectionMatrix", 1, identity4f()));
  textureUniform_ = glGetUniformLocation(shader_->id(), "depthTexture");
  shader_->addUniform(scene_->inverseViewProjectionMatrixUniform());
  shader_->addUniform(scene_->viewportUniform());
  shader_->addUniform(scene_->deltaTUniform());
  shader_->addUniform(previousViewProjectionUniform_.get());
}
void MotionBlurPass::render()
{
  shader_->enableShader();

  glActiveTexture(GL_TEXTURE5);
  if(velocityTexture_.get()) {
    velocityTexture_->bind();
  } else {
    scene_->depthTexture().bind();
  }
  glUniform1i(textureUniform_, 5);

  UnitOrthoRenderPass::render(false);

  if(velocityTexture_.get()==NULL) {
    previousViewProjectionUniform_->set_value(
        scene_->viewProjectionMatrixUniform()->value());
  }
}
