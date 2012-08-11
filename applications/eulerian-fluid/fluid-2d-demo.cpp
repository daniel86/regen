
#include "glut-app.h"

#include <camera-manipulator.h>
#include <animation-manager.h>

#include "eulerian/include/fluid.h"
#include "eulerian/include/liquid.h"
#include "eulerian/include/smoke.h"
#include "eulerian/include/fire.h"
#include "eulerian/include/primitive.h"

class FluidApp : public GlutApp {
public:
  FluidApp(int argc, char** argv)
  : GlutApp(argc, argv,
      "2D fluid demo",
      600, 600,
      GL_RGBA, GL_DEPTH_COMPONENT24),
    zoomedFluid_(NULL)
  {
    set_showFPS( true );
    scene_->set_skyColor( Vec4f(0,0,0,1) );

    // simple camera animation for zooming in/out to/from a single simulation
    camManipulator_ = ref_ptr<CameraLinearPositionManipulator>::manage(
        new CameraLinearPositionManipulator(camera_, 10) );
    camManipulator_->setStepLength( 0.06f );
    AnimationManager::get().addAnimation( camManipulator_ );

    hotSmokeColor_ = (Vec3f) {1.0f, 0.0f, 0.0f};
    coldSmokeColor_ = (Vec3f) {0.0f, 0.0f, 1.0f};
    smokeMouseColor_ = (Vec3f) {1.0f, 1.0f, 0.0f};
    obstaclesColor_ = (Vec3f) {0.8f, 0.7f, 0.6f};

    float tx = 0.0f, ty = 0.0f;
    for(int i=0; i<4; ++i) {
      UnitQuad *set = new UnitQuad();
      ref_ptr<EulerianFluid> fluid;

      set->createVertexData(
          (Vec3f){0.0,0.0,0.0}, // rotation
          (Vec3f){1.0, 1.0, 1.0}, // scale
          (Vec2f){1.0, 1.0}, // uv scale
          1, // LOD
          true, // texco
          false, // normal
          false // center at origin
          );
      set->set_vboUsage(VertexBufferObject::USAGE_STATIC);
      set->set_useAlpha(true);
      set->material().set_shading( Material::NO_SHADING );

      float space = 1.0f;
      if(i==2) {
        tx = 0.0f;
        ty = space;
      }
      set->translate( Vec3f(tx-space, ty+space, 1.59f), 0.0f );
      tx += space;

      switch(i) {
      case 0:
        fluid = makeSmoke();
        smokePrimitive_ = set;
        break;
      case 1:
        fluid = makeFire();
        firePrimitive_ = set;
        break;
      case 2:
        fluid = makeLiquid();
        liquidPrimitive_ = set;
        break;
      case 3:
        fluid = makeTestingFluid();
        testingPrimitive_ = set;
        break;
      }

      scene_->addPrePass(fluid);

      ref_ptr<Mesh> setRef = ref_ptr<Mesh>::manage(set);
      scene_->addMesh(setRef);
    }
    resetInkTextures();
  }


