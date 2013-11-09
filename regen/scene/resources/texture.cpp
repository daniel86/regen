/*
 * texture.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "texture.h"
using namespace regen::scene;
using namespace regen;

#include <regen/gl-types/gl-enum.h>
#include <regen/av/video-texture.h>
#include <regen/textures/texture-loader.h>
#include <regen/textures/noise-texture.h>

#define REGEN_TEXTURE_CATEGORY "texture"

/**
 * Resizes Texture texture when the window size changed
 */
class TextureResizer : public EventHandler
{
public:
  /**
   * Default constructor.
   * @param tex Texture reference.
   * @param windowViewport The window dimensions.
   * @param wScale The width scale.
   * @param hScale The height scale.
   */
  TextureResizer(const ref_ptr<Texture> &tex,
      const ref_ptr<ShaderInput2i> &windowViewport,
      GLfloat wScale, GLfloat hScale)
  : EventHandler(),
    tex_(tex),
    windowViewport_(windowViewport),
    wScale_(wScale), hScale_(hScale) { }

  void call(EventObject*, EventData*) {
    const Vec2i& winSize = windowViewport_->getVertex(0);

    tex_->set_rectangleSize(winSize.x*wScale_, winSize.y*hScale_);
    RenderState::get()->textures().push(7, tex_->textureBind());
    tex_->texImage();
    RenderState::get()->textures().pop(7);
  }

protected:
  ref_ptr<Texture> tex_;
  ref_ptr<ShaderInput2i> windowViewport_;
  GLfloat wScale_, hScale_;
};

Vec3i TextureResource::getSize(
    const ref_ptr<ShaderInput2i> &viewport,
    const string &sizeMode,
    const Vec3f &size)
{
  if(sizeMode == "abs") {
    return Vec3i(size.x,size.y,size.z);
  }
  else if(sizeMode=="rel") {
    Vec2i v = viewport->getVertex(0);
    return Vec3i(
        (GLint)(size.x*v.x),
        (GLint)(size.y*v.y),1);
  }
  else {
    REGEN_WARN("Unknown size mode '" << sizeMode << "'.");
    return Vec3i(size.x,size.y,size.z);
  }
}

TextureResource::TextureResource()
: ResourceProvider(REGEN_TEXTURE_CATEGORY)
{}

