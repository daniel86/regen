
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/models/quad.h>
#include <ogle/states/assimp-importer.h>
#include <ogle/animations/animation-manager.h>

#include <applications/glut-render-tree.h>

/**
  virtual void handleKey(bool isUp, unsigned char key)
  {
    // offset for ticks
    Vec2d offset = (Vec2d) {-1.0, -1.0};

    if( (int)key == 13 && !isUp ) {
      boneAnim->setAnimationIndexActive( 0, anims["attack1"]+offset );
    } else if( (int)key == 32 && !isUp  ) {
      boneAnim->setAnimationIndexActive( 0, anims["jump"]+offset );
    } else if( key == 'w'  && !isUp ) {
      boneAnim->setAnimationIndexActive( 0, anims["run"]+offset );
    } else if( key == 'y'  && !isUp ) {
      boneAnim->setAnimationIndexActive( 0, anims["yes"]+offset );
    } else if( key == 'n'  && !isUp ) {
      boneAnim->setAnimationIndexActive( 0, anims["no"]+offset );
    } else if( key == 'b'  && !isUp ) {
      boneAnim->setAnimationIndexActive( 0, anims["block"]+offset );
    } else if( key == '1'  && !isUp ) {
      boneAnim->setAnimationIndexActive( 0, anims["attack1"]+offset );
    } else if( key == '2'  && !isUp ) {
      boneAnim->setAnimationIndexActive( 0, anims["attack2"]+offset );
    } else if( key == '3'  && !isUp ) {
      boneAnim->setAnimationIndexActive( 0, anims["attack3"]+offset );
    } else if( key == '4'  && !isUp ) {
      boneAnim->setAnimationIndexActive( 0, anims["attack4"]+offset );
    } else if( key == '5'  && !isUp ) {
      boneAnim->setAnimationIndexActive( 0, anims["attack5"]+offset );
    }
  }
 */

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
    string modelPath = "res/models/psionic/dwarf/x";
    string modelName = "dwarf2.x";

    AssimpImporter importer(
        modelPath + "/" + modelName,
        modelPath,
        0);

    list< ref_ptr<AttributeState> > meshes = importer.loadMeshes(Vec3f( 0.0f ));
    Vec3f translation( 0.0, -0.5f*15.0f*0.3f, 0.0f );

    cerr << "LOADED meshes=" << meshes.size() << endl;

    ref_ptr<ModelTransformationState> modelMat;
    ref_ptr<Material> material;

    for(list< ref_ptr<AttributeState> >::iterator
        it=meshes.begin(); it!=meshes.end(); ++it)
    {
      ref_ptr<AttributeState> mesh = *it;

      material = importer.getMeshMaterial(mesh.get());
      material->set_shading(Material::PHONG_SHADING);
      material->set_specular( Vec4f(0.0f) );

      modelMat = ref_ptr<ModelTransformationState>::manage(
          new ModelTransformationState);
      modelMat->translate(translation, 0.0f);

      ref_ptr<StateNode> meshNode = application->addMesh(mesh, modelMat, material);

      ref_ptr<BonesState> bonesState =
          importer.loadMeshBones(mesh.get());
      if(bonesState.get()==NULL) {
        WARN_LOG("No bones state!");
      } else {
        meshNode->state()->joinStates(ref_ptr<State>::cast(bonesState));
      }
    }

    // mapping from different types of animations
    // to matching ticks
    map< string, Vec2d > anims;
    anims["none"] = (Vec2d) { -1.0, -1.0 };
    anims["complete"] = (Vec2d) { 0.0, 361.0 };
    anims["run"] = (Vec2d) { 16.0, 26.0 };
    anims["jump"] = (Vec2d) { 28.0, 40.0 };
    anims["jumpSpot"] = (Vec2d) { 42.0, 54.0 };
    anims["crouch"] = (Vec2d) { 56.0, 59.0 };
    anims["crouchLoop"] = (Vec2d) { 60.0, 69.0 };
    anims["getUp"] = (Vec2d) { 70.0, 74.0 };
    anims["battleIdle1"] = (Vec2d) { 75.0, 88.0 };
    anims["battleIdle2"] = (Vec2d) { 90.0, 110.0 };
    anims["attack1"] = (Vec2d) { 112.0, 126.0 };
    anims["attack2"] = (Vec2d) { 128.0, 142.0 };
    anims["attack3"] = (Vec2d) { 144.0, 160.0 };
    anims["attack4"] = (Vec2d) { 162.0, 180.0 };
    anims["attack5"] = (Vec2d) { 182.0, 192.0 };
    anims["block"] = (Vec2d) { 194.0, 210.0 };
    anims["dieFwd"] = (Vec2d) { 212.0, 227.0 };
    anims["dieBack"] = (Vec2d) { 230.0, 251.0 };
    anims["yes"] = (Vec2d) { 253.0, 272.0 };
    anims["no"] = (Vec2d) { 274.0, 290.0 };
    anims["idle1"] = (Vec2d) { 292.0, 325.0 };
    anims["idle2"] = (Vec2d) { 327.0, 360.0 };

    bool forceChannelStates=true;
    AnimationBehaviour forcedPostState = ANIM_BEHAVIOR_LINEAR;
    AnimationBehaviour forcedPreState = ANIM_BEHAVIOR_LINEAR;
    double defaultTicksPerSecond=20.0;
    ref_ptr<NodeAnimation> boneAnim = importer.loadNodeAnimation(
        forceChannelStates,
        forcedPostState,
        forcedPreState,
        defaultTicksPerSecond);
    boneAnim->set_timeFactor(1.0);
    boneAnim->setAnimationIndexActive(0, anims["idle1"]+Vec2d(-1.0, -1.0) );

    AnimationManager::get().addAnimation(
        ref_ptr<Animation>::cast(boneAnim));
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
