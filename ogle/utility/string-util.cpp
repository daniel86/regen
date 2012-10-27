
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <map>

#include "string-util.h"

bool hasPrefix(const string &s, const string &prefix)
{
  return boost::starts_with(s, prefix);
}
string truncPrefix(const string &s, const string &prefix)
{
  if(hasPrefix(s,prefix)) {
    return s.substr(prefix.size());
  } else {
    return s;
  }
}

unsigned int getNumLines(const string &s)
{
  unsigned int numLines = 1;
  size_t pos = 0;
  while((pos = s.find_first_of('\n',pos+1)) != string::npos) {
    numLines += 1;
  }
  return numLines;
}
unsigned int getFirstLine(const string &s)
{
  static const int l = string("#line ").length();
  if(boost::starts_with(s, "#line ")) {
    char *pEnd;
    size_t pos = s.find_first_of('\n');
    if(pos==string::npos) {
      return 1;
    } else {
      return strtoul(s.substr(l,pos-l).c_str(), &pEnd, 0);
    }
  } else {
    return 1;
  }
}

static bool isVarCharacter(char c) {
  return isalnum(c) || c=='_';
}
void replaceVariable(
    const string &fromName,
    const string &toName,
    string *code)
{
  size_t inSize = fromName.size();
  size_t start = 0;
  size_t pos;
  list<size_t> replaced;
  string &codeStr = *code;

  while( (pos = codeStr.find(fromName,start)) != string::npos )
  {
    start = pos+1;
    size_t end = pos + inSize;
    // check if character before and after are not part of the var
    if(pos>0 && isVarCharacter(codeStr[pos-1])) continue;
    if(end<codeStr.size() && isVarCharacter(codeStr[end])) continue;
    // remember replacement
    replaced.push_front( pos );
  }

  // adding last replacement first, this way indices stay valid
  for(list<size_t>::iterator it=replaced.begin(); it!=replaced.end(); ++it)
  {
    codeStr.replace( *it, inSize, toName );
  }
}

bool isInteger(const string & s)
{
  try  {
    boost::lexical_cast<int>(s);
    return true;
  }
  catch(...) { return false; }
}
bool isFloat(const string & s)
{
  try  {
    boost::lexical_cast<double>(s);
    return true;
  }
  catch(...) { return false; }
}
bool isNumber(const string & s)
{
  return isInteger(s) || isFloat(s);
}

//////////////////////////////////
//////////////////////////////////
//////////////////////////////////

/**
 * Models the nested nature of #ifdef/#if/#else/#endif statements.
 */
struct MacroBranch {
  bool isDefined_;
  bool isAnyChildDefined_;
  list<MacroBranch> childs_;
  MacroBranch *parent_;

  MacroBranch& getActive() {
    return childs_.empty() ? *this : childs_.back().getActive();
  }
  void open(bool isDefined) {
    // open a new #if / #ifdef branch
    MacroBranch &active = getActive();
    active.childs_.push_back( MacroBranch(
        isDefined&&active.isDefined_, &active) );
    if(isDefined) {
      // remember if the argument is defined (all #else cases can be skipped then)
      active.isAnyChildDefined_ = true;
    }
  }
  void add(bool isDefined) {
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
  void close() {
    MacroBranch &active = getActive();
    MacroBranch *parent = active.parent_;
    if(parent==NULL) { return; }
    parent->childs_.clear();
    parent->isAnyChildDefined_ = false;
  }
  int depth() {
    return childs_.empty() ? 1 : 1+childs_.back().depth();
  }

  MacroBranch()
  : isDefined_(true), parent_(NULL), isAnyChildDefined_(false) {}
  MacroBranch(bool isDefined, MacroBranch *parent)
  : isDefined_(isDefined), parent_(parent), isAnyChildDefined_(false) {}
  MacroBranch(const MacroBranch &other)
  : isDefined_(other.isDefined_),
    parent_(other.parent_),
    isAnyChildDefined_(other.isAnyChildDefined_) {}
};

/**
 * Keeps track of definitions, evaluates expressions
 * and uses MacroBranch to keep track of the context.
 */
struct MacroTree {
  map<string,string> defines_;
  MacroBranch root_;

