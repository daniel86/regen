
#include "../include/pressure.h"
#include "../include/liquid.h"
#include "../include/helper.h"

class PressureShader : public ShaderFunctions {
public:
  PressureShader(EulerianPrimitive *primitive)
  : ShaderFunctions("fluidPressure", makeShaderArgs()),
    primitive_(primitive)
  {
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", EulerianFluid::PRESSURE } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", EulerianFluid::DIVERGENCE } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", EulerianFluid::OBSTACLES } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "vec2" : "vec3", "inverseGridSize" } );
    addUniform( (GLSLUniform) { "float", "alpha" } );
    addUniform( (GLSLUniform) { "float", "inverseBeta" } );
    addDependencyCode("isNonEmptyCell", isNonEmptyCellGLSL(primitive_->is2D()));

    if(primitive_->isLiquid()) {
      addDependencyCode("isOutsideSimulationDomain",
          isOutsideSimulationDomainGLSL(primitive_->is2D()));
      addUniform( (GLSLUniform) { primitive_->is2D() ?
          "sampler2D" : "sampler3D", EulerianLiquid::LEVEL_SET } );
    }
  }
  virtual string code() const {
    stringstream s;
    s << "void " << myName_ << "(out vec4 outCol)" << endl;
    s << "{" << endl;
    if(primitive_->is2D()) {
      s << "     ivec2 pos = ivec2(gl_FragCoord.xy);" << endl;
    } else {
      s << "     ivec3 pos = ivec3(vec3(gl_FragCoord.xy,f_layer));" << endl;
    }
    s << "     " << endl;

    if(primitive_->isLiquid()) {
      s << "     // air pressure" << endl;
      s << "     if( isOutsideSimulationDomain(inverseGridSize*gl_FragCoord.xy) ) {" << endl;
      s << "         outCol = vec4(0.0,0.0,0.0,1.0); return;" << endl;
      s << "     }" << endl;
      s << "     " << endl;
    }

    s << "     vec4 pC = texelFetch(" << EulerianFluid::PRESSURE << ", pos, 0);" << endl;
    s << findNeighborsGLSL("p", EulerianFluid::PRESSURE, primitive_->is2D());
    s << "     " << endl;
    s << "     vec4 dC = texelFetch(" << EulerianFluid::DIVERGENCE << ", pos, 0);" << endl;
    s << findNeighborsGLSL("o", EulerianFluid::OBSTACLES, primitive_->is2D());
    s << "     " << endl;
    s << "     // Make sure that the pressure in solid cells is effectively ignored." << endl;
    s << "     if (oN.x > 0) pN = pC;" << endl;
    s << "     if (oS.x > 0) pS = pC;" << endl;
    s << "     if (oE.x > 0) pE = pC;" << endl;
    s << "     if (oW.x > 0) pW = pC;" << endl;
    if(!primitive_->is2D()) {
      s << "     if (oF.x > 0) pF = pC;" << endl;
      s << "     if (oB.x > 0) pB = pC;" << endl;
    }
    s << "     " << endl;
    if(primitive_->is2D()) {
      s << "     outCol = (pW + pE + pS + pN - alpha * dC) * inverseBeta;" << endl;
    } else {
      s << "     outCol = (pW + pE + pS + pN + pF + pB - alpha * dC) * inverseBeta;" << endl;
    }
    s << "}" << endl;
    return s.str();
  }
  EulerianPrimitive *primitive_;
};

class SubstractGradientShader : public ShaderFunctions {
public:
  SubstractGradientShader(EulerianPrimitive *primitive)
  : ShaderFunctions("fluidSubstractGradient", makeShaderArgs()),
    primitive_(primitive)
  {
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", EulerianFluid::VELOCITY } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", EulerianFluid::PRESSURE } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", EulerianFluid::OBSTACLES } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "vec2" : "vec3", "inverseGridSize" } );
    addUniform( (GLSLUniform) { "float", "densityInverse" } );

