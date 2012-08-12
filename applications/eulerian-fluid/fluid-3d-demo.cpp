
#include "glut-app.h"

#include <camera-manipulator.h>
#include <animation-manager.h>
#include <cube-image-texture.h>
#include <texel-transfer.h>
#include <volume.h>

#include "eulerian/include/fluid.h"
#include "eulerian/include/liquid.h"
#include "eulerian/include/smoke.h"
#include "eulerian/include/fire.h"
#include "eulerian/include/primitive.h"

class FluidApp : public GlutApp {
public:
  FluidApp(int argc, char** argv)
  : GlutApp(argc, argv, "3D fluid demo",
      600, 600,
      GL_RGBA, GL_DEPTH_COMPONENT24)
  {
    set_showFPS( true );
    // look in demos/textures/skybox for top.png,bottom.png,...
    ref_ptr<Texture2D> img = ref_ptr<Texture2D>::manage(
        new CubeImageTexture("demos/textures/cube-clouds", "png"));
    scene_->updateSky(img, Scene::SKY_BOX);

    // simple camera animation for zooming in/out to/from a single simulation
    camManipulator_ = ref_ptr<LookAtCameraManipulator>::manage(
        new LookAtCameraManipulator(camera_, 10) );
    camManipulator_->set_height( 2.0f );
    camManipulator_->set_lookAt( (Vec3f) {0.0f, 0.0f, 0.0f} );
    camManipulator_->set_radius( 4.0f );
    camManipulator_->setStepLength( M_PI*0.002 );
    AnimationManager::get().addAnimation( camManipulator_ );

    hotSmokeColor_ = (Vec3f) {1.0f, 0.0f, 0.0f};
    coldSmokeColor_ = (Vec3f) {0.0f, 0.0f, 1.0f};
    obstaclesColor_ = (Vec3f) {0.8f, 0.7f, 0.6f};

    for(int i=0; i<1; ++i) {
      ref_ptr<EulerianFluid> fluid;
      BoxVolume *set = new BoxVolume();

      set->material().set_shading( Material::NO_SHADING );

      switch(i) {
      case 0:
        fluid = makeSmoke();
        break;
      }

      scene_->addPrePass(fluid);
      //ref_ptr<Texture> tex = smokeObstacles_->tex();
      //ref_ptr<Texture> tex = visualizer->tex();
      ref_ptr<Texture> tex = smoke_->densityBuffer().tex;
      //ref_ptr<Texture> tex = smoke_->pressureBuffer().tex;
      //ref_ptr<Texture> tex = smoke_->temperatureBuffer().tex;
      //ref_ptr<Texture> tex = smoke_->velocityBuffer().tex;
      tex->addMapTo(MAP_TO_VOLUME);

      ref_ptr<ScalarToAlphaTransfer> transfer =
          ref_ptr<ScalarToAlphaTransfer>::manage( new ScalarToAlphaTransfer() );
      transfer->fillColorPositive_->set_value( (Vec3f) { 1.0f, 1.0f, 1.0f } );
      transfer->texelFactor_->set_value( 1.0f );
      tex->set_transfer(transfer);

      set->joinStates(tex);

      //ref_ptr<Texture> tex2 = smokeObstacles_->tex();
      //tex2->addMapTo(MAP_TO_VOLUME);
      //set->joinStates(tex2);

      ref_ptr<Mesh> setRef = ref_ptr<Mesh>::manage(set);
      scene_->addMesh(setRef);
    }
  }