  string& define(string &arg) {
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
  string evaluateInner(list<string> &inner) {
    if(inner.empty()) {
      return "0";
    }
    else if(inner.size()==1) {
      // single inner argument
      string &arg = inner.front();
      if(isNumber(arg)) {
        return "1";
      } else {
        map<string,string>::iterator it = defines_.find(arg);
        if(it==defines_.end() || it->second=="0") {
          return "0";
        } else {
          return "1";
        }
      }
    }
    else if(inner.size()==3) {
      // two arguments and an operator
      string &arg0 = define(inner.front());
      string &op = *(++inner.begin());
      string &arg1 = define(inner.back());

      if(op == "==")            return (arg0==arg1 ? "1" : "0");
      else if(op == "!=")       return (arg0!=arg1 ? "1" : "0");

      // numeric operators left
      if(!isNumber(arg0) || !isNumber(arg1)) { return "0"; }
      float val0=0.0f, val1=0.0f;
      stringstream(arg0) >> val0;
      stringstream(arg1) >> val1;

      if(op == "<=")            return (val0<=val1 ? "1" : "0");
      else if(op == ">=")       return (val0>=val1 ? "1" : "0");
      else if(op == ">")        return (val0>val1 ? "1" : "0");
      else if(op == "<")        return (val0<val1 ? "1" : "0");
      else                      return "0";
    }
    else {
      return "0";
    }
  }
  bool evaluate(const string &expression) {
    static const string operators[] = {
        "&&", "||", "==", "!=", "<=", ">=", "<", ">"
    };
    static const int numOperators = sizeof(operators)/sizeof(string);

    string exp(expression);
    // surround known operators with newlines so that
    // we can easily read them with getline
    for(int i=0; i<numOperators; ++i) {
      boost::algorithm::replace_all(exp,
          operators[i], FORMAT_STRING("\n"<<operators[i]<<"\n"));
    }
    stringstream in(exp);

    list<string> inner;
    list<string> outer;

    // evaluate inner, collect outer sequence
    string line;
    while( getline(in, line) ) {
      boost::trim(line);
      if(line=="||" || line=="&&") {
        outer.push_back(evaluateInner(inner));
        outer.push_back(line);
        inner.clear();
      }
      else {
        inner.push_back(line);
      }
    }
    if(!inner.empty()) {
      outer.push_back(evaluateInner(inner));
    }

    // evaluate outer
    bool isDefined = !outer.empty();
    bool isAndOperation = true;
    for(list<string>::iterator it=outer.begin(); it!=outer.end(); ++it)
    {
      line = *it;
      if(line=="||") {
        isAndOperation = false;
      } else if(line=="&&") {
        isAndOperation = true;
      } else {
        if(isAndOperation) {
          isDefined = isDefined && (line=="1");
        } else {
          isDefined = isDefined || (line=="1");
        }
      }
    }

    return isDefined;
  }

  void _define(const string &s) {
    if(!isDefined()) { return; }

    size_t pos = s.find_first_of(" ");
    if(pos == string::npos) {
      defines_[ s ] = "1";
    } else {
      defines_[ s.substr(0,pos) ] = s.substr(pos+1);
    }
  }
  void _undef(const string &s) {
    defines_.erase(s);
  }
  void _ifdef(const string &s) {
    root_.open(evaluate(s));
  }
  void _ifndef(const string &s) {
    root_.open(!evaluate(s));
  }
  void _if(const string &s) {
    root_.open(evaluate(s));
  }
  void _elif(const string &s) {
    root_.add(evaluate(s));
  }
  void _else() {
    root_.add(true);
  }
  void _endif() {
    root_.close();
  }
  bool isDefined() {
    MacroBranch &active = root_.getActive();
    return active.isDefined_;
  }
};

string evaluateMacros(const string &code)
{
  istringstream in(code);
  ostringstream out;

  // helps keeping track of defined code
  MacroTree tree;

  string line;
  // read the code line by line and call the MacroTree when
  // #define / #undef / #if / #ifdef / #else / #endif occurs.
  // Only add lines where tree.isDefined() holds true.
  while (std::getline(in, line)) {
    string statement(line);
    boost::trim(statement);

    if(hasPrefix(statement, "#line ")) {
      // for now remove line directives
      continue;
    }
    else if(hasPrefix(statement, "#define ")) {
      tree._define(truncPrefix(statement, "#define "));
      if(tree.isDefined()) {
        out << line << endl;
      }
    }
    else if(hasPrefix(statement, "#undef ")) {
      tree._undef(truncPrefix(statement, "#undef "));
      if(tree.isDefined()) {
        out << line << endl;
      }
    }
    else if(hasPrefix(statement, "#ifdef ")) {
      tree._ifdef(truncPrefix(statement, "#ifdef "));
    }
    else if(hasPrefix(statement, "#ifndef ")) {
      tree._ifndef(truncPrefix(statement, "#ifndef "));
    }
    else if(hasPrefix(statement, "#if ")) {
      tree._ifdef(truncPrefix(statement, "#if "));
    }
    else if(hasPrefix(statement, "#elif ")) {
      tree._elif(truncPrefix(statement, "#elif "));
    }
    else if(hasPrefix(statement, "#else")) {
      tree._else();
    }
    else if(hasPrefix(statement, "#endif")) {
      tree._endif();
    }
    else if(tree.isDefined()) {
      out << line << endl;
    }
    //cout << (tree.isDefined() ? "+ " : "- ") << line <<
    //    " (" << tree.root_.depth() << ")" << endl;
  }

  return out.str();
}
