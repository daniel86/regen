/*
 * fire-transfer.cpp
 *
 *  Created on: 16.08.2012
 *      Author: daniel
 */


FireTransfer::FireTransfer(ref_ptr<Texture> pattern)
: TexelTransfer(),
  smokeColor_( ref_ptr<UniformVec3>::manage( new UniformVec3("") )),
  rednessFactor_( ref_ptr<UniformInt>::manage( new UniformInt("") )),
  smokeColorMultiplier_( ref_ptr<UniformFloat>::manage( new UniformFloat("") )),
  smokeAlphaMultiplier_( ref_ptr<UniformFloat>::manage( new UniformFloat("") )),
  fireAlphaMultiplier_( ref_ptr<UniformFloat>::manage( new UniformFloat("") )),
  texelFactor_( ref_ptr<UniformFloat>::manage( new UniformFloat("") )),
  fireWeight_( ref_ptr<UniformFloat>::manage( new UniformFloat("") )),
  pattern_(pattern)
{
  stringstream s;
  texelFactor_->set_value( 1.0f );
  SET_NAME("texelFactor", texelFactor_);
  joinUniform(ref_ptr<Uniform>::cast(texelFactor_));

  rednessFactor_->set_value( 5 );
  SET_NAME("rednessFactor", rednessFactor_);
  joinUniform(ref_ptr<Uniform>::cast(rednessFactor_));

  fireAlphaMultiplier_->set_value( 0.4f );
  SET_NAME("fireAlphaMultiplier", fireAlphaMultiplier_);
  joinUniform(ref_ptr<Uniform>::cast(fireAlphaMultiplier_));

  fireWeight_->set_value( 2.0f );
  SET_NAME("fireWeight", fireWeight_);
  joinUniform(ref_ptr<Uniform>::cast(fireWeight_));

  smokeColor_->set_value( Vec3f(0.9,0.15,0.055) );
  SET_NAME("smokeColor", smokeColor_);
  joinUniform(ref_ptr<Uniform>::cast(smokeColor_));

  smokeColorMultiplier_->set_value( 2.0f );
  SET_NAME("smokeColorMultiplier", smokeColorMultiplier_);
  joinUniform(ref_ptr<Uniform>::cast(smokeColorMultiplier_));

  smokeAlphaMultiplier_->set_value( 0.1f );
  SET_NAME("smokeAlphaMultiplier", smokeAlphaMultiplier_);
  joinUniform(ref_ptr<Uniform>::cast(smokeAlphaMultiplier_));

  SET_NAME("fireTransfer", pattern_);
  ref_ptr<State> tex = ref_ptr<State>::manage(new TextureState(pattern_));
  joinStates(tex);
}
void FireTransfer::addShaderInputs(ShaderFunctions *shader)
{
  shader->addUniform( GLSLUniform( "float", "texelFactor") );
  shader->addUniform( GLSLUniform( "int", "rednessFactor") );
  shader->addUniform( GLSLUniform( "float", "fireAlphaMultiplier") );
  shader->addUniform( GLSLUniform( "float", "fireWeight") );
  shader->addUniform( GLSLUniform( "vec3", "smokeColor") );
  shader->addUniform( GLSLUniform( "float", "smokeColorMultiplier") );
  shader->addUniform( GLSLUniform( "float", "smokeAlphaMultiplier") );
  shader->addUniform( GLSLUniform( "sampler2D", pattern_->name()) );
}
string FireTransfer::transfer() {
  stringstream s;
  s << "vec4 " << transferFuncName_ << "(vec4 v) {" << endl;
  s << "    vec4 outCol;" << endl;
  s << "    const float threshold = 1.4;" << endl;
  s << "    const float maxValue = 5;" << endl;
  s << "    " << endl;
  s << "    float s = v.r * in_texelFactor;" << endl;
  s << "    s = clamp(s,0,maxValue);" << endl;
  s << "    " << endl;
  s << "    if( s > threshold ) { //render fire" << endl;
  s << "        float lookUpVal = ( (s-threshold)/(maxValue-threshold) );" << endl;
  s << "        lookUpVal = 1.0 - pow(lookUpVal, in_rednessFactor);" << endl;
  s << "        lookUpVal = clamp(lookUpVal,0,1);" << endl;
  s << "        vec3 interpColor = texture(in_" << pattern_->name() << ", vec2(1.0-lookUpVal,0)).rgb; " << endl;
  s << "        vec4 tmp = vec4(interpColor,1); " << endl;
  s << "        float mult = (s-threshold);" << endl;
  s << "        outCol.rgb = in_in_fireWeight*tmp.rgb;" << endl;
  s << "        outCol.a = min(1.0, in_fireWeight*mult*mult*in_fireAlphaMultiplier + 0.5); " << endl;
  s << "    } else { // render smoke" << endl;
  s << "        outCol.rgb = vec3(in_fireWeight*s);" << endl;
  s << "        outCol.a = min(1.0, outCol.r*in_smokeAlphaMultiplier);" << endl;
  s << "        outCol.rgb = outCol.a * outCol.rrr * in_smokeColor * in_smokeColorMultiplier; " << endl;
  s << "    }" << endl;
  s << "    return outCol;" << endl;
  s << "}" << endl;
  return s.str();
}
