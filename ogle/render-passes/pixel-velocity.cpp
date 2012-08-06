/*
 * pixel-velocity.cpp
 *
 *  Created on: 18.07.2012
 *      Author: daniel
 */

#include <pixel-velocity.h>

class ProjectionChangedPV : public EventCallable {
public:
  ProjectionChangedPV(PixelVelocityPass *p) : EventCallable(), p_(p) {}
  virtual void call(EventObject*,void*) {
    p_->resize();
  }
  PixelVelocityPass *p_;
};

PixelVelocityPass::PixelVelocityPass(Scene* scene,
    CoordinateSpace velocitySpace,
    bool useDepthBuffer,
    bool useSceneDepthBuffer,
    float depthBias,
    float sizeScale)
: RenderPass(),
  velocitySpace_(velocitySpace),
  scene_(scene),
  updateInterval_(0.05f),
  sizeScale_(sizeScale),
  depthBias_(depthBias),
  useDepthBuffer_(useDepthBuffer),
  useSceneDepthBuffer_(useSceneDepthBuffer),
  velocityPassShader_(ref_ptr<Shader>::manage(new Shader(StateObject::SETTER))),
  projectionChangedCB_(ref_ptr<EventCallable>::manage(new ProjectionChangedPV(this)))
{
  switch(velocitySpace_) {
  case OBJECT_SPACE:
    WARN_LOG("OBJECT_SPACE position not transfered to fragment shader.");
    WARN_LOG("    using WORLD_SPACE instead.");
    velocitySpace_ = WORLD_SPACE;
    break;
  case WORLD_SPACE:
  case EYE_SPACE:
  case SCREEN_SPACE:
    break;
  }

  bufferSize_ = Vec2ui(
      scene_->width()*sizeScale_,
      scene_->height()*sizeScale_);
  { // create buffer/textures used for rendering the velocity
    velocityBuffer_ = ref_ptr<FrameBufferObject>::manage(new FrameBufferObject);
    velocityBuffer_->bind();
    velocityBuffer_->set_size(bufferSize_.x, bufferSize_.y);
    { // create the velocity texture
      velocityTex_ = ref_ptr<Texture2D>::manage(new Texture2D);
      velocityTex_->set_size(bufferSize_.x, bufferSize_.y);
      velocityTex_->set_format(GL_RG);
      // use 2d float format
      velocityTex_->set_internalFormat(GL_RG16F);
      velocityTex_->bind();
      velocityTex_->set_filter(GL_LINEAR,GL_LINEAR);
      velocityTex_->texImage();
      velocityBuffer_->addColorAttachment(*velocityTex_.get());
    }
    if(useDepthBuffer_) {
      depthTexture_ = ref_ptr<Texture2D>::manage(new DepthTexture2D());
      depthTexture_->set_size(bufferSize_.x, bufferSize_.y);
      depthTexture_->set_internalFormat(GL_DEPTH_COMPONENT24);
      depthTexture_->bind();
      depthTexture_->texImage();
      velocityBuffer_->set_depthAttachment(*depthTexture_.get());
    }
  }
  // get notification if scene projection changes
  scene_->connect(Scene::PROJECTION_CHANGED, projectionChangedCB_);

  ShaderFunctions fragmentShader, vertexShader;

  // VS: just transfers last and current position to FS
  vertexShader.addInput( GLSLTransfer("vec4", "v_pos0"));
  vertexShader.addInput( GLSLTransfer("vec4", "v_pos1"));
  vertexShader.addOutput( GLSLTransfer("vec2", "f_pos0") );
  vertexShader.addOutput( GLSLTransfer("vec2", "f_pos1") );
  vertexShader.addExport( GLSLExport("f_pos0", "v_pos0.xy") );
  vertexShader.addExport( GLSLExport("f_pos1", "v_pos1.xy") );
  switch(velocitySpace_) {
  case OBJECT_SPACE:
    break;
  case WORLD_SPACE:
    vertexShader.addExport( GLSLExport("gl_Position", "viewProjectionMatrix * v_pos0") );
    break;
  case EYE_SPACE:
    vertexShader.addExport( GLSLExport("gl_Position", "projectionMatrix * v_pos0") );
    break;
  case SCREEN_SPACE:
    vertexShader.addExport( GLSLExport("gl_Position", "v_pos0") );
    break;
  }

  // FS: calculates xy-velocity.
  // optional depth test against scene depth buffer
  fragmentShader.addUniform( GLSLUniform("uvec2", "viewport") );
  fragmentShader.addInput( GLSLTransfer("vec2", "f_pos0"));
  fragmentShader.addInput( GLSLTransfer("vec2", "f_pos1"));
  // output the velocity in velocitySpace_ units per second
  fragmentShader.addFragmentOutput( GLSLFragmentOutput(
      "vec2", "velocityOutput", GL_COLOR_ATTACHMENT0 ) );
  if(useSceneDepthBuffer_) {
    fragmentShader.addUniform( GLSLUniform("sampler2D", "depthTexture") );
    // bias used for scene depth buffer access
    fragmentShader.addMainVar( GLSLVariable(
        "const float", "depthBias", FORMAT_STRING(depthBias_)) );
    // use the fragment coordinate to find the texture coordinates of
    // this fragment in the scene depth buffer.
    // gl_FragCoord.xy is in window space, divided by the buffer size
    // we get the range [0,1] that can be used for texture access.
    fragmentShader.addMainVar( GLSLVariable(
        "vec2", "depthTexco", "vec2(gl_FragCoord.x/viewport.x, gl_FragCoord.y/viewport.y)") );
    // depth at this pixel obtained in main pass.
    // this is non linear depth in the range [0,1].
    fragmentShader.addMainVar( GLSLVariable(
        "float", "sceneDepth", "texture(depthTexture, depthTexco).r") );
    // do the depth test against gl_FragCoord.z using a bias.
    // bias is used to avoid flickering
    fragmentShader.addStatement( GLSLStatement(
        "if( gl_FragCoord.z > sceneDepth+depthBias ) { discard; }" ) );
  }
  fragmentShader.addExport( GLSLExport(
      "velocityOutput", "(f_pos0 - f_pos1)/deltaT" ));

  // upload shader to gl
  velocityPassShader_->set_functions(fragmentShader, GL_FRAGMENT_SHADER);
  velocityPassShader_->set_functions(vertexShader, GL_VERTEX_SHADER);
  velocityPassShader_->compile(true);
  velocityPassShader_->setupUniforms();
  glBindFragDataLocation(velocityPassShader_->id(), 0, "velocityOutput");

  { // query attribute and uniform locations
    posAttLoc0_ = glGetAttribLocation( velocityPassShader_->id(), "v_pos0" );
    if(posAttLoc0_==-1) {
      ERROR_LOG("PixelVelocityPass has no 'v_pos0' attribute.");
    }
    posAttLoc1_ = glGetAttribLocation( velocityPassShader_->id(), "v_pos1" );
    if(posAttLoc1_==-1) {
      ERROR_LOG("PixelVelocityPass has no 'v_pos1' attribute.");
    }

    velocityPassShader_->addUniform( scene_->deltaTUniform() );
    viewportLoc_ = glGetUniformLocation( velocityPassShader_->id(), "viewport" );
    if(useSceneDepthBuffer_) {
      depthTextureLoc_ = glGetUniformLocation(
          velocityPassShader_->id(), "depthTexture" );
    }
    switch(velocitySpace_) {
    case OBJECT_SPACE:
    case SCREEN_SPACE:
      break;
    case WORLD_SPACE:
      matLoc_ = glGetUniformLocation(
          velocityPassShader_->id(), "viewProjectionMatrix");
      break;
    case EYE_SPACE:
      matLoc_ = glGetUniformLocation(
          velocityPassShader_->id(), "projectionMatrix");
      break;
    }
  }
}

