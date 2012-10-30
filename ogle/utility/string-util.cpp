
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

