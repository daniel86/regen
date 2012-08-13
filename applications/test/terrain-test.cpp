
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/models/quad.h>
#include <ogle/textures/video-texture.h>

#include <applications/glut-render-tree.h>

int main(int argc, char** argv)
{
  GlutRenderTree *application = new GlutRenderTree(argc, argv, "Hello World!");

  ref_ptr<FBOState> fboState = application->setRenderToTexture(
      800,600,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      // with sky box there is no need to clear the color buffer
      GL_FALSE,
      Vec4f(0.0f)
  );

  application->setLight();

  {
    // TODO: implement, but normal-map-test should work before
    /*
    Vec2i numPatches = (Vec2i) { 16, 16 };
    Vec2f terrainSize = (Vec2f) { 40.0f, 40.0f };
    mesh_ = ref_ptr<Terrain>::manage(
        new Terrain(terrainSize, numPatches));
    mesh_->set_vboUsage(VertexBufferObject::USAGE_STATIC);
    mesh_->material().set_shading( Material::GOURAD_SHADING );
    col_ = mesh_->loadColorMap("demos/textures/terrain/color.jpg");
    nor_ = mesh_->loadNormalMap("demos/textures/terrain/normal.jpg");
    height_ = mesh_->loadHeightMap("demos/textures/terrain/height.jpg");
    height_->set_factor( 5.0f , true );
    mesh_->set_lodFactor(0.1);
    mesh_->translate( Vec3f(0.0f, -5.0f, 0.0f), 0.0f );
    scene_->addMesh(mesh_);
     */
  }

  // makes sense to add sky box last, because it looses depth test against
  // all other objects
  application->addSkyBox("res/textures/cube-clouds");
  application->setShowFPS();

  // TODO: screen blit must know screen width/height
  application->setBlitToScreen(
      fboState->fbo(), GL_COLOR_ATTACHMENT0);

  application->mainLoop();
  return 0;
}