PixelVelocityPass::~PixelVelocityPass()
{
  scene_->disconnect(projectionChangedCB_);
}

void PixelVelocityPass::resize()
{
  { // update the velocity FBO size
    bufferSize_ = Vec2ui(
        scene_->width()*sizeScale_,
        scene_->height()*sizeScale_);
    velocityBuffer_->set_size(bufferSize_.x, bufferSize_.y);
    velocityTex_->set_size(bufferSize_.x, bufferSize_.y);
    velocityTex_->bind();
    velocityTex_->texImage();
    if(useDepthBuffer_) {
      depthTexture_->set_size(bufferSize_.x, bufferSize_.y);
      depthTexture_->bind();
      depthTexture_->texImage();
    }
  }
}

void PixelVelocityPass::addMesh(Mesh *mesh)
{
  PixelVelocityMesh mesh_;
  mesh_.mesh = mesh;

  switch(velocitySpace_) {
  case OBJECT_SPACE:
    break;
  case WORLD_SPACE:
    mesh_.posAtt0 = ref_ptr<VertexAttribute>::manage(
        new VertexAttributefv( "posWorld", 4 ));
    break;
  case EYE_SPACE:
    mesh_.posAtt0 = ref_ptr<VertexAttribute>::manage(
        new VertexAttributefv( "posEye", 4 ));
    break;
  case SCREEN_SPACE:
    mesh_.posAtt0 = ref_ptr<VertexAttribute>::manage(
        new VertexAttributefv( "Position", 4 ));
    break;
  }
  mesh->addTransformFeedbackAttribute(mesh_.posAtt0);

  // ping pong attribute
  mesh_.posAtt1 = ref_ptr<VertexAttribute>::manage(
      new VertexAttributefv( "", 4 ));
  mesh->addTransformFeedbackAttribute(mesh_.posAtt1);

  meshes_.push_back(mesh_);
}

