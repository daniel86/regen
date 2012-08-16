
#include "../include/advect.h"
#include "../include/liquid.h"
#include "../include/helper.h"

class AdvectShader : public ShaderFunctions {
public:
  AdvectShader(EulerianPrimitive *primitive)
  : ShaderFunctions("fluidAdvect", makeShaderArgs()),
    primitive_(primitive)
  {
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", EulerianFluid::VELOCITY } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", EulerianFluid::OBSTACLES } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", "quantityTexture" } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "vec2" : "vec3", "inverseGridSize" } );
    addUniform( (GLSLUniform) { "float", "deltaT" } );
    addUniform( (GLSLUniform) { "float", "quantityLoss" } );
    addUniform( (GLSLUniform) { "float", "decayAmount" } );
    addUniform( (GLSLUniform) { "int", "treatAsLiquid" } );
    if(primitive_->isLiquid()) {
      addDependencyCode("isOutsideSimulationDomain",
          isOutsideSimulationDomainGLSL(primitive_->is2D()));
      addUniform( (GLSLUniform) { primitive_->is2D() ?
          "sampler2D" : "sampler3D", EulerianLiquid::LEVEL_SET } );
    }
    addDependencyCode("isNonEmptyCell", isNonEmptyCellGLSL(primitive_->is2D()));
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

    if(primitive_->isLiquid()) {
      s << "     if( treatAsLiquid==1 && isOutsideSimulationDomain(fragCoord) ) {" << endl;
      s << "         outCol = texture(" <<
          EulerianFluid::VELOCITY << ", fragCoord); return;" << endl;
      s << "         return;" << endl;
      s << "     }" << endl;
    }
    s << "     if( isNonEmptyCell(fragCoord) ) { outCol = vec4(0); return; }" << endl;
    s << endl;

    s << "     // follow the velocity field 'back in time'" << endl;
    if(primitive_->is2D()) {
      s << "     vec2 pos = gl_FragCoord.xy - deltaT * texture(" <<
          EulerianFluid::VELOCITY << ", fragCoord).xy;" << endl;
    } else {
      s << "     vec3 pos = vec3(gl_FragCoord.xy,f_layer) - deltaT * texture(" <<
          EulerianFluid::VELOCITY << ", fragCoord).xyz;" << endl;
    }
    s << endl;

    s << "     outCol = decayAmount * texture(quantityTexture, inverseGridSize * pos);" << endl;
    s << "     if(quantityLoss>0.0) outCol -= quantityLoss*deltaT;" << endl;
    s << "     " << endl;
    s << "}" << endl;
    return s.str();
  }
  EulerianPrimitive *primitive_;
};

