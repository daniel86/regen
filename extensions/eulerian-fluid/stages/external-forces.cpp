
#include "../include/external-forces.h"
#include "../include/smoke.h"
#include "../include/liquid.h"
#include "../include/helper.h"

class SplatShader : public ShaderFunctions {
public:
  SplatShader(EulerianPrimitive *primitive)
  : ShaderFunctions("fluidSplat", makeShaderArgs()),
    primitive_(primitive)
  {
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", EulerianFluid::OBSTACLES } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", "splatTexture" } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "vec2" : "vec3", "splatPoint" } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "vec2" : "vec3", "inverseGridSize" } );
    addUniform( (GLSLUniform) { "float", "deltaT" } );
    addUniform( (GLSLUniform) { "float", "splatRadius" } );
    addUniform( (GLSLUniform) { "float", "width"} );
    addUniform( (GLSLUniform) { "float", "height" } );
    addUniform( (GLSLUniform) { "int", "splatMode" } );
    addUniform( (GLSLUniform) { "vec3", "fillColor" } );
    addDependencyCode("isNonEmptyCell",
        isNonEmptyCellGLSL(primitive_->is2D()));
  }
  virtual string code() const {
    stringstream s;
    s << "void " << myName_ << "(out vec4 outCol)" << endl;
    s << "{" << endl;
    if(primitive_->is2D()) {
      s << "     vec2 fragCoord = inverseGridSize*gl_FragCoord.xy;" << endl;
    } else {
      s << "     vec3 fragCoord = inverseGridSize*vec3(gl_FragCoord.xy,f_layer);" << endl;
    }

    s << "     if( isNonEmptyCell(fragCoord) ) discard;" << endl;
    s << endl;

    s << "     if(splatMode == 0) { // splat a circle" << endl;
    if(primitive_->is2D()) {
      s << "         float dist = distance(splatPoint, gl_FragCoord.xy);" << endl;
    } else {
      s << "         float dist = distance(splatPoint, vec3(gl_FragCoord.xy,f_layer));" << endl;
    }
    s << "         if (dist > splatRadius) discard;" << endl;
    s << "         outCol = vec4(deltaT * fillColor, exp( -dist*dist ) );" << endl;
    s << "     } else if( splatMode == 1 ) { // splat a texture" << endl;
    if(primitive_->is2D()) {
      s << "         float val = texture(splatTexture, vec2(fragCoord.x,-fragCoord.y)).r;" << endl;
    } else {
      s << "         float val = texture(splatTexture, vec3(fragCoord.x,-fragCoord.y,fragCoord.z)).r;" << endl;
    }
    s << "         if (val <= 0.0) discard;" << endl;
    s << "         outCol = vec4(deltaT * fillColor, 1.0f );" << endl;
    s << "     } else { // splat a rectangle" << endl;
    s << "         if (abs(gl_FragCoord.x-splatPoint.x) > 0.5*width) discard;" << endl;
    s << "         if (abs(gl_FragCoord.y-splatPoint.y) > 0.5*height) discard;" << endl;
    s << "         outCol = vec4(deltaT * fillColor, 1.0f );" << endl;
    s << "     }" << endl;
    s << "}" << endl;
    return s.str();
  }
  EulerianPrimitive *primitive_;
};

EulerianSplat::EulerianSplat(EulerianPrimitive *primitive)
: EulerianStage(primitive)
{
  splatShader_ = primitive->makeShader( SplatShader(primitive), "vec4" );
  splatShader_->addUniform( primitive->inverseSize().get() );
  splatShader_->addUniform( primitive->timeStep() );
  positionLoc_ = glGetUniformLocation(splatShader_->id(), "splatPoint");
  radiusLoc_ = glGetUniformLocation(splatShader_->id(), "splatRadius");
  splatModeLoc_ = glGetUniformLocation(splatShader_->id(), "splatMode");
  widthLoc_ = glGetUniformLocation(splatShader_->id(), "width");
  heightLoc_ = glGetUniformLocation(splatShader_->id(), "height");
  fillColorLoc_ = glGetUniformLocation(splatShader_->id(), "fillColor");
  splatTextureLoc_ = glGetUniformLocation(splatShader_->id(), "splatTexture");
}

void EulerianSplat::addSplatSource(ref_ptr<SplatSource> source)
{
  slpatSources_.push_back(source);
}
void EulerianSplat::removeSplatSource(ref_ptr<SplatSource> source)
{
  slpatSources_.remove(source);
}

void EulerianSplat::set_obstaclesTexture(ref_ptr<Texture> tex)
{
  tex->set_name( EulerianFluid::OBSTACLES );
  obstaclesTexture_ = tex;
  splatShader_->addTexture( obstaclesTexture_.get() );
}

