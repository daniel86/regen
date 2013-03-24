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
#include <regen/external/glsw/glsw.h>

#include "glsl-directive-processor.h"
using namespace regen;

GLSLDirectiveProcessor::MacroBranch& GLSLDirectiveProcessor::MacroBranch::getActive() {
  return childs_.empty() ? *this : childs_.back().getActive();
}
void GLSLDirectiveProcessor::MacroBranch::open(bool isDefined) {
  // open a new #if / #ifdef branch
  MacroBranch &active = getActive();
  active.childs_.push_back( MacroBranch(
      isDefined&&active.isDefined_, &active) );
  if(isDefined) {
    // remember if the argument is defined (all #else cases can be skipped then)
    active.isAnyChildDefined_ = true;
  }
}
void GLSLDirectiveProcessor::MacroBranch::add(bool isDefined) {
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
void GLSLDirectiveProcessor::MacroBranch::close() {
  MacroBranch &active = getActive();
  MacroBranch *parent = active.parent_;
  if(parent==NULL) { return; }
  parent->childs_.clear();
  parent->isAnyChildDefined_ = false;
}
int GLSLDirectiveProcessor::MacroBranch::depth() {
  return childs_.empty() ? 1 : 1+childs_.back().depth();
}

GLSLDirectiveProcessor::MacroBranch::MacroBranch()
: isDefined_(true), isAnyChildDefined_(false), parent_(NULL) {}
GLSLDirectiveProcessor::MacroBranch::MacroBranch(bool isDefined, MacroBranch *parent)
: isDefined_(isDefined), isAnyChildDefined_(false), parent_(parent) {}
GLSLDirectiveProcessor::MacroBranch::MacroBranch(const MacroBranch &other)
: isDefined_(other.isDefined_),
  isAnyChildDefined_(other.isAnyChildDefined_),
  parent_(other.parent_) {}

//////////////
//////////////

GLboolean GLSLDirectiveProcessor::MacroTree::isDefined(const string &arg) {
  return defines_.count(arg)>0;
}
const string& GLSLDirectiveProcessor::MacroTree::define(const string &arg) {
  if(isNumber(arg)) {
    return arg;
  } else {
    map<string,string>::iterator it = defines_.find(arg);
    if(it==defines_.end()) {
      return arg;
    } else {
      return it->second;
    }
  }
}

bool GLSLDirectiveProcessor::MacroTree::evaluateInner(const string &expression)
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
bool GLSLDirectiveProcessor::MacroTree::evaluate(const string &expression)
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

void GLSLDirectiveProcessor::MacroTree::_define(const string &s) {
  if(!isDefined()) { return; }

  size_t pos = s.find_first_of(" ");
  if(pos == string::npos) {
    defines_[ s ] = "1";
  } else {
    defines_[ s.substr(0,pos) ] = s.substr(pos+1);
  }
}
void GLSLDirectiveProcessor::MacroTree::_undef(const string &s) {
  defines_.erase(s);
}
void GLSLDirectiveProcessor::MacroTree::_ifdef(const string &s) {
  root_.open(evaluate(s));
}
void GLSLDirectiveProcessor::MacroTree::_ifndef(const string &s) {
  root_.open(!evaluate(s));
}
void GLSLDirectiveProcessor::MacroTree::_if(const string &s) {
  root_.open(evaluate(s));
}
void GLSLDirectiveProcessor::MacroTree::_elif(const string &s) {
  root_.add(evaluate(s));
}
void GLSLDirectiveProcessor::MacroTree::_else() {
  root_.add(true);
}
void GLSLDirectiveProcessor::MacroTree::_endif() {
  root_.close();
}
bool GLSLDirectiveProcessor::MacroTree::isDefined() {
  MacroBranch &active = root_.getActive();
  return active.isDefined_;
}

////////////
///////////

GLboolean GLSLDirectiveProcessor::canInclude(const string &s)
{
  if(boost::contains(s, "\n")) {
    return GL_FALSE;
  }
  if(boost::contains(s, "#")) {
    return GL_FALSE;
  }
  if(!boost::contains(s, ".")) {
    return GL_FALSE;
  }
  return GL_TRUE;
}

string GLSLDirectiveProcessor::include(const string &effectKey)
{
  const char *code_c = glswGetShader(effectKey.c_str());
  if(code_c==NULL) { return ""; }
  return string(code_c);
}

GLSLDirectiveProcessor::GLSLDirectiveProcessor(
    istream &in,
    const map<string,string> &functions)
: in_(in),
  continuedLine_(""),
  wasEmpty_(GL_TRUE),
  version_(130),
  functions_(functions)
{
  tree_ = new MacroTree;
  inputs_.push_front(&in);
}
GLSLDirectiveProcessor::~GLSLDirectiveProcessor()
{
  for(list<istream*>::iterator it=inputs_.begin(); it!=inputs_.end(); ++it) {
    if(&in_ != *it) { delete *it; }
  }
  inputs_.clear();
  delete tree_;
}


void GLSLDirectiveProcessor::parseVariables(string &line)
{
  static const char* variablePattern = "\\$\\{[ ]*([^ \\}\\{]+)[ ]*\\}";
  static boost::regex variableRegex(variablePattern);

  set<string> variableNames;
  boost::sregex_iterator regexIt(line.begin(), line.end(), variableRegex);

  if(regexIt==NO_REGEX_MATCH) { return; }

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
    if(tree_->isDefined(name)) {
      const string &value = tree_->define(name);
      boost::replace_all(line, "${"+name+"}", value);
      replacedSomething = GL_TRUE;
    }
  }

  // parse nested vars
  if(replacedSomething==GL_TRUE) parseVariables(line);
}

