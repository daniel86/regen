
#include "../include/redistance.h"
#include "../include/liquid.h"
#include "../include/helper.h"

class RedistanceShader : public ShaderFunctions {
public:
  RedistanceShader(EulerianPrimitive *primitive)
  : ShaderFunctions("fluidRedistance", makeShaderArgs()),
    primitive_(primitive)
  {
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", "levelSetTexture" } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", "initialLevelSet" } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "vec2" : "vec3", "inverseGridSize" } );
    addUniform( (GLSLUniform) { "float", "deltaT" } );
    addDependencyCode("gradientLiquid", gradientLiquidGLSL(primitive_->is2D()));
  }
  virtual string code() const {
    stringstream s;
    s << "void " << myName_ << "(out float outCol)" << endl;
    s << "{" << endl;
    if(primitive_->is2D()) {
      s << "     vec2 fragCoord = inverseGridSize*gl_FragCoord.xy;" << endl;
    } else {
      s << "     vec3 fragCoord = inverseGridSize*gl_FragCoord.xyz;" << endl;
    }
    s << "     float phiC = texture( levelSetTexture, fragCoord, 0).r;" << endl;
    s << "     //const float dt = 0.1f;" << endl;
    s << "     const float dt = 0.1f;" << endl;
    s << "     const float dt2 = 0.3f;" << endl;
    s << "     " << endl;

    s << "     // avoid redistancing near boundaries, where gradients are ill-defined" << endl;
    s << "     if( (gl_FragCoord.x < 3) || (gl_FragCoord.x > (1.0/inverseGridSize.x-4)) ) { outCol = phiC; return; }" << endl;
    s << "     if( (gl_FragCoord.y < 3) || (gl_FragCoord.y > (1.0/inverseGridSize.y-4)) ) { outCol = phiC; return; }" << endl;
    if(!primitive_->is2D()) s << "     if( (gl_FragCoord.z < 3) || (gl_FragCoord.z > (1.0/inverseGridSize.z-4)) ) { outCol = phiC; return; };" << endl;
    s << "     " << endl;

    s << "     bool isBoundary;" << endl;
    s << "     bool hasHighSlope;" << endl;
    s << "     vec2 gradPhi = gradientLiquid(levelSetTexture, ivec2(gl_FragCoord.xy), 1.01f, isBoundary, hasHighSlope);" << endl;
    s << "     float normGradPhi = length(gradPhi);" << endl;
    s << "     if( isBoundary || !hasHighSlope || ( normGradPhi < 0.01f ) ) { outCol = phiC; return; }" << endl;
    s << "     " << endl;

    s << "     float phiC0 = texture( initialLevelSet, fragCoord ).r;" << endl;
    s << "     //float signPhi = phiC > 0 ? 1 : -1;" << endl;
    s << "     float signPhi = phiC0 / sqrt( (phiC0*phiC0) + 1);" << endl;
    s << "     //float signPhi = phiC / sqrt( (phiC*phiC) + (normGradPhi*normGradPhi));" << endl;
    if(primitive_->is2D()) {
      s << "     //vec2 backwardsPos = gl_FragCoord.xy - (gradPhi.xy/normGradPhi) * signPhi * deltaT * 50.0 * 0.05 ;" << endl;
      s << "     vec2 backwardsPos = gl_FragCoord.xy - (gradPhi.xy/normGradPhi) * signPhi * deltaT * 50.0 * 0.05 ;" << endl;
      s << "     vec2 npos = inverseGridSize*vec2(backwardsPos.x, backwardsPos.y);" << endl;
    } else {
      s << "     vec3 backwardsPos = gl_FragCoord.xyz - (gradPhi/normGradPhi) * signPhi * deltaT * 50.0;" << endl;
      s << "     vec3 npos = inverseGridSize*vec2(backwardsPos.x, backwardsPos.y+0.5, backwardsPos.z);" << endl;
    }
    s << "     //outCol = texture( levelSetTexture, npos ).r + (signPhi * deltaT * 50.0 * 0.075);" << endl;
    s << "     outCol = texture( levelSetTexture, npos ).r + (signPhi * deltaT * 50.0 * 0.075);" << endl;
    s << "}" << endl;
    return s.str();
  }
  EulerianPrimitive *primitive_;
};

EulerianRedistance::EulerianRedistance(EulerianPrimitive *primitive)
: EulerianStage(primitive),
  numRedistanceIterations_(2)
{
  redistanceShader_ = primitive_->makeShader( RedistanceShader(primitive), "float" );
  redistanceShader_->addUniform( primitive->timeStep() );
  redistanceShader_->addUniform( primitive->inverseSize().get() );
  levelSetCurrentLoc_ = glGetUniformLocation(redistanceShader_->id(), "levelSetTexture");
  initialLevelSetLoc_ = glGetUniformLocation(redistanceShader_->id(), "initialLevelSet");

  // create tmp buffer for mac cormack advection
  tmpBuffer_ = createSlab(
      primitive, 1, 1,
      primitive->useHalfFloats());
}

void EulerianRedistance::set_levelSetBuffer(const FluidBuffer &buffer)
{
  buffer.tex->set_name( EulerianLiquid::LEVEL_SET );
  levelSetBuffer_ = buffer;


  /*
  initialLevelSetTexture_ = createTexture(
      primitive_->width(), primitive_->height(), primitive_->depth(),
      1, 1, primitive_->useHalfFloats());
  initialLevelSetAttachment_ = levelSetBuffer_.fbo->addColorAttachment(
      *( (Texture2D*) initialLevelSetTexture_.get() )) - GL_COLOR_ATTACHMENT0;
  */
}
void EulerianRedistance::set_initialLevelSetTexture(ref_ptr<Texture> tex)
{
  initialLevelSetTexture_ = tex;
}

void EulerianRedistance::update()
{
  /*
  FrameBufferObject::blitCopy(
      *levelSetBuffer_.fbo.get(), *tmpBuffer_.fbo.get(),
      GL_COLOR_ATTACHMENT0+levelSetBuffer_.tex->bufferIndex(),
      GL_COLOR_ATTACHMENT0,
      GL_COLOR_BUFFER_BIT,
      GL_NEAREST);
  glActiveTexture(GL_TEXTURE0); tmpBuffer_.tex->bind();
  */
  glActiveTexture(GL_TEXTURE0); initialLevelSetTexture_->bind();

  primitive_->enableShader(redistanceShader_);
  glUniform1i(initialLevelSetLoc_, 0);
  glUniform1i(levelSetCurrentLoc_, 1);
  levelSetBuffer_.fbo->bind();

  glActiveTexture(GL_TEXTURE1);

  for(int i=1; i<numRedistanceIterations_; ++i) {
    levelSetBuffer_.tex->bind();
    levelSetBuffer_.fbo->drawBufferMRT();
    primitive_->draw();
    swapBuffer(levelSetBuffer_);
  }
}