  void addScalarTexture(Mesh *set,
      ref_ptr<Texture> tex, const Vec3f &col,
      float factor=1.0f, TextureBlendMode blendMode=BLEND_MODE_ALPHA)
  {
    ref_ptr<ScalarToAlphaTransfer> transfer;
    tex->addMapTo(MAP_TO_COLOR);
    tex->set_blendMode(blendMode);
    transfer = ref_ptr<ScalarToAlphaTransfer>::manage( new ScalarToAlphaTransfer() );
    transfer->fillColorPositive_->set_value( col );
    transfer->fillColorNegative_->set_value( col );
    transfer->texelFactor_->set_value( factor );
    tex->set_transfer(transfer);
    set->joinStates(tex);
  }
  void addRGBTexture(Mesh *set,
      ref_ptr<Texture> tex, float factor=3.0f)
  {
    ref_ptr<RGBColorfullTransfer> transfer;
    tex->addMapTo(MAP_TO_COLOR);
    tex->set_blendMode(BLEND_MODE_SRC);
    transfer = ref_ptr<RGBColorfullTransfer>::manage( new RGBColorfullTransfer() );
    transfer->texelFactor_->set_value( factor );
    tex->set_transfer(transfer);
    set->joinStates(tex);
  }
  void addLevelSetTexture(Mesh *set, ref_ptr<Texture> tex, float factor=1.0f)
  {
    ref_ptr<LevelSetTransfer> transfer;
    tex->addMapTo(MAP_TO_COLOR);
    tex->set_blendMode(BLEND_MODE_SRC);
    transfer = ref_ptr<LevelSetTransfer>::manage( new LevelSetTransfer() );
    transfer->texelFactor_->set_value( factor );
    tex->set_transfer(transfer);
    set->joinStates(tex);
  }
  void addFireTexture(Mesh *set,
      ref_ptr<Texture> tex, float t1, float t2,
      TextureBlendMode blendMode=BLEND_MODE_ALPHA)
  {
    ref_ptr<FireTexture> pattern =
        ref_ptr<FireTexture>::manage( new FireTexture );
    ref_ptr<FireTransfer> transfer;

    pattern->set_spectrum(t1, t2, 64);

    tex->addMapTo(MAP_TO_COLOR);
    tex->set_blendMode(blendMode);

    transfer = ref_ptr<FireTransfer>::manage( new FireTransfer(pattern) );
    transfer->smokeColor_->set_value( (Vec3f) {0.9,0.1,0.055} );
    transfer->texelFactor_->set_value( 1.0f );
    transfer->rednessFactor_->set_value( 5 );
    transfer->smokeColorMultiplier_->set_value( 2.0f );
    transfer->smokeAlphaMultiplier_->set_value( 0.1f );
    transfer->fireAlphaMultiplier_->set_value( 0.4f );
    transfer->fireWeight_->set_value( 2.0f );

    tex->set_transfer(transfer);
    set->joinStates(tex);
  }

  void clearTextures(Mesh *set)
  {
    list< Texture* > textures = set->textureStates();
    for(list< Texture* >::iterator it=textures.begin();
        it!=textures.end(); ++it)
    {
      set->disjoinStates(*it);
    }
  }

  void setVelocity(Mesh *set, ref_ptr<EulerianFluid> fluid)
  {
    clearTextures(set);
    addRGBTexture(set, fluid->velocityBuffer().tex, 0.05f);
    addScalarTexture(set, fluid->obstaclesTexture(), obstaclesColor_, 1.0f, BLEND_MODE_ALPHA);
  }
  void setPressure(Mesh *set, ref_ptr<EulerianFluid> fluid)
  {
    const Vec3f pressureColor = (Vec3f) {1.0f,1.0f,1.0f};
    clearTextures(set);
    addScalarTexture(set, fluid->pressureBuffer().tex, pressureColor, 0.1f);
    addScalarTexture(set, fluid->obstaclesTexture(), obstaclesColor_);
  }
  void setDensity(Mesh *set, ref_ptr<EulerianSmoke> fluid)
  {
    Vec3f densColor = (Vec3f) {0.0f,1.0f,0.0f};
    clearTextures(set);
    addScalarTexture(set, fluid->densityBuffer().tex, densColor);
    addScalarTexture(set, fluid->obstaclesTexture(), obstaclesColor_);
  }
  void setTemperature(Mesh *set, ref_ptr<EulerianSmoke> fluid)
  {
    Vec3f tempColor = (Vec3f) {1.0f,0.0f,0.0f};
    clearTextures(set);
    addScalarTexture(set, fluid->temperatureBuffer().tex, tempColor);
    addScalarTexture(set, fluid->obstaclesTexture(), obstaclesColor_);
  }

