/*
 * shader-generator.h
 *
 *  Created on: 30.10.2011
 *      Author: daniel
 */

#ifndef SHADER_GENERATOR_H_
#define SHADER_GENERATOR_H_

#include <config.h>

#include <string>
#include <list>

#include <ogle/shader/shader-function.h>
#include <ogle/gl-types/tbo.h>
#include <ogle/gl-types/vertex-attribute.h>
#include <ogle/gl-types/texture.h>
#include <ogle/gl-types/tesselation-config.h>
#include <ogle/states/material-state.h>
#include <ogle/states/light-state.h>
#include <ogle/states/state.h>

struct TexcoGenerator {
  GLuint unit;
  string type;
  string name;
  string functionCall;
  string functionCode;
  bool needsIncident;
  TexcoGenerator(
      GLuint _unit,
      string _type,
      string _name,
      string _functionCall,
      string _functionCode,
      bool _needsIncident)
  : unit(_unit),
    type(_type),
    name(_name),
    functionCall(_functionCall),
    functionCode(_functionCode),
    needsIncident(_needsIncident)
  {}
};

/**
 * Helps dynamically generating shaders,
 */
class ShaderGenerator
{
public:
  /**
   * Default constructor.
   */
  ShaderGenerator();

  map<GLenum, ShaderFunctions> getShaderStages();

  /**
   * Generate the shader.
   * Afterwards generated shaders can be accessed with vertexShader(),fragmentShader(),...
   */
  void generate(const ShaderConfiguration *cfg);

private:
  ShaderFunctions vertexShader_;
  ShaderFunctions fragmentShader_;
  ShaderFunctions geometryShader_;
  ShaderFunctions tessControlShader_;
  ShaderFunctions tessEvalShader_;

  VertexAttribute *primaryColAttribute_;
  VertexAttribute *posAttribute_;
  VertexAttribute *norAttribute_;
  list<VertexAttribute*> uvAttributes_;
  list<TexcoGenerator> texcoGens_;

  typedef map< TextureMapTo, list< State* > > TextureMapToMap;
  TextureMapToMap mapToMap_;
  map<GLuint, set<TextureMapTo> > uvMapToMap_;

  set< pair<string,string> > customAttributeNames_;

  bool transferNorToTES_;
  bool hasInstanceMat_;
  bool useFog_;
  bool useTessShader_;
  bool useShading_;
  bool useFragmentShading_;
  bool useVertexShading_;
  bool isTwoSided_;
  bool hasNormalMapInTangentSpace_;

  GLuint maxNumBoneWeights_;

  bool ignoreViewRotation_;
  bool ignoreViewTranslation_;
  Material::Shading shading_;

  Tesselation tessConfig_;

  ///////////////////

  void setupAttributes(const set<VertexAttribute*> &attributes);
  void setupTextures(const set<State*> &textures);
  void setupLights(const set<State*> &lights);
  void setupMaterial(const State *material);

  ///////////////////

  void setupPosition();
  void setupNormal(const list<Light*> &lights);
  void setupTexco();
  void setupColor();
  void setupFog();

  ///////////////////

  void setFragmentVars();
  void setFragmentExports(const set<ShaderFragmentOutput*>&);
  void setFragmentFunctions(
      const list<Light*> &lights,
      const State *material);

  ///////////////////

  void setShading(
      ShaderFunctions &shader,
      const list<Light*> &lights,
      const State *material,
      GLenum shaderType);

  //////////////////

  string texel(
      const State *textureState,
      ShaderFunctions &func,
      bool addMainVar=true);

  void addColorMaps(
      TextureMapTo mapTo,
      string outputColor);
  void addShininessMaps();
  void addAlphaMaps();
  void addReflectionMaps();
  void addDiffuseReflectionMaps();
  void addSpecularReflectionMaps();

  void addLightMaps();

  void addVolumeMaps();

  void addNormalMaps(
      ShaderFunctions &shader,
      const list<Light*> &lights,
      bool calcNor);

  string interpolate(
      const string &a,
      const string &n);
  void addOffsets(
      ShaderFunctions &shader,
      string &pos,
      bool isVec4);
  void addHeightMaps(
      ShaderFunctions &shader,
      string &pos,
      bool isVec4);
  void addShellOffset(
      ShaderFunctions &shader,
      string &pos,
      bool isVec4);
  void addDisplacementMaps(
      ShaderFunctions &shader,
      string &pos,
      bool isVec4);

  void addVolumeSliceMaps();

  void addUniform(
      ShaderFunctions &shader,
      const string &type,
      const string &name);
  void addUniformToAll(
      const string &type,
      const string &name);
  void addConstant(
      const GLSLConstant &v);

  void transferVertToTES(
      const GLSLTransfer &t,
      const string &name,
      const string &vertexVarName);
  void transferVertToTCS(
      const GLSLTransfer &t,
      const string &name,
      const string &vertexVarName);
  void transferToFrag(
      const string &t,
      const string &n,
      const string &v);
  void transferFromVertexToFragment(
      const string &t,
      const string &n,
      const string &v);
  void texcoFindMapTo(
      GLuint unit,
      bool *useFragmentUV,
      bool *useVertexUV);
  string stageTexcoName(
      ShaderFunctions &f,
      const string &n);
};

#endif /* SHADER_GENERATOR_H_ */