void EulerianSplat::update()
{
  primitive_->enableShader(splatShader_);

  activateTexture(splatShader_.get(), obstaclesTexture_.get(), 1);

  glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);
  for(list< ref_ptr<SplatSource> >::iterator it=slpatSources_.begin();
      it!=slpatSources_.end(); ++it)
  {
    ref_ptr<SplatSource> source = *it;

    if(primitive_->is2D()) {
      glUniform2fv(positionLoc_, 1, &source->pos.x);
    } else {
      glUniform3fv(positionLoc_, 1, &source->pos.x);
    }
    glUniform1i(splatModeLoc_, source->mode);
    glUniform1f(radiusLoc_, source->radius);
    glUniform1f(widthLoc_, source->width);
    glUniform1f(heightLoc_, source->height);
    glUniform3fv(fillColorLoc_, 1, &source->value.x);
    glUniform1i(splatTextureLoc_, 0);

    if(source->tex.get() != NULL) {
      glActiveTexture(GL_TEXTURE0); source->tex->bind();
    }

    swapBuffer(source->buffer); {
      source->buffer.fbo->bind();
      source->buffer.fbo->drawBufferMRT();
      primitive_->draw();
    } swapBuffer(source->buffer);
  }
  glDisable(GL_BLEND);
}


///////////////


class GravityShader : public ShaderFunctions {
public:
  GravityShader(EulerianPrimitive *primitive)
  : ShaderFunctions("fluidGravity", makeShaderArgs()),
    primitive_(primitive)
  {
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", EulerianFluid::OBSTACLES } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "vec2" : "vec3", "inverseGridSize" } );
    addUniform( (GLSLUniform) { "vec3", "gravity" } );
    addUniform( (GLSLUniform) { "float", "deltaT" } );
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
      s << "     vec2 fragCoord = inverseGridSize*gl_FragCoord.xy;" << endl;
    } else {
      s << "     vec3 fragCoord = inverseGridSize*gl_FragCoord.xyz;" << endl;
    }

    if(primitive_->isLiquid()) {
      s << "     if( isOutsideSimulationDomain(fragCoord) ) discard;" << endl;
    }
    s << "     if( isNonEmptyCell(fragCoord) ) discard;" << endl;
    s << endl;
    s << "     else outCol = vec4(deltaT * gravity, 1.0);" << endl;
    s << "}" << endl;
    return s.str();
  }
  EulerianPrimitive *primitive_;
};

EulerianGravity::EulerianGravity(EulerianPrimitive *primitive)
: EulerianStage(primitive)
{
  gravityShader_ = primitive->makeShader( GravityShader(primitive), "vec4" );
  gravityLoc_ = glGetUniformLocation(gravityShader_->id(), "gravity");
  gravityShader_->addUniform( primitive->timeStep() );
  gravityShader_->addUniform( primitive->inverseSize().get() );
}

void EulerianGravity::set_velocityBuffer(const FluidBuffer &buffer)
{
  velocityBuffer_ = buffer;
}
void EulerianGravity::set_obstaclesTexture(ref_ptr<Texture> tex)
{
  tex->set_name( EulerianFluid::OBSTACLES );
  obstaclesTexture_ = tex;
  gravityShader_->addTexture( obstaclesTexture_.get() );
}
void EulerianGravity::set_levelSetTexture(ref_ptr<Texture> tex)
{
  tex->set_name( EulerianLiquid::LEVEL_SET );
  levelSetTexture_ = tex;
  gravityShader_->addTexture( levelSetTexture_.get() );
}

void EulerianGravity::addGravitySource(const Vec3f &gravity)
{
  gravitySources_.push_back(gravity);
}

void EulerianGravity::update()
{
  primitive_->enableShader(gravityShader_);

  activateTexture(gravityShader_.get(), obstaclesTexture_.get(), 0);
  if(levelSetTexture_.get() != NULL) {
    activateTexture(gravityShader_.get(), levelSetTexture_.get(), 1);
  }

  // use additive blending for gravity
  glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);

  swapBuffer(velocityBuffer_); {
    velocityBuffer_.fbo->bind();
    velocityBuffer_.fbo->drawBufferMRT();
    for(list< Vec3f >::iterator it=gravitySources_.begin();
        it!=gravitySources_.end(); ++it)
    {
      glUniform3fv(gravityLoc_, 1, &it->x);
      primitive_->draw();
    }
  } swapBuffer(velocityBuffer_);

  glDisable(GL_BLEND);
}

//////////////

class LiquidStreamShader : public ShaderFunctions {
public:
  LiquidStreamShader(EulerianPrimitive *primitive)
  : ShaderFunctions("fluidDerivativeVelocityShader", makeShaderArgs()),
    primitive_(primitive)
  {
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", EulerianFluid::OBSTACLES } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "vec2" : "vec3", "inverseGridSize" } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "vec2" : "vec3", "streamCenter" } );
    addUniform( (GLSLUniform) { "vec3", "streamColor" } );
    addUniform( (GLSLUniform) { "float", "streamRadius" } );
    addUniform( (GLSLUniform) { "bool", "outputColor" } );
    addDependencyCode("isNonEmptyCell",
        isNonEmptyCellGLSL(primitive_->is2D()));
  }
  virtual string code() const {
    stringstream s;
    s << "void " << myName_ << "(out vec4 outCol)" << endl;
    s << "{" << endl;
    if(primitive_->is2D()) {
      s << "     if( isNonEmptyCell(inverseGridSize*gl_FragCoord.xy) ) discard;" << endl;
    } else {
      s << "     if( isNonEmptyCell(inverseGridSize*gl_FragCoord.xyz) ) discard;" << endl;
    }
    s << endl;

    if(primitive_->is2D()) {
      s << "     float dist = length(gl_FragCoord.xy - streamCenter);" << endl;
    } else {
      s << "     float dist = length(gl_FragCoord.xyz - streamCenter);" << endl;
    }
    s << "     if( dist > streamRadius ) discard;" << endl;
    s << endl;
    s << "     if( outputColor ) outCol.rgb = streamColor;" << endl;
    s << "     else outCol.r = (dist - streamRadius);" << endl;
    s << "}" << endl;
    return s.str();
  }
  EulerianPrimitive *primitive_;
};

