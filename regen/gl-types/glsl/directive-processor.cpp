/*
 * glsl-preprocessor.cpp
 *
 *  Created on: 29.10.2012
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#define NO_REGEX_MATCH boost::sregex_iterator()

#include <list>
#include <string>
#include <map>
#include <set>

#include <regen/utility/string-util.h>
#include <regen/utility/logging.h>

#include "includer.h"
#include "directive-processor.h"
using namespace regen;

DirectiveProcessor::MacroBranch::MacroBranch()
: isDefined_(true),
  isAnyChildDefined_(false),
  parent_(NULL)
{}

DirectiveProcessor::MacroBranch::MacroBranch(bool isDefined, MacroBranch *parent)
: isDefined_(isDefined),
  isAnyChildDefined_(false),
  parent_(parent)
{}

DirectiveProcessor::MacroBranch::MacroBranch(const MacroBranch &other)
: isDefined_(other.isDefined_),
  isAnyChildDefined_(other.isAnyChildDefined_),
  parent_(other.parent_)
{}

DirectiveProcessor::MacroBranch& DirectiveProcessor::MacroBranch::getActive()
{
  return childs_.empty() ? *this : childs_.back().getActive();
}

void DirectiveProcessor::MacroBranch::open(bool isDefined)
{
  // open a new #if / #ifdef branch
  MacroBranch &active = getActive();
  active.childs_.push_back( MacroBranch(
      isDefined&&active.isDefined_, &active) );
  if(isDefined) {
    // remember if the argument is defined (all #else cases can be skipped then)
    active.isAnyChildDefined_ = true;
  }
}

void DirectiveProcessor::MacroBranch::add(bool isDefined)
{
  MacroBranch &active = getActive();
  MacroBranch *parent = active.parent_;
  if(parent==NULL) { return; }

  bool defined;
  if(parent->isAnyChildDefined_) {
    defined = false;
  } else {
    defined = isDefined&&parent->isDefined_;
  }
  if(isDefined) {
    // remember if the argument is defined (all following #else cases can be skipped then)
    parent->isAnyChildDefined_ = true;
  }

  parent->childs_.push_back( MacroBranch(defined, parent));
}

void DirectiveProcessor::MacroBranch::close()
{
  MacroBranch &active = getActive();
  MacroBranch *parent = active.parent_;
  if(parent==NULL) { return; }
  parent->childs_.clear();
  parent->isAnyChildDefined_ = false;
}

int DirectiveProcessor::MacroBranch::depth()
{
  return childs_.empty() ? 1 : 1+childs_.back().depth();
}

//////////////
//////////////
//////////////
//////////////

void DirectiveProcessor::MacroTree::clear()
{
  defines_.clear();
  root_ = MacroBranch();
}

GLboolean DirectiveProcessor::MacroTree::isDefined(const string &arg)
{
  return defines_.count(arg)>0;
}

const string& DirectiveProcessor::MacroTree::define(const string &arg)
{
  if(isNumber(arg)) {
    return arg;
  }
  else {
    map<string,string>::iterator it = defines_.find(arg);
    if(it==defines_.end()) {
      return arg;
    } else {
      return it->second;
    }
  }
}

bool DirectiveProcessor::MacroTree::evaluateInner(const string &expression)
{
  static const string operatorsPattern = "(.+)[ ]*(==|!=|<=|>=|<|>)[ ]*(.+)";
  static boost::regex operatorsRegex(operatorsPattern);

  boost::sregex_iterator it(expression.begin(), expression.end(), operatorsRegex);
  if(it!=NO_REGEX_MATCH) {
    string arg0_ = (*it)[1]; boost::trim(arg0_);
    string op    = (*it)[2]; boost::trim(op);
    string arg1_ = (*it)[3]; boost::trim(arg1_);
    const string &arg0 = define(arg0_);
    const string &arg1 = define(arg1_);

    if(op == "==")      return (arg0==arg1);
    else if(op == "!=") return (arg0!=arg1);

    // numeric operators left
    if(!isNumber(arg0) || !isNumber(arg1)) { return false; }
    float val0=0.0f, val1=0.0f;
    stringstream(arg0) >> val0;
    stringstream(arg1) >> val1;

    if(op == "<=")      return (val0<=val1);
    else if(op == ">=") return (val0>=val1);
    else if(op == ">")  return (val0>val1);
    else if(op == "<")  return (val0<val1);
    else                return false;
  }

  // single inner argument
  string arg = expression; boost::trim(arg);
  if(isNumber(arg)) {
    return arg!="0";
  } else {
    map<string,string>::iterator it = defines_.find(arg);
    if(it==defines_.end() || it->second=="0") {
      return false;
    } else {
      return true;
    }
  }
}

bool DirectiveProcessor::MacroTree::evaluate(const string &expression)
{
  static const char* bracketsPattern = "[ ]*\\((.+)\\)[ ]*(\\|\\||\\&\\&)[ ]*([^\\)]+)";
  static const char* pattern = "[ ]*([^\\&\\|]+)[ ]*(\\|\\||\\&\\&)[ ]*(.+)";
  static boost::regex bracketsRegex(bracketsPattern);
  static boost::regex expRegex(pattern);

  // match leading bracket...
  boost::sregex_iterator it(expression.begin(), expression.end(), bracketsRegex);
  if(it!=NO_REGEX_MATCH) {
    string exp0 = (*it)[1]; boost::trim(exp0);
    string op   = (*it)[2]; boost::trim(op);
    string exp1 = (*it)[3]; boost::trim(exp1);
    if(op == "&&") {
      return evaluate(exp0) && evaluate(exp1);
    } else {
      return evaluate(exp0) || evaluate(exp1);
    }
  }
  // match logical expression...
  it = boost::sregex_iterator(expression.begin(), expression.end(), expRegex);
  if(it!=NO_REGEX_MATCH) {
    string exp0 = (*it)[1]; boost::trim(exp0);
    string op   = (*it)[2]; boost::trim(op);
    string exp1 = (*it)[3]; boost::trim(exp1);
    if(op == "&&") {
      return evaluateInner(exp0) && evaluate(exp1);
    } else {
      return evaluateInner(exp0) || evaluate(exp1);
    }
  }
  // no logical operator found...
  return evaluateInner(expression);
}

void DirectiveProcessor::MacroTree::_define(const string &s) {
  if(!isDefined()) { return; }

  size_t pos = s.find_first_of(" ");
  if(pos == string::npos) {
    defines_[ s ] = "1";
  } else {
    defines_[ s.substr(0,pos) ] = s.substr(pos+1);
  }
}

void DirectiveProcessor::MacroTree::_undef(const string &s)
{ defines_.erase(s); }
void DirectiveProcessor::MacroTree::_ifdef(const string &s)
{ root_.open(evaluate(s)); }
void DirectiveProcessor::MacroTree::_ifndef(const string &s)
{ root_.open(!evaluate(s)); }
void DirectiveProcessor::MacroTree::_if(const string &s)
{ root_.open(evaluate(s)); }
void DirectiveProcessor::MacroTree::_elif(const string &s)
{ root_.add(evaluate(s)); }
void DirectiveProcessor::MacroTree::_else()
{ root_.add(GL_TRUE); }
void DirectiveProcessor::MacroTree::_endif()
{ root_.close(); }
bool DirectiveProcessor::MacroTree::isDefined()
{ return root_.getActive().isDefined_; }

////////////
////////////
////////////
////////////

DirectiveProcessor::DirectiveProcessor()
: continuedLine_(""),
  wasEmpty_(GL_TRUE),
  lastStage_(GL_NONE)
{}

void DirectiveProcessor::parseVariables(string &line)
{
  static const char* variablePattern = "\\$\\{[ ]*([^ \\}\\{]+)[ ]*\\}";
  static boost::regex variableRegex(variablePattern);

  set<string> variableNames;
  boost::sregex_iterator regexIt(line.begin(), line.end(), variableRegex);

  if(regexIt==NO_REGEX_MATCH) { return; }

  REGEN_DEBUG("DirectiveProcessor::parseVariables in  '" << line << "'");
  GLboolean replacedSomething = GL_FALSE;

  for(; regexIt!=NO_REGEX_MATCH; ++regexIt)
  {
    const string &match = (*regexIt)[1];
    variableNames.insert(match);
  }
  for(set<string>::iterator it=variableNames.begin(); it!=variableNames.end(); ++it)
  {
    const string &define = *it;
    string name = define.substr(0,define.find_first_of(" "));
    if(tree_.isDefined(name)) {
      const string &value = tree_.define(name);
      boost::replace_all(line, "${"+name+"}", value);
      replacedSomething = GL_TRUE;
    }
  }

  // parse nested vars
  if(replacedSomething==GL_TRUE) parseVariables(line);
  REGEN_DEBUG("DirectiveProcessor::parseVariables out '" << line << "'");
}

void DirectiveProcessor::clear()
{
  lastStage_ = GL_NONE;
  inputs_.clear();
}

bool DirectiveProcessor::getline(PreProcessorState &state, string &line)
{
  if(lastStage_ != state.currStage) {
    lastStage_ = state.currStage;
    tree_.clear();
    continuedLine_.clear();
    forBranches_.clear();
    wasEmpty_ = GL_TRUE;
  }

  if(inputs_.empty()) {
    if(parent_.get()) {
      inputs_.push_back(parent_);
    } else {
      return false;
    }
  }

  ref_ptr<GLSLProcessor> &in = inputs_.front();
  if(!in->getline(state, line)) {
    inputs_.pop_front();
    if(inputs_.empty()) return false;
    else DirectiveProcessor::getline(state,line);
  }

  // the line stopped with '\' character
  if(*line.rbegin() == '\\') {
    continuedLine_ += line + "\n";
    return DirectiveProcessor::getline(state,line);
  }
  if(!continuedLine_.empty()) {
    line = continuedLine_ + line;
    continuedLine_ = "";
  }
  REGEN_DEBUG("DirectiveProcessor::getline in  '" << line << "'");

  // evaluate ${..}
  if(tree_.isDefined() && forBranches_.empty()) { parseVariables(line); }

  string statement(line);
  boost::trim(statement);

  GLboolean isEmpty = statement.empty();
  if(isEmpty && wasEmpty_) {
    return DirectiveProcessor::getline(state,line);
  }
  wasEmpty_ = isEmpty;

  if(hasPrefix(statement, "#line ")) {
    // for now remove line directives
    return DirectiveProcessor::getline(state,line);
  }
  else if(hasPrefix(statement, "#version ")) {
    string versionStr = truncPrefix(statement, "#version ");
    boost::trim(versionStr);
    GLuint version = (GLuint)atoi(versionStr.c_str());
    if(state.version<version) { state.version=version; }
    // remove version directives, must be prepended later
    return DirectiveProcessor::getline(state,line);
  }
  else if(hasPrefix(statement, "#for ")) {
    static const char* forPattern = "#for (.+) to (.+)";
    static boost::regex forRegex(forPattern);
    boost::sregex_iterator regexIt(statement.begin(), statement.end(), forRegex);

    if(regexIt!=NO_REGEX_MATCH) {
      ForBranch branch;
      branch.variableName = (*regexIt)[1]; boost::trim(branch.variableName);
      branch.upToValue    = (*regexIt)[2]; boost::trim(branch.upToValue);
      branch.lines = "";
      forBranches_.push_front(branch);
    }
    else {
      line = "#warning Invalid Syntax: '" + statement + "'. Example: '#for INDEX to 9'.";
      return true;
    }

    return DirectiveProcessor::getline(state,line);
  }
  else if(hasPrefix(statement, "#endfor")) {
    if(forBranches_.empty()) {
      line = "#warning Closing #endfor without opening #for.";
      return true;
    }
    ForBranch &branch = forBranches_.front();

    ref_ptr<StreamProcessor> forLoop = ref_ptr<StreamProcessor>::alloc();
    stringstream &ss = forLoop->stream();

    const string &def = tree_.define(branch.upToValue);
    if(!isNumber(def)) {
      ss << "#error " << branch.upToValue << " is not a number" << endl;
    } else {
      int count = boost::lexical_cast<int>(def);

      for(int i=0; i<count; ++i) {
        ss << "#define2 " << branch.variableName << " " << i << endl;
        ss << branch.lines;
      }
    }
    forBranches_.pop_front();
    inputs_.push_front(forLoop);

    return DirectiveProcessor::getline(state,line);
  }
  else if(!forBranches_.empty()) {
    forBranches_.front().lines += line + "\n";
    return DirectiveProcessor::getline(state,line);
  }
  else if(hasPrefix(statement, "#include ")) {
    if(!tree_.isDefined()) {
      return DirectiveProcessor::getline(state,line);
    }
    string key = truncPrefix(statement, "#include ");
    boost::trim(key);
    string imported;
    map<string,string>::const_iterator needle = state.in.externFunctions.find(key);
    if(needle != state.in.externFunctions.end()) {
      imported = needle->second;
    } else {
      imported = Includer::get().include(key); boost::trim(key);
    }
    if(imported.empty()) {
      line = "#warning Failed to include " + key + ". Make sure GLSW path is set up.";
      REGEN_WARN(Includer::get().errorMessage());
      return true;
    } else {
      inputs_.push_front(ref_ptr<StreamProcessor>::alloc(imported));
    }
    return DirectiveProcessor::getline(state,line);
  }
  else if(hasPrefix(statement, "#define2 ")) {
    string v = truncPrefix(statement, "#define2 ");
    boost::trim(v);
    tree_._define(v);
  }
  else if(hasPrefix(statement, "#define ")) {
    string v = truncPrefix(statement, "#define ");
    boost::trim(v);
    tree_._define(v);
    if(tree_.isDefined()) {
      REGEN_DEBUG("DirectiveProcessor::getline out '" << line << "'");
      return true;
    }
  }
  else if(hasPrefix(statement, "#undef ")) {
    string v = truncPrefix(statement, "#undef ");
    boost::trim(v);
    tree_._undef(v);
    if(tree_.isDefined()) {
      REGEN_DEBUG("DirectiveProcessor::getline out '" << line << "'");
      return true;
    }
  }
  else if(hasPrefix(statement, "#ifdef ")) {
    string v = truncPrefix(statement, "#ifdef ");
    boost::trim(v);
    tree_._ifdef(v);
  }
  else if(hasPrefix(statement, "#ifndef ")) {
    string v = truncPrefix(statement, "#ifndef ");
    boost::trim(v);
    tree_._ifndef(v);
  }
  else if(hasPrefix(statement, "#if ")) {
    string v = truncPrefix(statement, "#if ");
    boost::trim(v);
    tree_._ifdef(v);
  }
  else if(hasPrefix(statement, "#elif ")) {
    string v = truncPrefix(statement, "#elif ");
    boost::trim(v);
    tree_._elif(v);
  }
  else if(hasPrefix(statement, "#else")) {
    tree_._else();
  }
  else if(hasPrefix(statement, "#endif")) {
    tree_._endif();
  }
  else if(tree_.isDefined())
  {
    REGEN_DEBUG("DirectiveProcessor::getline out '" << line << "'");
    return true;
  }
  return DirectiveProcessor::getline(state,line);
}
