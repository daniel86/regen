/*
 * hello-world.cpp
 *
 *  Created on: 09.08.2012
 *      Author: daniel
 */

#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>

#include <applications/glut-render-tree.h>

int main(int argc, char** argv)
{
  GlutRenderTree *application = new GlutRenderTree;
  application->setClearScreenColor(Vec4f(1.0f, 0.4f, 0.2f, 1.0f));
  application->addMesh(ref_ptr<AttributeState>::manage(new UnitCube));
  //application->setShowFPS();
  application->mainLoop();
  return 0;
}
