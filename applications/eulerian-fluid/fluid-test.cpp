/*
 * fluid-test.cpp
 *
 *  Created on: 06.10.2012
 *      Author: daniel
 */

#include "fluid-parser.h"

#include <ogle/external/glsw/glsw.h>
#include <ogle/render-tree/render-tree.h>
#include <applications/glut-render-tree.h>

int main(int argc, char** argv)
{
  GlutRenderTree *application = new GlutRenderTree(argc, argv, "Fluid simulation");
  MeshState *textureQuad = application->orthoQuad().get();

  ref_ptr<FBOState> fboState = application->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      GL_TRUE,
      Vec4f(0.0f)
  );

  // FIXME: quad not added to vbo without ortho pass
  FXAA::Config aaCfg;
  aaCfg.spanMax = 8.0;
  aaCfg.reduceMin = 1.0/128.0;
  aaCfg.reduceMul = 1.0/8.0;
  aaCfg.edgeThreshold = 1.0/8.0;
  aaCfg.edgeThresholdMin = 1.0/16.0;
  application->addAntiAliasingPass(aaCfg);

  application->setShowFPS();
  application->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT1);

  glswSetPath("applications/eulerian-fluid/shader/", ".glsl");

  Fluid *fluidTest = FluidParser::readFluidFileXML(
      textureQuad, "applications/eulerian-fluid/res/fluid-test.xml");
  Fluid *smokeTest = FluidParser::readFluidFileXML(
      textureQuad, "applications/eulerian-fluid/res/smoke-test.xml");
  Fluid *fireTest = FluidParser::readFluidFileXML(
      textureQuad, "applications/eulerian-fluid/res/fire-test.xml");
  Fluid *liquidTest = FluidParser::readFluidFileXML(
      textureQuad, "applications/eulerian-fluid/res/liquid-test.xml");

  fluidTest->executeOperations(fluidTest->initialOperations());
  smokeTest->executeOperations(fluidTest->initialOperations());
  fireTest->executeOperations(fluidTest->initialOperations());
  liquidTest->executeOperations(fluidTest->initialOperations());

  // TODO: show fluid buffers....
  //   * define fluid animation
  //   * render on the fly again.
  //   * an app where fluids can be modified live would be nice

  application->mainLoop();

  delete fluidTest;
  delete smokeTest;
  delete fireTest;
  delete liquidTest;

  return 0;
}