void PixelVelocityPass::render()
{
  // velocity buffer is render target
  velocityBuffer_->bind();
  velocityTex_->set_viewport();
  // draw to velocity texture
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  // clear velocity to zero
  glClearColor(0.0,0.0,0.0,0.0);
  glClear(GL_COLOR_BUFFER_BIT);

  if(useDepthBuffer_) {
    // velocity FBO has depth buffer attached
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
  }

  velocityPassShader_->enableShader();

  switch(velocitySpace_) {
  case OBJECT_SPACE:
  case SCREEN_SPACE:
    break;
  case WORLD_SPACE:
    scene_->viewProjectionMatrixUniform()->apply(matLoc_);
    break;
  case EYE_SPACE:
    scene_->projectionMatrixUniform()->apply(matLoc_);
    break;
  }

  glUniform2uiv(viewportLoc_, 1, &bufferSize_.x);

  if(useSceneDepthBuffer_) {
    // the shader uses a depth test in the FS against the
    // depth buffer from main pass.
    // Note that there are some precision issues with that
    // that's the reason there is also the possibility
    // to use a depth buffer attached to the velocity FBO
    glActiveTexture(GL_TEXTURE5);
    scene_->depthTexture().bind();
    glUniform1i(depthTextureLoc_, 5);
  }

  // draw the added meshes into the velocity buffer
  // using transform feedback data from main pass.
  for(list<PixelVelocityMesh>::iterator
      it = meshes_.begin(); it != meshes_.end(); ++it)
  {
    Mesh *mesh = it->mesh;

    // bind the transform feedback buffer
    VertexBufferObject *vbo = mesh->transformFeedbackBuffer();
    if(!vbo) continue;
    glBindBuffer(GL_ARRAY_BUFFER, vbo->id());

    // bind attributes to the shader
    // posAtt0 is the feedback attribute from this frame
    it->posAtt0->set_location( posAttLoc0_ );
    it->posAtt0->enable();
    // posAtt1 is the feedback attribute from last frame
    it->posAtt1->set_location( posAttLoc1_ );
    it->posAtt1->enable();
    // draw the transform feedback buffer
    mesh->drawTransformFeedback();

    // ping pong TF buffer
    // we just change the info how to access this
    // attribute in the TF VBO.
    // TODO: bad to do it here!
    //  maybe someone else is watching at the pos TF.
    //  best would be to do the ping pong pre rendering frame
    //  scene would need new interface for that...
    //  maybe a pre-render signal ??
    {
      GLuint buf;
      buf = it->posAtt0->offset();
      it->posAtt0->set_offset( it->posAtt1->offset() );
      it->posAtt1->set_offset( buf );
      buf = it->posAtt0->stride();
      it->posAtt0->set_stride( it->posAtt1->stride() );
      it->posAtt1->set_stride( buf );
    }
  }

  if(useDepthBuffer_) {
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
  }
}
