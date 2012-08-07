
#include "../include/vorticity.h"
#include "../include/helper.h"
#include "../include/fluid.h"

string EulerianVorticity::vorticityTextureName_ = "vorticityTexture_";

/**
 * Compute vorticity omega = cross(NABLA, u)
 */
class ComputeVorticityShader : public ShaderFunctions {
public:
  ComputeVorticityShader(EulerianPrimitive *primitive)
  : ShaderFunctions("fluidVorticity", makeShaderArgs()),
    primitive_(primitive)
  {
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", EulerianFluid::VELOCITY } );
  }
  virtual string code() const {
    stringstream s;
    if(primitive_->is2D()) {
      s << "     ivec2 pos = ivec2(gl_FragCoord.xy);" << endl;
      s << "void " << myName_ << "(out vec2 outCol) {" << endl;
      s << findNeighborsGLSL("v", EulerianFluid::VELOCITY, primitive_->is2D());
      s << "     // using central differences: D0_x = (D+_x - D-_x) / 2;" << endl;
      s << "     outCol = 0.5*vec2(vN.y - vS.y, vE.x - vW.x);" << endl;
    } else {
      s << "     ivec3 pos = ivec3(gl_FragCoord.xyz);" << endl;
      s << "void " << myName_ << "(out vec3 outCol) {" << endl;
      s << findNeighborsGLSL("v", EulerianFluid::VELOCITY, primitive_->is2D());
      s << "     // using central differences: D0_x = (D+_x - D-_x) / 2;" << endl;
      s << "     outCol = 0.5*vec3( (( vN.z - vS.z ) - ( vF.y - vB.y )) ," << endl;
      s << "                        (( vF.x - vB.x ) - ( vE.z - vW.z )) ," << endl;
      s << "                        (( vE.y - vW.y ) - ( vN.x - vS.x )) );" << endl;
    }
    s << "}" << endl;
    return s.str();
  }
  EulerianPrimitive *primitive_;
};
class VorticityConfinementShader : public ShaderFunctions {
public:
  VorticityConfinementShader(EulerianPrimitive *primitive)
  : ShaderFunctions("fluidVorticity", makeShaderArgs()),
    primitive_(primitive)
  {
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", EulerianFluid::OBSTACLES } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", EulerianFluid::VORTICITY } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "vec2" : "vec3", "inverseGridSize" } );
    addUniform( (GLSLUniform) { "float", "deltaT" } );
    addUniform( (GLSLUniform) { "float", "vortConfinementScale" } );
  }
  virtual string code() const {
    stringstream s;

    if(primitive_->is2D()) {
      s << "void " << myName_ << "(out vec2 outCol) {" << endl;
      s << "    ivec2 pos = ivec2(gl_FragCoord.xy);" << endl;
    } else {
      s << "void " << myName_ << "(out vec3 outCol) {" << endl;
      s << "    ivec3 pos = ivec3(gl_FragCoord.xyz);" << endl;
    }

    s << "    // discard obstacle fragments" << endl;
    s << "    if (texelFetch(" <<
        EulerianFluid::OBSTACLES << ", pos, 0).x > 0) discard;" << endl;
    s << "    " << endl;

    s << "    vec4 omega = texelFetch(" <<
        EulerianFluid::VORTICITY << ", pos, 0);" << endl;
    s << findNeighborsGLSL("omega",
        EulerianFluid::VORTICITY, primitive_->is2D());
    s << "    " << endl;

    s << "    // compute normalized vorticity vector field psi:" << endl;
    s << "    //     psi = eta / |eta| , with eta = NABLA omega" << endl;
    if(primitive_->is2D()) {
      s << "    vec2 eta = 0.5 * vec2(length(omegaE.xy) - length(omegaW.xy)," << endl;
      s << "                          length(omegaN.xy) - length(omegaS.xy));" << endl;
      s << "    eta = normalize( eta + vec2(0.001) );" << endl;
    } else {
      s << "    vec3 eta = 0.5 * vec3(length(omegaE.xyz) - length(omegaW.xyz)," << endl;
      s << "                          length(omegaN.xyz) - length(omegaS.xyz)," << endl;
      s << "                          length(omegaF.xyz) - length(omegaB.xyz));" << endl;
      s << "    eta = normalize( eta + vec3(0.001) );" << endl;
    }

    s << "    // compute the vorticity force by:" << endl;
    s << "    //     f = epsilon * cross( psi, omega ) * delta_x" << endl;
    if(primitive_->is2D()) {
      s << "    outCol = vortConfinementScale  * eta.yx * omega.xy;" << endl;
    } else {
      s << "    outCol = vortConfinementScale * vec3(" << endl;
      s << "                   eta.y * omega.z - eta.z * omega.y," << endl;
      s << "                   eta.z * omega.x - eta.x * omega.z," << endl;
      s << "                   eta.x * omega.y - eta.y * omega.x);" << endl;
    }
    s << "}" << endl;
    return s.str();
  }
  EulerianPrimitive *primitive_;
};

EulerianVorticity::EulerianVorticity(EulerianPrimitive *primitive)
: EulerianStage(primitive),
  velocityBuffer_(),
  obstaclesTexture_(),
  vorticityBuffer_(),
  vortConfinementScale_( ref_ptr<UniformFloat>::manage(
      new UniformFloat("vortConfinementScale") ))
{
  vortConfinementScale_->set_value(0.12f);
  computeShader_ = primitive_->makeShader(
      ComputeVorticityShader(primitive_), primitive_->is2D() ? "vec2" : "vec3" );

  confinementShader_ = primitive_->makeShader(
      VorticityConfinementShader(primitive_), primitive_->is2D() ? "vec2" : "vec3" );
  confinementShader_->addUniform( primitive_->timeStep() );
  confinementShader_->addUniform( primitive_->inverseSize().get() );
  confinementShader_->addUniform( vortConfinementScale_.get() );
}

void EulerianVorticity::set_vorticityBuffer(const FluidBuffer &buffer)
{
  buffer.tex->set_name( EulerianFluid::VORTICITY );
  vorticityBuffer_ = buffer;
  confinementShader_->addTexture( buffer.tex.get() );
}
void EulerianVorticity::set_velocityBuffer(const FluidBuffer &buffer)
{
  buffer.tex->set_name( EulerianFluid::VELOCITY );
  velocityBuffer_ = buffer;
  computeShader_->addTexture( buffer.tex.get() );
}
void EulerianVorticity::set_obstaclesTexture(ref_ptr<Texture> tex)
{
  tex->set_name( EulerianFluid::OBSTACLES );
  obstaclesTexture_ = tex;
  confinementShader_->addTexture( obstaclesTexture_.get() );
}

void EulerianVorticity::update()
{
  { // compute vorticity using velocity field
    primitive_->enableShader(computeShader_);
    activateTexture(computeShader_.get(), velocityBuffer_.tex.get(), 0);
    vorticityBuffer_.fbo->bind();
    vorticityBuffer_.fbo->drawBufferMRT();
    primitive_->draw();
    swapBuffer(vorticityBuffer_);
  }

  { // change velocity field using vorticity
    primitive_->enableShader(confinementShader_);
    activateTexture(confinementShader_.get(), vorticityBuffer_.tex.get(), 0);
    activateTexture(confinementShader_.get(), obstaclesTexture_.get(), 1);
    glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);
    swapBuffer(velocityBuffer_); {
      velocityBuffer_.fbo->bind();
      velocityBuffer_.fbo->drawBufferMRT();
      primitive_->draw();
    } swapBuffer(velocityBuffer_);
    glDisable(GL_BLEND);
  }
}
