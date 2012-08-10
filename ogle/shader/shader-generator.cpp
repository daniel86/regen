/*
 * shader-generator.cpp
 *
 *  Created on: 30.10.2011
 *      Author: daniel
 */

#include "shader-generator.h"
#include "blending-shader.h"
#include "light-shader.h"
#include "normal-mapping.h"
#include "raycast-shader.h"
#include "tesselation-shader.h"
#include "shader-manager.h"
#include <ogle/states/texture-state.h>
#include <ogle/states/attribute-state.h>
#include <ogle/utility/string-util.h>

class MultiplyFloatFloatFunction : public ShaderFunctions {
public:
  MultiplyFloatFloatFunction(const vector<string> &args) :
          ShaderFunctions("multiplyFloatFloat", args) {};
  virtual string code() const {
    return
        "void multiplyFloatFloat(float f1, inout float f2) {\n"
        "    f2 *= f1;\n";
        "}\n";
  }
};
class MultiplyFloatVec4Function : public ShaderFunctions {
public:
  MultiplyFloatVec4Function(const vector<string> &args) :
          ShaderFunctions("multiplyFloatVec4", args) {};
  virtual string code() const {
    return
        "void multiplyFloatVec4(float f1, inout vec4 f2) {\n"
        "    f2 *= f1;\n"
        "}\n";
  }
};
class CalculateFragColor : public ShaderFunctions {
public:
  CalculateFragColor(const vector<string> &args, bool useFog) :
          ShaderFunctions("calculateColor", args), useFog_(useFog) {};
  virtual string code() const {
    stringstream s;
    s << "void calculateColor(inout vec4 outcol, vec4 materialAmbient, " << endl;
    s << "           vec4 emission, vec4 ambient, vec4 diffuse, vec4 specular," << endl;
    s << "           float brightness, float alpha) {" << endl;
    s << "    float outcolAlpha = outcol.a;" << endl;
    s << "    outcol = (emission + ambient + brightness*diffuse) * outcol + brightness*specular;" << endl;
    s << "    outcol.a = outcolAlpha * alpha;" << endl;
    s << "" << endl;
    if(useFog_)
    {
      s << "    float fog = clamp(fogScale*(fogEnd + gl_FragCoord.z), 0.0, 1.0);" << endl;
      s << "    outcol = mix(fogColor, outcol, fog);" << endl;
    }
    s << "}" << endl << endl;
    return s.str();
  }
private:
  bool useFog_;
};
class CalculateFragColorUnshaded : public ShaderFunctions {
public:
  CalculateFragColorUnshaded(const vector<string> &args, bool useFog) :
          ShaderFunctions("calculateColor", args), useFog_(useFog) {};
  virtual string code() const {
    stringstream s;
    s << "void calculateColor(inout vec4 outcol, float brightness, float alpha) {" << endl;
    s << "    outcol = vec4(brightness*outcol.xyz, outcol.a * alpha);" << endl;
    s << "" << endl;
    if(useFog_)
    {
      s << "    float fog = clamp(fogScale*(fogEnd + gl_FragCoord.z), 0.0, 1.0);" << endl;
      s << "    outcol = mix(fogColor, outcol, fog);" << endl;
    }
    s << "}" << endl << endl;
    return s.str();
  }
private:
  bool useFog_;
};
class LightMapFunction : public ShaderFunctions {
public:
  LightMapFunction(const vector<string> &args) :
          ShaderFunctions("lightMapFunc", args) {};
  virtual string code() const {
    stringstream s;
    s << "void lightMapFunc( " << endl;
    s << "        inout vec4 ambient, inout vec4 diffuse, inout vec4 specular, " << endl;
    s << "        vec4 matAmbient, vec4 matDiffuse, vec4 matSpecular, " << endl;
    s << "        vec4 texel) {" << endl;
    s << "    ambient += matAmbient*texel;" << endl;
    s << "    diffuse += matDiffuse*texel;" << endl;
    s << "    specular += matSpecular*texel;" << endl;
    s << "}" << endl << endl;
    return s.str();
  }
};
static string interpolateQuad(const string &a, const string &n)
{
  // gl_TessCoord is a uv coordinate for triangles
  stringstream ss;
  ss << "mix(";
    ss << " mix("<<a<<"[1]."<<n<<", "<<a<<"[0]."<<n<<", gl_TessCoord.x),";
    ss << " mix("<<a<<"[2]."<<n<<", "<<a<<"[3]."<<n<<", gl_TessCoord.x),";
  ss << " gl_TessCoord.y)";
  return ss.str();
}
static string interpolateTriangle(const string &a, const string &n)
{
  // gl_TessCoord is a barycentric coordinate for triangles
  stringstream ss;
  ss << "(";
    ss << " gl_TessCoord.z * "<<a<<"[0]."<<n<<" + ";
    ss << " gl_TessCoord.x * "<<a<<"[1]."<<n<<" + ";
    ss << " gl_TessCoord.y * "<<a<<"[2]."<<n<<"   ";
  ss << ")";
  return ss.str();
}




ShaderGenerator::ShaderGenerator()
: useFog_(false),
  ignoreViewRotation_(false),
  hasInstanceMat_(false),
  useTessShader_(false),
  hasNormalMapInTangentSpace_(false),
  primaryColAttribute_(NULL),
  posAttribute_(NULL),
  norAttribute_(NULL),
  tessConfig_(TESS_PRIMITVE_TRIANGLES, 3)
{
}

void ShaderGenerator::addUniform(ShaderFunctions &shader, const string &type, const string &name)
{
  shader.addUniform( GLSLUniform(type, name) );
}
void ShaderGenerator::addUniformToAll(const string &type, const string &name)
{
  addUniform(fragmentShader_, type, name);
  addUniform(vertexShader_, type, name);
  addUniform(tessEvalShader_, type, name);
  addUniform(tessControlShader_, type, name);
  addUniform(geometryShader_, type, name);
}
void ShaderGenerator::addConstant(const GLSLConstant &v)
{
  fragmentShader_.addConstant(v);
  vertexShader_.addConstant(v);
  tessControlShader_.addConstant(v);
  tessEvalShader_.addConstant(v);
  geometryShader_.addConstant(v);
}

string ShaderGenerator::interpolate(const string &a, const string &n)
{
  if(tessConfig_.primitive == TESS_PRIMITVE_QUADS) {
    return interpolateQuad(a,n);
  } else if(tessConfig_.primitive == TESS_PRIMITVE_TRIANGLES) {
    return interpolateTriangle(a,n);
  } else {
    return a+"[0]."+n;
  }
}

#define IS_STAGE_USED(s) (\
    s.inputs().size()>0 ||\
    s.outputs().size()>0)
map<GLenum, ShaderFunctions> ShaderGenerator::getShaderStages()
{
  map<GLenum, ShaderFunctions> stages;
  if(IS_STAGE_USED(vertexShader_)) {
    stages[GL_VERTEX_SHADER] = vertexShader_;
  }
  if(IS_STAGE_USED(fragmentShader_)) {
    stages[GL_FRAGMENT_SHADER] = fragmentShader_;
  }
  if(IS_STAGE_USED(geometryShader_)) {
    stages[GL_GEOMETRY_SHADER] = geometryShader_;
  }
  if(IS_STAGE_USED(tessControlShader_)) {
    stages[GL_TESS_CONTROL_SHADER] = tessControlShader_;
  }
  if(IS_STAGE_USED(tessEvalShader_)) {
    stages[GL_TESS_EVALUATION_SHADER] = tessEvalShader_;
  }
  return stages;
}
#undef IS_STAGE_USED

