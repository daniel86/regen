
osg::ref_ptr<osg::Image> HighCloudLayerGeode::createNoiseSlice(
    const unsigned int texSize
,   const unsigned int octave)
{
    const unsigned int size2 = texSize * texSize;
    const float oneOverTexSize = 1.f / static_cast<float>(texSize);

    Noise n(1 << (octave + 2), _randf(0.f, 1.f), _randf(0.f, 1.f));

    float *noise = new float[size2];
    unsigned int o;

    for(unsigned int s = 0; s < texSize; ++s)
    for(unsigned int t = 0; t < texSize; ++t)
    {
        o = t * texSize + s;
        noise[o] = n.noise2(s * oneOverTexSize, t * oneOverTexSize, octave) * 0.5 + 0.5;
    }

    osg::ref_ptr<osg::Image> image = new osg::Image();
    image->setImage(texSize, texSize, 1
        , GL_LUMINANCE16F_ARB, GL_LUMINANCE, GL_FLOAT
        , reinterpret_cast<unsigned char*>(noise), osg::Image::USE_NEW_DELETE);

    return image;
}

void DubeCloudLayerGeode::update(const Himmel &himmel)
{
    // TODO: starmap and planets also require this ... - find better place
    const float fov = himmel.getCameraFovHint();
    const float height = himmel.getViewSizeHeightHint();

    u_q->set(static_cast<float>(sqrt(2.0) * 2.0 * tan(_rad(fov * 0.5)) / height));

    u_time->set(static_cast<float>(himmel.getTime()->getf()));
}
void DubeCloudLayerGeode::setupTextures(osg::StateSet* stateSet)
{
    srand(time(NULL));

    m_preNoise = new osg::Texture2D;

    osg::Group *preNoise(HighCloudLayerGeode::createPreRenderedNoise(m_noiseSize, m_preNoise));
    osg::StateSet *pnStateSet(preNoise->getOrCreateStateSet());

    // precompute tilable permutations
    // generate textures 128, 256, 512, 1024 with rank 8, 16, 32, 64

    // TODO: the use of m_noise as an array insead of a texture array was due to problems
    // with osg and not enough time and vigor to fix this.. :D

    m_noise[0] = HighCloudLayerGeode::createNoiseArray(1 << 6, 3, 4);
    m_noise[1] = HighCloudLayerGeode::createNoiseArray(1 << 7, 4, 4);
    m_noise[2] = HighCloudLayerGeode::createNoiseArray(1 << 8, 5, 4);
    m_noise[3] = HighCloudLayerGeode::createNoiseArray(1 << 8, 6, 4);

    pnStateSet->addUniform(u_time);

    pnStateSet->addUniform(u_noise0);
    u_noise0->set(1);
    pnStateSet->addUniform(u_noise1);
    u_noise1->set(2);
    pnStateSet->addUniform(u_noise2);
    u_noise2->set(3);
    pnStateSet->addUniform(u_noise3);
    u_noise3->set(4);

    pnStateSet->setTextureAttributeAndModes(1, m_noise[0]);
    pnStateSet->setTextureAttributeAndModes(2, m_noise[1]);
    pnStateSet->setTextureAttributeAndModes(3, m_noise[2]);
    pnStateSet->setTextureAttributeAndModes(4, m_noise[3]);


    addChild(preNoise);

    u_clouds->set(0);
    stateSet->setTextureAttributeAndModes(0, m_preNoise, osg::StateAttribute::ON);
    u_noise->set(1);
    stateSet->setTextureAttributeAndModes(1, m_noise[3], osg::StateAttribute::ON);
}

-- layerIntersectionOrDiscard
float layerIntersectionOrDiscard(const vec3 d, const float altitude) {
  vec3  o = vec3(0.0, 0.0, in_cmn[1] + in_cmn[0]);
  float r = in_cmn[1] + altitude;
  // for now, ignore if altitude is above cloud layer
  if(o.z > r) discard;
  
  float a = dot(d, d);
  float b = 2 * dot(d, o);
  float c = dot(o, o) - r * r;
  float B = b * b - 4 * a * c;
  if(B < 0) discard;
  B = sqrt(B);
  return (-b + B) * 0.5 / a;
}

