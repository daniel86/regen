/*
 * sky.cpp
 *
 *  Created on: Jan 5, 2014
 *      Author: daniel
 */

#include "sky.h"
using namespace regen::scene;
using namespace regen;

#include <regen/scene/resource-manager.h>
#include <regen/scene/input-processors.h>

#include <regen/sky/sky.h>
#include <regen/sky/atmosphere.h>
#include <regen/sky/cloud-layer.h>

#define REGEN_SKY_CATEGORY "sky"

SkyResource::SkyResource()
: ResourceProvider(REGEN_SKY_CATEGORY)
{}

ref_ptr<Sky> SkyResource::createResource(
    SceneParser *parser, SceneInputNode &input)
{
  if(!input.hasAttribute("camera")) {
    REGEN_WARN("No user-camera attribute specified for '" << input.getDescription() << "'.");
    return ref_ptr<Sky>();
  }
  ref_ptr<Camera> cam = parser->getResources()->getCamera(parser,input.getValue("camera"));
  if(cam.get()==NULL) {
    REGEN_WARN("No Camera can be found for '" << input.getDescription() << "'.");
    return ref_ptr<Sky>();
  }

  ref_ptr<Sky> sky = ref_ptr<Sky>::alloc(cam, parser->getViewport());

  if(input.hasAttribute("noon-color"))
    sky->set_noonColor(input.getValue<Vec3f>("noon-color", Vec3f(0.5)));
  if(input.hasAttribute("dawn-color"))
    sky->set_dawnColor(input.getValue<Vec3f>("dawn-color", Vec3f(0.2,0.15,0.15)));

  if(input.hasAttribute("moon-reflectance"))
    sky->set_moonSunLightReflectance(input.getValue<GLfloat>("moon-reflectance", 0.4));

  sky->set_altitude(input.getValue<GLfloat>("altitude", 0.043));
  sky->set_longitude(input.getValue<GLfloat>("longitude", 13.3611));
  sky->set_latitude(input.getValue<GLfloat>("latitude", 52.5491));

  if(input.hasAttribute("date"))
    sky->set_date(input.getValue("date"));
  if(input.hasAttribute("timestamp"))
    sky->set_timestamp(input.getValue<double>("timestamp", 0.0));
  sky->set_secondsPerCycle(
     input.getValue<double>("seconds-per-cycle", 3600.0));

  const list< ref_ptr<SceneInputNode> > &childs = input.getChildren();
  for(list< ref_ptr<SceneInputNode> >::const_iterator
      it=childs.begin(); it!=childs.end(); ++it)
  {
    ref_ptr<SceneInputNode> n = *it;

    if(n->getCategory() == "atmosphere") {
      createAtmosphereLayer(sky, parser, *n.get());
    }
    else if(n->getCategory() == "cloud-layer") {
      createCloudLayer(sky, parser, *n.get());
    }
    else if(n->getCategory() == "moon") {
      createMoonLayer(sky, parser, *n.get());
    }
    else if(n->getCategory() == "star-map") {
      createStarMapLayer(sky, parser, *n.get());
    }
    else if(n->getCategory() == "stars") {
      createStarsLayer(sky, parser, *n.get());
    }
  }

  // The Sky also exposes a Light (the sun) and a Texture (the cube map)
  parser->getResources()->putLight(input.getName()+"-sun", sky->sun());
  parser->getResources()->putLight(input.getName()+"-moon", sky->moon());
  parser->getResources()->putSky(input.getName(), sky);
  parser->putState(input.getName()+"-sun", sky->sun());
  parser->putState(input.getName()+"-moon", sky->moon());

  return sky;
}

ref_ptr<StarMapLayer> SkyResource::createStarMapLayer(const ref_ptr<Sky> &sky,
    SceneParser *parser, SceneInputNode &input)
{
  ref_ptr<StarMapLayer> starMap = ref_ptr<StarMapLayer>::alloc(sky);

  if(input.hasAttribute("texture"))
    starMap->set_texture(input.getValue("texture"));

  if(input.hasAttribute("scattering"))
    starMap->set_scattering(input.getValue<GLdouble>("scattering", starMap->defaultScattering()));

  if(input.hasAttribute("apparentMagnitude"))
    starMap->set_apparentMagnitude(input.getValue<GLdouble>("apparentMagnitude", starMap->defaultApparentMagnitude()));

  if(input.hasAttribute("colorRatio"))
    starMap->set_colorRatio(input.getValue<GLdouble>("colorRatio", starMap->defaultColorRatio()));

  if(input.hasAttribute("color"))
    starMap->set_color(input.getValue<Vec3f>("color", starMap->defaultColor()));

  starMap->set_updateInterval(
      input.getValue<GLdouble>("update-interval", 4000.0));
  sky->addLayer(starMap);

  return starMap;
}