    if(primitive_->isLiquid()) {
      addDependencyCode("isOutsideSimulationDomain",
          isOutsideSimulationDomainGLSL(primitive_->is2D()));
      addUniform( (GLSLUniform) { primitive_->is2D() ?
          "sampler2D" : "sampler3D", EulerianLiquid::LEVEL_SET } );
    }
  }
  virtual string code() const {
    stringstream s;
    if(primitive_->is2D()) {
      s << "void " << myName_ << "(out vec2 outCol)" << endl;
    } else {
      s << "void " << myName_ << "(out vec3 outCol)" << endl;
    }
    s << "{" << endl;

    if(primitive_->is2D()) {
      s << "     ivec2 pos = ivec2(gl_FragCoord.xy);" << endl;
      s << "     vec2 fragCoord = inverseGridSize*gl_FragCoord.xy;" << endl;
    } else {
      s << "     ivec3 pos = ivec3(vec3(gl_FragCoord.xy,f_layer));" << endl;
      s << "     vec3 fragCoord = inverseGridSize*vec3(gl_FragCoord.xy,f_layer);" << endl;
    }
    s << "     " << endl;

    s << "     vec4 oldV = texelFetch(" << EulerianFluid::VELOCITY << ", pos, 0);" << endl;
    if(primitive_->isLiquid()) {
      s << "     if( isOutsideSimulationDomain(fragCoord) ) { outCol = vec2(0); return; }" << endl;
      s << "     " << endl;
    }

    s << "     vec4 oC = texelFetch(" << EulerianFluid::OBSTACLES << ", pos, 0);" << endl;
    if(primitive_->is2D()) s << "         if (oC.x > 0) { outCol = oC.yz; return; }" << endl;
    else                   s << "         if (oC.x > 0) { outCol = oC.yzw; return; }" << endl;
    s << findNeighborsGLSL("o", EulerianFluid::OBSTACLES, primitive_->is2D());
    s << "     " << endl;
    s << "     float pC = texelFetch(" << EulerianFluid::PRESSURE << ", pos, 0).r;" << endl;
    s << findNeighborsGLSL("p", EulerianFluid::PRESSURE, primitive_->is2D());
    s << "     " << endl;

    if(primitive_->is2D()) {
      s << "     vec2 obstV = vec2(0);" << endl;
      s << "     vec2 vMask = vec2(1);" << endl;
    } else {
      s << "     vec3 obstV = vec3(0);" << endl;
      s << "     vec3 vMask = vec3(1);" << endl;
    }
    s << "     " << endl;

    s << "     // Use center pressure for solid cells" << endl;
    s << "     if (oN.x > 0) { pN.x = pC; obstV.y = oN.z; }" << endl;
    s << "     if (oS.x > 0) { pS.x = pC; obstV.y = oS.z; }" << endl;
    s << "     if (oE.x > 0) { pE.x = pC; obstV.x = oE.y; }" << endl;
    s << "     if (oW.x > 0) { pW.x = pC; obstV.x = oW.y; }" << endl;
    if(!primitive_->is2D()) {
      s << "     if (oF.x > 0) { pF.x = pC; obstV.z = 0.0f; vMask.z = 0; }" << endl;
      s << "     if (oB.x > 0) { pB.x = pC; obstV.z = 0.0f; vMask.z = 0; }" << endl;
    }
    s << "     " << endl;

    if(primitive_->is2D()) {
      s << "     vec2 v = oldV.xy - vec2(pE.x - pW.x, pN.x - pS.x) * 0.5f * densityInverse;" << endl;
    } else {
      s << "     vec3 v = oldV.xyz - vec3(pE.x - pW.x, pN.x - pS.x, pF.x - pB.x) * 0.5f * densityInverse;" << endl;
    }

    // there are some artifacts with fluid sticking to obstacles with that code...
    s << "     " << endl;
    s << "     //if ( (oN.x > 0 && v.y > 0.0) || (oS.x > 0 && v.y < 0.0) ) { vMask.y = 0; }" << endl;
    s << "     //if ( (oE.x > 0 && v.x > 0.0) || (oW.x > 0 && v.x < 0.0) ) { vMask.x = 0; }" << endl;

    s << "     outCol = ((vMask * v) + obstV);" << endl;
    s << "}" << endl;
    return s.str();
  }
  EulerianPrimitive *primitive_;
};

