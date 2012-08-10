
#include "../include/extrapolate-velocity.h"
#include "../include/liquid.h"
#include "../include/helper.h"

class ExtrapolateVelocityShader : public ShaderFunctions {
public:
  ExtrapolateVelocityShader(EulerianPrimitive *primitive)
  : ShaderFunctions("fluidExtrapolateVelocity", makeShaderArgs()),
    primitive_(primitive)
  {
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", EulerianFluid::VELOCITY } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", EulerianLiquid::LEVEL_SET } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", EulerianFluid::OBSTACLES } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "vec2" : "vec3", "inverseGridSize" } );
    addUniform( (GLSLUniform) { "float", "deltaT" } );

    addDependencyCode("isOutsideSimulationDomain",
        isOutsideSimulationDomainGLSL(primitive_->is2D()));
    addDependencyCode("isNonEmptyCell",
        isNonEmptyCellGLSL(primitive_->is2D()));
    addDependencyCode("gradient",
        gradientGLSL(primitive_->is2D()));
  }
  virtual string code() const {
    stringstream s;
    s << "void " << myName_ << "(out vec4 outCol)" << endl;
    s << "{" << endl;
    if(primitive_->is2D()) {
      s << "     vec2 fragCoord = inverseGridSize*gl_FragCoord.xy;" << endl;
    } else {
      s << "     vec3 fragCoord = inverseGridSize*gl_FragCoord.xyz;" << endl;
    }

    s << "     if( isNonEmptyCell(fragCoord) ) { outCol = vec4(0); return; }" << endl;
    s << endl;
    if(primitive_->isLiquid()) {
      s << "     if( !isOutsideSimulationDomain(fragCoord) ) { outCol = texture(" <<
          EulerianFluid::VELOCITY << ", fragCoord); return; }" << endl;
    }

    if(primitive_->is2D()) {
      s << "     vec2 normalizedGradLS = normalize( gradient(" <<
          EulerianLiquid::LEVEL_SET << ", ivec2(gl_FragCoord.xy)) );" << endl;
      s << "     vec2 backwardsPos = gl_FragCoord.xy"
          " - normalizedGradLS * deltaT * 61.25;" << endl;
      s << "     vec2 npos = inverseGridSize * backwardsPos;" << endl;
    }
    else {
      s << "     vec3 normalizedGradLS = normalize( gradient(" <<
          EulerianLiquid::LEVEL_SET << ", ivec3(gl_FragCoord.xyz)) );" << endl;
      s << "     vec3 backwardsPos = gl_FragCoord.xyz"
          " - normalizedGradLS * deltaT * 50.0 * 0.25;" << endl;
      s << "     vec3 npos = inverseGridSize * backwardsPos;" << endl;
    }
    s << "     outCol = texture(" << EulerianFluid::VELOCITY << ", npos);" << endl;
    s << "}" << endl;
    return s.str();
  }
  EulerianPrimitive *primitive_;
};

EulerianExtrapolateVelocity::EulerianExtrapolateVelocity(EulerianPrimitive *primitive)
: EulerianStage(primitive),
  obstaclesTexture_(),
  numOfExtrapolationIterations_(6)
{
  extrapolateVelocityShader_ = primitive_->makeShader(
      ExtrapolateVelocityShader(primitive_), "vec4" );
  extrapolateVelocityShader_->addUniform( primitive_->timeStep() );
  extrapolateVelocityShader_->addUniform( primitive_->inverseSize().get() );
}

void EulerianExtrapolateVelocity::set_velocityBuffer(const FluidBuffer &buffer)
{
  buffer.tex->set_name( EulerianFluid::VELOCITY );
  velocityBuffer_ = buffer;
  extrapolateVelocityShader_->addTexture( buffer.tex.get() );
}
void EulerianExtrapolateVelocity::set_levelSetTexture(ref_ptr<Texture> tex)
{
  tex->set_name( EulerianLiquid::LEVEL_SET );
  levelSetTexture_ = tex;
  extrapolateVelocityShader_->addTexture( levelSetTexture_.get() );
}
void EulerianExtrapolateVelocity::set_obstaclesTexture(ref_ptr<Texture> tex)
{
  tex->set_name( EulerianFluid::OBSTACLES );
  obstaclesTexture_ = tex;
  extrapolateVelocityShader_->addTexture( obstaclesTexture_.get() );
}

void EulerianExtrapolateVelocity::update()
{
  primitive_->enableShader(extrapolateVelocityShader_);

  activateTexture(extrapolateVelocityShader_.get(), obstaclesTexture_.get(), 1);
  activateTexture(extrapolateVelocityShader_.get(), levelSetTexture_.get(), 2);

  // flip flop with velocity buffer for n iterations
  glActiveTexture(GL_TEXTURE0);
  for(int i=0; i<numOfExtrapolationIterations_; ++i) {
    velocityBuffer_.tex->bind();
    velocityBuffer_.fbo->bind();
    velocityBuffer_.fbo->drawBufferMRT();
    primitive_->draw();
    swapBuffer(velocityBuffer_);
  }
}