EulerianLiquidStream::EulerianLiquidStream(EulerianPrimitive *primitive)
: EulerianStage(primitive)
{
  liquidStreamShader_ = primitive_->makeShader(
      LiquidStreamShader(primitive), "vec4" );
  liquidStreamShader_->addUniform(
      primitive_->inverseSize().get() );
  streamColorLoc_ = glGetUniformLocation(
      liquidStreamShader_->id(), "streamColor");
  streamCenterLoc_ = glGetUniformLocation(
      liquidStreamShader_->id(), "streamCenter");
  streamRadiusLoc_ = glGetUniformLocation(
      liquidStreamShader_->id(), "streamRadius");
  outputColorLoc_ = glGetUniformLocation(
      liquidStreamShader_->id(), "outputColor");
}

void EulerianLiquidStream::set_velocityBuffer(const FluidBuffer &buffer)
{
  velocityBuffer_ = buffer;
}
void EulerianLiquidStream::set_levelSetBuffer(const FluidBuffer &buffer)
{
  levelSetBuffer_ = buffer;
}
void EulerianLiquidStream::set_obstaclesTexture(ref_ptr<Texture> tex)
{
  tex->set_name( EulerianFluid::OBSTACLES );
  obstaclesTexture_ = tex;
  liquidStreamShader_->addTexture( obstaclesTexture_.get() );
}

void EulerianLiquidStream::update()
{
  primitive_->enableShader(liquidStreamShader_);

  // first update level set
  swapBuffer(levelSetBuffer_);
  levelSetBuffer_.fbo->bind();
  levelSetBuffer_.fbo->drawBufferMRT();
  glUniform1i(outputColorLoc_, 0);
  for(list< ref_ptr<LiquidStreamSource> >::iterator it=streamSources_.begin();
      it!=streamSources_.end(); ++it)
  {
    ref_ptr<LiquidStreamSource> source = *it;
    glUniform2fv(streamCenterLoc_, 1, &source->center.x);
    glUniform1f(streamRadiusLoc_, source->radius);
    primitive_->draw();
  }
  swapBuffer(levelSetBuffer_);

  glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);
  // then the velocity field
  swapBuffer(velocityBuffer_);
  velocityBuffer_.fbo->bind();
  velocityBuffer_.fbo->drawBufferMRT();
  glUniform1i(outputColorLoc_, 1);
  for(list< ref_ptr<LiquidStreamSource> >::iterator it=streamSources_.begin();
      it!=streamSources_.end(); ++it)
  {
    ref_ptr<LiquidStreamSource> source = *it;
    glUniform2fv(streamCenterLoc_, 1, &source->center.x);
    glUniform1f(streamRadiusLoc_, source->radius);
    glUniform3fv(streamColorLoc_, 1, &source->velocity.x);
    primitive_->draw();
  }
  swapBuffer(velocityBuffer_);
  glDisable(GL_BLEND);
}

///////////

class InjectLiquidShader : public ShaderFunctions {
public:
  InjectLiquidShader() : ShaderFunctions("injectLiquid", makeShaderArgs()) {
    addUniform( (GLSLUniform) { "float", "liquidHeight" } );
  }
  virtual string code() const {
    stringstream s;
    s << "void " << myName_ << "(out float outCol) {" << endl;
    s << "     float d = gl_FragCoord.y - liquidHeight;" << endl;
    s << "     outCol = (d < 0 ? 0.001*d : 0.0);" << endl;
    s << "}" << endl;
    return s.str();
  }
};

InjectLiquidStage::InjectLiquidStage(
    EulerianPrimitive *primitive, UniformFloat *liquidHeight_)
: EulerianStage(primitive)
{
  injectLiquidShader_ = primitive_->makeShader( InjectLiquidShader(), "float" );
  injectLiquidShader_->addUniform(liquidHeight_);
}

void InjectLiquidStage::set_levelSetBuffer(const FluidBuffer &buffer)
{
  levelSetBuffer_ = buffer;
}

void InjectLiquidStage::update()
{
  primitive_->enableShader(injectLiquidShader_);
  glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);
  swapBuffer(levelSetBuffer_); {
    levelSetBuffer_.fbo->bind();
    levelSetBuffer_.fbo->drawBufferMRT();
    primitive_->draw();
  } swapBuffer(levelSetBuffer_);
  glDisable(GL_BLEND);
}