void ShaderGenerator::generate(const ShaderConfiguration *cfg)
{
  // TODO: unset uniforms should be handled.
  //   maybe reset some to default value, for some
  //   make simpler calculations (without view/ mat,...)
  Material *mat = (Material*)cfg->material();
  shading_ = (mat!=NULL ? mat->shading() : Material::NO_SHADING);
  transferNorToTES_ = false;
  hasInstanceMat_ = false;

  maxNumBoneWeights_ = cfg->maxNumBoneWeights();

  ignoreViewRotation_ = cfg->ignoreCameraRotation();
  ignoreViewTranslation_ = cfg->ignoreCameraTranslation();
  isTwoSided_ = (mat!=NULL ? mat->twoSided() : false);
  // use fog only under some circumstances
  useFog_ = cfg->useFog();
  useTessShader_ = cfg->useTesselation();
  useShading_ = shading_!=Material::NO_SHADING && cfg->lights().size()>0;
  tessConfig_ = cfg->tessCfg();
  if(!useShading_) {
    useVertexShading_ = false;
    useFragmentShading_ = false;
  } else {
    useVertexShading_ = ( shading_==Material::GOURAD_SHADING );
    useFragmentShading_ = !useVertexShading_;
  }

  // configure shaders to use tesselation
  if(useTessShader_) {
    tessControlShader_.setMinVersion(400);
    fragmentShader_.setMinVersion(400);
    vertexShader_.setMinVersion(400);
    tessEvalShader_.setMinVersion(400);

    tessEvalShader_.set_tessPrimitive(tessConfig_.primitive);
    tessEvalShader_.set_tessSpacing(tessConfig_.spacing);
    tessEvalShader_.set_tessOrdering(tessConfig_.ordering);
  }

  // TODO SHADERGEN: gl_PointSize
  //   there is a glEnable for point size
  // TODO SHADERGEN: gl_ClipDistance
  /**
// Indices of refraction
const float Air = 1.0;
const float Glass = 1.51714;

// Air to glass ratio of the indices of refraction (Eta)
const float Eta = Air / Glass;

// see http://en.wikipedia.org/wiki/Refractive_index Reflectivity
const float R0 = ((Air - Glass) * (Air - Glass)) / ((Air + Glass) * (Air + Glass));

v_refraction = refract(incident, normal, Eta);
v_reflection = reflect(incident, normal);

// see http://en.wikipedia.org/wiki/Schlick%27s_approximation
v_fresnel = R0 + (1.0 - R0) * pow((1.0 - dot(-incident, normal)), 5.0);

/// FRAG
vec4 refractionColor = texture(u_cubemap, normalize(v_refraction));
vec4 reflectionColor = texture(u_cubemap, normalize(v_reflection));
fragColor = mix(refractionColor, reflectionColor, v_fresnel);
   */

  addUniformToAll("vec3", "cameraPosition");

  setupTextures(cfg->textures());
  setupLights(cfg->lights());
  setupAttributes(cfg->attributes());
  if(cfg->material()!=NULL) { setupMaterial(cfg->material()); }

  list<Light*> lights;
  for(set<State*>::const_iterator
      it=cfg->lights().begin(); it!=cfg->lights().end(); ++it)
  {
    lights.push_back((Light*)(*it));
  }

  setupPosition();
  setupNormal(lights);
  setupTexco();
  setupColor();
  setupFog();

  if(useShading_) {
    if(useTessShader_) {
      setShading(tessEvalShader_,
          lights, cfg->material(), GL_VERTEX_SHADER);
    } else {
      setShading(vertexShader_,
          lights, cfg->material(), GL_VERTEX_SHADER);
    }
  }

  setFragmentVars();
  setFragmentFunctions(lights, cfg->material());
  setFragmentExports(cfg->fragmentOutputs());

  // there may be some custom vertex attributes used outside vertex shader
  // take a look if they are used somewhere and setup transfer
  if(customAttributeNames_.size() > 0) {
    for(set< pair<string,string> >::iterator
        it=customAttributeNames_.begin(); it!=customAttributeNames_.end(); ++it)
    {
      const string &attType = it->first;
      const string &attName = it->second;
      GLSLTransfer vertAtt( attType, FORMAT_STRING("v_"<<attName) );

      if(useTessShader_) {
        if(ShaderManager::containsInputVar(attName, fragmentShader_) ||
           ShaderManager::containsInputVar(FORMAT_STRING("f_"<<attName), fragmentShader_))
        {
          // transfer to fragment shader
          transferVertToTES(vertAtt, attName, vertAtt.name);
          transferToFrag(attType, attName,
              interpolate("In", FORMAT_STRING("tes_"<<attName)));
          continue;
        } else if(ShaderManager::containsInputVar(attName, tessEvalShader_) ||
            ShaderManager::containsInputVar(FORMAT_STRING("tes_"<<attName), tessEvalShader_))
        {
          // transfer to tes shader
          transferVertToTES(vertAtt, attName, vertAtt.name);
          continue;
        } else if(ShaderManager::containsInputVar(attName, tessControlShader_) ||
            ShaderManager::containsInputVar(FORMAT_STRING("tcs_"<<attName), tessControlShader_))
        {
          // transfer to tcs shader
          if(tessConfig_.isAdaptive) transferVertToTCS(vertAtt, attName, vertAtt.name);
          continue;
        }
      } else {
        if(ShaderManager::containsInputVar(attName, fragmentShader_) ||
            ShaderManager::containsInputVar(FORMAT_STRING("f_"<<attName), fragmentShader_))
        {
          // transfer custom attribute from VS to FS
          transferToFrag(attType, attName, vertAtt.name);
          continue;
        }
      }
    }
  }
}

//////////////////

void ShaderGenerator::setupAttributes(const set<VertexAttribute*> &attributes)
{
  int unit=0;

  for(set<VertexAttribute*>::const_iterator
      jt = attributes.begin(); jt != attributes.end(); ++jt)
  {
    VertexAttribute *att = *jt;
    const string& attName = att->name();

    if(attName.compare("padding") == 0) {
      continue; // padding attributes can be ignored
    }

    GLSLTransfer transfer;
    transfer.forceArray = false;
    transfer.numElems = att->elementCount();
    transfer.name = FORMAT_STRING("v_" << attName);
    transfer.interpolation = "";
    switch(att->dataType()) {
    case GL_INT:
      transfer.type = (att->valsPerElement() == 1 ?
          "int" : FORMAT_STRING("ivec" << att->valsPerElement()));
      break;
    case GL_UNSIGNED_INT:
      transfer.type = (att->valsPerElement() == 1 ?
          "unsigned int" : FORMAT_STRING("uvec" << att->valsPerElement()));
      break;
    default:
      if(att->valsPerElement() == 1) {
        transfer.type = "float";
      } else if(att->valsPerElement() == 16) {
        transfer.type = "mat4";
      } else if(att->valsPerElement() == 9) {
        transfer.type = "mat3";
      } else {
        transfer.type = FORMAT_STRING("vec" << att->valsPerElement());
      }
      break;
    }

    vertexShader_.addInput( transfer );

    if(attName.compare( ATTRIBUTE_NAME_COL0 ) == 0) {
      primaryColAttribute_ = att;
    } else if(attName.compare( ATTRIBUTE_NAME_POS ) == 0) {
      posAttribute_ = att;
    } else if(attName.compare( ATTRIBUTE_NAME_NOR ) == 0) {
      norAttribute_ = att;
    } else if(attName.compare( "instanceMat" ) == 0) {
      hasInstanceMat_ = true;
    } else if(sscanf(attName.c_str(), "uv%d", &unit) == 1) {
      uvAttributes_.push_back(att);
    } else {
      customAttributeNames_.insert( pair<string,string>(transfer.type, attName) );
    }
  }
}

