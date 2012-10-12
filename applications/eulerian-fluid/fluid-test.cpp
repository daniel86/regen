/*
 * fluid-test.cpp
 *
 *  Created on: 06.10.2012
 *      Author: daniel
 */

#include "fluid-parser.h"
#include "fluid-animation.h"

#include <ogle/external/glsw/glsw.h>
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/quad.h>
#include <ogle/textures/image-texture.h>
#include <ogle/animations/animation-manager.h>
#include <applications/glut-render-tree.h>

int main(int argc, char** argv)
{
  GlutRenderTree *application = new GlutRenderTree(
      argc, argv,
      "Fluid simulation",
      800,800,
      GLUT_RGB,
      ref_ptr<RenderTree>(),
      ref_ptr<RenderState>(),
      GL_FALSE
      );
  ref_ptr<FBOState> fboState = application->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_NONE,
      GL_FALSE,
      GL_FALSE,
      Vec4f(0.0f)
  );
  MeshState *textureQuad = application->orthoQuad().get();

  glswSetPath("applications/eulerian-fluid/shader/", ".glsl");

  string fluidFile = "applications/eulerian-fluid/res/dummy.xml";
  //string fluidFile = "applications/eulerian-fluid/res/fluid-test.xml";
  //string fluidFile = "applications/eulerian-fluid/res/smoke-test.xml";
  //string fluidFile = "applications/eulerian-fluid/res/fire-test.xml";
  //string fluidFile = "applications/eulerian-fluid/res/liquid-test.xml";

  Fluid *fluid = FluidParser::readFluidFileXML(textureQuad, fluidFile);
  FluidBuffer *fluidBuffer = fluid->getBuffer("fluid");
  ref_ptr<Texture> &tex = fluidBuffer->fluidTexture();

  tex->addMapTo(MAP_TO_COLOR);

  {
    UnitQuad::Config quadConfig;
    quadConfig.levelOfDetail = 0;
    quadConfig.isTexcoRequired = GL_TRUE;
    quadConfig.isNormalRequired = GL_FALSE;
    quadConfig.centerAtOrigin = GL_TRUE;
    quadConfig.rotation = Vec3f(0.5*M_PI, 0.0*M_PI, 1.0*M_PI);
    quadConfig.posScale = Vec3f(3.32f, 3.32f, 3.32f);
    ref_ptr<MeshState> quad =
        ref_ptr<MeshState>::manage(new UnitQuad(quadConfig));

    ref_ptr<ModelTransformationState> modelMat;
    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 1.0f, 0.0f), 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_shading( Material::NO_SHADING );
    material->addTexture(tex);
    material->setConstantUniforms(GL_TRUE);

    application->addMesh(quad, modelMat, material);
  }

  // TODO: without ortho quad not added to vbo
  FXAA::Config aaCfg;
  aaCfg.spanMax = 8.0;
  aaCfg.reduceMin = 1.0/128.0;
  aaCfg.reduceMul = 1.0/8.0;
  aaCfg.edgeThreshold = 1.0/8.0;
  aaCfg.edgeThresholdMin = 1.0/16.0;
  application->addAntiAliasingPass(aaCfg);

  ref_ptr<Animation> fluidAnim = ref_ptr<Animation>::manage(new FluidAnimation(fluid));
  AnimationManager::get().addAnimation(fluidAnim);

  // TODO: show fluid buffers....
  //   * render on the fly again.
  //   * an app where fluids can be modified live would be nice

  application->setShowFPS();
  application->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT1);
  application->render(0.0f);

  fluid->executeOperations(fluid->initialOperations());

  application->mainLoop();

  delete fluid;

  return 0;
}