  ref_ptr<EulerianFluid> makeSmoke()
  {
    GLuint width = 128, height = 128, depth = 128;
    bool isLiquid = false;

    ref_ptr<EulerianPrimitive> primitive = ref_ptr<EulerianPrimitive>::manage(
            new EulerianPrimitive(
                width, height, depth,
                scene_->unitOrthoQuad(),
                scene_->projectionUnitMatrixOrthoUniform(),
                scene_->deltaTUniform(),
                isLiquid ));

    smokeObstacles_ = ref_ptr<SphereObstacle>::manage(
        new SphereObstacle(primitive, 15.0f) );
    smokeObstacles_->set_position( (Vec3f) {width*0.5f, height*0.5f, depth*0.5f} );
    smokeObstacles_->updateTextureData();

    smoke_ = ref_ptr<EulerianSmoke>::manage(
        new EulerianSmoke(primitive, smokeObstacles_));

    ref_ptr<AdvectionTarget> t;
    t = smoke_->createPassiveQuantity("ink1", 1,
        smoke_->densityAdvectionTarget().decayAmount,
        smoke_->densityAdvectionTarget().quantityLoss, false);
    smokeInk1_ = t->buffer;
    t = smoke_->createPassiveQuantity("ink2", 1,
        smoke_->densityAdvectionTarget().decayAmount,
        smoke_->densityAdvectionTarget().quantityLoss, false);
    smokeInk2_ = t->buffer;

    float impulseTemperature_ = 33.375 * 1.04;
    float impulseDensity_ = 4.5 * 1.04;
    float splatRadius = width*0.1;
    Vec3f pos;

    pos = (Vec3f) { width*0.5f, height*0.025f, depth*0.5f };
    smoke_->addSplat(EulerianSmoke::TEMPERATURE, pos,
        (Vec3f) { impulseTemperature_, 0.0f, 0.0f }, splatRadius);
    smoke_->addSplat(EulerianSmoke::DENSITY, pos,
        (Vec3f) { impulseDensity_, 0.0f, 0.0f }, splatRadius);
    smoke_->addSplat("ink1", pos,
        (Vec3f) { impulseDensity_, 0.0f, 0.0f }, splatRadius);

    pos = (Vec3f) { width*0.5f, height*0.925f, depth*0.5f };
    smoke_->addSplat(EulerianSmoke::TEMPERATURE, pos,
        (Vec3f) { -impulseTemperature_, 0.0f, 0.0f }, splatRadius);
    smoke_->addSplat(EulerianSmoke::DENSITY, pos,
        (Vec3f) { impulseDensity_*0.5f, 0.0f, 0.0f }, splatRadius);
    smoke_->addSplat("ink3", pos,
        (Vec3f) { impulseDensity_*0.5f, 0.0f, 0.0f }, splatRadius);

    return smoke_;
  }

  virtual void handleMouseMotion(double dt, int dx, int dy)
  {
    if(buttonPressed_) {
      camManipulator_->set_height( camManipulator_->height() + ((float)dy)*0.02f, dt );
      camManipulator_->setStepLength( ((float)dx)*0.001f, dt );
    }
  }
  virtual void handleButton(double dt, bool isDown, int button, int x, int y)
  {
    float gridX=0.0f, gridY=0.0f;

    if(button == 0) {
      buttonPressed_ = isDown;
      if(isDown) {
        camManipulator_->setStepLength( 0.0f );
      }
    } else if (button == 4 && !isDown) {
      camManipulator_->set_radius( camManipulator_->radius()+0.1f );
    } else if (button == 3 && !isDown) {
      camManipulator_->set_radius( camManipulator_->radius()-0.1f );
    }
  }

  virtual void handleKey(bool isUp, unsigned char key)
  {
    if(key == '1' && isUp) {
      cout << "Resetting fluid simulations..." << endl;
      smoke_->reset();
      clearSlab(smokeInk1_);
      clearSlab(smokeInk2_);
    } else if( ( key == '2' || key == '3' || key == '4' || key == '5' ) && isUp) {
      static bool toggle = false;
      if(toggle) {
        cout << "Resetting to default ink textures..." << endl;
        toggle = false;
      } else {
        if( key == '2' ) {
          cout << "Toggling velocity ink texture..." << endl;
        } else if ( key == '3' ) {
          cout << "Toggling pressure ink texture..." << endl;
        } else if ( key == '4' ) {
          cout << "Toggling density ink texture..." << endl;
        } else if ( key == '5' ) {
          cout << "Toggling temperature ink texture..." << endl;
        }
        toggle = true;
      }
    }
  }

  ref_ptr<EulerianSmoke> smoke_;
  FluidBuffer smokeInk1_;
  FluidBuffer smokeInk2_;

  bool buttonPressed_;

  Vec3f hotSmokeColor_;
  Vec3f coldSmokeColor_;
  Vec3f obstaclesColor_;

  ref_ptr<LookAtCameraManipulator> camManipulator_;
  ref_ptr<SphereObstacle> smokeObstacles_;
};


int main(int argc, char** argv)
{
  FluidApp *app = new FluidApp(argc, argv);
  app->mainLoop();
  delete app;
  return 0;
}