class AdvectMacCormackShader : public ShaderFunctions {
public:
  AdvectMacCormackShader(EulerianPrimitive *primitive)
  : ShaderFunctions("fluidAdvectMacCormack", makeShaderArgs()),
    primitive_(primitive)
  {
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", EulerianFluid::VELOCITY } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", EulerianFluid::OBSTACLES } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", "quantityTexture" } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", "quantityTextureHat" } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "vec2" : "vec3", "inverseGridSize" } );
    addUniform( (GLSLUniform) { "float", "deltaT" } );
    addUniform( (GLSLUniform) { "float", "quantityLoss" } );
    addUniform( (GLSLUniform) { "float", "decayAmount" } );
    addUniform( (GLSLUniform) { "int", "treatAsLiquid" } );
    if(primitive_->isLiquid()) {
      addDependencyCode("isOutsideSimulationDomain",
          isOutsideSimulationDomainGLSL(primitive_->is2D()));
      addUniform( (GLSLUniform) { primitive_->is2D() ?
          "sampler2D" : "sampler3D", EulerianLiquid::LEVEL_SET } );
    }
    addDependencyCode("isNonEmptyCell", isNonEmptyCellGLSL(primitive_->is2D()));
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
      s << "     if( treatAsLiquid==1 && isOutsideSimulationDomain(fragCoord) ) {" << endl;
      s << "         outCol = vec4(0); return;" << endl;
      s << "     }" << endl;
    }
    s << "     if( isNonEmptyCell(fragCoord) ) { outCol = vec4(0); return; }" << endl;
    s << endl;

    s << "     // follow the velocity field 'back in time'" << endl;
    if(primitive_->is2D()) {
      s << "     vec2 pos = gl_FragCoord.xy - deltaT * texture(" <<
          EulerianFluid::VELOCITY << ", fragCoord).xy;" << endl;
    } else {
      s << "     vec3 pos = gl_FragCoord.xyz - deltaT * texture(" <<
          EulerianFluid::VELOCITY << ", fragCoord).xyz;" << endl;
    }
    s << endl;

    s << "     // convert new position to texture coordinates" << endl;
    if(primitive_->is2D()) {
      s << "     vec2 nposTC = inverseGridSize * pos;" << endl;
    } else {
      s << "     vec3 nposTC = inverseGridSize * pos;" << endl;
    }
    s << endl;

    s << "     // find the texel corner closest to the semi-Lagrangian 'particle'" << endl;
    if(primitive_->is2D()) {
      s << "     vec2 nposTexel = floor( pos + vec2( 0.5f ) );" << endl;
      s << "     vec2 nposTexelTC = inverseGridSize*nposTexel;" << endl;
    } else {
      s << "     vec3 nposTexel = floor( pos + vec3( 0.5f ) );" << endl;
      s << "     vec3 nposTexelTC = inverseGridSize*nposTexel;" << endl;
    }
    s << endl;

    s << "     // ht (half-texel)" << endl;
    if(primitive_->is2D()) {
      s << "     vec2 ht = 0.5*inverseGridSize;" << endl;
    } else {
      s << "     vec3 ht = 0.5*inverseGridSize;" << endl;
    }
    s << endl;

    s << "     // get the values of nodes that contribute to the interpolated value" << endl;
    s << "     // (texel centers are at half-integer locations)" << endl;
    if(primitive_->is2D()) {
      s << "     vec4 nodeValues[4];" << endl;
      s << "     nodeValues[0] = texture( quantityTexture,";
      s << "            nposTexelTC + vec2(-ht.x, -ht.y) );" << endl;
      s << "     nodeValues[1] = texture( quantityTexture," << endl;
      s << "            nposTexelTC + vec2(-ht.x,  ht.y) );" << endl;
      s << "     nodeValues[2] = texture( quantityTexture," << endl;
      s << "            nposTexelTC + vec2( ht.x, -ht.y) );" << endl;
      s << "     nodeValues[3] = texture( quantityTexture," << endl;
      s << "            nposTexelTC + vec2( ht.x,  ht.y) );" << endl;
      s << endl;
    } else {
      s << "     vec4 nodeValues[8];" << endl;
      s << "     nodeValues[0] = texture( quantityTexture," << endl;
      s << "            nposTexelTC + vec3(-ht.x, -ht.y, -ht.z) );" << endl;
      s << "     nodeValues[1] = texture( quantityTexture," << endl;
      s << "            nposTexelTC + vec3(-ht.x, -ht.y,  ht.z) );" << endl;
      s << "     nodeValues[2] = texture( quantityTexture," << endl;
      s << "            nposTexelTC + vec3(-ht.x,  ht.y, -ht.z) );" << endl;
      s << "     nodeValues[3] = texture( quantityTexture," << endl;
      s << "            nposTexelTC + vec3(-ht.x,  ht.y,  ht.z) );" << endl;
      s << "     nodeValues[4] = texture( quantityTexture," << endl;
      s << "            nposTexelTC + vec3( ht.x, -ht.y, -ht.z) );" << endl;
      s << "     nodeValues[5] = texture( quantityTexture," << endl;
      s << "            nposTexelTC + vec3( ht.x, -ht.y,  ht.z) );" << endl;
      s << "     nodeValues[6] = texture( quantityTexture," << endl;
      s << "            nposTexelTC + vec3( ht.x,  ht.y, -ht.z) );" << endl;
      s << "     nodeValues[7] = texture( quantityTexture," << endl;
      s << "            nposTexelTC + vec3( ht.x, ht.y,  ht.z) );" << endl;
      s << endl;
    }

    s << "     // determine a valid range for the result" << endl;
    s << "     vec4 phiMin = min(min(min(nodeValues[0]," << endl;
    s << "            nodeValues [1]), nodeValues [2]), nodeValues [3]);" << endl;
    if(!primitive_->is2D()) {
      s << "     phiMin = min(min(min(min(phiMin, nodeValues [4])," << endl;
      s << "            nodeValues [5]), nodeValues [6]), nodeValues [7]);" << endl;
    }
    s << endl;

    s << "     vec4 phiMax = max(max(max(nodeValues[0]," << endl;
    s << "            nodeValues [1]), nodeValues [2]), nodeValues [3]);" << endl;
    if(!primitive_->is2D()) {
      s << "     phiMax = max(max(max(max(phiMax, nodeValues [4])," << endl;
      s << "            nodeValues [5]), nodeValues [6]), nodeValues [7]);" << endl;
    }
    s << endl;

    s << "     // Perform final MACCORMACK advection step" << endl;
    s << "     outCol = texture( quantityTexture, nposTC, 0)" << endl;
    s << "         + 0.5 * ( texture( quantityTexture, fragCoord ) -" << endl;
    s << "                   texture( quantityTextureHat, fragCoord ) );" << endl;
    s << endl;

    s << "     // clamp result to the desired range" << endl;
    s << "     outCol = max( min( outCol, phiMax ), phiMin ) * decayAmount;" << endl;
    s << "     if(quantityLoss>0.0) outCol -= quantityLoss*deltaT;" << endl;
    s << endl;

    s << "}" << endl;
    return s.str();
  }
  EulerianPrimitive *primitive_;
};