void ShaderGenerator::setupTextures(const set<State*> &textures)
{
  TextureMapToMap::iterator needle;

  for(set<State*>::const_iterator
      it = textures.begin(); it != textures.end(); ++it)
  {
    TextureState *textureState = (TextureState*)(*it);
    Texture *texture = textureState->texture().get();

    addUniformToAll(texture->samplerType(), texture->name());

    // handle generated texco
    switch(texture->mapping())
    {
    case MAPPING_UV:
    case MAPPING_LAST:
      break;
    case MAPPING_FLAT:
      texcoGens_.push_back( TexcoGenerator(
          texture->texcoChannel(), "vec2", "flatUV",
          "getFlatUV(_posScreen.xyz, _normal)",
          ShaderFunctions::getFlatUV,
          false) );
      break;
    case MAPPING_CUBE:
      texcoGens_.push_back( TexcoGenerator(
          texture->texcoChannel(), "vec3", "cubeUV",
          "getCubeUV(_posScreen.xyz, _normal)",
          ShaderFunctions::getCubeUV,
          false) );
      break;
    case MAPPING_TUBE:
      texcoGens_.push_back( TexcoGenerator(
          texture->texcoChannel(), "vec2", "tubeUV",
          "getTubeUV(_posScreen.xyz, _normal)",
          ShaderFunctions::getTubeUV,
          false) );
      break;
    case MAPPING_SPHERE:
      texcoGens_.push_back( TexcoGenerator(
          texture->texcoChannel(), "vec2", "sphereUV",
          "getSphereUV(_posScreen.xyz, _normal)",
          ShaderFunctions::getSphereUV,
          false) );
      break;
    case MAPPING_REFLECTION_REFRACTION:
      texcoGens_.push_back( TexcoGenerator(
          texture->texcoChannel(), "vec3", "refractionUV",
          "refract(_incident.xyz, _normal, materialRefractionIndex)",
          "",
          true) );
      texcoGens_.push_back( TexcoGenerator(
          texture->texcoChannel(), "vec3", "reflectionUV",
          "reflect(_incident.xyz, _normal)",
          "",
          true) );
      transferNorToTES_ = true;
      break;
    case MAPPING_REFLECTION:
      texcoGens_.push_back( TexcoGenerator(
          texture->texcoChannel(), "vec3", "reflectionUV",
          "reflect(_incident.xyz, _normal)",
          "",
          true) );
      transferNorToTES_ = true;
      break;
    case MAPPING_REFRACTION:
      texcoGens_.push_back( TexcoGenerator(
          texture->texcoChannel(), "vec3", "refractionUV",
          "refract(_incident.xyz, _normal, materialRefractionIndex)",
          "",
          true) );
      transferNorToTES_ = true;
      break;
    }

    bool usedInVS = false;
    const set<TextureMapTo>& maptTo = texture->mapTo();
    for(set<TextureMapTo>::const_iterator
        jt = maptTo.begin(); jt != maptTo.end(); ++jt)
    {
      needle = mapToMap_.find(*jt);
      if(needle != mapToMap_.end()) {
        needle->second.push_back( textureState );
      } else {
        list< State* > l;
        l.push_back( textureState );
        mapToMap_[*jt] = l;
      }
      uvMapToMap_[texture->texcoChannel()].insert(*jt);
      switch(*jt) {
      case MAP_TO_HEIGHT:
      case MAP_TO_DISPLACEMENT:
      case MAP_TO_NORMAL:
        usedInVS = true;
      default: break;
      }
    }

    if(texture->samplerType() == "sampler2DRect") {
      fragmentShader_.enableExtension("GL_ARB_texture_rectangle");
      if(usedInVS) {
        if(useTessShader_) {
          tessEvalShader_.enableExtension("GL_ARB_texture_rectangle");
        } else {
          vertexShader_.enableExtension("GL_ARB_texture_rectangle");
        }
      }
    }

    TexelTransfer *transfer = textureState->transfer().get();
    if(transfer!=NULL) {
      // use a transfer function for texel
      transfer->addUniforms(&fragmentShader_);
      fragmentShader_.addDependencyCode(
          transfer->name(), transfer->transfer() );
      if(usedInVS) {
        if(useTessShader_) {
          tessEvalShader_.addDependencyCode(
              transfer->name(), transfer->transfer() );
        } else {
          vertexShader_.addDependencyCode(
              transfer->name(), transfer->transfer() );
        }
      }
    }
  }
}

void ShaderGenerator::setupMaterial(const State *material)
{
  addUniformToAll( "float", "materialRefractionIndex");
  addUniformToAll( "vec4", "materialSpecular");
  addUniformToAll( "vec4", "materialEmission");
  addUniformToAll( "float", "materialShininess");
  addUniformToAll( "float", "materialShininessStrength");
  addUniformToAll( "float", "materialRoughness");
  addUniformToAll( "vec4", "materialAmbient");
  addUniformToAll( "vec4", "materialDiffuse");
  addUniformToAll( "float", "materialAlpha");
  addUniformToAll( "float", "materialReflection");
}

void ShaderGenerator::setupLights(const set<State*> &lights)
{
  if(!useShading_) return;

  // populate shaders with light uniforms
  for(set<State*>::const_iterator
      it = lights.begin(); it != lights.end(); ++it)
  {
    const Light *light = (const Light*)(*it);

    stringstream name;

    addUniformToAll( "vec4", FORMAT_STRING("lightPosition" << light));
    addUniformToAll( "vec4", FORMAT_STRING("lightAmbient" << light));
    addUniformToAll( "vec4", FORMAT_STRING("lightDiffuse" << light));
    addUniformToAll( "vec4", FORMAT_STRING("lightSpecular" << light));

    switch(light->getLightType())
    {
    case Light::DIRECTIONAL:
      break;
    case Light::SPOT:
      addUniformToAll( "float", FORMAT_STRING("lightInnerConeAngle" << light));
      addUniformToAll( "float", FORMAT_STRING("lightOuterConeAngle" << light));
      addUniformToAll( "vec3", FORMAT_STRING("lightSpotDirection" << light));
      addUniformToAll( "float", FORMAT_STRING("lightSpotExponent" << light));
      // fall through
    case Light::POINT:
      addUniformToAll( "float", FORMAT_STRING("lightConstantAttenuation" << light));
      addUniformToAll( "float", FORMAT_STRING("lightLinearAttenuation" << light));
      addUniformToAll( "float", FORMAT_STRING("lightQuadricAttenuation" << light));
      break;
    }
  }
}

//////