  void resetInkTextures() {
    setVelocity(testingPrimitive_, testingFluid_);

    clearTextures(smokePrimitive_);
    addScalarTexture(smokePrimitive_, smokeInk1_.tex,
        hotSmokeColor_, 1.0f, BLEND_MODE_SRC);
    addScalarTexture(smokePrimitive_, smokeInk3_.tex,
        coldSmokeColor_, 1.0f, BLEND_MODE_ADD_NORMALIZED);
    addScalarTexture(smokePrimitive_, smokeInk2_.tex,
        smokeMouseColor_, 1.0f, BLEND_MODE_ADD_NORMALIZED);
    addScalarTexture(smokePrimitive_, smoke_->obstaclesTexture(),
        obstaclesColor_, 1.0f, BLEND_MODE_ALPHA);

    clearTextures(firePrimitive_);
    addFireTexture(firePrimitive_,
        fireInk1_.tex, 300.0, 500.0, BLEND_MODE_SRC);
    addFireTexture(firePrimitive_,
        fireInk2_.tex, 250.0, 450.0, BLEND_MODE_ADD_NORMALIZED);
    addScalarTexture(firePrimitive_, fire_->obstaclesTexture(),
        obstaclesColor_, 1.0f, BLEND_MODE_ALPHA);

    clearTextures(liquidPrimitive_);
    addLevelSetTexture(liquidPrimitive_, liquid_->levelSetBuffer().tex);
    addScalarTexture(liquidPrimitive_, liquid_->obstaclesTexture(),
        obstaclesColor_, 1.0f, BLEND_MODE_ALPHA);
  }

  ref_ptr<EulerianFluid> makeSmoke()
  {
    GLuint width = 256, height = 256, depth = 0;
    bool isLiquid = false;

    ref_ptr<EulerianPrimitive> primitive = ref_ptr<EulerianPrimitive>::manage(
            new EulerianPrimitive(
                width, height, depth,
                scene_->unitOrthoQuad(),
                scene_->projectionUnitMatrixOrthoUniform(),
                scene_->deltaTUniform(),
                isLiquid ));

    ref_ptr<CircleObstacle> obstacles = ref_ptr<CircleObstacle>::manage(
        new CircleObstacle(primitive) );
    obstacles->updateTextureData();

    smoke_ = ref_ptr<EulerianSmoke>::manage(
        new EulerianSmoke(primitive, obstacles));

    ref_ptr<AdvectionTarget> t;
    t = smoke_->createPassiveQuantity("ink1", 1,
        smoke_->densityAdvectionTarget().decayAmount,
        smoke_->densityAdvectionTarget().quantityLoss, false);
    smokeInk1_ = t->buffer;
    t = smoke_->createPassiveQuantity("ink2", 1,
        smoke_->densityAdvectionTarget().decayAmount,
        smoke_->densityAdvectionTarget().quantityLoss, false);
    smokeInk2_ = t->buffer;
    t = smoke_->createPassiveQuantity("ink3", 1,
        smoke_->densityAdvectionTarget().decayAmount,
        smoke_->densityAdvectionTarget().quantityLoss, false);
    smokeInk3_ = t->buffer;

    float impulseTemperature_ = 33.375;
    float impulseDensity_ = 4.5;
    float splatRadius = width*0.05;
    Vec3f pos;

    pos = (Vec3f) { width*0.5f, height*0.025f, 0.0f };
    smoke_->addSplat(EulerianSmoke::TEMPERATURE, pos,
        (Vec3f) { impulseTemperature_, 0.0f, 0.0f }, splatRadius);
    smoke_->addSplat(EulerianSmoke::DENSITY, pos,
        (Vec3f) { impulseDensity_, 0.0f, 0.0f }, splatRadius);
    smoke_->addSplat("ink1", pos,
        (Vec3f) { impulseDensity_, 0.0f, 0.0f }, splatRadius);

    pos = (Vec3f) { width*0.5f, height*0.925f, 0.0f };
    smoke_->addSplat(EulerianSmoke::TEMPERATURE, pos,
        (Vec3f) { -impulseTemperature_, 0.0f, 0.0f }, splatRadius);
    smoke_->addSplat(EulerianSmoke::DENSITY, pos,
        (Vec3f) { impulseDensity_*0.5f, 0.0f, 0.0f }, splatRadius);
    smoke_->addSplat("ink3", pos,
        (Vec3f) { impulseDensity_*0.5f, 0.0f, 0.0f }, splatRadius);

    return smoke_;
  }