ref_ptr<StarsLayer> SkyResource::createStarsLayer(
    const ref_ptr<Sky> &sky,
    SceneParser *parser, SceneInputNode &input)
{
  ref_ptr<StarsLayer> stars = ref_ptr<StarsLayer>::alloc(sky);

  if(input.hasAttribute("catalog"))
    stars->set_brightStarsFile(input.getValue("catalog"));

  if(input.hasAttribute("scattering"))
    stars->set_scattering(input.getValue<GLfloat>("scattering", stars->defaultScattering()));

  if(input.hasAttribute("apparent-magnitude"))
    stars->set_apparentMagnitude(input.getValue<GLfloat>("apparent-magnitude", stars->defaultApparentMagnitude()));

  if(input.hasAttribute("color"))
    stars->set_color(input.getValue<Vec3f>("color", stars->defaultColor()));

  if(input.hasAttribute("color-ratio"))
    stars->set_colorRatio(input.getValue<GLfloat>("color-ratio", stars->defaultColorRatio()));

  if(input.hasAttribute("glare-intensity"))
    stars->set_glareIntensity(input.getValue<GLfloat>("glare-intensity", 1.0f));

  if(input.hasAttribute("glare-scale"))
    stars->set_glareScale(input.getValue<GLfloat>("glare-scale", stars->defaultGlareScale()));

  if(input.hasAttribute("scintillation"))
    stars->set_scintillation(input.getValue<GLfloat>("scintillation", stars->defaultScintillation()));

  if(input.hasAttribute("scale"))
    stars->set_scale(input.getValue<GLfloat>("scale", 1.0f));

  stars->set_updateInterval(
      input.getValue<GLdouble>("update-interval", 4000.0));
  sky->addLayer(stars);

  return stars;
}

ref_ptr<Atmosphere> SkyResource::createAtmosphereLayer(const ref_ptr<Sky> &sky,
    SceneParser *parser, SceneInputNode &input)
{
  ref_ptr<Atmosphere> atmosphere = ref_ptr<Atmosphere>::alloc(sky,
      input.getValue<GLuint>("size", 512),
      input.getValue<GLuint>("use-float", false),
      input.getValue<GLuint>("lod",0));

  const string preset = input.getValue<string>("preset", "earth");
  if(preset == "earth")       atmosphere->setEarth();
  else if(preset == "mars")   atmosphere->setMars();
  else if(preset == "venus")  atmosphere->setVenus();
  else if(preset == "uranus") atmosphere->setUranus();
  else if(preset == "alien")  atmosphere->setAlien();
  else if(preset == "custom") {
    const Vec3f absorbtion =
        input.getValue<Vec3f>("absorbtion", Vec3f(
            0.18867780436772762,
            0.4978442963618773,
            0.6616065586417131));
    const Vec3f rayleigh =
        input.getValue<Vec3f>("rayleigh", Vec3f(19.0,359.0,81.0));
    const Vec4f mie =
        input.getValue<Vec4f>("mie", Vec4f(44.0,308.0,39.0,74.0));
    const GLfloat spot =
        input.getValue<GLfloat>("spot", 373.0);
    const GLfloat strength =
        input.getValue<GLfloat>("strength", 54.0);

    atmosphere->setRayleighBrightness(rayleigh.x);
    atmosphere->setRayleighStrength(rayleigh.y);
    atmosphere->setRayleighCollect(rayleigh.z);
    atmosphere->setMieBrightness(mie.x);
    atmosphere->setMieStrength(mie.y);
    atmosphere->setMieCollect(mie.z);
    atmosphere->setMieDistribution(mie.w);
    atmosphere->setSpotBrightness(spot);
    atmosphere->setScatterStrength(strength);
    atmosphere->setAbsorbtion(absorbtion);
  }
  else REGEN_WARN("Ignoring unknown sky preset '" << preset <<
      "' for node " << input.getDescription() << ".");

  atmosphere->set_updateInterval(
      input.getValue<GLdouble>("update-interval", 4000.0));

  parser->getResources()->putTexture(input.getName(), atmosphere->cubeMap());

  sky->addLayer(atmosphere);

  return atmosphere;
}