--------------------
// Intersection of view ray (d) with a sphere of radius = mean earth
// radius + altitude (altitude). Support is only for rays starting
// below the cloud layer (o must be inside the sphere...).
// (http://wiki.cgsociety.org/index.php/Ray_Sphere_Intersection) and
// (http://www.siggraph.org/education/materials/HyperGraph/raytrace/rtinter1.htm)
-- layerIntersection
bool layerIntersection(const vec3 d, const vec3 o, const float altitude, out float t)
{
  float r = in_cmn[1] + altitude;
  // for now, ignore if altitude is above cloud layer
  if(o.z > r) return false;
  
  float a = dot(d, d);
  float b = 2 * dot(d, o);
  float c = dot(o, o) - r * r;
  float B = b * b - 4 * a * c;
  B = sqrt(B);
  
  float q;
  if(b < 0) q = (-b - B) * 0.5;
  else      q = (-b + B) * 0.5;
  
  float t0 = q / a;
  float t1 = c / q;
  if(t0 > t1) {
    q  = t0;
    t0 = t1;
    t1 = q;
  }
  if(t1 < 0) return false;
  
  t = t0 < 0 ? t1 : t0;
  return true;
}

-- T
float T(sampler2D tex, vec2 uv) {
  return texture(tex, uv * in_scale).r;
}
float T(sampler2D tex, vec3 stu) {
  float m = 2.0 * (1.0 + stu.z);
  vec2 uv = vec2(stu.x / m + 0.5, stu.y / m + 0.5);
  return T(tex,uv);
}