ref_ptr<Texture> TextureResource::createResource(
    SceneParser *parser, SceneInputNode &input)
{
  ref_ptr<Texture> tex;

  if(input.hasAttribute("file")) {
    GLenum mipmapFlag = GL_DONT_CARE;
    GLenum forcedType = glenum::pixelType(
        input.getValue<string>("forced-type","NONE"));
    GLenum forcedInternalFormat = glenum::textureInternalFormat(
        input.getValue<string>("forced-internal-format","NONE"));
    GLenum forcedFormat = glenum::textureFormat(
        input.getValue<string>("forced-format","NONE"));
    Vec3ui forcedSize =
        input.getValue<Vec3ui>("forced-size",Vec3ui(0u));
    const string filePath =
        getResourcePath(input.getValue("file"));

    try {
      if(input.getValue<bool>("is-cube",false)) {
        tex = textures::loadCube(
            filePath,
            input.getValue<bool>("cube-flip-back",false),
            mipmapFlag,
            forcedInternalFormat,
            forcedFormat,
            forcedType,
            forcedSize);
      }
      else if(input.getValue<bool>("is-array",false)) {
        tex = textures::loadArray(
            filePath,
            input.getValue<string>("name-pattern","[ ]*"),
            mipmapFlag,
            forcedInternalFormat,
            forcedFormat,
            forcedType,
            forcedSize);
      }
      else if(input.getValue<bool>("is-raw",false)) {
        tex = textures::loadRAW(
            filePath,
            input.getValue<Vec3ui>("raw-size",Vec3ui(256u)),
            input.getValue<GLuint>("raw-components",3u),
            input.getValue<GLuint>("raw-bytes",4u));
      }
      else {
        tex = textures::load(
            filePath,
            mipmapFlag,
            forcedInternalFormat,
            forcedFormat,
            forcedType,
            forcedSize);
      }
    }
    catch(textures::Error &ie) {
      REGEN_ERROR("Failed to load Texture at " << filePath << ".");
    }
  }
  else if(input.hasAttribute("video")) {
    const string filePath = getResourcePath(input.getValue("file"));
    ref_ptr<VideoTexture> video = ref_ptr<VideoTexture>::alloc();
    video->stopAnimation();
    try
    {
      video->set_file(filePath);
      video->demuxer()->set_repeat(
          input.getValue<bool>("repeat",1));

      tex = video;
      video->startAnimation();
    }
    catch(VideoTexture::Error &ve)
    {
      REGEN_ERROR("Failed to load Video at " << filePath << ".");
    }
  }
  else if(input.hasAttribute("noise")) {
    const string noiseMode = input.getValue("noise");

    string sizeMode = input.getValue<string>("size-mode", "abs");
    Vec3f sizeRel   = input.getValue<Vec3f>("size", Vec3f(256.0,256.0,1.0));
    Vec3i sizeAbs   = getSize(parser->getViewport(),sizeMode,sizeRel);

    GLint randomSeed = input.getValue<GLint>("random-seed", rand());
    bool isSeamless  = input.getValue<bool>("is-seamless", false);

    textures::PerlinNoiseConfig perlinCfg;
    perlinCfg.baseFrequency = input.getValue<GLfloat>("base-frequency", 4.0);
    perlinCfg.persistence   = input.getValue<GLfloat>("persistence", 0.5);
    perlinCfg.lacunarity    = input.getValue<GLfloat>("lacunarity", 2.5);
    perlinCfg.octaveCount   = input.getValue<GLuint>("octave-count", 4);

    if(noiseMode == "cloud") {
      tex = regen::textures::clouds2D(
          sizeAbs.x, sizeAbs.y, randomSeed, isSeamless);
    }
    else if(noiseMode == "wood") {
      tex = regen::textures::wood(
          sizeAbs.x, sizeAbs.y, randomSeed, isSeamless);
    }
    else if(noiseMode == "granite") {
      tex = regen::textures::granite(
          sizeAbs.x, sizeAbs.y, randomSeed, isSeamless);
    }
    else if(noiseMode == "perlin-2d" || noiseMode == "perlin") {
      tex = regen::textures::perlin2D(
          sizeAbs.x, sizeAbs.y,
          perlinCfg,
          randomSeed, isSeamless);
    }
    else if(noiseMode == "perlin-3d") {
      tex = regen::textures::perlin3D(
          sizeAbs.x, sizeAbs.y, sizeAbs.z,
          perlinCfg,
          randomSeed, isSeamless);
    }
  }
  else {
    string sizeMode = input.getValue<string>("size-mode", "abs");
    Vec3f sizeRel   = input.getValue<Vec3f>("size", Vec3f(256.0,256.0,1.0));
    Vec3i sizeAbs   = getSize(parser->getViewport(),sizeMode,sizeRel);

    GLuint texCount        = input.getValue<GLuint>("count", 1);
    GLuint pixelSize       = input.getValue<GLuint>("pixel-size", 16);
    GLuint pixelComponents = input.getValue<GLuint>("pixel-components", 4);
    GLenum pixelType = glenum::pixelType(
        input.getValue<string>("pixel-type", "UNSIGNED_BYTE"));

    tex = FBO::createTexture(
        sizeAbs.x,sizeAbs.y,sizeAbs.z,
        texCount,
        sizeAbs.z>1 ? GL_TEXTURE_3D : GL_TEXTURE_2D,
        glenum::textureFormat(pixelComponents),
        glenum::textureInternalFormat(pixelType,pixelComponents,pixelSize),
        pixelType);

    if(sizeMode == "rel") {
      ref_ptr<TextureResizer> resizer = ref_ptr<TextureResizer>::alloc(
          tex, parser->getViewport(), sizeRel.x, sizeRel.y);
      parser->addEventHandler(Application::RESIZE_EVENT, resizer);
    }
  }

  if(tex.get()==NULL) {
    REGEN_WARN("Failed to create Texture for '" << input.getDescription() << ".");
    return tex;
  }

  tex->begin(RenderState::get(), 0); {
    if(input.hasAttribute("wrapping")) {
      tex->wrapping().push(glenum::wrappingMode(
          input.getValue<string>("wrapping","CLAMP_TO_EDGE")));
    }
    if(input.hasAttribute("aniso")) {
      tex->aniso().push(input.getValue<GLfloat>("aniso",2.0f));
    }
    if(input.hasAttribute("lod")) {
      tex->lod().push(input.getValue<Vec2f>("lod",Vec2f(1.0f)));
    }
    if(input.hasAttribute("swizzle-r") ||
       input.hasAttribute("swizzle-g") ||
       input.hasAttribute("swizzle-b") ||
       input.hasAttribute("swizzle-a")) {
      GLenum swizzleR = glenum::textureSwizzle(
          input.getValue<string>("swizzle-r","RED"));
      GLenum swizzleG = glenum::textureSwizzle(
          input.getValue<string>("swizzle-g","GREEN"));
      GLenum swizzleB = glenum::textureSwizzle(
          input.getValue<string>("swizzle-b","BLUE"));
      GLenum swizzleA = glenum::textureSwizzle(
          input.getValue<string>("swizzle-a","ALPHA"));
      tex->swizzle().push(Vec4i(swizzleR,swizzleG,swizzleB,swizzleA));
    }
    if(input.hasAttribute("compare-mode")) {
      GLenum function = glenum::compareFunction(
          input.getValue<string>("compare-function","LEQUAL"));
      GLenum mode = glenum::compareMode(
          input.getValue<string>("compare-mode","NONE"));
      tex->compare().push(Vec2i(function,mode));
    }
    if(input.hasAttribute("max-level")) {
      tex->maxLevel().push(input.getValue<GLint>("max-level",1000));
    }

    if(input.hasAttribute("min-filter") && input.hasAttribute("mag-filter")) {
      GLenum min = glenum::filterMode(input.getValue("min-filter"));
      GLenum mag = glenum::filterMode(input.getValue("mag-filter"));
      tex->filter().push(TextureFilter(min,mag));
    }
    else if(input.hasAttribute("min-filter") || input.hasAttribute("mag-filter")) {
      REGEN_WARN("Minifiacation and magnification filters must be specified both." <<
          " One missing for '" << input.getDescription() << "'.");
    }
  } tex->end(RenderState::get(), 0);

  return tex;
}


