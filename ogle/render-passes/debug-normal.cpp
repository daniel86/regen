/*
 * debug-normal.cpp
 *
 *  Created on: 29.07.2012
 *      Author: daniel
 */

#include <debug-normal.h>

class DebugNormalShaderGeom  : public ShaderFunctions
{
public:
  DebugNormalShaderGeom(Mesh *mesh, const vector<string> &args, float normalLength=0.1)
  : ShaderFunctions("debugNormal", args),
    normalLength_(normalLength)
  {
    setMinVersion(150);
    enableExtension("GL_EXT_geometry_shader4");
    addInput( GLSLTransfer( "vec3", "g_nor",
        mesh->numFaceVertices(), true, "smooth" ) );
    addInput( GLSLTransfer(  "vec4", "g_pos",
        mesh->numFaceVertices(), true, "smooth" ) );
  }
  string code() const
  {
    stringstream s;
    s << "void debugNormal() {" << endl;
    s << "    for(int i=0; i< gl_VerticesIn; i++){" << endl;
    s << "        vec4 posV = g_pos[i];" << endl;
    s << "        vec3 norV = g_nor[i];" << endl;
    s << "        " << endl;
    s << "        gl_Position = posV; EmitVertex();" << endl;
    s << "        gl_Position = posV + vec4(norV,0) * " << normalLength_ << "; EmitVertex();" << endl;
    s << "        EndPrimitive();" << endl;
    s << "    }" << endl;
    s << "}" << endl;
    return s.str();
  }
private:
  float normalLength_;
};
DebugNormalPass::DebugNormalPass(Scene* scene, Mesh* mesh)
: RenderPass(),
  mesh_(mesh),
  normalPassShader_(ref_ptr<Shader>::manage(new Shader(StateObject::SETTER)))
{
  ShaderFunctions fragmentShader, vertexShader;

  // feedback for pos in eye space
  posAtt_ = ref_ptr<VertexAttribute>::manage(
      new VertexAttributefv( "Position", 4 ));
  mesh_->addTransformFeedbackAttribute(posAtt_);
  // feedback for nor in world space
  norAtt_ = ref_ptr<VertexAttribute>::manage(
      new VertexAttributefv( ATTRIBUTE_NAME_NOR ));
  mesh_->addTransformFeedbackAttribute(norAtt_);

  vertexShader.enableExtension("GL_EXT_gpu_shader4");
  vertexShader.addUniform( GLSLUniform("mat4", "viewProjectionMatrix") );
  vertexShader.addInput( GLSLTransfer("vec3", "v_nor"));
  vertexShader.addOutput( GLSLTransfer( "vec3", "g_nor", 1, false, "smooth" ) );
  vertexShader.addExport( GLSLExport("g_nor",
      "normalize(viewProjectionMatrix * vec4(v_nor,0.0)).xyz" ) );
  vertexShader.addInput( GLSLTransfer("vec4", "v_pos"));
  vertexShader.addOutput( GLSLTransfer("vec4", "g_pos", 1, false, "smooth") );
  vertexShader.addExport( GLSLExport("g_pos", "v_pos") );
  vertexShader.addExport( GLSLExport("gl_Position", "v_pos") );
  vertexShader.setMinVersion(150);

  fragmentShader.addExport( (GLSLExport) {
    "defaultColorOutput", "vec4(1.0, 1.0, 0.0, 1.0)" });
  fragmentShader.addFragmentOutput( (GLSLFragmentOutput) {
    "vec4", "defaultColorOutput", GL_COLOR_ATTACHMENT0 } );
  fragmentShader.setMinVersion(150);

  vector<string> args;
  DebugNormalShaderGeom geomFuncs(mesh_, args);

  normalPassShader_->set_functions(fragmentShader, GL_FRAGMENT_SHADER);
  normalPassShader_->set_functions(vertexShader, GL_VERTEX_SHADER);
  normalPassShader_->set_functions(geomFuncs, GL_GEOMETRY_SHADER);
  normalPassShader_->compile(false);
  glProgramParameteriEXT(normalPassShader_->id(),
      GL_GEOMETRY_VERTICES_OUT_EXT, mesh_->numFaceVertices()*2);
  glProgramParameteriEXT(normalPassShader_->id(),
      GL_GEOMETRY_INPUT_TYPE_EXT, mesh_->primitive());
  glProgramParameteriEXT(normalPassShader_->id(),
      GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_LINE_STRIP);
  glLinkProgram(normalPassShader_->id());
  normalPassShader_->setupUniforms();
  glBindFragDataLocation(normalPassShader_->id(), 0, "defaultColorOutput");

  posAttLoc_ = glGetAttribLocation( normalPassShader_->id(), "v_pos" );
  norAttLoc_ = glGetAttribLocation( normalPassShader_->id(), "v_nor" );

  normalPassShader_->addUniform( scene->viewProjectionMatrixUniform() );
}
void DebugNormalPass::render()
{
  VertexBufferObject *vbo = mesh_->transformFeedbackBuffer();
  if(!vbo) return;

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  normalPassShader_->enableShader();
  // bind the transform feedback buffer
  glBindBuffer(GL_ARRAY_BUFFER, vbo->id());

  posAtt_->set_location( posAttLoc_ ); posAtt_->enable();
  norAtt_->set_location( norAttLoc_ ); norAtt_->enable();

  mesh_->drawTransformFeedback();

  glDepthFunc(GL_LESS);
  glDisable(GL_DEPTH_TEST);
}
Shader& DebugNormalPass::shader()
{
  return *normalPassShader_.get();
}