ref_ptr<CloudLayer> SkyResource::createCloudLayer(const ref_ptr<Sky> &sky,
    SceneParser *parser, SceneInputNode &input)
{
  ref_ptr<CloudLayer> cloudLayer = ref_ptr<CloudLayer>::alloc(sky,
      input.getValue<GLuint>("texture-size", 2048));

  if(input.hasAttribute("use-scatter") && string("TRUE") == (input.getValue("use-scatter")))
    cloudLayer->state()->shaderDefine("USE_SCATTER", "TRUE");

  if(input.hasAttribute("altitude"))
    cloudLayer->set_altitude(input.getValue<GLdouble>("altitude", cloudLayer->defaultAltitudeHigh()));
  if(input.hasAttribute("sharpness"))
    cloudLayer->set_sharpness(input.getValue<GLdouble>("sharpness", 0.5f));
  if(input.hasAttribute("coverage"))
    cloudLayer->set_coverage(input.getValue<GLdouble>("coverage", 0.2f));
  if(input.hasAttribute("change"))
    cloudLayer->set_change(input.getValue<GLdouble>("change", cloudLayer->defaultChangeHigh()));
  if(input.hasAttribute("scale"))
    cloudLayer->set_scale(input.getValue<Vec2f>("scale", cloudLayer->defaultScaleHigh()));
  if(input.hasAttribute("wind"))
    cloudLayer->set_wind(input.getValue<Vec2f>("wind", Vec2f(0.f, 0.f)));
  if(input.hasAttribute("color"))
    cloudLayer->set_color(input.getValue<Vec3f>("color", Vec3f(1.f, 1.f, 1.f)));
  if(input.hasAttribute("top-color"))
    cloudLayer->set_topColor(input.getValue<Vec3f>("top-color", Vec3f(1.f, 1.f, 1.f)));
  if(input.hasAttribute("bottom-color"))
    cloudLayer->set_bottomColor(input.getValue<Vec3f>("bottom-color", Vec3f(1.f, 1.f, 1.f)));
  if(input.hasAttribute("offset"))
    cloudLayer->set_offset(input.getValue<GLdouble>("offset", -0.5f));
  if(input.hasAttribute("thickness"))
    cloudLayer->set_thickness(input.getValue<GLdouble>("thickness", 3.0f));

  cloudLayer->set_updateInterval(
      input.getValue<GLdouble>("update-interval", 4000.0));
  sky->addLayer(cloudLayer);

  return cloudLayer;
}

ref_ptr<MoonLayer> SkyResource::createMoonLayer(const ref_ptr<Sky> &sky,
    SceneParser *parser, SceneInputNode &input)
{
  const string textureFile = getResourcePath(input.getValue("texture"));
  ref_ptr<MoonLayer> moon = ref_ptr<MoonLayer>::alloc(sky, textureFile);

  if(input.hasAttribute("scale"))
    moon->set_scale(input.getValue<GLdouble>("scale", moon->defaultScale()));

  if(input.hasAttribute("sun-shine-color"))
    moon->set_sunShineColor(input.getValue<Vec3f>("sun-shine-color", moon->defaultSunShineColor()));

  if(input.hasAttribute("earth-shine-color"))
    moon->set_earthShineColor(input.getValue<Vec3f>("earth-shine-color", moon->defaultEarthShineColor()));

  if(input.hasAttribute("sun-shine-intensity"))
    moon->set_sunShineIntensity(input.getValue<GLfloat>("sun-shine-intensity", moon->defaultSunShineIntensity()));

  if(input.hasAttribute("earth-shine-intensity"))
    moon->set_earthShineIntensity(input.getValue<GLfloat>("earth-shine-intensity", moon->defaultEarthShineIntensity()));

  moon->set_updateInterval(
      input.getValue<GLdouble>("update-interval", 4000.0));
  sky->addLayer(moon);

  return moon;
}