  ref_ptr<EulerianFluid> makeFire()
  {
    GLuint width = 256, height = 256, depth = 0;
    bool isLiquid = false;

    ref_ptr<EulerianPrimitive> primitive = ref_ptr<EulerianPrimitive>::manage(
            new EulerianPrimitive(
                width, height, depth,
                scene_->unitOrthoQuad(),
                scene_->projectionUnitMatrixOrthoUniform(),
                scene_->deltaTUniform(),
                isLiquid ));

    ref_ptr<MovableObstacle2D> obstacles = ref_ptr<MovableObstacle2D>::manage(
        new MovableObstacle2D(primitive,0.9f) );

    fire_ = ref_ptr<EulerianFire>::manage(
        new EulerianFire(primitive,obstacles));

    ref_ptr<AdvectionTarget> t;
    t = fire_->createPassiveQuantity("ink1", 1,
        fire_->densityAdvectionTarget().decayAmount,
        fire_->densityAdvectionTarget().quantityLoss,
        fire_->densityAdvectionTarget().treatAsLiquid);
    fireInk1_ = t->buffer;
    t = fire_->createPassiveQuantity("ink2", 1,
        fire_->densityAdvectionTarget().decayAmount,
        fire_->densityAdvectionTarget().quantityLoss,
        fire_->densityAdvectionTarget().treatAsLiquid);
    fireInk2_ = t->buffer;

    Vec3f pos = (Vec3f) { width*0.5f, 5.0f, 0.0f };
    fire_->addSplat(EulerianFire::TEMPERATURE,
        pos, (Vec3f) { 7.5, 0.0f, 0.0f }, width*0.8f, 10.0);
    fire_->addSplat(EulerianFire::DENSITY,
        pos, (Vec3f) { 22.7, 0.0f, 0.0f }, width*0.8f, 10.0);
    fire_->addSplat("ink1",
        pos, (Vec3f) { 22.7, 0.0f, 0.0f }, width*0.8f, 10.0);

    ref_ptr<Texture> splatTex = ref_ptr<Texture>::manage(
        new ImageTexture("demos/textures/splat.png",
            primitive->width(),
            primitive->height(),
            primitive->depth()));
    fireTempSplat_ = fire_->addSplat(EulerianFire::TEMPERATURE,
        (Vec3f) { 10.5, 0.0f, 0.0f }, splatTex);
    fire_->addSplat(EulerianFire::DENSITY,
        (Vec3f) { 17.7, 0.0f, 0.0f }, splatTex);
    fireDensSplat_ = fire_->addSplat("ink1",
        (Vec3f) { 17.7, 0.0f, 0.0f }, splatTex);

    return fire_;
  }

  ref_ptr<EulerianFluid> makeLiquid()
  {
    GLuint width = 256, height = 256, depth = 0;
    bool isLiquid = true;
    ref_ptr<EulerianPrimitive> primitive = ref_ptr<EulerianPrimitive>::manage(
            new EulerianPrimitive(
                width, height, depth,
                scene_->unitOrthoQuad(),
                scene_->projectionUnitMatrixOrthoUniform(),
                scene_->deltaTUniform(),
                isLiquid ));

    ref_ptr<CircleObstacle> obstacles = ref_ptr<CircleObstacle>::manage(
        new CircleObstacle(primitive) );

    liquid_ = ref_ptr<EulerianLiquid>::manage(
        new EulerianLiquid(primitive, obstacles));

    return liquid_;
  }

  ref_ptr<EulerianFluid> makeTestingFluid()
  {
    GLuint width = 256, height = 256, depth = 0;
    bool isLiquid = false;

    ref_ptr<EulerianPrimitive> primitive = ref_ptr<EulerianPrimitive>::manage(
            new EulerianPrimitive(
                width, height, depth,
                scene_->unitOrthoQuad(),
                scene_->projectionUnitMatrixOrthoUniform(),
                scene_->deltaTUniform(),
                isLiquid ));

    ref_ptr<RectangleObstacle> obstacles = ref_ptr<RectangleObstacle>::manage(
        new RectangleObstacle(primitive, 70.0f, 30.0f) );
    obstacles->set_position( (Vec2f) { 0.5f*width - 30.0f, 0.5f*height + 40.0f } );

    testingFluid_ = ref_ptr<EulerianFluid>::manage(
        new EulerianFluid(primitive, obstacles));

    testingFluid_->addSplat(EulerianFluid::VELOCITY,
        (Vec3f) { width*0.5f, height*0.09f, 0.0f },
        (Vec3f) { 0.0f, 1000.0, 0.0f },
        width*0.05f);

    return testingFluid_;
  }