EulerianAdvection::EulerianAdvection(EulerianPrimitive *primitive)
: EulerianStage(primitive),
  velocityTexture_(),
  obstaclesTexture_(),
  useMacCormack_(false)
{
  advectShader_ = primitive_->makeShader( AdvectShader(primitive_), "vec4" );
  advectShader_->addUniform( primitive_->timeStep() );
  advectShader_->addUniform( primitive_->inverseSize().get() );
  decayAmountLoc_ = glGetUniformLocation(
      advectShader_->id(), "decayAmount");
  quantityLossLoc_ = glGetUniformLocation(
      advectShader_->id(), "quantityLoss");
  advectSourceLoc_ = glGetUniformLocation(
      advectShader_->id(), "quantityTexture");
  treatAsLiquidLoc_ = glGetUniformLocation(
      advectShader_->id(), "treatAsLiquid");

  advectMacCormackShader_ = primitive_->makeShader(
      AdvectMacCormackShader(primitive_), "vec4" );
  advectMacCormackShader_->addUniform( primitive_->timeStep() );
  advectMacCormackShader_->addUniform( primitive_->inverseSize().get() );
  decayAmountLoc2_ = glGetUniformLocation(
      advectMacCormackShader_->id(), "decayAmount");
  quantityLossLoc2_ = glGetUniformLocation(
      advectMacCormackShader_->id(), "quantityLoss");
  advectSourceLoc2_ = glGetUniformLocation(
      advectMacCormackShader_->id(), "quantityTexture");
  advectSourceHatLoc2_ = glGetUniformLocation(
      advectMacCormackShader_->id(), "quantityTextureHat");
  treatAsLiquidLoc2_ = glGetUniformLocation(
      advectMacCormackShader_->id(), "treatAsLiquid");

  // create tmp buffer for mac cormack advection
  tmpBuffer_ = createSlab(
      primitive, 4, 1,
      primitive->useHalfFloats());
}

void EulerianAdvection::set_velocityTexture(ref_ptr<Texture> tex)
{
  tex->set_name( EulerianFluid::VELOCITY );
  velocityTexture_ = tex;
  advectShader_->addTexture( velocityTexture_.get() );
  advectMacCormackShader_->addTexture( velocityTexture_.get() );
}
void EulerianAdvection::set_obstaclesTexture(ref_ptr<Texture> tex)
{
  tex->set_name( EulerianFluid::OBSTACLES );
  obstaclesTexture_ = tex;
  advectShader_->addTexture( obstaclesTexture_.get() );
  advectMacCormackShader_->addTexture( obstaclesTexture_.get() );
}
void EulerianAdvection::set_levelSetTexture(ref_ptr<Texture> tex)
{
  tex->set_name( EulerianLiquid::LEVEL_SET );
  levelSetTexture_ = tex;
  advectShader_->addTexture( levelSetTexture_.get() );
  advectMacCormackShader_->addTexture( levelSetTexture_.get() );
}

void EulerianAdvection::addAdvectionTarget(ref_ptr<AdvectionTarget> tex)
{
  advectionTagets_.push_back(tex);
}
void EulerianAdvection::removeAdvectionTarget(ref_ptr<AdvectionTarget> tex)
{
  advectionTagets_.remove(tex);
}

void EulerianAdvection::update()
{
  if(useMacCormack_) {
    advectMacCormack();
  } else { // simple advection
    advect();
  }
}

