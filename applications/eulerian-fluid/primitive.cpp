/*
 * primitive.cpp
 *
 *  Created on: 11.03.2012
 *      Author: daniel
 */

#include "include/primitive.h"

EulerianPrimitive::EulerianPrimitive(
    GLuint width, GLuint height, GLuint depth,
    Mesh *updatePrimitive,
    UniformMat4 *unitOrthoMat,
    UniformFloat *deltaT,
    bool isLiquid, bool useHalfFloats)
: updatePrimitive_(updatePrimitive),
  unitOrthoMat_(unitOrthoMat),
  timeStep_(deltaT),
  width_(width), height_(height), depth_(depth),
  isLiquid_(isLiquid),
  useHalfFloats_(useHalfFloats)
{
  if(is2D()) {
    inverseSize_ = ref_ptr<Uniform>::manage(
        new UniformVec2("inverseGridSize") );
    ((UniformVec2*) inverseSize_.get())->set_value( (Vec2f) { 1.0f/width, 1.0f/height } );
  } else {
    inverseSize_ = ref_ptr<Uniform>::manage(
        new UniformVec3("inverseGridSize") );
    ((UniformVec3*) inverseSize_.get())->set_value( (Vec3f) { 1.0f/width, 1.0f/height, 1.0f/depth } );
  }
}

class SelectLayerShaderGeom  : public ShaderFunctions
{
public:
  SelectLayerShaderGeom(const vector<string> &args)
  : ShaderFunctions("selectLayer", args)
  {
    setMinVersion(150);
    enableExtension("GL_EXT_geometry_shader4");

    addInput( GLSLTransfer( "vec4", "g_pos", 3, false, "smooth" ) );
    addInput( GLSLTransfer( "int", "g_instanceID", 3, false, "smooth" ) );
    addOutput( GLSLTransfer( "float", "f_layer" ) );
  }
  string code() const
  {
    stringstream s;
    s << "void selectLayer() {" << endl;
    s << "    gl_Layer = g_instanceID[0];" << endl;
    s << "    f_layer = float(g_instanceID[0]) + 0.5;" << endl;
    s << "    gl_Position = g_pos[0]; EmitVertex();" << endl;
    s << "    gl_Position = g_pos[1]; EmitVertex();" << endl;
    s << "    gl_Position = g_pos[2]; EmitVertex();" << endl;
    s << "    EndPrimitive();" << endl;
    s << "}" << endl;
    return s.str();
  }
private:
  float normalLength_;
};

ref_ptr<Shader> EulerianPrimitive::makeShader(
    const ShaderFunctions &func,
    const string &dimensionVec)
{
  ShaderFunctions fragmentShader, vertexShader, geometryShader;

  vertexShader.addInput( GLSLTransfer( "vec3", "v_pos" ) );
  vertexShader.addMainVar( (GLSLVariable) {
    "vec4", "pos_", "projectionUnitMatrixOrtho * (vec4(v_pos,1.0))"} );
  if(is2D()) {
    vertexShader.addExport( (GLSLExport) { "gl_Position", "pos_"} );
  } else {
    vertexShader.enableExtension("GL_EXT_gpu_shader4");

    vertexShader.addOutput( GLSLTransfer( "vec4", "g_pos" ) );
    vertexShader.addExport( (GLSLExport) { "g_pos", "pos_" } );

    vertexShader.addOutput( GLSLTransfer( "int", "g_instanceID" ) );
    vertexShader.addExport( (GLSLExport) { "g_instanceID", "gl_InstanceID" } );

    fragmentShader.addInput( GLSLTransfer( "float", "f_layer" ) );

    vertexShader.addExport( (GLSLExport) { "gl_Position", "pos_"} );

    vector<string> args;
    SelectLayerShaderGeom selectLayer(args);
    geometryShader.operator+=( selectLayer );
  }

  fragmentShader.addMainVar( (GLSLVariable) { dimensionVec, "fragmentColor_", ""} );
  fragmentShader.addExport( (GLSLExport) { "defaultColorOutput", "fragmentColor_" });
  fragmentShader.addFragmentOutput( (GLSLFragmentOutput)
      { dimensionVec, "defaultColorOutput", GL_COLOR_ATTACHMENT0 } );
  fragmentShader.operator+=( func );

  ref_ptr<Shader> shader_ = ref_ptr<Shader>::manage( new Shader() );
  shader_->set_functions(fragmentShader, GL_FRAGMENT_SHADER);
  shader_->set_functions(vertexShader, GL_VERTEX_SHADER);
  if(!is2D()) shader_->set_functions(geometryShader, GL_GEOMETRY_SHADER);
  shader_->compile(true);

  if(!is2D()) {
    glProgramParameteriEXT(shader_->id(),
        GL_GEOMETRY_VERTICES_OUT_EXT, 3);
    glProgramParameteriEXT(shader_->id(),
        GL_GEOMETRY_INPUT_TYPE_EXT, GL_TRIANGLES);
    glProgramParameteriEXT(shader_->id(),
        GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLES);
  }

  glBindFragDataLocation(shader_->id(), 0, "defaultColorOutput");
  glBindFragDataLocation(shader_->id(), 1, "defaultColorOutput");
  glBindFragDataLocation(shader_->id(), 2, "defaultColorOutput");

  shader_->addUniform(unitOrthoMat_);

  return shader_;
}

void EulerianPrimitive::enableShader(ref_ptr<Shader> shader) {
  glUseProgram( shader->id() );

  list< ref_ptr<VertexAttribute> > *atts = updatePrimitive_->inputs();
  for(list< ref_ptr<VertexAttribute> >::iterator it  = atts->begin();
          it != atts->end(); ++it)
  {
    VertexAttribute *att = it->get();
    att->set_location( glGetAttribLocation(
        shader->id(), FORMAT_STRING("v_"<<att->name()).c_str()) );
    att->enable();
  }
  // set uniforms
  map<Uniform*, int>::iterator uit;
  for(uit = shader->uniformToLocation().begin();
      uit != shader->uniformToLocation().end(); ++uit)
  {
    uit->first->apply( uit->second );
  }
}

void EulerianPrimitive::bind() {
  glBindBuffer(GL_ARRAY_BUFFER, updatePrimitive_->buffer());
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, updatePrimitive_->indexBuffer());
}

void EulerianPrimitive::draw() {
  if(is2D()) {
    updatePrimitive_->drawMesh();
  } else {
    // draw quad depth times offsetting the z value
    glDrawElementsInstancedEXT(
        updatePrimitive_->primitive(),
        updatePrimitive_->numIndexes(),
        updatePrimitive_->indicesDataType(),
        updatePrimitive_->vboOffset(),
        depth_);
  }
}