void ShaderGenerator::transferVertToTCS(
    const GLSLTransfer &t,
    const string &name,
    const string &vertexVarName)
{
  GLSLTransfer tcs(t.type, FORMAT_STRING("tcs_"<<name), t.numElems, t.forceArray, t.interpolation);
  vertexShader_.addOutput( tcs );
  vertexShader_.addExport( GLSLExport(tcs.name, vertexVarName) );
  tessControlShader_.addInput( tcs );
}
void ShaderGenerator::transferVertToTES(
    const GLSLTransfer &t,
    const string &name,
    const string &vertexVarName)
{
  GLSLTransfer tes(t.type, FORMAT_STRING("tes_"<<name), t.numElems, t.forceArray, t.interpolation);
  if(tessConfig_.isAdaptive) {
    transferVertToTCS(t, name, vertexVarName );

    tessControlShader_.addOutput( tes );
    tessControlShader_.addExport(GLSLExport(
        FORMAT_STRING("Out[gl_InvocationID]." << tes.name),
        FORMAT_STRING("tcs_"<<name << "[gl_InvocationID]")
    ));
  } else {
    vertexShader_.addOutput( tes );
    vertexShader_.addExport( GLSLExport(tes.name, vertexVarName) );
  }
  tessEvalShader_.addInput( tes );
}
void ShaderGenerator::transferToFrag(
    const string &t,
    const string &n,
    const string &v)
{
  GLSLTransfer fragNor( t, FORMAT_STRING("f_"<<n) );
  ShaderFunctions &f = (useTessShader_ ? tessEvalShader_ : vertexShader_);
  f.addOutput(fragNor);
  f.addExport(GLSLExport(fragNor.name, v));
  fragmentShader_.addInput(fragNor);
}
void ShaderGenerator::transferFromVertexToFragment(
    const string &type,
    const string &name,
    const string &vertexVarName)
{
  if(useTessShader_) {
    GLSLTransfer transfer(type, name);
    transferVertToTES(transfer, name, vertexVarName);
    transferToFrag(type, name, interpolate("In",
        FORMAT_STRING("tes_"<<name)));
  } else {
    transferToFrag(type, name, vertexVarName);
  }
  fragmentShader_.addMainVar(
      GLSLVariable(type,
          FORMAT_STRING("_"<<name),
          FORMAT_STRING("f_"<<name))
  );
}

void ShaderGenerator::setupPosition()
{
  // load height and displacement maps
  // if tesselation enabled we need the displacement vector in the TCS stage.
  string worldPosVSName = "_posWorld";
  string worldPosVS = ShaderFunctions::posWorldSpace(
      vertexShader_, "v_pos", hasInstanceMat_, maxNumBoneWeights_);
  bool useDisplacement = false;
  if(useTessShader_) {
    string worldDisplacementVS = "";
    addOffsets(vertexShader_, worldDisplacementVS, false);
    if(worldDisplacementVS.size()>0) {
      if(tessConfig_.isAdaptive) {
        transferVertToTCS(
            GLSLTransfer("vec3", "tcs_displacement"),
            "displacement",
            worldDisplacementVS
         );
      }
      useDisplacement = true;
    }
  } else {
    size_t buf = worldPosVSName.size();
    addOffsets(vertexShader_, worldPosVSName, true);
    if(worldPosVSName.size() > buf) {
      vertexShader_.addStatement( GLSLExport("_posWorld", worldPosVSName) );
    }
  }

  vertexShader_.addMainVar(GLSLVariable("vec4", "_posWorld", worldPosVS));

  if(useTessShader_) {
    // pass position from VS over TCS to TES
    vertexShader_.addExport( GLSLExport( "gl_Position", "_posWorld" ) );
    tessControlShader_.addExport( GLSLExport(
      "gl_out[gl_InvocationID].gl_Position",
      "gl_in[gl_InvocationID].gl_Position"
    ));

    // calculate world and eye space position for TES
    string posWorldTES = interpolate("gl_in", "gl_Position");
    string posWorldTESName = "_posWorld";
    if(useDisplacement) {
      size_t buf = posWorldTESName.size();
      addOffsets(tessEvalShader_, posWorldTESName, true);
      if(posWorldTESName.size() > buf) {
        tessEvalShader_.addStatement( GLSLExport("_posWorld", posWorldTESName) );
      }
    }
    tessEvalShader_.addMainVar(GLSLVariable("vec4", "_posWorld", posWorldTES));

    tessEvalShader_.addMainVar(GLSLVariable("vec4", "_posScreen"));
    if(useShading_) {
      tessEvalShader_.addMainVar(GLSLVariable("vec4", "_posEye"));
      tessEvalShader_.addStatement(
          GLSLExport("_posEye", "viewMatrix * _posWorld"));
    }
    tessEvalShader_.addStatement(
        GLSLExport("_posScreen", "viewProjectionMatrix * _posWorld"));
    tessEvalShader_.addExport(GLSLExport( "gl_Position", "_posScreen" ) );

    // setup TCS stage
    vector<string> args;
    if(tessConfig_.isAdaptive) {
      if(tessConfig_.primitive == TESS_PRIMITVE_QUADS) {
        QuadTesselationControl ctrl(args, useDisplacement);
        ctrl.set_lodMetric(tessConfig_.lodMetric);
        tessControlShader_.operator +=(ctrl);
        tessControlShader_.set_tessNumVertices( 4 );
      } else if(tessConfig_.primitive == TESS_PRIMITVE_TRIANGLES) {
        TriangleTesselationControl ctrl(args, useDisplacement);
        ctrl.set_lodMetric(tessConfig_.lodMetric);
        tessControlShader_.operator +=(ctrl);
        tessControlShader_.set_tessNumVertices( 3 );
      }
    }
  } else {
    // do pos transformations in vertex shader
    vertexShader_.addMainVar(GLSLVariable("vec4", "_posScreen"));
    if(useShading_) {
      vertexShader_.addMainVar(GLSLVariable("vec4", "_posEye"));
    }

    string posScreen;
    string posEye;
    if(ignoreViewRotation_) {
      posEye = "(vec4(viewMatrix[3].xyz,0.0) + _posWorld)";
      if(useShading_) {
        posScreen = "projectionMatrix * _posEye";
      } else {
        posScreen = FORMAT_STRING("projectionMatrix * " << posEye);
      }
    } else if(ignoreViewTranslation_) {
      posEye = "(mat4(viewMatrix[0], viewMatrix[1], viewMatrix[2], vec3(0.0), 1.0) * _posWorld)";
      if(useShading_) {
        posScreen = "projectionMatrix * _posEye";
      } else {
        posScreen = FORMAT_STRING("projectionMatrix * " << posEye);
      }
    } else {
      posEye = "viewMatrix * _posWorld";
      posScreen = "viewProjectionMatrix * _posWorld";
    }

    if(useShading_) {
      vertexShader_.addStatement( GLSLExport("_posEye", posEye) );
    }
    vertexShader_.addStatement( GLSLExport("_posScreen", posScreen) );
    vertexShader_.addExport( GLSLExport( "gl_Position", "_posScreen" ) );
  }

  if(useShading_)
  {
    transferToFrag("vec4", "posEye", "_posEye");
  }
  if(useShading_ ||
      mapToMap_.find(MAP_TO_VOLUME)!=mapToMap_.end())
  {
    transferToFrag("vec4", "posWorld", "_posWorld");
  }
}