void EulerianAdvection::advectMacCormack() {
  glActiveTexture(GL_TEXTURE3);
  for(list< ref_ptr<AdvectionTarget> >::iterator it=advectionTagets_.begin();
      it!=advectionTagets_.end(); ++it)
  {
    ref_ptr<AdvectionTarget> target = *it;
    float dt = primitive_->timeStep()->value();

    primitive_->enableShader(advectShader_);
    glUniform1i(treatAsLiquidLoc_, target->treatAsLiquid);
    glUniform1f(quantityLossLoc_, target->quantityLoss);
    glUniform1f(decayAmountLoc_, 1.0f);
    glUniform1i(advectSourceLoc_, 3);

    { // advect backward -dt to get \bar{\phi} lifetime set to 1.0
      primitive_->timeStep()->set_value(-dt);
      target->buffer.tex->bind();
      // draw to tmp texture
      tmpBuffer_.fbo->bind();
      tmpBuffer_.fbo->drawBufferMRT();
      primitive_->draw();
    }

    target->buffer.fbo->bind();

    { // advect forward +dt to get \phi^(n+1) lifetime set to 1.0
      primitive_->timeStep()->set_value(dt);
      target->buffer.tex->bind();
      target->buffer.fbo->drawBufferMRT();
      primitive_->draw();
      swapBuffer(target->buffer);
    }

    {  // now call mac cormack using \phi^(n+1) and \bar{\phi}
      primitive_->enableShader(advectMacCormackShader_);
      glUniform1i(treatAsLiquidLoc2_, target->treatAsLiquid);
      glUniform1f(quantityLossLoc2_, target->quantityLoss);
      glUniform1f(decayAmountLoc2_, target->decayAmount);
      glUniform1i(advectSourceLoc2_, 3);
      glUniform1i(advectSourceHatLoc2_, 4);

      glActiveTexture(GL_TEXTURE4); tmpBuffer_.tex->bind();
      glActiveTexture(GL_TEXTURE3); target->buffer.tex->bind();
      target->buffer.fbo->drawBufferMRT();
      primitive_->draw();
      swapBuffer(target->buffer);
    }

    // make sure to bind updated texture
    if(target->buffer.tex.get() == velocityTexture_.get()) {
      glActiveTexture(GL_TEXTURE0); velocityTexture_->bind();
      glActiveTexture(GL_TEXTURE3);
    } else if(target->buffer.tex.get() == obstaclesTexture_.get()) {
      glActiveTexture(GL_TEXTURE1); obstaclesTexture_->bind();
      glActiveTexture(GL_TEXTURE3);
    } else if(target->buffer.tex.get() == levelSetTexture_.get()) {
      glActiveTexture(GL_TEXTURE2); levelSetTexture_->bind();
      glActiveTexture(GL_TEXTURE3);
    }
  }
}

void EulerianAdvection::advect()
{
  primitive_->enableShader(advectShader_);
  glUniform1i(advectSourceLoc_, 1);

  activateTexture(advectShader_.get(), velocityTexture_.get(), 0);
  activateTexture(advectShader_.get(), obstaclesTexture_.get(), 2);
  if(levelSetTexture_.get() != NULL) {
    activateTexture(advectShader_.get(), levelSetTexture_.get(), 3);
  }
  glActiveTexture(GL_TEXTURE1);

  for(list< ref_ptr<AdvectionTarget> >::iterator it=advectionTagets_.begin();
      it!=advectionTagets_.end(); ++it)
  {
    ref_ptr<AdvectionTarget> target = *it;
    target->buffer.tex->bind();
    target->buffer.fbo->bind();
    target->buffer.fbo->drawBufferMRT();
    glUniform1i(treatAsLiquidLoc_, target->treatAsLiquid ? 1 : 0);
    glUniform1f(decayAmountLoc_, target->decayAmount);

    glUniform1f(quantityLossLoc_, target->quantityLoss);
    primitive_->draw();
    swapBuffer(target->buffer);

    if(target->buffer.tex.get() == velocityTexture_.get()) {
      glActiveTexture(GL_TEXTURE0); velocityTexture_->bind();
      glActiveTexture(GL_TEXTURE1);
    } else if(target->buffer.tex.get() == obstaclesTexture_.get()) {
      glActiveTexture(GL_TEXTURE2); obstaclesTexture_->bind();
      glActiveTexture(GL_TEXTURE1);
    } else if(target->buffer.tex.get() == levelSetTexture_.get()) {
      glActiveTexture(GL_TEXTURE3); levelSetTexture_->bind();
      glActiveTexture(GL_TEXTURE1);
    }
  }
}
