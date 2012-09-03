/*
 * shader-generator.h
 *
 *  Created on: 30.10.2011
 *      Author: daniel
 */

#ifndef SHADER_GENERATOR_H_
#define SHADER_GENERATOR_H_

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
  GLboolean needsIncident;
  TexcoGenerator(
      GLuint _unit,
      const string &_type,
      const string &_name,
      const string &_functionCall,
      const string &_functionCode,
      GLboolean _needsIncident)
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

  map<GLenum, ShaderFunctions*> getShaderStages();

  /**
   * Generate the shader.
   * Afterwards generated shaders can be accessed with vertexShader(),fragmentShader(),...
   */
  void generate(ShaderConfiguration *cfg);

private:
  typedef map< TextureMapTo, list< State* > > TextureMapToMap;

  ShaderFunctions vertexShader_;
  ShaderFunctions fragmentShader_;
  ShaderFunctions geometryShader_;
  ShaderFunctions tessControlShader_;
  ShaderFunctions tessEvalShader_;

  TextureMapToMap mapToMap_;
  map<GLuint, set<TextureMapTo> > texcoMapToMap_;
  list<TexcoGenerator> texcoGens_;

  GLboolean transferNorToTES_;
  GLboolean useTessShader_;
  GLboolean useShading_;
  GLboolean useFragmentShading_;
  GLboolean useVertexShading_;
  GLboolean isTwoSided_;
  GLboolean hasNormalMapInTangentSpace_;

  Material::Shading shading_;

  Tesselation tessConfig_;

  void setupInputs(map< string, ref_ptr<ShaderInput> > &inputs);
  void setupAttribute(ShaderInput *attribute);
  void setupPosition(
      GLboolean hasModelMat,
      GLboolean ignoreViewRotation,
      GLboolean ignoreViewTranslation,
      GLuint numBoneWeights);
  void setupNormal(
      GLboolean hasModelMat,
      GLuint numBoneWeights);
  void setupTexco();

  void setupTextures(
      const map<string,State*> &textures);
  void setupFragmentShader(
      const list<Light*> &lights,
      const list<ShaderFragmentOutput*> &fragmentOutputFunctions,
      GLboolean useFog);

  ///////////////////

  void setShading(
      ShaderFunctions &shader,
      const list<Light*> &lights,
      GLenum shaderType);

  //////////////////

  string texel(
      const State *textureState,
      ShaderFunctions &func,
      GLboolean addMainVar=GL_TRUE);

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
      GLboolean calcNor);

  string interpolate(
      const string &a,
      const string &n);
  void addOffsets(
      ShaderFunctions &shader,
      string &pos,
      GLboolean isVec4);
  void addHeightMaps(
      ShaderFunctions &shader,
      string &pos,
      GLboolean isVec4);
  void addDisplacementMaps(
      ShaderFunctions &shader,
      string &pos,
      GLboolean isVec4);

  void addUniform(const GLSLUniform &v);
  void addConstant(const GLSLConstant &v);

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
      GLboolean *useFragmentUV,
      GLboolean *useVertexUV);
};

#endif /* SHADER_GENERATOR_H_ */
