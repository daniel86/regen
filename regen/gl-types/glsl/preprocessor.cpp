/*
 * preprocessor.cpp
 *
 *  Created on: 12.05.2013
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#define NO_REGEX_MATCH boost::sregex_iterator()

#include <set>
#include <map>
#include <list>
using namespace std;

#include <regen/utility/string-util.h>
#include <regen/gl-types/gl-enum.h>
#include <regen/gl-types/glsl/includer.h>
#include "directive-processor.h"
#include "preprocessor.h"
using namespace regen;

static const char* mainPattern =
    ".*void[ \n]+main\\([ \n]*|[ \n]*void[ \n]*\\)[ \n]*\\{.*\\}.*";

PreProcessor::PreProcessor()
{}

void PreProcessor::addProcessor(const ref_ptr<GLSLProcessor> &processor)
{
  processor->setParent(lastProcessor_);
  lastProcessor_ = processor;
}
void PreProcessor::removeProcessor(GLSLProcessor *processor)
{
  ref_ptr<GLSLProcessor> child;
  for(ref_ptr<GLSLProcessor> p=lastProcessor_;
      p.get()!=NULL; p=p->getParent())
  {
    if(p.get()==processor) {
      if(child.get()) {
        child->setParent(p->getParent());
      }
      p->setParent(ref_ptr<GLSLProcessor>());
      break;
    }
    child = p;
  }
}

map<GLenum,string> PreProcessor::processStages(const PreProcessorInput &in)
{
  static boost::regex mainRegex(mainPattern);
  map<GLenum,string> processed;
  const string *currentCode=NULL;

  // find effect names
  set<string> effectNames;
  for(map<GLenum,string>::const_iterator
      it=in.unprocessed.begin(); it!=in.unprocessed.end(); ++it)
  {
    if(it->second.empty()) { continue; }
    if(Includer::get().isKeyValid(it->second)) {
      list<string> path;
      boost::split(path, it->second, boost::is_any_of("."));
      effectNames.insert(*path.begin());
    }
  }

  PreProcessorState state_(in);
  state_.nextStage = GL_NONE;
  state_.version = 150;

  // reverse process stages, because stages must know inputs
  // of next stages.
  for(GLint i=glenum::glslStageCount()-1; i>=0; --i)
  {
    GLenum stage = glenum::glslStages()[i];

    map<GLenum,string>::const_iterator it = in.unprocessed.find(stage);
    if(it!=in.unprocessed.end() && !it->second.empty())
    {
      currentCode = &it->second;
    }
    else {
      if(stage==GL_VERTEX_SHADER) {
        // no vertex shader specified. try to find one with
        // specified effect keys.
        for(set<string>::iterator it=effectNames.begin(); it!=effectNames.end(); ++it)
        {
          string defaultVSName = REGEN_STRING((*it) << ".vs");
          const string &vsCode = Includer::get().include(defaultVSName);
          if(!vsCode.empty())
          {
            currentCode = &vsCode;
            break;
          }
        }
      }
      if(currentCode==NULL) continue;
    }

    state_.currStage = stage;

    // fill input stream
    state_.inStream.clear();
    state_.inStream << "#define SHADER_STAGE " << glenum::glslStagePrefix(stage) << endl;
    state_.inStream << in.header << endl;
    if(Includer::get().isKeyValid(*currentCode))
    { state_.inStream << "#include " << (*currentCode) << endl; }
    else
    { state_.inStream << (*currentCode) << endl; }
    currentCode = NULL;

    // execute processor pipeline
    stringstream out; string line;
    while(lastProcessor_->getline(state_,line)) out<<line<<endl;
    // post process pipeline output
    stringstream postProcessed;
    // insert version statement at the top. ATI is strict about this.
    postProcessed << "#version " << state_.version << endl;
    postProcessed << out.str();
    it = processed.insert(make_pair(stage,postProcessed.str())).first;

    // check if a main function is defined
    boost::sregex_iterator regexIt(it->second.begin(), it->second.end(), mainRegex);
    if(regexIt==NO_REGEX_MATCH) {
      processed.erase(stage);
      continue;
    }

    state_.nextStage = stage;
  }

  for(ref_ptr<GLSLProcessor> p=lastProcessor_;
      p.get()!=NULL; p=p->getParent())
  { p->clear(); }

  return processed;
}