  void getGridForWinPos(int x, int y,
      float *gridx, float *gridy,
      EulerianFluid **fluid)
  {
    int gridSize = winH_/2;
    int gridBorderLeft = (winW_/2) - gridSize;
    int gridBorderRight = winW_ - gridBorderLeft;
    float winToGridScale_;

    *fluid = NULL;

    // check if outside of domain
    if( x < gridBorderLeft ) return;
    if( x > gridBorderRight ) return;

    if(zoomedFluid_ != NULL) {
      *fluid = zoomedFluid_;
      *gridy = winH_ - y;
      *gridx = x - gridBorderLeft;
      winToGridScale_ = ((float) (*fluid)->primitive().width()) / ((float) winH_);
    } else {
      bool topRow = ( y < (winH_/2) );
      bool leftRow = ( x < (winW_/2) );
      if(topRow && leftRow) {
        *gridx = x - gridBorderLeft;
        *gridy = gridSize - y;
        *fluid = liquid_.get();
      } else if(topRow) {
        *gridx = x - gridBorderLeft - gridSize;
        *gridy = gridSize - y;
        *fluid = testingFluid_.get();
      } else if(leftRow) { // smoke
        *gridx = x - gridBorderLeft;
        *gridy = 2*gridSize - y;
        *fluid = smoke_.get();
      } else { // fire
        *gridx = x - gridBorderLeft - gridSize;
        *gridy = 2*gridSize - y;
        *fluid = fire_.get();
      }
      winToGridScale_ = ((float) (*fluid)->primitive().width()) / ((float) gridSize);
    }
    *gridx = (*gridx) * winToGridScale_;
    *gridy = (*gridy) * winToGridScale_;
  }


