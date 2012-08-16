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
#include <ogle/utility/string-util.h>

static string interpolateQuad(const string &a, const string &n)
{
  // gl_TessCoord is a texco coordinate for triangles
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

static string inputType(ShaderInput *input)
{
  if(input->dataType() == GL_FLOAT)
  {
    if(input->valsPerElement() == 9) {
      return "mat3";
    } else if(input->valsPerElement() == 16) {
      return "mat4";
    }
  }

  string baseType, vecType;

  switch(input->dataType())
  {
  case GL_INT:
    baseType = "int";
    vecType = "ivec";
    break;
  case GL_UNSIGNED_INT:
    baseType = "unsigned int";
    vecType = "uvec";
    break;
  case GL_FLOAT:
    baseType = "float";
    vecType = "vec";
    break;
  case GL_DOUBLE:
    baseType = "double";
    vecType = "dvec";
    break;
  case GL_BOOL:
    baseType = "bool";
    vecType = "bvec";
    break;
  default:
    WARN_LOG("unknown GL data type " << input->dataType() << ".");
    return "unknown";
  }

  if(input->valsPerElement()==1) {
    return baseType;
  } else if(input->valsPerElement()<5) {
    return FORMAT_STRING(vecType << input->valsPerElement());
  } else {
    WARN_LOG("invalid number of values per element " <<
        input->valsPerElement() << " for GL type " << input->dataType() << ".");
    return baseType;
  }
}
static string inputValue(ShaderInput *input)
{
  string typeStr = inputType(input);

  stringstream value;
  value << typeStr << "(";

  byte *inputData = input->dataPtr();
  switch(input->dataType())
  {
  case GL_INT:
    for(GLuint i=0; i<input->valsPerElement(); ++i) {
      if(i>0) { value << ", "; }
      value << ((GLint*)inputData)[i];
    }
    break;
  case GL_BOOL:
    for(GLuint i=0; i<input->valsPerElement(); ++i) {
      if(i>0) { value << ", "; }
      value << ((GLboolean*)inputData)[i];
    }
    break;
  case GL_UNSIGNED_INT:
    for(GLuint i=0; i<input->valsPerElement(); ++i) {
      if(i>0) { value << ", "; }
      value << ((GLuint*)inputData)[i];
    }
    break;
  case GL_FLOAT:
    for(GLuint i=0; i<input->valsPerElement(); ++i) {
      if(i>0) { value << ", "; }
      value << ((GLfloat*)inputData)[i];
    }
    break;
  case GL_DOUBLE:
    for(GLuint i=0; i<input->valsPerElement(); ++i) {
      if(i>0) { value << ", "; }
      value << ((GLdouble*)inputData)[i];
    }
    break;
  default:
    break;
  }

  value << ")";
  return value.str();
}

ShaderGenerator::ShaderGenerator()
: useTessShader_(GL_FALSE),
  hasNormalMapInTangentSpace_(GL_FALSE),
  tessConfig_(TESS_PRIMITVE_TRIANGLES, 3)
{
}

void ShaderGenerator::addUniform(const GLSLUniform &v)
{
  fragmentShader_.addUniform(v);
  vertexShader_.addUniform(v);
  tessEvalShader_.addUniform(v);
  tessControlShader_.addUniform(v);
  geometryShader_.addUniform(v);
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

map<GLenum, ShaderFunctions> ShaderGenerator::getShaderStages()
{
  map<GLenum, ShaderFunctions> stages;
  stages[GL_VERTEX_SHADER] = vertexShader_;
  stages[GL_FRAGMENT_SHADER] = fragmentShader_;
#define IS_STAGE_USED(s) (s.inputs().size()>0 || s.outputs().size()>0)
  if(IS_STAGE_USED(geometryShader_)) {
    stages[GL_GEOMETRY_SHADER] = geometryShader_;
  }
  if(IS_STAGE_USED(tessControlShader_)) {
    stages[GL_TESS_CONTROL_SHADER] = tessControlShader_;
  }
  if(IS_STAGE_USED(tessEvalShader_)) {
    stages[GL_TESS_EVALUATION_SHADER] = tessEvalShader_;
  }
#undef IS_STAGE_USED
  return stages;
}

void ShaderGenerator::transferVertToTCS(
    const GLSLTransfer &t,
    const string &name,
    const string &vertexVarName)
{
  vertexShader_.addOutput( GLSLTransfer(
      t.type, FORMAT_STRING("out_"<<name), t.numElems, t.forceArray, t.interpolation) );
  tessControlShader_.addInput( GLSLTransfer(
      t.type, FORMAT_STRING("in_"<<name), t.numElems, t.forceArray, t.interpolation) );

  vertexShader_.addExport( GLSLExport(FORMAT_STRING("out_"<<name), vertexVarName) );
}
void ShaderGenerator::transferVertToTES(
    const GLSLTransfer &t,
    const string &name,
    const string &vertexVarName)
{
  GLSLTransfer tes(t.type, FORMAT_STRING("in_"<<name), t.numElems, t.forceArray, t.interpolation);
  if(tessConfig_.isAdaptive) {
    transferVertToTCS(t, name, vertexVarName );

    tessControlShader_.addOutput( GLSLTransfer(
        t.type, FORMAT_STRING("out_"<<name), t.numElems, t.forceArray, t.interpolation) );
    tessControlShader_.addExport(GLSLExport(
        FORMAT_STRING("Out[gl_InvocationID].out_" << name),
        FORMAT_STRING("in_"<<name << "[gl_InvocationID]")
    ));
  } else {
    vertexShader_.addOutput( GLSLTransfer(
        t.type, FORMAT_STRING("out_"<<name), t.numElems, t.forceArray, t.interpolation) );
    vertexShader_.addExport( GLSLExport(FORMAT_STRING("out_"<<name), vertexVarName) );
  }
  tessEvalShader_.addInput( GLSLTransfer(
      t.type, FORMAT_STRING("in_"<<name), t.numElems, t.forceArray, t.interpolation) );
  // also add a main var in the TES stage that contains interpolated value of input.
  // thats done so it is easy to avoid calculating the interpolation twice
  tessEvalShader_.addMainVar(GLSLVariable(
      t.type, FORMAT_STRING("tes_"<<name),
      interpolate("In", FORMAT_STRING("in_"<<name))
  ));
}
void ShaderGenerator::transferToFrag(
    const string &t,
    const string &n,
    const string &v)
{
  ShaderFunctions &f = (useTessShader_ ? tessEvalShader_ : vertexShader_);
  fragmentShader_.addInput(GLSLTransfer( t, FORMAT_STRING("in_"<<n) ));
  f.addOutput(GLSLTransfer( t, FORMAT_STRING("out_"<<n) ));
  f.addExport(GLSLExport(FORMAT_STRING("out_"<<n), v));
}
void ShaderGenerator::transferFromVertexToFragment(
    const string &type,
    const string &name,
    const string &vertexVarName)
{
  if(useTessShader_) {
    GLSLTransfer transfer(type, name);
    transferVertToTES(transfer, name, vertexVarName);
    transferToFrag(type, name, FORMAT_STRING("tes_"<<name));
  } else {
    transferToFrag(type, name, vertexVarName);
  }
  fragmentShader_.addMainVar(
      GLSLVariable(type, FORMAT_STRING("_"<<name), FORMAT_STRING("in_"<<name))
  );
}

void ShaderGenerator::generate(ShaderConfiguration *cfg)
{
  Material *mat = (Material*)cfg->material();
  list<Light*> lights;

  GLboolean hasModelMat = (cfg->inputs().count("modelMat")>0);

  // check if we have a position defined.
  // just printing a warning here, shader is generated anyway.
  if(cfg->inputs().count("pos")==0)
  {
    WARN_LOG("there seems to be no 'pos' attribute specified.");
  }

  for(set<State*>::const_iterator
      it=cfg->lights().begin(); it!=cfg->lights().end(); ++it)
  {
    lights.push_back((Light*)(*it));
  }

  // with this flag normal is inverted for back faces
  isTwoSided_ = (mat!=NULL ? mat->twoSided() : false);

  // find the shading mode
  if(cfg->lights().empty()) {
    // no shading with no lights
    shading_ = Material::NO_SHADING;
  } else if(mat==NULL) {
    // fall back to gourad shading if no material set
    shading_ = Material::GOURAD_SHADING;
  } else {
    // material defines shading mode
    shading_ = mat->shading();
  }
  useShading_ = ( shading_!=Material::NO_SHADING );
  if(useShading_)
  {
    // gourad does shading in vertex shader all other in fragment shader
    useVertexShading_ = ( shading_==Material::GOURAD_SHADING );
    useFragmentShading_ = !useVertexShading_;
  }
  else
  {
    useVertexShading_ = GL_FALSE;
    useFragmentShading_ = GL_FALSE;
  }

  // configure shaders to use tesselation
  useTessShader_ = cfg->useTesselation();
  tessConfig_ = cfg->tessCfg();
  if(useTessShader_) {
    tessControlShader_.setMinVersion(400);
    fragmentShader_.setMinVersion(400);
    vertexShader_.setMinVersion(400);
    tessEvalShader_.setMinVersion(400);

    tessEvalShader_.set_tessPrimitive(tessConfig_.primitive);
    tessEvalShader_.set_tessSpacing(tessConfig_.spacing);
    tessEvalShader_.set_tessOrdering(tessConfig_.ordering);
  }

  transferNorToTES_ = GL_FALSE;

  // adding some default constants for the case that
  // no material node was defined (or for the case that just a few uniforms
  // were defined).
  addConstant(GLSLConstant( "vec4", "in_materialEmission", "vec4(0.0)" ));
  addConstant(GLSLConstant( "vec4", "in_materialSpecular", "vec4(1.0)" ));
  addConstant(GLSLConstant( "vec4", "in_materialAmbient", "vec4(1.0)" ));
  addConstant(GLSLConstant( "vec4", "in_materialDiffuse", "vec4(1.0)" ));
  addConstant(GLSLConstant( "float", "in_materialShininess", "0.0" ));
  addConstant(GLSLConstant( "float", "in_materialShininessStrength", "1.0" ));
  addConstant(GLSLConstant( "float", "in_materialRefractionIndex", "0.95" ));
  addConstant(GLSLConstant( "float", "in_materialRoughness", "1.0" ));
  addConstant(GLSLConstant( "float", "in_materialAlpha", "1.0" ));
  addConstant(GLSLConstant( "float", "in_materialReflection", "0.0" ));
  addConstant(GLSLConstant( "vec4", "in_fogColor", "vec4(1.0)" ));
  addConstant(GLSLConstant( "float", "in_fogEnd", "200.0" ));
  addConstant(GLSLConstant( "float", "in_fogScale", "0.005" ));
  addConstant(GLSLConstant( "vec3", "in_nor", "vec3(0.0, 1.0, 0.0)" ));

  // textures require some special setup because they may use custom
  // transfer functions and they may use generates texture coordinates.
  setupTextures(cfg->textures());

  // position is the only shader input that must be a per vertex attribute.
  // it requires special setup because it is transformed to common
  // coordinate spaces in vertex shader.
  // following stages may want to do some calculations in one of the
  // spaces.
  setupPosition(
      hasModelMat,
      cfg->ignoreCameraRotation(),
      cfg->ignoreCameraTranslation(),
      cfg->maxNumBoneWeights());
  // normal is also transformed by rotational part of transformation
  setupNormal(
      hasModelMat,
      cfg->maxNumBoneWeights());
  // set up generated texture coordinates
  setupTexco();

  // setup shading in VS / TES. For Gourad shading all calculations done there
  // for other shadings there might be some stuff that makes sense to calculate
  // before the fragment shading
  if(useShading_) {
    if(useTessShader_) {
      setShading(tessEvalShader_, lights, GL_VERTEX_SHADER);
    } else {
      setShading(vertexShader_, lights, GL_VERTEX_SHADER);
    }
  }

  // modify normal by texture
  if(useFragmentShading_) {
    addNormalMaps((useTessShader_ ? tessEvalShader_ : vertexShader_), lights, GL_FALSE);
    addNormalMaps(fragmentShader_, lights, GL_TRUE);
  }

  // calculate the fragment output
  setupFragmentShader(
      lights,
      cfg->fragmentOutputs(),
      cfg->useFog());

  // lastly setup inputs. This overwrites previously declared input
  // with the same name. For example the material property constants above
  // could be overwritten by input from material state.
  // Inputs are automatically transformed (using default interpolation)
  // to the stages where the inputs are used.
  // Modifying input and passing it modified to the next stage is not supported yet
  // (position and normal are handled separate for now)
  setupInputs(cfg->inputs());
}

//////////////////

void ShaderGenerator::setupAttribute(ShaderInput *input)
{
  GLSLTransfer transfer(
      inputType(input),
      FORMAT_STRING("in_" << input->name()),
      input->elementCount(),
      input->forceArray(),
      input->interpolationMode()
      );
  vertexShader_.addInput( transfer );

  if(string(ATTRIBUTE_NAME_POS).compare(input->name())==0 ||
      string(ATTRIBUTE_NAME_NOR).compare(input->name())==0)
  {
    // special handling for 'nor' and 'pos' was done before....
    return;
  }

  // try to find out the last stage using this input attribute
  // and transfer it to this stage.
  if(useTessShader_)
  {
    if(ShaderManager::containsInputVar(
        FORMAT_STRING("in_"<<input->name()), fragmentShader_))
    {
      // transfer to fragment shader
      transferVertToTES(transfer, input->name(), transfer.name);
      transferToFrag(transfer.type, input->name(), FORMAT_STRING("tes_"<<input->name()));
    }
    else if(ShaderManager::containsInputVar(
        FORMAT_STRING("in_"<<input->name()), tessEvalShader_))
    {
      // transfer to tes shader
      transferVertToTES(transfer, input->name(), transfer.name);
    }
    else if(ShaderManager::containsInputVar(
        FORMAT_STRING("in_"<<input->name()), tessControlShader_))
    {
      // transfer to tcs shader
      if(tessConfig_.isAdaptive) {
        transferVertToTCS(transfer, input->name(), transfer.name);
      }
    }
  }
  else if(ShaderManager::containsInputVar(
        FORMAT_STRING("in_"<<input->name()), fragmentShader_))
  {
    // transfer custom attribute from VS to FS
    transferToFrag(transfer.type, input->name(), transfer.name);
  }
}

void ShaderGenerator::setupInputs(map< string, ref_ptr<ShaderInput> > &inputs)
{
  for(map< string, ref_ptr<ShaderInput> >::iterator
      jt = inputs.begin(); jt != inputs.end(); ++jt)
  {
    ShaderInput *input = jt->second.get();
    if(input->isVertexAttribute())
    {
      setupAttribute(input);
    }
    else if(input->isConstant())
    {
      addConstant(GLSLConstant(
          inputType(input),
          FORMAT_STRING("in_" << input->name()),
          inputValue(input),
          input->elementCount(),
          input->forceArray()
          ));
    }
    else // is uniform
    {
      addUniform(GLSLUniform(
          inputType(input),
          FORMAT_STRING("in_" << input->name()),
          input->elementCount(),
          input->forceArray()
          ));
    }
  }
}

void ShaderGenerator::setupTextures(const map<string,State*> &textures)
{
  TextureMapToMap::iterator needle;

  for(map<string,State*>::const_iterator
      it = textures.begin(); it != textures.end(); ++it)
  {
    TextureState *textureState = (TextureState*)it->second;
    Texture *texture = textureState->texture().get();

    addUniform(GLSLUniform(texture->samplerType(),
        FORMAT_STRING("in_" << texture->name())));

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
          ShaderFunctions::getFlatTexco,
          false) );
      break;
    case MAPPING_CUBE:
      texcoGens_.push_back( TexcoGenerator(
          texture->texcoChannel(), "vec3", "cubeUV",
          "getCubeUV(_posScreen.xyz, _normal)",
          ShaderFunctions::getCubeTexco,
          false) );
      break;
    case MAPPING_TUBE:
      texcoGens_.push_back( TexcoGenerator(
          texture->texcoChannel(), "vec2", "tubeUV",
          "getTubeUV(_posScreen.xyz, _normal)",
          ShaderFunctions::getTubeTexco,
          false) );
      break;
    case MAPPING_SPHERE:
      texcoGens_.push_back( TexcoGenerator(
          texture->texcoChannel(), "vec2", "sphereUV",
          "getSphereUV(_posScreen.xyz, _normal)",
          ShaderFunctions::getSphereTexco,
          false) );
      break;
    case MAPPING_REFLECTION_REFRACTION:
      texcoGens_.push_back( TexcoGenerator(
          texture->texcoChannel(), "vec3", "refractionUV",
          "refract(_incident.xyz, _normal, in_materialRefractionIndex)",
          "",
          true) );
      texcoGens_.push_back( TexcoGenerator(
          texture->texcoChannel(), "vec3", "reflectionUV",
          "reflect(_incident.xyz, _normal)",
          "",
          true) );
      transferNorToTES_ = GL_TRUE;
      break;
    case MAPPING_REFLECTION:
      texcoGens_.push_back( TexcoGenerator(
          texture->texcoChannel(), "vec3", "reflectionUV",
          "reflect(_incident.xyz, _normal)",
          "",
          true) );
      transferNorToTES_ = GL_TRUE;
      break;
    case MAPPING_REFRACTION:
      texcoGens_.push_back( TexcoGenerator(
          texture->texcoChannel(), "vec3", "refractionUV",
          "refract(_incident.xyz, _normal, in_materialRefractionIndex)",
          "",
          true) );
      transferNorToTES_ = GL_TRUE;
      break;
    }

    GLboolean usedInVS = GL_FALSE;
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
      texcoMapToMap_[texture->texcoChannel()].insert(*jt);
      switch(*jt) {
      case MAP_TO_HEIGHT:
      case MAP_TO_DISPLACEMENT:
      case MAP_TO_NORMAL:
        usedInVS = GL_TRUE;
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
      transfer->addShaderInputs(&fragmentShader_);
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

//////

void ShaderGenerator::setupPosition(
    GLboolean hasModelMat,
    GLboolean ignoreViewRotation,
    GLboolean ignoreViewTranslation,
    GLuint numBoneWeights)
{
  // load height and displacement maps
  // if tesselation enabled we need the displacement vector in the TCS stage.
  string worldPosVSName = "_posWorld";
  string worldPosVS = ShaderFunctions::posWorldSpace(
      vertexShader_, "in_pos", hasModelMat, numBoneWeights);
  GLboolean useDisplacement = GL_FALSE;
  if(useTessShader_) {
    string worldDisplacementVS = "";
    addOffsets(vertexShader_, worldDisplacementVS, GL_FALSE);
    if(worldDisplacementVS.size()>0) {
      if(tessConfig_.isAdaptive) {
        transferVertToTCS(
            GLSLTransfer("vec3", "in_displacement"),
            "displacement",
            worldDisplacementVS
         );
      }
      useDisplacement = GL_TRUE;
    }
  } else {
    size_t buf = worldPosVSName.size();
    addOffsets(vertexShader_, worldPosVSName, GL_TRUE);
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
      addOffsets(tessEvalShader_, posWorldTESName, GL_TRUE);
      if(posWorldTESName.size() > buf) {
        tessEvalShader_.addStatement( GLSLExport("_posWorld", posWorldTESName) );
      }
    }
    tessEvalShader_.addMainVar(GLSLVariable("vec4", "_posWorld", posWorldTES));

    tessEvalShader_.addMainVar(GLSLVariable("vec4", "_posScreen"));
    if(useShading_) {
      tessEvalShader_.addMainVar(GLSLVariable("vec4", "_posEye"));
      tessEvalShader_.addStatement(
          GLSLExport("_posEye", "in_viewMatrix * _posWorld"));
    }
    tessEvalShader_.addStatement(
        GLSLExport("_posScreen", "in_viewProjectionMatrix * _posWorld"));
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
    if(ignoreViewRotation) {
      posEye = "(vec4(in_viewMatrix[3].xyz,0.0) + _posWorld)";
      if(useShading_) {
        posScreen = "in_projectionMatrix * _posEye";
      } else {
        posScreen = FORMAT_STRING("in_projectionMatrix * " << posEye);
      }
    } else if(ignoreViewTranslation) {
      posEye = "(mat4(in_viewMatrix[0], in_viewMatrix[1], in_viewMatrix[2], vec3(0.0), 1.0) * _posWorld)";
      if(useShading_) {
        posScreen = "in_projectionMatrix * _posEye";
      } else {
        posScreen = FORMAT_STRING("in_projectionMatrix * " << posEye);
      }
    } else {
      posEye = "in_viewMatrix * _posWorld";
      posScreen = "in_viewProjectionMatrix * _posWorld";
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

void ShaderGenerator::setupNormal(
    GLboolean hasModelMat,
    GLuint numBoneWeights)
{
  GLboolean transferNorToFS = useFragmentShading_
      || mapToMap_.find(MAP_TO_VOLUME) != mapToMap_.end();

  // TODO: this is done to complicated.
  // also i do not like the transferNorToTES_ flag.
  // better use/extend interface for passing varyings through stages

  // there is a per vertex normal attribute

  GLSLTransfer vertNor("vec3", "in_nor");

  // FIXME SHADERGEN: normal transform is wrong for scaled objects
  //   http://www.lighthouse3d.com/tutorials/glsl-tutorial/the-normal-matrix/
  string worldNorVS = ShaderFunctions::norWorldSpace(
      vertexShader_, vertNor.name, hasModelMat, numBoneWeights);
  vertexShader_.addMainVar(GLSLVariable("vec3", "_normal", worldNorVS));

  if(useVertexShading_)
  {
    if(useTessShader_) {
      transferVertToTES(vertNor, ATTRIBUTE_NAME_NOR, "_normal");
      // interpolate normal for use in TES
      tessEvalShader_.addMainVar(GLSLVariable("vec3", "_normal", "tes_nor"));
      if(transferNorToFS) {
        transferToFrag("vec3", ATTRIBUTE_NAME_NOR, "_normal");
        fragmentShader_.addMainVar( GLSLVariable("vec3", "_normal", "in_nor") );
      }
    } else {
      // nothing to do, only vertex shader needs normal
      if(transferNorToFS) {
        transferVertToTES(vertNor, ATTRIBUTE_NAME_NOR, "_normal");
        transferToFrag("vec3", ATTRIBUTE_NAME_NOR, "in_nor");
        if(isTwoSided_) {
          fragmentShader_.addMainVar( GLSLVariable(
            "vec3", "_normal", "(gl_FrontFacing ? in_nor : -in_nor)") );
        } else {
          fragmentShader_.addMainVar( GLSLVariable("vec3", "_normal", "in_nor") );
        }
      }
    }
  }
  else if(useFragmentShading_)
  {
    if(useTessShader_) {
      transferVertToTES(vertNor, ATTRIBUTE_NAME_NOR, "_normal");
      tessEvalShader_.addOutput(GLSLTransfer( "vec3", "out_nor" ));
      tessEvalShader_.addMainVar(GLSLVariable("vec3", "_normal", "tes_nor"));
      // interpolate normal and pass to fragment!
      tessEvalShader_.addExport(GLSLExport("out_nor", "_normal"));
    } else {
      // interpolate from vert to frag shader
      vertexShader_.addOutput(GLSLTransfer( "vec3", "out_nor" ));
      vertexShader_.addExport(GLSLExport("out_nor", "_normal"));
    }

    fragmentShader_.addInput(GLSLTransfer( "vec3", "in_nor" ));
    if(isTwoSided_) {
      fragmentShader_.addMainVar( GLSLVariable(
        "vec3", "_normal", "(gl_FrontFacing ? normalize(in_nor) : -normalize(in_nor))") );
    } else {
      fragmentShader_.addMainVar( GLSLVariable(
        "vec3", "_normal", "normalize(in_nor)") );
    }
  }
  else if(transferNorToFS)
  {
    if(useTessShader_) {
      transferVertToTES(vertNor, ATTRIBUTE_NAME_NOR, "_normal");
      if(transferNorToTES_) {
        tessEvalShader_.addMainVar(GLSLVariable("vec3", "_normal", "tes_nor"));
        transferToFrag("vec3", ATTRIBUTE_NAME_NOR, "_normal");
      } else {
        transferToFrag("vec3", ATTRIBUTE_NAME_NOR, "tes_nor");
      }
    } else {
      transferToFrag("vec3", ATTRIBUTE_NAME_NOR, "in_nor");
    }
    fragmentShader_.addMainVar( GLSLVariable("vec3", "_normal", "in_nor") );
  }
  else if(transferNorToTES_ && useTessShader_)
  {
    // interpolate normal for use in TES
    transferVertToTES(vertNor, ATTRIBUTE_NAME_NOR, "_normal");
    tessEvalShader_.addMainVar( GLSLVariable("vec3", "_normal", "in_nor") );
  }
}

void ShaderGenerator::texcoFindMapTo(
    GLuint unit, GLboolean *useFragmentUV, GLboolean *useVertexUV)
{
  // lookup mapTo modes for texco unit and check if texco used in fragment shader and/or before
  set<TextureMapTo> &mapToSet = texcoMapToMap_[unit];

  // flag if this texco attribute is not used in fragment shader
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
  // setup generated texco
  for(list<TexcoGenerator>::iterator
      it=texcoGens_.begin(); it!=texcoGens_.end(); ++it)
  {
    TexcoGenerator &gen = *it;
    GLboolean useFragmentUV, useVertexUV;
    texcoFindMapTo(gen.unit, &useFragmentUV, &useVertexUV);
    ShaderFunctions &f = (useTessShader_ ? tessEvalShader_ : vertexShader_);
    if(gen.needsIncident) {
      f.addMainVar( GLSLVariable("vec3", "_incident", "normalize( _posWorld.xyz - in_cameraPosition.xyz )" ) );
      break;
    }
  }
  for(list<TexcoGenerator>::iterator
      it=texcoGens_.begin(); it!=texcoGens_.end(); ++it)
  {
    TexcoGenerator &gen = *it;

    GLboolean useFragmentUV, useVertexUV;
    texcoFindMapTo(gen.unit, &useFragmentUV, &useVertexUV);

    if(useVertexUV) {
      // calculate texco for use in TES or vertex shader
      ShaderFunctions &f = (useTessShader_ ? tessEvalShader_ : vertexShader_);
      GLSLVariable texcoVar(gen.type, "", gen.functionCall);
      // pretend to have the texco as input, adding it with matching name as var
      if(useTessShader_) {
        texcoVar.name = FORMAT_STRING("_"<<gen.name);
      } else {
        texcoVar.name = FORMAT_STRING("in_"<<gen.name);
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

void ShaderGenerator::setupFragmentShader(
    const list<Light*> &lights,
    const list<ShaderFragmentOutput*> &fragmentOutputFunctions,
    GLboolean useFog)
{
  // set the local var _color to the primaray color.
  // the default value is 1.0 but it can be overwritten by
  // sader inputs.
  fragmentShader_.addConstant( GLSLConstant("vec4", "in_col0", "vec4(1.0)") );
  fragmentShader_.addMainVar( GLSLVariable("vec4", "_color", "in_col0") );

  fragmentShader_.addMainVar( GLSLVariable(
    "float", "_alpha", "in_materialAlpha" ));

  if(useShading_)
  {
    fragmentShader_.addMainVar( GLSLVariable(
      "float", "_brightness", "1.0" ));
    fragmentShader_.addMainVar( GLSLVariable(
      "vec4", "_ambientTerm", "vec4(0.0)" ));
    fragmentShader_.addMainVar( GLSLVariable(
      "vec4", "_specularTerm", "vec4(0.0)" ));
    fragmentShader_.addMainVar( GLSLVariable(
      "vec4", "_emissionTerm", "in_materialEmission" ));
    fragmentShader_.addMainVar( GLSLVariable(
      "vec4", "_diffuseTerm", "vec4(0.0)" ));

    if(useFragmentShading_)
    {
      // prepare local vars for shading in fragment shader,
      // setting them to material properties. Light shader will combine
      // them with light properties
      fragmentShader_.addMainVar( GLSLVariable(
        "vec4", "_materialSpecular", "in_materialSpecular" ));
      fragmentShader_.addMainVar( GLSLVariable(
        "vec4", "_materialAmbient", "in_materialAmbient" ));
      fragmentShader_.addMainVar( GLSLVariable(
        "vec4", "_materialDiffuse", "in_materialDiffuse" ));
      fragmentShader_.addMainVar( GLSLVariable(
        "float", "_materialShininess", "in_materialShininess" ));
      fragmentShader_.addMainVar( GLSLVariable(
        "float", "_materialShininessStrength", "in_materialShininessStrength" ));
    }
    else if(useVertexShading_)
    {
      // ambient,diffuse and specular term calculated in stage before
      // and passed to fragment shader
      fragmentShader_.addMainVar( GLSLVariable(
        "vec4", "_materialAmbient", "vec4(in_lightAmbient,1.0)" ));
      fragmentShader_.addMainVar( GLSLVariable(
        "vec4", "_materialDiffuse", "vec4(in_lightDiffuse,1.0)" ));
      fragmentShader_.addMainVar( GLSLVariable(
        "vec4", "_materialSpecular", "vec4(in_lightSpecular,1.0)" ));
      vertexShader_.addMainVar( GLSLVariable(
        "vec4", "_materialSpecular", "in_materialSpecular" ));
      vertexShader_.addMainVar( GLSLVariable(
        "vec4", "_materialAmbient", "in_materialAmbient" ));
      vertexShader_.addMainVar( GLSLVariable(
        "vec4", "_materialDiffuse", "in_materialDiffuse" ));
      vertexShader_.addMainVar( GLSLVariable(
        "float", "_materialShininess", "in_materialShininess" ));
      vertexShader_.addMainVar( GLSLVariable(
        "float", "_materialShininessStrength", "in_materialShininessStrength" ));
    }
  }

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
    setShading(fragmentShader_, lights, GL_FRAGMENT_SHADER);
  }

  // calculate reflection factor for diffuse and specular light
  addDiffuseReflectionMaps();
  addSpecularReflectionMaps();
  // set alpha value of output
  addAlphaMaps();
  addReflectionMaps();

  if(useShading_)
  {
    fragmentShader_.addStatement(
        GLSLEquation("_color",
            "(_emissionTerm + _ambientTerm + _brightness*_diffuseTerm)*_color + _brightness*_specularTerm"));
    fragmentShader_.addStatement(
        GLSLEquation("_color.a", "_color.a * _alpha"));
  }
  else
  {
    fragmentShader_.addStatement(
        GLSLEquation("_color.a", "_color.a * _alpha"));
  }
  if(useFog)
  {
    fragmentShader_.addMainVar(GLSLVariable("float", "_fogVar",
        "clamp(in_fogScale*(in_fogEnd + gl_FragCoord.z), 0.0, 1.0);"));
    fragmentShader_.addStatement(
        GLSLEquation("_color", "mix(in_fogColor, _color, _fogVar)"));
  }

  for(list<ShaderFragmentOutput*>::const_iterator
      it=fragmentOutputFunctions.begin(); it!=fragmentOutputFunctions.end(); ++it)
  {
    (*it)->addOutput(fragmentShader_);
  }
  if(fragmentOutputFunctions.size()==0)
  {
    DefaultFragmentOutput defaultOutput;
    defaultOutput.addOutput(fragmentShader_);
  }
}

///////////////////

void ShaderGenerator::setShading(
    ShaderFunctions &shader,
    const list<Light*> &lights,
    GLenum shaderType)
{
  vector<string> args1, args2;

  if(shaderType == GL_VERTEX_SHADER) {
    args1.push_back("_posEye");
    args2.push_back("_posEye");
    args2.push_back("_normal");
    if(shading_ == Material::GOURAD_SHADING) {
      args2.push_back("_materialAmbient");
      args2.push_back("_materialDiffuse");
      args2.push_back("_materialSpecular");
      args2.push_back("_materialShininess");
      args2.push_back("_materialShininessStrength");
    }

    ShaderFunctions func;
    switch(shading_) {
    case Material::PHONG_SHADING:
      func = PhongShadingVert(args1, lights);
      break;
    case Material::GOURAD_SHADING:
      func = GouradShadingVert(args2, lights);
      break;
    case Material::BLINN_SHADING:
      func = BlinnShadingVert(args1, lights);
      break;
    case Material::TOON_SHADING:
      func = ToonShadingVert(args1, lights);
      break;
    case Material::ORENNAYER_SHADING:
      func = OrenNayerShadingVert(args1, lights);
      break;
    case Material::MINNAERT_SHADING:
      func = MinnaertShadingVert(args1, lights);
      break;
    case Material::COOKTORRANCE_SHADING:
      func = CookTorranceShadingVert(args1, lights);
      break;
    case Material::NO_SHADING: return;
    }
    shader.operator+=(func);
  }
  else if(shaderType == GL_FRAGMENT_SHADER)
  {
    if(hasNormalMapInTangentSpace_) {
      args1.push_back("in_posTangent");
    } else {
      args1.push_back("in_posEye");
    }
    args1.push_back("_normal");
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
    case Material::PHONG_SHADING:
      func = PhongShadingFrag(args1, lights);
      break;
    case Material::GOURAD_SHADING:
      func = GouradShadingFrag(args2, lights);
      break;
    case Material::BLINN_SHADING:
      func = BlinnShadingFrag(args1, lights);
      break;
    case Material::TOON_SHADING:
      func = ToonShadingFrag(args1, lights);
      break;
    case Material::ORENNAYER_SHADING:
      func = OrenNayerShadingFrag(args1, lights);
      break;
    case Material::MINNAERT_SHADING:
      func = MinnaertShadingFrag(args1, lights);
      break;
    case Material::COOKTORRANCE_SHADING:
      func = CookTorranceShadingFrag(args1, lights);
      break;
    case Material::NO_SHADING: return;
    }
    shader.operator+=(func);
  }
}

//////////////////

string ShaderGenerator::texel(
    const State *state,
    ShaderFunctions &func,
    GLboolean addMainvar)
{
  string texelLookup;
  TextureState *textureState = (TextureState*)state;
  Texture *texture = textureState->texture().get();

  if(texture->mapping() == MAPPING_REFLECTION_REFRACTION) {
    texelLookup = FORMAT_STRING("mix( " <<
        "texture( in_" << texture->name() << ", in_refractionTexco ), " <<
        "texture( in_" << texture->name() << ", in_reflectionTexco ), " <<
        "in_materialReflection )");
  } else {
    string texco;
    switch(texture->mapping()) {
    case MAPPING_SPHERE:
      texco = "sphereTexco";
      break;
    case MAPPING_TUBE:
      texco = "tubeTexco";
      break;
    case MAPPING_CUBE:
      texco = "cubeTexco";
      break;
    case MAPPING_FLAT:
      texco = "flatTexco";
      break;
    case MAPPING_REFLECTION:
      texco = "reflectionTexco";
      break;
    case MAPPING_REFRACTION:
      texco = "refractionTexco";
      break;
    default: texco = FORMAT_STRING("texco" << texture->texcoChannel());
    }
    texelLookup = FORMAT_STRING("texture( in_" << texture->name() << ", in_" << texco << " )");
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
  if(textures == mapToMap_.end()) { return; }

  vector<string> args;

  for(list<State*>::iterator
      it = textures->second.begin(); it != textures->second.end(); ++it)
  {
    TextureState *textureState = (TextureState*)(*it);
    Texture *texture = textureState->texture().get();

    string texelVar = texel(textureState, fragmentShader_);

    if(texture->contrast() != 1.0f) {
      args.push_back(texelVar);
      args.push_back(FORMAT_STRING(texture->contrast()));
      ContrastBlender blend(args);
      fragmentShader_.operator+=(blend);
      args.clear();
    }

    if(texture->blendMode() == BLEND_MODE_SRC)
    {
      fragmentShader_.addStatement(GLSLExport(outputColor, texelVar));
    }
    else
    {
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
  if(textures == mapToMap_.end()) { return; }

  for(list<State*>::iterator
      it = textures->second.begin(); it != textures->second.end(); ++it)
  {
    TextureState *textureState = (TextureState*)(*it);
    Texture *texture = textureState->texture().get();

    string texelVar = texel(textureState, fragmentShader_);

    fragmentShader_.addStatement(GLSLEquation("_materialShininess",
        FORMAT_STRING("_materialShininess * " << texelVar << ".r")));
  }
}
void ShaderGenerator::addAlphaMaps()
{
  TextureMapToMap::iterator textures = mapToMap_.find(MAP_TO_ALPHA);
  if(textures == mapToMap_.end()) return;

  for(list<State*>::iterator
      it = textures->second.begin(); it != textures->second.end(); ++it)
  {
    TextureState *textureState = (TextureState*)(*it);
    Texture *texture = textureState->texture().get();

    string texelVar = texel(textureState, fragmentShader_);

    fragmentShader_.addStatement(GLSLEquation("_materialColor.a",
        FORMAT_STRING("_materialColor.a * " << texelVar << ".r")));
  }
}
void ShaderGenerator::addReflectionMaps()
{
  TextureMapToMap::iterator textures = mapToMap_.find(MAP_TO_REFLECTION);
  if(textures == mapToMap_.end()) { return; }

  for(list<State*>::iterator
      it = textures->second.begin(); it != textures->second.end(); ++it)
  {
    TextureState *textureState = (TextureState*)(*it);
    Texture *texture = textureState->texture().get();

    string texelVar = texel(textureState, fragmentShader_);

    fragmentShader_.addStatement(GLSLEquation("_materialReflection",
        FORMAT_STRING("_materialReflection * " << texelVar << ".r")));
  }
}
void ShaderGenerator::addDiffuseReflectionMaps()
{
  TextureMapToMap::iterator textures = mapToMap_.find(MAP_TO_DIFFUSE_REFLECTION);
  if(textures == mapToMap_.end()) { return; }

  for(list<State*>::iterator
      it = textures->second.begin(); it != textures->second.end(); ++it)
  {
    TextureState *textureState = (TextureState*)(*it);
    Texture *texture = textureState->texture().get();

    string texelVar = texel(textureState, fragmentShader_);

    fragmentShader_.addStatement(GLSLEquation("_diffuseTerm",
        FORMAT_STRING("_diffuseTerm * " << texelVar << ".r")));
  }
}
void ShaderGenerator::addSpecularReflectionMaps()
{
  TextureMapToMap::iterator textures = mapToMap_.find(MAP_TO_SPECULAR_REFLECTION);
  if(textures == mapToMap_.end()) { return; }

  for(list<State*>::iterator
      it = textures->second.begin(); it != textures->second.end(); ++it)
  {
    TextureState *textureState = (TextureState*)(*it);
    Texture *texture = textureState->texture().get();

    string texelVar = texel(textureState, fragmentShader_);

    fragmentShader_.addStatement(GLSLEquation("_diffuseTerm",
        FORMAT_STRING("_specularTerm * " << texelVar << ".r")));
  }
}

void ShaderGenerator::addLightMaps()
{
  TextureMapToMap::iterator textures = mapToMap_.find(MAP_TO_LIGHT);
  if(textures == mapToMap_.end()) { return; }

  for(list<State*>::iterator
      it = textures->second.begin(); it != textures->second.end(); ++it)
  {
    TextureState *textureState = (TextureState*)(*it);
    Texture *texture = textureState->texture().get();

    string texelVar = texel(textureState, fragmentShader_);

    fragmentShader_.addStatement(GLSLEquation("_ambientTerm",
        FORMAT_STRING("_ambientTerm + _materialAmbient*" << texelVar << ".r")));
    fragmentShader_.addStatement(GLSLEquation("_diffuseTerm",
        FORMAT_STRING("_diffuseTerm + _materialDiffuse*" << texelVar << ".r")));
    fragmentShader_.addStatement(GLSLEquation("_specularTerm",
        FORMAT_STRING("_specularTerm + _materialSpecular*" << texelVar << ".r")));
  }
}

void ShaderGenerator::addVolumeMaps()
{
  TextureMapToMap::iterator textures = mapToMap_.find(MAP_TO_VOLUME);
  if(textures == mapToMap_.end()) { return; }
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
    GLboolean calcNor)
{
  // create eye space normal mapping shader,
  // tangent space cannot be calculated by all primitives.
  TextureMapToMap::iterator textures = mapToMap_.find(MAP_TO_NORMAL);
  if(textures == mapToMap_.end()) { return; }

  vector<string> args;
  for(list<State*>::iterator
      it = textures->second.begin(); it != textures->second.end(); ++it)
  {
    TextureState *textureState = (TextureState*)(*it);
    Texture *texture = textureState->texture().get();

    if(calcNor) {
      string texelVar = texel(textureState, shader);
      args.push_back(FORMAT_STRING(texelVar << " * " << texture->heightScale()));
      args.push_back("_normal");
      BumpMapFrag bump(args, isTwoSided_);
      shader.operator+=(bump);
      args.clear();
    } else {
      // input vars
      args.push_back("_normal");
      args.push_back("in_tan");
      // output vars
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

void ShaderGenerator::addOffsets(ShaderFunctions &shader, string &pos, GLboolean isVec4)
{
  addHeightMaps(shader, pos, isVec4);
  addDisplacementMaps(shader, pos, isVec4);
}
void ShaderGenerator::addHeightMaps(ShaderFunctions &shader, string &pos, GLboolean isVec4)
{
  TextureMapToMap::iterator textures = mapToMap_.find(MAP_TO_HEIGHT);
  if(textures == mapToMap_.end()) { return; }
  for(list<State*>::iterator
      it = textures->second.begin(); it != textures->second.end(); ++it)
  {
    TextureState *textureState = (TextureState*)(*it);
    Texture *t = textureState->texture().get();
    string texelVar = texel(*it, shader);
    string translation = FORMAT_STRING(
        "_normal*("<<texelVar<<".r*("<<t->heightScale()<<"))");
    if(isVec4) translation = FORMAT_STRING("vec4(" << translation << ", 0.0)");
    pos = (pos.size() == 0 ? translation : FORMAT_STRING(pos << " + " << translation));
  }
  pos = FORMAT_STRING("(" << pos << ")");
}
void ShaderGenerator::addDisplacementMaps(ShaderFunctions &shader, string &pos, GLboolean isVec4)
{
  TextureMapToMap::iterator textures = mapToMap_.find(MAP_TO_DISPLACEMENT);
  if(textures == mapToMap_.end()) { return; }
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