void ShaderGenerator::setupNormal(const list<Light*> &lights)
{
  if(!useShading_) { return; }

  bool transferNorToFS = false;
  if(useFragmentShading_) transferNorToFS = true;
  // volumes do fragment shading
  if(mapToMap_.find(MAP_TO_VOLUME) != mapToMap_.end()) transferNorToFS = true;

  if(norAttribute_) {
    GLSLTransfer vertNor("vec3", "v_nor");

    // there is a per vertex normal attribute
    // TODO SHADERGEN: normal not always used in VS !!
    //   * auto remove unused main vars ??
    // TODO SHADERGEN: normal transform is wrong for scaled objects
    //   http://www.lighthouse3d.com/tutorials/glsl-tutorial/the-normal-matrix/
    vertexShader_.addMainVar(GLSLVariable(
        "vec3",
        "_normal",
        ShaderFunctions::norWorldSpace(vertexShader_,
            vertNor.name, hasInstanceMat_, maxNumBoneWeights_)
    ));

    if(useVertexShading_)
    {
      if(useTessShader_) {
        transferVertToTES(vertNor, ATTRIBUTE_NAME_NOR, "_normal");
        // interpolate normal for use in TES
        tessEvalShader_.addMainVar( GLSLVariable(
            "vec3", "_normal", interpolate("In", "tes_nor")) );
        if(transferNorToFS) {
          transferToFrag("vec3", ATTRIBUTE_NAME_NOR, "_normal");
          fragmentShader_.addMainVar( GLSLVariable("vec3", "_normal", "f_nor") );
        }
      } else {
        // nothing to do, only vertex shader needs normal
        if(transferNorToFS) {
          transferVertToTES(vertNor, ATTRIBUTE_NAME_NOR, "_normal");
          transferToFrag("vec3", ATTRIBUTE_NAME_NOR, "v_nor");
          fragmentShader_.addMainVar( GLSLVariable("vec3", "_normal", "f_nor") );
        }
      }
    } else if(useFragmentShading_) {
      GLSLTransfer fragNor( "vec3", "f_nor" );
      if(useTessShader_) {
        transferVertToTES(vertNor, ATTRIBUTE_NAME_NOR, "_normal");
        tessEvalShader_.addOutput(fragNor);
        // TODO SHADERGEN: only needed for heightmap
        //   * auto remove unused main vars ??
        tessEvalShader_.addMainVar( GLSLVariable("vec3", "_normal", interpolate("In", "tes_nor")) );
        // interpolate normal and pass to fragment!
        tessEvalShader_.addExport(GLSLExport("f_nor", "_normal"));
      } else {
        // interpolate from vert to frag shader
        vertexShader_.addOutput(fragNor);
        vertexShader_.addExport(GLSLExport("f_nor", "_normal"));
      }

      fragmentShader_.addInput(fragNor);
      if(isTwoSided_) {
        fragmentShader_.addMainVar( GLSLVariable(
          "vec3", "_normal", "(gl_FrontFacing ? normalize(f_nor) : -normalize(f_nor))") );
      } else {
        fragmentShader_.addMainVar( GLSLVariable(
          "vec3", "_normal", "normalize(f_nor)") );
      }
    } else if(transferNorToFS) {
      if(useTessShader_) {
        transferVertToTES(vertNor, ATTRIBUTE_NAME_NOR, "_normal");
        if(transferNorToTES_) {
          tessEvalShader_.addMainVar( GLSLVariable(
              "vec3", "_normal", interpolate("In", "tes_nor")) );
          transferToFrag("vec3", ATTRIBUTE_NAME_NOR, "_normal");
        } else {
          transferToFrag("vec3", ATTRIBUTE_NAME_NOR, interpolate("In", "tes_nor"));
        }
      } else {
        transferToFrag("vec3", ATTRIBUTE_NAME_NOR, "v_nor");
      }
      fragmentShader_.addMainVar( GLSLVariable("vec3", "_normal", "f_nor") );
    } else if(transferNorToTES_ && useTessShader_) {
      // interpolate normal for use in TES
      transferVertToTES(vertNor, ATTRIBUTE_NAME_NOR, "_normal");
      tessEvalShader_.addMainVar( GLSLVariable(
          "vec3", "_normal", interpolate("In", "tes_nor")) );
    }
  } else {
    // normal needed anyway for light calculation
    // use fallback or normal that is obtained by normal map
    GLSLVariable fallbackNormal("vec3", "_normal", "vec3(0.0,1.0,0.0)");
    if(useVertexShading_) {
      if(useTessShader_) {
        tessEvalShader_.addMainVar( fallbackNormal );
      } else {
        vertexShader_.addMainVar( fallbackNormal );
      }
    }
    if(useFragmentShading_ || transferNorToFS) {
      fragmentShader_.addMainVar( fallbackNormal );
    }
  }

  // modify normal by texture
  if(useFragmentShading_) {
    addNormalMaps((useTessShader_ ? tessEvalShader_ : vertexShader_), lights, false);
    addNormalMaps(fragmentShader_, lights, true);
  }
}

void ShaderGenerator::setupColor()
{
  if(primaryColAttribute_) {
    // there is a per vertex color attribute
    vertexShader_.addMainVar( GLSLVariable("vec4", "_color", "v_col0") );
    transferFromVertexToFragment("vec4", "col0", "_color");
  } else {
    fragmentShader_.addMainVar( GLSLVariable("vec4", "_color", "vec4(1.0)") );
  }
}

void ShaderGenerator::texcoFindMapTo(
    GLuint unit, bool *useFragmentUV, bool *useVertexUV)
{
  // lookup mapTo modes for uv unit and check if uv used in fragment shader and/or before
  set<TextureMapTo> &mapToSet = uvMapToMap_[unit];

  // flag if this uv attribute is not used in fragment shader
  int mapSize = mapToSet.size();
  int fragmentMapToCount = mapSize;
  for(set<TextureMapTo>::iterator
      jt=mapToSet.begin(); jt!=mapToSet.end(); ++jt)
  {
    const TextureMapTo &mapTo = *jt;
    if(mapTo==MAP_TO_HEIGHT) { *useVertexUV = true; }
    else if(mapTo==MAP_TO_DISPLACEMENT) { *useVertexUV = true;  }
    else if(mapTo==MAP_TO_NORMAL) { *useVertexUV = true;  *useFragmentUV = true;  }
    else { *useFragmentUV = true; }
  }
}
void ShaderGenerator::setupTexco()
{
  // setup texco for attributes passed to VS
  for(list< VertexAttribute* >::iterator
      it = uvAttributes_.begin(); it != uvAttributes_.end(); ++it)
  {
    TexcoAttribute *texco = dynamic_cast<TexcoAttribute*>(*it);
    if(!texco) {
      ERROR_LOG("try to add attribute named '" << texco->name() <<
          "' to shader but it is not a subclass of UVAttribute. skipping.");
      continue;
    }

    string texcoType;
    if(texco->valsPerElement()==1) {
      texcoType = "float";
    } else if(texco->valsPerElement()==2) {
      texcoType = "vec2";
    } else if(texco->valsPerElement()==3) {
      texcoType = "vec3";
    } else if(texco->valsPerElement()==4) {
      texcoType = "vec4";
    }

    bool useFragmentUV, useVertexUV;
    texcoFindMapTo(texco->channel(), &useFragmentUV, &useVertexUV);

    GLSLTransfer vertTexco(texcoType, FORMAT_STRING("v_"<<texco->name()));
    if(useVertexUV) {
      if(useTessShader_) {
        transferVertToTES(vertTexco, texco->name(), vertTexco.name);
        // interpolate texco for use in TES
        tessEvalShader_.addMainVar( GLSLVariable(
            texcoType, FORMAT_STRING("tes_"<<texco->name()),
            interpolate("In", FORMAT_STRING("tes_"<<texco->name()))) );
        if(useFragmentUV) {
          transferToFrag(texcoType, texco->name(), FORMAT_STRING("tes_"<<texco->name()));
        }
      } else {
        vertexShader_.addMainVar( GLSLVariable(texcoType, "_uv", vertTexco.name) );
        if(useFragmentUV) {
          transferToFrag(texcoType, texco->name(), vertTexco.name);
        }
      }
    } else if(useFragmentUV) {
      if(useTessShader_) {
        transferVertToTES(vertTexco, texco->name(), vertTexco.name);
        // interpolate uv for fragment
        transferToFrag(texcoType, texco->name(),
            interpolate("In", FORMAT_STRING("tes_"<<texco->name())));
      } else {
        // interpolate from vert to frag shader
        transferToFrag(texcoType, texco->name(), vertTexco.name);
      }
    }
  }

  // setup generated texco
  for(list<TexcoGenerator>::iterator
      it=texcoGens_.begin(); it!=texcoGens_.end(); ++it)
  {
    TexcoGenerator &gen = *it;
    bool useFragmentUV, useVertexUV;
    texcoFindMapTo(gen.unit, &useFragmentUV, &useVertexUV);
    ShaderFunctions &f = (useTessShader_ ? tessEvalShader_ : vertexShader_);
    if(gen.needsIncident) {
      f.addMainVar( GLSLVariable("vec3", "_incident", "normalize( _posWorld.xyz - cameraPosition.xyz )" ) );
      break;
    }
  }
  for(list<TexcoGenerator>::iterator
      it=texcoGens_.begin(); it!=texcoGens_.end(); ++it)
  {
    TexcoGenerator &gen = *it;

    bool useFragmentUV, useVertexUV;
    texcoFindMapTo(gen.unit, &useFragmentUV, &useVertexUV);

    if(useVertexUV) {
      // calculate texco for use in TES or vertex shader
      ShaderFunctions &f = (useTessShader_ ? tessEvalShader_ : vertexShader_);
      GLSLVariable texcoVar(gen.type, "", gen.functionCall);
      // pretend to have the uv as input, adding it with matching name as var
      if(useTessShader_) {
        texcoVar.name = FORMAT_STRING("_"<<gen.name);
      } else {
        texcoVar.name = FORMAT_STRING("v_"<<gen.name);
      }
      f.addMainVar( texcoVar );
      if(gen.functionCode.size()>0)
        f.addDependencyCode(texcoVar.name, gen.functionCode );
      if(useFragmentUV) {
        transferToFrag(gen.type, gen.name, texcoVar.name);
      }
    } else {
      // calculate texco in VS or TES and pass to fragment shader
      ShaderFunctions &f = (useTessShader_ ? tessEvalShader_ : vertexShader_);
      transferToFrag(gen.type, gen.name, gen.functionCall);
      if(gen.functionCode.size()>0)
        f.addDependencyCode(gen.name, gen.functionCode );
    }
  }
}