#if 0
ref_ptr<State> StateResource::createSkyState(
    SceneParser *parser, SceneInputNode &input)
{
  const string stateType = input.getValue("type");
  ref_ptr<State> state;

  if(stateType == "star-map") {
    const string textureFile = getResourcePath(input.getValue("texture"));
    ref_ptr<StarMap> starmap = ref_ptr<StarMap>::alloc(textureFile);

    starmap->set_apparentMagnitude(
        input.getValue<GLfloat>("apparent-magnitude", 6.0));

    state = starmap;
  }
  else if(stateType == "bright-stars") {
    const string catalogue = getResourcePath(input.getValue("catalogue"));
    ref_ptr<BrightStars> stars = ref_ptr<BrightStars>::alloc(catalogue);

    stars->scattering()->setVertex(0,
        input.getValue<GLfloat>("scattering", 4.0));
    stars->scintillations()->setVertex(0,
        input.getValue<GLfloat>("scintillations", 20.0));
    stars->scale()->setVertex(0,
        input.getValue<GLfloat>("scale", 1.0));
    stars->glareIntensity()->setVertex(0,
        input.getValue<GLfloat>("glare-intensity", 1.0));
    stars->glareScale()->setVertex(0,
        input.getValue<GLfloat>("glare-scale", 1.0));
    stars->apparentMagnitude()->setVertex(0,
        input.getValue<GLfloat>("apparent-magnitude", 7.0));
    stars->starColor()->setVertex(0,
        input.getValue<Vec4f>("color", Vec4f(0.66,0.78,1.0,0.66)));

    ref_ptr<MeshVector> meshVec = ref_ptr<MeshVector>::alloc();
    meshVec.get()->push_back(stars->mesh());
    parser->getResources()->putMesh(input.getName(),meshVec);

    state = stars;
  }
  else if(stateType == "moon") {
    const string textureFile = getResourcePath(input.getValue("texture"));
    ref_ptr<Moon> moon = ref_ptr<Moon>::alloc(textureFile);

    moon->set_sunShineColor(
        input.getValue<Vec3f>("sun-shine-color", Vec3f(0.923,0.786,0.636)));
    moon->set_sunShineIntensity(
        input.getValue<GLfloat>("sun-shine-intensity", 56.0));

    moon->set_earthShineColor(
        input.getValue<Vec3f>("earth-shine-color", Vec3f(0.88,0.96,1.0)));
    moon->set_earthShineIntensity(
        input.getValue<GLfloat>("earth-shine-intensity", 1.0));

    moon->set_scale(
        input.getValue<GLfloat>("scale", 2.0));

    state = moon;
  }
  else if(stateType == "atmosphere") {
    ref_ptr<Atmosphere> atmosphere = ref_ptr<Atmosphere>::alloc();

    state = atmosphere;
  }
  else if(stateType == "high-clouds") {
  }
  else if(stateType == "low-clouds") {
  }
  else if(stateType == "sky") {
    if(!input.hasAttribute("user-camera")) {
      REGEN_WARN("No user-camera attribute specified for '" << input.getDescription() << "'.");
      return state;
    }
    ref_ptr<Camera> cam = parser->getResources()->getCamera(parser,input.getValue("user-camera"));
    if(cam.get()==NULL) {
      REGEN_WARN("No Camera can be found for '" << input.getDescription() << "'.");
      return state;
    }

    ref_ptr<Sky2> sky = ref_ptr<Sky2>::alloc(cam, parser->getViewport());

    // TODO: handle time
    // time="12.07.2007 01:00"
    // sky->setTime(const time_t &time);

    // TODO: handle UTC offset
    // utc-offset="-7200"
    // sky->setUTCOffset(const time_t &offset);

    sky->setSecondsPerCycle(
        input.getValue<long double>("seconds-per-cycle", 3600.0L));

    sky->setAltitude(
        input.getValue<float>("altitude", 0.043));
    sky->setLongitude(
        input.getValue<float>("longitude", 13.3611));
    sky->setLatitude(
        input.getValue<float>("latitude", 52.5491));

    const string layerStr = input.getValue("layer");
    vector<string> layerVec;
    list< ref_ptr<SkyLayer> > layerList;
    boost::split(layerVec, layerStr, boost::is_any_of(","));
    for(GLuint i=0u; i<layerVec.size(); ++i) {
      ref_ptr<State> layerState = parser->getResources()->getState(parser,layerVec[i]);
      ref_ptr<SkyLayer> layer = ref_ptr<SkyLayer>::upCast(layerState);
      if(layer.get()==NULL) {
        REGEN_WARN("Unable to find Sky layer state '" << layerVec[i] << "'.");
      } else {
        layerList.push_back(layer);
      }
    }
    sky->set_skyLayer(layerList);

    state = sky;
  }
  if(state.get()==NULL) return state;

  return state;
}
#endif



