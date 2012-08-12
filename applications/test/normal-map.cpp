
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/models/quad.h>
#include <ogle/states/debug-normal.h>
#include <ogle/states/tesselation-state.h>
#include <ogle/textures/image-texture.h>

#include <applications/glut-render-tree.h>

int main(int argc, char** argv)
{
  GlutRenderTree *application = new GlutRenderTree(argc, argv, "Transform feedback test");

  application->setClearScreenColor(Vec4f(0.10045f, 0.0056f, 0.012f, 1.0f));
  application->globalStates()->state()->addEnabler(
      ref_ptr<Callable>::manage(new ClearDepthState));

  application->setLight();

  ref_ptr<ModelTransformationState> modelMat;


  {
    GLenum meshPrimitive = GL_QUADS;
    TessPrimitive tessPrimitive = TESS_PRIMITVE_QUADS;
    GLuint tessVertices = 4;
    TessVertexSpacing tessSpacing = TESS_SPACING_FRACTIONAL_ODD;
    TessVertexOrdering tessOrdering = TESS_ORDERING_CW;
    TessLodMetric tessMetric = TESS_LOD_EDGE_DEVICE_DISTANCE;

    UnitQuad::Config quadConfig;
    quadConfig.levelOfDetail = 0;
    quadConfig.isTexcoRequired = GL_TRUE;
    quadConfig.isNormalRequired = GL_TRUE;
    quadConfig.centerAtOrigin = GL_TRUE;
    quadConfig.rotation = Vec3f(0.5*M_PI, 0.0f, 0.0f);
    quadConfig.posScale = Vec3f(2.0f, 2.0f, 2.0f);
    ref_ptr<AttributeState> quad =
        ref_ptr<AttributeState>::manage(new UnitQuad(quadConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.0f, 0.0f), 0.0f);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);

    /*
    Tesselation tessCfg(tessPrimitive, tessVertices);
    tessCfg.ordering = tessOrdering;
    tessCfg.spacing = tessSpacing;
    tessCfg.lodMetric = tessMetric;
    ref_ptr<TesselationState> tessState =
        ref_ptr<TesselationState>::manage(new TesselationState(tessCfg));
    tessState->set_lodFactor(0.4f);
    quad->set_primitive(GL_PATCHES);
    material->joinStates(ref_ptr<State>::cast(tessState));
    */

    ref_ptr<Texture> colMap_ = ref_ptr<Texture>::manage(
        new ImageTexture("res/textures/brick/color.jpg"));
    colMap_->addMapTo(MAP_TO_DIFFUSE);
    material->addTexture(colMap_);

    // FIXME: something broken with that
    ref_ptr<Texture> norMap_ = ref_ptr<Texture>::manage(
        new ImageTexture("res/textures/brick/normal.jpg"));
    norMap_->addMapTo(MAP_TO_NORMAL);
    material->addTexture(norMap_);

    /*
    ref_ptr<Texture> heightMap_ = ref_ptr<Texture>::manage(
        new ImageTexture("res/textures/brick/height.jpg"));
    heightMap_->addMapTo(MAP_TO_HEIGHT);
    heightMap_->set_heightScale(-0.05f);
    material->addTexture(heightMap_);
    */

    material->set_shading( Material::PHONG_SHADING );
    material->set_twoSided(true);

    application->addMesh(
        ref_ptr<AttributeState>::manage(new UnitQuad(quadConfig)),
        modelMat,
        material);
  }


  //application->setShowFPS();

  application->mainLoop();
  return 0;
}