void ShaderGenerator::setupFog()
{
  // no fog used
  if(!useFog_) return;

  // fog is always calculated in fragment shader

  addUniform(fragmentShader_, "vec4", "fogColor");
  addUniform(fragmentShader_, "float", "fogEnd");
  addUniform(fragmentShader_, "float", "fogScale");
}

///////////////////

void ShaderGenerator::setFragmentVars()
{
  fragmentShader_.addMainVar( (GLSLVariable) {
    "float", "_alpha", "materialAlpha" } );

  if(useShading_) {
    fragmentShader_.addMainVar( (GLSLVariable) {
      "vec4", "_ambientTerm", "vec4(0.0)" } );
    fragmentShader_.addMainVar( (GLSLVariable) {
      "vec4", "_specularTerm", "vec4(0.0)" } );
    fragmentShader_.addMainVar( (GLSLVariable) {
      "vec4", "_emissionTerm", "materialEmission" } );
    fragmentShader_.addMainVar( (GLSLVariable) {
      "vec4", "_diffuseTerm", "vec4(0.0)" } );

    if(useVertexShading_) {
      fragmentShader_.addMainVar( (GLSLVariable) {
        "vec4", "_materialAmbient", "vec4(f_lightAmbient,1.0)" } );
      fragmentShader_.addMainVar( (GLSLVariable) {
        "vec4", "_materialDiffuse", "vec4(f_lightDiffuse,1.0)" } );
      fragmentShader_.addMainVar( (GLSLVariable) {
        "vec4", "_materialSpecular", "vec4(f_lightSpecular,1.0)" } );
    } else if(useFragmentShading_) {
      fragmentShader_.addMainVar( (GLSLVariable) {
        "vec4", "_materialSpecular", "materialSpecular" } );
      fragmentShader_.addMainVar( (GLSLVariable) {
        "vec4", "_materialAmbient", "materialAmbient" } );
      fragmentShader_.addMainVar( (GLSLVariable) {
        "vec4", "_materialDiffuse", "materialDiffuse" } );
      fragmentShader_.addMainVar( (GLSLVariable) {
        "float", "_materialShininess", "materialShininess" } );
      fragmentShader_.addMainVar( (GLSLVariable) {
        "float", "_materialShininessStrength", "materialShininessStrength" } );
    }
  }
  fragmentShader_.addMainVar( (GLSLVariable) {
    "float", "_brightness", "1.0" } );
}
void ShaderGenerator::setFragmentExports(
    const set<ShaderFragmentOutput*> &fragmentOutputFunctions)
{
  for(set<ShaderFragmentOutput*>::const_iterator
      it=fragmentOutputFunctions.begin(); it!=fragmentOutputFunctions.end(); ++it)
  {
    (*it)->addOutput(fragmentShader_);
  }
}

void ShaderGenerator::setFragmentFunctions(
    const list<Light*> &lights,
    const State *material)
{
  // change material colors with maps and also calculate
  // normal for the volume
  addVolumeMaps();

  // change material colors with maps
  addColorMaps(MAP_TO_COLOR, "_color");

  // calculate ambient/diffuse/specular term
  if(useShading_) {
    addColorMaps(MAP_TO_AMBIENT, "_materialAmbient");
    addColorMaps(MAP_TO_DIFFUSE, "_materialDiffuse");
    addColorMaps(MAP_TO_SPECULAR, "_materialSpecular");
    addColorMaps(MAP_TO_EMISSION, "_materialEmission");

    // calculate shininess exponent
    addShininessMaps();

    if(useFragmentShading_) {
      addLightMaps();
    }
    setShading(fragmentShader_, lights, material, GL_FRAGMENT_SHADER);
  }

  // calculate reflection factor for diffuse and specular light
  addDiffuseReflectionMaps();
  addSpecularReflectionMaps();
  // set alpha value of output
  addAlphaMaps();
  addReflectionMaps();

  if(useShading_) {
    vector<string> args;
    args.push_back("_color");
    args.push_back("_materialAmbient");
    args.push_back("_emissionTerm");
    args.push_back("_ambientTerm");
    args.push_back("_diffuseTerm");
    args.push_back("_specularTerm");
    args.push_back("_brightness");
    args.push_back("_alpha");
    CalculateFragColor calcCol(args, useFog_);
    fragmentShader_.operator+=(calcCol);
    args.clear();
  } else {
    vector<string> args;
    args.push_back("_color");
    args.push_back("_brightness");
    args.push_back("_alpha");
    CalculateFragColorUnshaded calcCol(args, useFog_);
    fragmentShader_.operator+=(calcCol);
    args.clear();
  }
}

///////////////////

