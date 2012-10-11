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
  GlutRenderTree *application = new GlutRenderTree(argc, argv, "Render To Texture Test");

  ref_ptr<FBOState> fboState = application->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      GL_TRUE,
      Vec4f(0.0f)
  );

  glswSetPath("applications/eulerian-fluid/shader/", ".glsl");
  Fluid *fluidTest = FluidParser::readFluidFileXML(
      "applications/eulerian-fluid/res/fluid-test.xml");
  Fluid *smokeTest = FluidParser::readFluidFileXML(
      "applications/eulerian-fluid/res/smoke-test.xml");
  Fluid *fireTest = FluidParser::readFluidFileXML(
      "applications/eulerian-fluid/res/fire-test.xml");
  Fluid *liquidTest = FluidParser::readFluidFileXML(
      "applications/eulerian-fluid/res/liquid-test.xml");

  application->setShowFPS();
  application->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);

  application->mainLoop();
  //delete fluidTest;
  return 0;
}