bool GLSLDirectiveProcessor::getline(string &line)
{
  if(inputs_.empty()) { return false; }

  istream &in = *(inputs_.front());
  if(!std::getline(in, line)) {
    if(&in_ != &in) { delete (&in); }
    inputs_.pop_front();
    return GLSLDirectiveProcessor::getline(line);
  }

  // the line stopped with '\' character
  if(*line.rbegin() == '\\') {
    continuedLine_ += line + "\n";
    return GLSLDirectiveProcessor::getline(line);
  }
  if(!continuedLine_.empty()) {
    line = continuedLine_ + line;
    continuedLine_ = "";
  }

  // evaluate ${..}
  if(tree_->isDefined() && forBranches_.empty()) { parseVariables(line); }

  string statement(line);
  boost::trim(statement);

  GLboolean isEmpty = statement.empty();
  if(isEmpty && wasEmpty_) {
    return GLSLDirectiveProcessor::getline(line);
  }
  wasEmpty_ = isEmpty;

  if(hasPrefix(statement, "#line ")) {
    // for now remove line directives
    return GLSLDirectiveProcessor::getline(line);
  }
  else if(hasPrefix(statement, "#version ")) {
    string versionStr = truncPrefix(statement, "#version ");
    boost::trim(versionStr);
    GLint version = atoi(versionStr.c_str());
    if(version_<version) { version_=version; }
    // remove version directives, must be prepended later
    return GLSLDirectiveProcessor::getline(line);
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

    return GLSLDirectiveProcessor::getline(line);
  }
  else if(hasPrefix(statement, "#endfor")) {
    if(forBranches_.empty()) {
      line = "#warning Closing #endfor without opening #for.";
      return true;
    }
    ForBranch &branch = forBranches_.front();

    stringstream *forLoop = new stringstream;
    stringstream &ss = *forLoop;
    inputs_.push_front(forLoop);

    const string &def = tree_->define(branch.upToValue);
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

    return GLSLDirectiveProcessor::getline(line);
  }
  else if(!forBranches_.empty()) {
    forBranches_.front().lines += line + "\n";
    return GLSLDirectiveProcessor::getline(line);
  }
  else if(hasPrefix(statement, "#include ")) {
    if(!tree_->isDefined()) {
      return GLSLDirectiveProcessor::getline(line);
    }
    string key = truncPrefix(statement, "#include ");
    boost::trim(key);
    string imported;
    map<string,string>::const_iterator needle = functions_.find(key);
    if(needle != functions_.end()) {
      imported = needle->second;
    } else {
      imported = include(key);
    }
    if(imported.empty()) {
      line = "#warning Failed to include " + key + ". Make sure GLSW path is set up.";
      return true;
    } else {
      stringstream *ss = new stringstream(imported);
      inputs_.push_front(ss);
    }
    return GLSLDirectiveProcessor::getline(line);
  }
  else if(hasPrefix(statement, "#define2 ")) {
    string v = truncPrefix(statement, "#define2 ");
    boost::trim(v);
    tree_->_define(v);
  }
  else if(hasPrefix(statement, "#define ")) {
    string v = truncPrefix(statement, "#define ");
    boost::trim(v);
    tree_->_define(v);
    if(tree_->isDefined()) { return true; }
  }
  else if(hasPrefix(statement, "#undef ")) {
    string v = truncPrefix(statement, "#undef ");
    boost::trim(v);
    tree_->_undef(v);
    if(tree_->isDefined()) { return true; }
  }
  else if(hasPrefix(statement, "#ifdef ")) {
    string v = truncPrefix(statement, "#ifdef ");
    boost::trim(v);
    tree_->_ifdef(v);
  }
  else if(hasPrefix(statement, "#ifndef ")) {
    string v = truncPrefix(statement, "#ifndef ");
    boost::trim(v);
    tree_->_ifndef(v);
  }
  else if(hasPrefix(statement, "#if ")) {
    string v = truncPrefix(statement, "#if ");
    boost::trim(v);
    tree_->_ifdef(v);
  }
  else if(hasPrefix(statement, "#elif ")) {
    string v = truncPrefix(statement, "#elif ");
    boost::trim(v);
    tree_->_elif(v);
  }
  else if(hasPrefix(statement, "#else")) {
    tree_->_else();
  }
  else if(hasPrefix(statement, "#endif")) {
    tree_->_endif();
  }
  else if(tree_->isDefined()) { return true; }
  return GLSLDirectiveProcessor::getline(line);
}

GLint GLSLDirectiveProcessor::version() const
{ return version_; }

void GLSLDirectiveProcessor::preProcess(ostream &out)
{
  string line;
  while(getline(line)) { out << line << endl; }
}