void ShaderGenerator::setShading(
    ShaderFunctions &shader,
    const list<Light*> &lights,
    const State *materialState,
    GLenum shaderType)
{
  vector<string> args1, args2;
  const Material *material = (const Material*)materialState;

  if(shaderType == GL_VERTEX_SHADER) {
    args1.push_back("_posEye");
    args2.push_back("_posEye");
    args2.push_back("_normal");

    ShaderFunctions func;
    switch(shading_) {
    case Material::PHONG_SHADING: func = PhongShadingVert(args1, lights, useFog_); break;
    case Material::GOURAD_SHADING: func = GouradShadingVert(args2, lights, useFog_); break;
    case Material::BLINN_SHADING: func = BlinnShadingVert(args1, lights, useFog_); break;
    case Material::TOON_SHADING: func = ToonShadingVert(args1, lights, useFog_); break;
    case Material::ORENNAYER_SHADING: func = OrenNayerShadingVert(args1, lights, useFog_); break;
    case Material::MINNAERT_SHADING: func = MinnaertShadingVert(args1, lights, useFog_); break;
    case Material::COOKTORRANCE_SHADING: func = CookTorranceShadingVert(args1, lights, useFog_); break;
    case Material::NO_SHADING: break;
    }
    shader.operator+=(func);
  } else if(shaderType == GL_FRAGMENT_SHADER) {
    if(hasNormalMapInTangentSpace_) {
      args1.push_back("f_posTangent");
    } else {
      args1.push_back("f_posEye");
    }
    args1.push_back("_normal");
    args1.push_back("_brightness");
    args1.push_back("_materialAmbient");
    args1.push_back("_materialDiffuse");
    args1.push_back("_materialSpecular");
    args1.push_back("_materialShininess");
    args1.push_back("_materialShininessStrength");
    args1.push_back("_ambientTerm");
    args1.push_back("_diffuseTerm");
    args1.push_back("_specularTerm");
    args2.push_back("_materialAmbient");
    args2.push_back("_materialDiffuse");
    args2.push_back("_materialSpecular");
    args2.push_back("_ambientTerm");
    args2.push_back("_diffuseTerm");
    args2.push_back("_specularTerm");

    ShaderFunctions func;
    switch(shading_) {
    case Material::PHONG_SHADING: func = PhongShadingFrag(args1, lights, useFog_); break;
    case Material::GOURAD_SHADING: func = GouradShadingFrag(args2, lights, useFog_); break;
    case Material::BLINN_SHADING: func = BlinnShadingFrag(args1, lights, useFog_); break;
    case Material::TOON_SHADING: func = ToonShadingFrag(args1, lights, useFog_); break;
    case Material::ORENNAYER_SHADING: func = OrenNayerShadingFrag(args1, lights, useFog_); break;
    case Material::MINNAERT_SHADING: func = MinnaertShadingFrag(args1, lights, useFog_); break;
    case Material::COOKTORRANCE_SHADING: func = CookTorranceShadingFrag(args1, lights, useFog_); break;
    case Material::NO_SHADING: break;
    }
    shader.operator+=(func);
  }
}

//////////////////

string ShaderGenerator::stageTexcoName(ShaderFunctions &f, const string &n)
{
  if(&f == &vertexShader_) {
    return FORMAT_STRING("v_" << n);
  } else if(&f == &tessControlShader_) {
    return FORMAT_STRING("tcs_" << n);
  } else if(&f == &tessEvalShader_) {
    return FORMAT_STRING("tes_" << n);
  } else if(&f == &geometryShader_) {
    return FORMAT_STRING("g_" << n);
  } else if(&f == &fragmentShader_) {
    return FORMAT_STRING("f_" << n);
  }
}

string ShaderGenerator::texel(
    const State *state,
    ShaderFunctions &func,
    bool addMainvar)
{
  string texelLookup;
  TextureState *textureState = (TextureState*)state;
  Texture *texture = textureState->texture().get();

  if(texture->mapping() == MAPPING_REFLECTION_REFRACTION) {
    string uvRefl = stageTexcoName(func, "reflectionUV");
    string uvRefr = stageTexcoName(func, "refractionUV");
    texelLookup = FORMAT_STRING("mix( " <<
        "texture( " << texture->name() << ", " << uvRefr << " ), " <<
        "texture( " << texture->name() << ", " << uvRefl << " ), " <<
        "materialReflection )");
  } else {
    string uv;
    switch(texture->mapping()) {
    case MAPPING_SPHERE: uv = "sphereUV"; break;
    case MAPPING_TUBE: uv = "tubeUV"; break;
    case MAPPING_CUBE: uv = "cubeUV"; break;
    case MAPPING_FLAT: uv = "flatUV"; break;
    case MAPPING_REFLECTION: uv = "reflectionUV"; break;
    case MAPPING_REFRACTION: uv = "refractionUV"; break;
    default: uv = FORMAT_STRING("uv" << texture->texcoChannel());
    }
    uv = stageTexcoName(func, uv);

    if(texture->mapTo().count(MAP_TO_VOLUME_SLICE) > 0) {
      texelLookup = FORMAT_STRING("texture( " << texture->name() << ", vec3(" << uv << ",shellTexco) )");
    } else {
      texelLookup = FORMAT_STRING("texture( " << texture->name() << ", " << uv << " )");
    }
  }

  if(texture->ignoreAlpha()) {
    texelLookup = FORMAT_STRING("vec4( (" << texelLookup << ").rgb, 1.0 )");
  }
  TexelTransfer *transfer = textureState->transfer().get();
  if(transfer!=NULL) { // use a transfer function for texel
    texelLookup = FORMAT_STRING(transfer->name() << "(" << texelLookup << ")");
  }
  if(texture->brightness() < 1.0f) {
    texelLookup = FORMAT_STRING(texture->brightness() << "*" << texelLookup);
  }
  if(texture->invert()) {
    texelLookup = FORMAT_STRING(texelLookup << "- vec4(1.0,1.0,1.0,0.0)");
  }
  if(addMainvar) {
    string texelName = FORMAT_STRING("_texel_" << texture->name());
    func.addMainVar(GLSLVariable("vec4", texelName, ""));
    func.addStatement(GLSLExport(texelName, texelLookup));
    return texelName;
  } else {
    return texelLookup;
  }
}