--------------------------------------------------------
--------------------------------------------------------
--------------------------------------------------------
// Depth(osg::Depth::LEQUAL, 1.0, 1.0)
// BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
-- high-layer.vs
in vec3 in_pos;
void main() {
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- high-layer.fs
out vec4 out_color;

uniform sampler2D in_cloudTexture;

const float in_altitude = 8.0;
const vec2 in_scale = vec2(32.0);
const vec3 in_color = vec3(1.0);

#include regen.sky.utility.belowHorizon
#include regen.sky.clouds.layerIntersection
#include regen.sky.clouds.T
#include regen.states.camera.transformTexcoToWorld

void main() {
  vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
  vec3 ws = transformTexcoToWorld(texco_2D, 1.0, 0);
  vec3 eye = normalize(vec3(ws.x,ws.z,ws.y));
  if(!belowHorizon(eye)) discard;
  
  float t;
  vec3 o = vec3(0, 0, in_cmn[1] + in_cmn[0]);
  layerIntersection(eye, o, in_altitude, t);
  
  out_color = vec4(in_color, T(in_cloudTexture, o + t * eye));
}

--------------------------------------------------------
--------------------------------------------------------
--------------------------------------------------------
// Depth(osg::Depth::LEQUAL, 1.0, 1.0)
// BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
-- low-layer.vs
#include regen.filter.sampling.vs
-- low-layer.gs
#include regen.filter.sampling.gs  
-- low-layer.fs
out vec4 out_color;

uniform sampler2D in_cloudTexture;

uniform vec3 in_sunPositionR;

const float in_altitude = 2.0;
const float in_offset = -0.5;
const float in_thickness = 3.0;
const vec2 in_scale = vec2(128.0);

const vec3 in_tcolor = vec3(1.0);
const vec3 in_bcolor = vec3(1.0);

const float in_sSteps = 128;
const float in_sRange =  8;
const int in_dSteps = 32;
const float in_dRange = 1.9;

#include regen.sky.utility.belowHorizon
#include regen.sky.clouds.layerIntersection
#include regen.sky.clouds.T
#include regen.states.camera.transformTexcoToView

float density(in vec3 stu0, in vec3 sun, in float aa0) {
    float iSteps = 1.0 / (in_dSteps - 1);
    float iRange = in_dRange * in_thickness * iSteps;

    vec3 stu1 = stu0 + sun * in_dRange * in_thickness;
    vec3 Dstu = (stu1 - stu0) * iSteps;
    vec3 stu  = stu0;

    float d = 0.0;
    float a1 = in_thickness + in_offset;
    for(int i = 0; i < in_dSteps; ++i) {
        float t = T(in_cloudTexture,stu);
        float a = aa0 + i * iRange;
        if(a > t * in_offset && a < t * a1) d += iSteps;
        stu += Dstu;
    }

    return d;
}

vec2 scatter(in vec3 eye, in vec3 sun) {
    vec3 o0 = vec3(0, 0, in_cmn[1] + in_cmn[0]);
    // check if intersects with lower cloud sphere    
    float t0, t1;
    float a1 = in_thickness + in_offset;

    if(!layerIntersection(eye, o0, in_altitude + in_offset, t0) ||
       !layerIntersection(eye, o0, in_altitude + a1, t1)) {
        return 0.0;
    }

    vec3 stu0 = o0 + t0 * eye;
    vec3 stu1 = o0 + t1 * eye;
    float iSteps = 1.0 / (in_sSteps - 1);
    vec3 Dstu = (stu1 - stu0) / (in_sSteps - 1);
    vec3 stu  = stu0;
    float Da = in_thickness * iSteps;

    vec2 sd = vec2(0.0);
    for(int i = 0; i < in_sSteps; ++i) {
        float t = T(in_cloudTexture,stu);
        float a = in_offset + i * Da;
        if(a > t * in_offset && a < t * a1) {
            sd.x += density(stu, sun, a);
            ++sd.y;
        }
        if(sd.y >= in_sRange) break;
        stu += Dstu;
    }
    sd.x /= in_sRange;
    sd.y /= in_sRange;

    return sd;
}

void main() {
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    // TODO: compute in VS
    vec3 eye = normalize( transformTexcoToView(texco_2D, 1.0, in_layer) );
    if(belowHorizon(eye)) discard;

    float t;
    vec3 o = vec3(0, 0, in_cmn[1] + in_cmn[0]);
    layerIntersection(eye, o, in_altitude, t);

    vec2 sd = scatter(eye, normalize(in_sunPositionR));
    sd.y *= (1.0 - pow(t, 0.8) * 12e-3);

    out_color = vec4(mix(in_tcolor, in_bcolor, sd.x) * (1 - sd.x), sd.y);
}

--------------------------------------------------------
--------------------------------------------------------
--------------------------------------------------------
// MIN_FILTER=LINEAR, MAG_FILTER=LINEAR
// internal:GL_LUMINANCE16F_ARB, format:GL_LUMINANCE
// clear color: 0.0f, 0.0f, 0.0f, 1.0f
-- pre-noise.vs
#include regen.filter.sampling.vs
-- pre-noise.gs
#include regen.filter.sampling.gs  
-- pre-noise.fs
out vec4 out_color;

uniform sampler3D in_noise0;
uniform sampler3D in_noise1;
uniform sampler3D in_noise2;
uniform sampler3D in_noise3;

const float in_time = 0.0;
const float in_coverage = 0.2;
const float in_sharpness = 0.5;
const float in_change = 0.1;
const vec2 in_wind = vec2(0.0);

void main() {
   vec2 uv = gl_FragCoord.xy*in_inverseViewport;
   float t = in_time * 3600.0;
   vec2 m = t * in_wind;
   t *= in_change;

   float n = 0;
   n += 1.00000 * texture(in_noise0, vec3(uv     + m * 0.18, t * 0.01)).r;
   n += 0.50000 * texture(in_noise1, vec3(uv     + m * 0.16, t * 0.02)).r;
   n += 0.25000 * texture(in_noise2, vec3(uv     + m * 0.14, t * 0.04)).r;
   n += 0.12500 * texture(in_noise3, vec3(uv     + m * 0.12, t * 0.08)).r;
   n += 0.06750 * texture(in_noise3, vec3(uv * 2 + m * 0.10, t * 0.16)).r;
   n *= 0.76;
   n = n - 1 + in_coverage;
   n /= in_coverage;
   n = max(0.0, n);
   n = pow(n, 1.0 - in_sharpness);

   out_color = vec4(n);
}