class DivergenceShader : public ShaderFunctions {
public:
  DivergenceShader(bool is2D)
  : ShaderFunctions("fluidDivergence", makeShaderArgs()),
    is2D_(is2D)
  {
    addUniform( (GLSLUniform) { is2D_ ?
        "sampler2D" : "sampler3D", EulerianFluid::VELOCITY } );
    addUniform( (GLSLUniform) { is2D_ ?
        "sampler2D" : "sampler3D", EulerianFluid::OBSTACLES } );
    addUniform( (GLSLUniform) { "float", "halfInverseCellSize" } );
  }
  virtual string code() const {
    stringstream s;
    s << "void " << myName_ << "(out float outCol)" << endl;
    s << "{" << endl;
    if(is2D_) {
      s << "     ivec2 pos = ivec2(gl_FragCoord.xy);" << endl;
    } else {
      s << "     ivec3 pos = ivec3(vec3(gl_FragCoord.xy,f_layer));" << endl;
    }
    s << "     " << endl;
    s << findNeighborsGLSL("velocity", EulerianFluid::VELOCITY, is2D_);
    s << findNeighborsGLSL("obstacle", EulerianFluid::OBSTACLES, is2D_);
    s << "     " << endl;
    s << "     // Use obstacle velocities for solid cells" << endl;
    if(is2D_) {
      s << "     if (obstacleN.x > 0) velocityN.xy = obstacleN.yz;" << endl;
      s << "     if (obstacleS.x > 0) velocityS.xy = obstacleS.yz;" << endl;
      s << "     if (obstacleE.x > 0) velocityE.xy = obstacleE.yz;" << endl;
      s << "     if (obstacleW.x > 0) velocityW.xy = obstacleW.yz;" << endl;
    } else {
      s << "     if (obstacleN.x > 0) velocityN.xyz = obstacleN.yzw;" << endl;
      s << "     if (obstacleS.x > 0) velocityS.xyz = obstacleS.yzw;" << endl;
      s << "     if (obstacleE.x > 0) velocityE.xyz = obstacleE.yzw;" << endl;
      s << "     if (obstacleW.x > 0) velocityW.xyz = obstacleW.yzw;" << endl;
      s << "     if (obstacleF.x > 0) velocityF.xyz = obstacleF.yzw;" << endl;
      s << "     if (obstacleB.x > 0) velocityB.xyz = obstacleB.yzw;" << endl;
    }
    s << "     " << endl;
    if(is2D_) {
      s << "     outCol = halfInverseCellSize * (velocityE.x - velocityW.x + velocityN.y - velocityS.y);" << endl;
    } else {
      s << "     outCol = halfInverseCellSize * (velocityE.x - velocityW.x + velocityN.y - velocityS.y + velocityF.z - velocityB.z);" << endl;
    }
    s << "}" << endl;
    return s.str();
  }
  bool is2D_;
};


EulerianPressure::EulerianPressure(EulerianPrimitive *primitive)
: EulerianStage(primitive),
  densityInverse_( ref_ptr<UniformFloat>::manage( new UniformFloat("densityInverse") )),
  alpha_( ref_ptr<UniformFloat>::manage( new UniformFloat("alpha") )),
  inverseBeta_( ref_ptr<UniformFloat>::manage( new UniformFloat("inverseBeta") )),
  numPressureIterations_(20),
  halfCell_( ref_ptr<UniformFloat>::manage( new UniformFloat("halfInverseCellSize") ))
{
  densityInverse_->set_value(1.25f / 2.25f);
  halfCell_->set_value( 0.5f / 1.25f );
  set_alpha(- 1.25f*1.25f);
  set_inverseBeta(0.25f);

  divergenceShader_ = primitive_->makeShader(
      DivergenceShader(primitive_->is2D()), "float" );
  divergenceShader_->addUniform( halfCell_.get() );

  subtractPressureGradientShader_ = primitive_->makeShader(
      SubstractGradientShader(primitive_), primitive_->is2D() ? "vec2" : "vec3" );
  subtractPressureGradientShader_->addUniform( densityInverse_.get() );
  subtractPressureGradientShader_->addUniform( primitive_->inverseSize().get() );

  pressureShader_ = primitive_->makeShader( PressureShader(primitive_), "vec4" );
  pressureShader_->addUniform( alpha_.get() );
  pressureShader_->addUniform( inverseBeta_.get() );
  pressureShader_->addUniform( primitive_->inverseSize().get() );
}