void ShaderGenerator::addColorMaps(TextureMapTo mapTo, string outputColor)
{
  TextureMapToMap::iterator textures = mapToMap_.find(mapTo);
  if(textures == mapToMap_.end()) return;

  vector<string> args;

  for(list<State*>::iterator
      it = textures->second.begin(); it != textures->second.end(); ++it)
  {
    TextureState *textureState = (TextureState*)(*it);
    Texture *texture = textureState->texture().get();

    string texelVar = texel(textureState, fragmentShader_);

    if(texture->contrast() != 1.0) {
      args.push_back(texelVar);
      args.push_back(FORMAT_STRING(texture->contrast()));
      ContrastBlender blend(args);
      fragmentShader_.operator+=(blend);
      args.clear();
    }

    if(texture->blendMode() == BLEND_MODE_SRC) {
      // TODO SHADERGEN: overwrite main var instead ?
      // like this there is a _color = FOO; declaration and maybe some more of the
      // form _color = FOO; or foo(_color, __); where foo changes _color.
      fragmentShader_.addStatement(GLSLExport(outputColor, texelVar));
    } else {
      args.push_back(texelVar);
      args.push_back(outputColor);
      args.push_back(FORMAT_STRING(texture->blendFactor()));
      TextureBlenderCol2 *blender = newBlender(texture->blendMode(), args);
      fragmentShader_.operator+=( *blender );
      delete blender;
      args.clear();
    }
  }
}
void ShaderGenerator::addShininessMaps()
{
  TextureMapToMap::iterator textures = mapToMap_.find(MAP_TO_SHININESS);
  if(textures == mapToMap_.end()) return;
  vector<string> args;

  for(list<State*>::iterator
      it = textures->second.begin(); it != textures->second.end(); ++it)
  {
    TextureState *textureState = (TextureState*)(*it);
    Texture *texture = textureState->texture().get();

    string texelVar = texel(textureState, fragmentShader_);

    args.push_back(FORMAT_STRING(texelVar << ".r"));
    args.push_back("_materialShininess");
    MultiplyFloatFloatFunction multiply(args);
    fragmentShader_.operator+=(multiply);
    args.clear();
  }
}
void ShaderGenerator::addAlphaMaps()
{
  TextureMapToMap::iterator textures = mapToMap_.find(MAP_TO_ALPHA);
  if(textures == mapToMap_.end()) return;
  vector<string> args;

  for(list<State*>::iterator
      it = textures->second.begin(); it != textures->second.end(); ++it)
  {
    TextureState *textureState = (TextureState*)(*it);
    Texture *texture = textureState->texture().get();

    string texelVar = texel(textureState, fragmentShader_);

    args.push_back(FORMAT_STRING(texelVar << ".r"));
    args.push_back("_materialColor.a");
    MultiplyFloatFloatFunction multiply(args);
    fragmentShader_.operator+=(multiply);
    args.clear();
  }
}
void ShaderGenerator::addReflectionMaps()
{
  TextureMapToMap::iterator textures = mapToMap_.find(MAP_TO_REFLECTION);
  if(textures == mapToMap_.end()) return;
  vector<string> args;

  for(list<State*>::iterator
      it = textures->second.begin(); it != textures->second.end(); ++it)
  {
    TextureState *textureState = (TextureState*)(*it);
    Texture *texture = textureState->texture().get();

    string texelVar = texel(textureState, fragmentShader_);

    args.push_back(FORMAT_STRING(texelVar << ".r"));
    args.push_back("_materialReflection");
    MultiplyFloatFloatFunction multiply(args);
    fragmentShader_.operator+=(multiply);
    args.clear();
  }
}
void ShaderGenerator::addDiffuseReflectionMaps()
{
  TextureMapToMap::iterator textures = mapToMap_.find(MAP_TO_DIFFUSE_REFLECTION);
  if(textures == mapToMap_.end()) return;
  vector<string> args;

  for(list<State*>::iterator
      it = textures->second.begin(); it != textures->second.end(); ++it)
  {
    TextureState *textureState = (TextureState*)(*it);
    Texture *texture = textureState->texture().get();

    string texelVar = texel(textureState, fragmentShader_);

    args.push_back(FORMAT_STRING(texelVar << ".r"));
    args.push_back("_diffuseTerm");
    MultiplyFloatVec4Function multiply(args);
    fragmentShader_.operator+=(multiply);
    args.clear();
  }
}
void ShaderGenerator::addSpecularReflectionMaps()
{
  TextureMapToMap::iterator textures = mapToMap_.find(MAP_TO_SPECULAR_REFLECTION);
  if(textures == mapToMap_.end()) return;
  vector<string> args;

  for(list<State*>::iterator
      it = textures->second.begin(); it != textures->second.end(); ++it)
  {
    TextureState *textureState = (TextureState*)(*it);
    Texture *texture = textureState->texture().get();

    string texelVar = texel(textureState, fragmentShader_);

    args.push_back(FORMAT_STRING(texelVar << ".r"));
    args.push_back("_specularTerm");
    MultiplyFloatVec4Function multiply(args);
    fragmentShader_.operator+=(multiply);
    args.clear();
  }
}

void ShaderGenerator::addLightMaps()
{
  TextureMapToMap::iterator textures = mapToMap_.find(MAP_TO_LIGHT);
  if(textures == mapToMap_.end()) return;
  vector<string> args;

  for(list<State*>::iterator
      it = textures->second.begin(); it != textures->second.end(); ++it)
  {
    TextureState *textureState = (TextureState*)(*it);
    Texture *texture = textureState->texture().get();

    string texelVar = texel(textureState, fragmentShader_);

    args.push_back("_ambientTerm");
    args.push_back("_diffuseTerm");
    args.push_back("_specularTerm");
    args.push_back("_materialAmbient");
    args.push_back("_materialDiffuse");
    args.push_back("_materialSpecular");
    args.push_back(texelVar);
    LightMapFunction light(args);
    fragmentShader_.operator+=(light);
    args.clear();
  }
}

void ShaderGenerator::addVolumeMaps()
{
  TextureMapToMap::iterator textures = mapToMap_.find(MAP_TO_VOLUME);
  if(textures == mapToMap_.end()) return;
  vector<string> args;

  for(list<State*>::iterator
      it = textures->second.begin(); it != textures->second.end(); ++it)
  {
    TextureState *textureState = (TextureState*)(*it);
    Texture *texture = textureState->texture().get();

    args.push_back("_normal");
    args.push_back("_color");
    RayCastShader raycast(textureState, args);
    fragmentShader_.operator+=(raycast);
    args.clear();
  }
}

void ShaderGenerator::addNormalMaps(
    ShaderFunctions &shader,
    const list<Light*> &lights,
    bool calcNor)
{
  // create eye space normal mapping shader,
  // tangent space cannot be calculated by all primitives.
  TextureMapToMap::iterator textures = mapToMap_.find(MAP_TO_NORMAL);
  if(textures == mapToMap_.end()) return;

  vector<string> args;
  for(list<State*>::iterator
      it = textures->second.begin(); it != textures->second.end(); ++it)
  {
    TextureState *textureState = (TextureState*)(*it);
    Texture *texture = textureState->texture().get();

    if(calcNor) {
      string texelVar = texel(textureState, shader);
      args.push_back(FORMAT_STRING(
          texelVar << " * " << texture->heightScale()));
      args.push_back("_normal");
      BumpMapFrag bump(args);
      shader.operator+=(bump);
      args.clear();
    } else {
      args.push_back("_normal");
      if(&shader == &tessEvalShader_) {
        args.push_back(interpolate("In", FORMAT_STRING("tes_tan")));
      } else {
        args.push_back("tan");
      }
      args.push_back("_posEye");
      args.push_back("_posTangent");
      BumpMapVert bumpV(args, lights);
      shader.operator+=(bumpV);
      args.clear();
      hasNormalMapInTangentSpace_ = true;
    }
  }

  if(!calcNor && hasNormalMapInTangentSpace_) {
    shader.addMainVar(GLSLVariable("vec4", "_posTangent"));
    transferToFrag("vec4", "posTangent", "_posTangent");
  }
}

void ShaderGenerator::addOffsets(ShaderFunctions &shader, string &pos, bool isVec4)
{
  addHeightMaps(shader, pos, isVec4);
  addDisplacementMaps(shader, pos, isVec4);
}
void ShaderGenerator::addHeightMaps(ShaderFunctions &shader, string &pos, bool isVec4)
{
  TextureMapToMap::iterator textures = mapToMap_.find(MAP_TO_HEIGHT);
  if(textures == mapToMap_.end()) return;
  for(list<State*>::iterator
      it = textures->second.begin(); it != textures->second.end(); ++it)
  {
    TextureState *textureState = (TextureState*)(*it);
    Texture *t = textureState->texture().get();
    string texelVar = texel(*it, shader);
    string translation = FORMAT_STRING(
        "_normal*("<<texelVar<<".r*"<<t->heightScale()<<")");
    if(isVec4) translation = FORMAT_STRING("vec4(" << translation << ", 0.0)");
    pos = (pos.size() == 0 ? translation : FORMAT_STRING(pos << " + " << translation));
  }
  pos = FORMAT_STRING("(" << pos << ")");
}
void ShaderGenerator::addDisplacementMaps(ShaderFunctions &shader, string &pos, bool isVec4)
{
  TextureMapToMap::iterator textures = mapToMap_.find(MAP_TO_DISPLACEMENT);
  if(textures == mapToMap_.end()) return;
  for(list<State*>::iterator
      it = textures->second.begin(); it != textures->second.end(); ++it)
  {
    TextureState *textureState = (TextureState*)(*it);
    Texture *t = textureState->texture().get();
    string texelVar = texel(*it, shader);
    string translation = FORMAT_STRING(
        texelVar<<".xyz*"<<t->heightScale());
    if(isVec4) translation = FORMAT_STRING("vec4(" << translation << ", 0.0)");
    pos = (pos.size() == 0 ? translation : FORMAT_STRING(pos << " + " << translation));
  }
  pos = FORMAT_STRING("(" << pos << ")");
}
