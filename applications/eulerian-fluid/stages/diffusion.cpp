
#include "../include/diffusion.h"
#include "../include/helper.h"

class DiffusionShader : public ShaderFunctions {
public:
  DiffusionShader(EulerianPrimitive *primitive)
  : ShaderFunctions("fluidDiffusion", makeShaderArgs()),
    primitive_(primitive)
  {
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", "initialTex" } );
    addUniform( (GLSLUniform) { primitive_->is2D() ?
        "sampler2D" : "sampler3D", "currentTex" } );
    addUniform( (GLSLUniform) { "float", "deltaT" } );
    addUniform( (GLSLUniform) { "float", "viscosity" } );
  }
  virtual string code() const {
    stringstream s;
    s << "void " << myName_ << "(out vec4 outCol)" << endl;
    s << "{" << endl;
    if(primitive_->is2D()) {
      s << "     ivec2 pos = ivec2(gl_FragCoord.xy);" << endl;
    } else {
      s << "     ivec3 pos = ivec3(gl_FragCoord.xyz);" << endl;
    }
    s << "     " << endl;
    s << findNeighborsGLSL("phin", "currentTex", primitive_->is2D());
    s << "     vec4 phiC = texelFetch(initialTex, pos, 0);" << endl;
    s << "     " << endl;
    s << "     float dX = 1;" << endl;
    if(primitive_->is2D()) {
      s << "     vec4 sum = phinN + phinE + phinS + phinW;" << endl;
    } else {
      s << "     vec4 sum = phinN + phinE + phinS + phinW + phinF + phinB;" << endl;
    }
    s << "     outCol = ( (phiC * dX*dX) - (dT * viscosity * sum) )";
    s << "                  / ((6 * deltaT * viscosity) + (dX*dX));" << endl;
    s << "     " << endl;
    s << "}" << endl;
    return s.str();
  }
  EulerianPrimitive *primitive_;
};

EulerianDiffusion::EulerianDiffusion(EulerianPrimitive *primitive)
: EulerianStage(primitive)
{
  diffusionShader_ = primitive_->makeShader(
      DiffusionShader(primitive_), "vec4" );
  diffusionShader_->addUniform( primitive_->timeStep() );
  viscosityLoc_ = glGetUniformLocation(
      diffusionShader_->id(), "viscosity");
  initialTexLoc_ = glGetUniformLocation(
      diffusionShader_->id(), "initialTex");
  currentTexLoc_ = glGetUniformLocation(
      diffusionShader_->id(), "currentTex");

  tmpBuffer_ = createSlab(primitive_, 4, 1, primitive_->useHalfFloats());
}

void EulerianDiffusion::addDiffusionTarget(ref_ptr<DiffusionTarget> tex)
{
  diffusionTargets_.push_back(tex);
}
void EulerianDiffusion::removeDiffusionTarget(ref_ptr<DiffusionTarget> tex)
{
  diffusionTargets_.remove(tex);
}

void EulerianDiffusion::update()
{
  primitive_->enableShader(diffusionShader_);
  glUniform1i(initialTexLoc_, 0);
  glUniform1i(currentTexLoc_, 1);

  for(list< ref_ptr<DiffusionTarget> >::iterator it=diffusionTargets_.begin();
      it!=diffusionTargets_.end(); ++it)
  {
#define DRAW(t0, t1, buf) \
    glActiveTexture(GL_TEXTURE0); t0->bind(); \
    glActiveTexture(GL_TEXTURE1); t1->bind(); \
    buf.fbo->bind(); \
    buf.fbo->drawBufferMRT(); \
    primitive_->draw();

    ref_ptr<DiffusionTarget> target = *it;

    glUniform1f(viscosityLoc_, target->viscosity);

    if(target->numIterations==1) {
      DRAW(target->buffer.tex, target->buffer.tex, target->buffer);
      swapBuffer(target->buffer);
      continue;
    }

    // first step, target->buffer contains initial buffer
    DRAW(target->buffer.tex, target->buffer.tex, tmpBuffer_);
    // second step, target->buffer contains initial buffer and tmp buffer1
    DRAW(target->buffer.tex, tmpBuffer_.tex, target->buffer);
    swapBuffer(target->buffer);

    glActiveTexture(GL_TEXTURE0); tmpBuffer_.tex->bind();
    glActiveTexture(GL_TEXTURE1);
    for(int i=2; i<target->numIterations; ++i) {
      target->buffer.tex->bind();
      target->buffer.fbo->bind();
      target->buffer.fbo->drawBufferMRT();
      primitive_->draw();
      swapBuffer(target->buffer);
    }
#undef DRAW
  }
}