  virtual void handleMouseMotion(double dt_, int dx, int dy)
  {
    if(isMouseSplatting_) {
      float gridX=0.0f, gridY=0.0f;
      EulerianFluid *splatFluid = NULL;
      getGridForWinPos(last_x, last_y, &gridX, &gridY, &splatFluid);
      mouseSplat1_->pos = (Vec3f) { gridX, gridY, 0.0f };
      if(mouseSmoke_ != NULL) {
        mouseSplat2_->pos = (Vec3f) { gridX, gridY, 0.0f };
        mouseSplat3_->pos = (Vec3f) { gridX, gridY, 0.0f };
      }
    } else if (isStreaming_) {
      float gridX=0.0f, gridY=0.0f;
      EulerianFluid *splatFluid = NULL;
      getGridForWinPos(last_x, last_y, &gridX, &gridY, &splatFluid);
      mouseStream_->center = (Vec3f) { gridX, gridY, 0.0f };
    } else if (isObstacleMoving_) {
      int gridSize = winH_/2;
      float winToGridScale_ = ((float) mouseFluid_->primitive().width()) /
          (float) (zoomedFluid_!=NULL ? winH_ : gridSize);
      mouseObstacles_->move( (Vec2f) {(float)winToGridScale_*dx, (float)-winToGridScale_*dy} );
    }
  }
  virtual void handleButton(double dt, bool isDown, int button, int x, int y)
  {
    float gridX=0.0f, gridY=0.0f;

    if(!isDown == true) {
      if(button==3) { // zoom in
        if(zoomedFluid_ != NULL) return;

        EulerianFluid *mouseFluid = NULL;
        getGridForWinPos(x, y, &gridX, &gridY, &zoomedFluid_);
        if(zoomedFluid_ == NULL) return;

        if(zoomedFluid_ == smoke_.get()) {
          camManipulator_->setDestinationPosition( (Vec3f) { -0.5, 0.5, 2.8 } );
        } else if(zoomedFluid_ == fire_.get()) {
          camManipulator_->setDestinationPosition( (Vec3f) { 0.5, 0.5, 2.8 } );
        } else if(zoomedFluid_ == liquid_.get()) {
          camManipulator_->setDestinationPosition( (Vec3f) { -0.5, 1.5, 2.8 } );
        } else {
          camManipulator_->setDestinationPosition( (Vec3f) { 0.5, 1.5, 2.8 } );
        }

        return;
      } else if(button == 4 && zoomedFluid_!=NULL) {
        zoomedFluid_ = NULL;
        camManipulator_->setDestinationPosition( (Vec3f) { 0.0, 1.0, 4.0} );
        return;
      }
    }

    if(button==0) {
      if(isDown == true) {
        float temperature, density, radius;
        Vec3f velocity;

        mouseFluid_ = NULL;
        getGridForWinPos(x, y, &gridX, &gridY, &mouseFluid_);
        if(mouseFluid_ == NULL) return;

        Vec3f gridPos = Vec3f( gridX, gridY, 0.0f );
        Vec2i gridPosi = Vec2i( (int)gridX, (int)gridY );

        MovableObstacle2D *obstacles = (MovableObstacle2D*)mouseFluid_->obstacles();

        if(obstacles->select(gridPosi)) {
          isObstacleMoving_ = true;
          mouseObstacles_ = obstacles;
        } else if(mouseFluid_ == smoke_.get()) {
          mouseSmoke_ = smoke_.get();
          temperature = 31.25;
          density = 9.5f;
          radius = mouseFluid_->primitive().width()*0.025f;
        } else if(mouseFluid_ == fire_.get()) {
          mouseSmoke_ = fire_.get();
          temperature = 95.625f;
          density = 36.89f;
          radius = mouseFluid_->primitive().width()*0.025f;
        } else if(mouseFluid_ == liquid_.get()) {
          mouseLiquid_ = liquid_.get();
          radius = 10.0f;
          velocity = (Vec3f) { 0.0f, -9.0f, 0.0f };
        } else {
          radius = mouseFluid_->primitive().width()*0.025f;
          velocity = (Vec3f) { 0.0f, 2750.0f, 0.0f };
        }

        if(mouseSmoke_ != NULL) {
          mouseSplat1_ = mouseSmoke_->addSplat(EulerianFire::TEMPERATURE,
              gridPos, (Vec3f) { temperature, 0.0f, 0.0f }, radius);
          mouseSplat2_ = mouseSmoke_->addSplat(EulerianFire::DENSITY,
              gridPos, (Vec3f) { density, 0.0f, 0.0f }, radius);
          mouseSplat3_ = mouseSmoke_->addSplat("ink2",
              gridPos, (Vec3f) { density, 0.0f, 0.0f }, radius);
          isMouseSplatting_ = true;
        } else if(mouseLiquid_ != NULL) {
          mouseStream_ = ref_ptr<LiquidStreamSource>::manage(new LiquidStreamSource);
          mouseStream_->center = gridPos;
          mouseStream_->radius = radius;
          mouseStream_->velocity = velocity;
          mouseLiquid_->liquidStreamStage().addStreamSource(mouseStream_);
          isStreaming_ = true;
        } else if(!isObstacleMoving_) {
          mouseSplat1_ = mouseFluid_->addSplat(
              EulerianFire::VELOCITY, gridPos, velocity, radius);
          isMouseSplatting_ = true;
        }

      } else {
        if(isMouseSplatting_ && mouseFluid_ != NULL) {
          mouseFluid_->removeSplat(mouseSplat1_);
          if(mouseSmoke_ != NULL) {
            mouseFluid_->removeSplat(mouseSplat2_);
            mouseFluid_->removeSplat(mouseSplat3_);
          }
        }
        if(isStreaming_ && mouseLiquid_ != NULL) {
          mouseLiquid_->liquidStreamStage().removeStreamSource(mouseStream_);
        }
        if(isObstacleMoving_) {
          mouseObstacles_->unselect();
        }
        isObstacleMoving_ = false;
        isStreaming_ = false;
        isMouseSplatting_ = false;
        mouseFluid_ = NULL;
        mouseSmoke_ = NULL;
        mouseLiquid_ = NULL;
        mouseObstacles_ = NULL;
      }
    }
  }