void EulerianPressure::set_pressureBuffer(const FluidBuffer &buffer)
{
  buffer.tex->set_name( EulerianFluid::PRESSURE );
  pressureBuffer_ = buffer;
  pressureShader_->addTexture( buffer.tex.get() );
  subtractPressureGradientShader_->addTexture( buffer.tex.get() );
}

void EulerianPressure::set_divergenceBuffer(const FluidBuffer &buffer)
{
  buffer.tex->set_name( EulerianFluid::DIVERGENCE );
  divergenceBuffer_ = buffer;
  pressureShader_->addTexture( buffer.tex.get() );
}

void EulerianPressure::set_velocityBuffer(const FluidBuffer &buffer)
{
  buffer.tex->set_name( EulerianFluid::VELOCITY );
  velocityBuffer_ = buffer;
  subtractPressureGradientShader_->addTexture( buffer.tex.get() );
  divergenceShader_->addTexture( buffer.tex.get() );
}

void EulerianPressure::set_obstaclesTexture(ref_ptr<Texture> tex)
{
  tex->set_name( EulerianFluid::OBSTACLES );
  obstaclesTexture_ = tex;
  pressureShader_->addTexture( obstaclesTexture_.get() );
  subtractPressureGradientShader_->addTexture( obstaclesTexture_.get() );
  divergenceShader_->addTexture( obstaclesTexture_.get() );
}

void EulerianPressure::set_levelSetTexture(ref_ptr<Texture> tex)
{
  tex->set_name( EulerianLiquid::LEVEL_SET );
  levelSetTexture_ = tex;
  subtractPressureGradientShader_->addTexture( levelSetTexture_.get() );
  pressureShader_->addTexture( levelSetTexture_.get() );
}


void EulerianPressure::update()
{
  { // compute divergence
    primitive_->enableShader(divergenceShader_);
    divergenceBuffer_.fbo->bind();
    divergenceBuffer_.fbo->drawBufferMRT();

    activateTexture(divergenceShader_.get(), velocityBuffer_.tex.get(), 0);
    activateTexture(divergenceShader_.get(), obstaclesTexture_.get(), 1);

    primitive_->draw();
  }

  { // compute pressure
    primitive_->enableShader(pressureShader_);

    swapBuffer(pressureBuffer_);
    pressureBuffer_.fbo->bind();
    pressureBuffer_.fbo->drawBufferMRT();
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    swapBuffer(pressureBuffer_);

    activateTexture(pressureShader_.get(), divergenceBuffer_.tex.get(), 1);
    activateTexture(pressureShader_.get(), obstaclesTexture_.get(), 2);
    if(levelSetTexture_.get() != NULL) {
      activateTexture(pressureShader_.get(), levelSetTexture_.get(), 3);
    }
    glActiveTexture(GL_TEXTURE0);

    for(int i = 0; i < numPressureIterations_; ++i) {
      pressureBuffer_.fbo->drawBufferMRT();
      pressureBuffer_.tex->bind();
      primitive_->draw();
      swapBuffer(pressureBuffer_);
    }
  }

  { // apply pressure on velocity field
    primitive_->enableShader(subtractPressureGradientShader_);
    velocityBuffer_.fbo->bind();
    velocityBuffer_.fbo->drawBufferMRT();

    activateTexture(subtractPressureGradientShader_.get(), velocityBuffer_.tex.get(), 0);
    activateTexture(subtractPressureGradientShader_.get(), pressureBuffer_.tex.get(), 1);
    activateTexture(subtractPressureGradientShader_.get(), obstaclesTexture_.get(), 2);
    if(levelSetTexture_.get() != NULL) {
      activateTexture(subtractPressureGradientShader_.get(), levelSetTexture_.get(), 3);
    }

    primitive_->draw();
    swapBuffer(velocityBuffer_);
  }
}