  virtual void handleKey(bool isUp, unsigned char key)
  {
    static int mode = 0;
    static int numModes = 2;

    if(key == '1' && isUp) {
      cout << "Resetting fluid simulations..." << endl;
      liquid_->reset();
      smoke_->reset();
      fire_->reset();
      testingFluid_->reset();
      clearSlab(smokeInk1_);
      clearSlab(smokeInk2_);
      clearSlab(smokeInk3_);
      clearSlab(fireInk1_);
      clearSlab(fireInk2_);
    } else if( ( key == '2' || key == '3' || key == '4' || key == '5' ) && isUp) {
      static bool toggle = false;
      if(toggle) {
        cout << "Resetting to default ink textures..." << endl;
        resetInkTextures();
        toggle = false;
      } else {
        if( key == '2' ) {
          cout << "Toggling velocity ink texture..." << endl;
          setVelocity(testingPrimitive_, testingFluid_);
          setVelocity(liquidPrimitive_, liquid_);
          setVelocity(firePrimitive_, fire_);
          setVelocity(smokePrimitive_, smoke_);
        } else if ( key == '3' ) {
          cout << "Toggling pressure ink texture..." << endl;
          setPressure(testingPrimitive_, testingFluid_);
          setPressure(liquidPrimitive_, liquid_);
          setPressure(firePrimitive_, fire_);
          setPressure(smokePrimitive_, smoke_);
        } else if ( key == '4' ) {
          cout << "Toggling density ink texture..." << endl;
          setDensity(firePrimitive_, fire_);
          setDensity(smokePrimitive_, smoke_);
        } else if ( key == '5' ) {
          cout << "Toggling temperature ink texture..." << endl;
          setTemperature(firePrimitive_, fire_);
          setTemperature(smokePrimitive_, smoke_);
        }
        toggle = true;
      }
    } else if(key == 'x' && isUp) {
      mode = (mode+1)%numModes;
    } else if(key == '+' && isUp) {
    } else if(key == '-' && isUp) {
    }
  }

  EulerianFluid *mouseFluid_;
  ref_ptr<EulerianLiquid> liquid_;
  EulerianLiquid *mouseLiquid_;
  ref_ptr<EulerianSmoke> smoke_;
  ref_ptr<EulerianFire> fire_;

  EulerianFluid *zoomedFluid_;

  Mesh *testingPrimitive_;
  Mesh *liquidPrimitive_;
  Mesh *firePrimitive_;
  Mesh *smokePrimitive_;

  EulerianSmoke *mouseSmoke_;

  FluidBuffer smokeInk1_;
  FluidBuffer smokeInk2_;
  FluidBuffer smokeInk3_;
  FluidBuffer fireInk1_;
  FluidBuffer fireInk2_;

  ref_ptr<SplatSource> mouseSplat1_;
  ref_ptr<SplatSource> mouseSplat2_;
  ref_ptr<SplatSource> mouseSplat3_;
  ref_ptr<SplatSource> fireTempSplat_;
  ref_ptr<SplatSource> fireDensSplat_;
  bool isMouseSplatting_;

  ref_ptr<LiquidStreamSource> mouseStream_;
  bool isStreaming_;

  MovableObstacle2D *mouseObstacles_;
  bool isObstacleMoving_;

  Vec3f hotSmokeColor_;
  Vec3f coldSmokeColor_;
  Vec3f smokeMouseColor_;
  Vec3f obstaclesColor_;

  ref_ptr<EulerianFluid> testingFluid_;

  ref_ptr<CameraLinearPositionManipulator> camManipulator_;
};


int main(int argc, char** argv)
{
  FluidApp *app = new FluidApp(argc, argv);
  app->mainLoop();
  delete app;
  return 0;
}

