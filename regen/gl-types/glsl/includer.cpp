/*
 * includer.cpp
 *
 *  Created on: 18.05.2013
 *      Author: daniel
 */

#include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/regex.hpp>

#include <regen/utility/string-util.h>

#include "includer.h"
using namespace regen;
using namespace std;

Includer& Includer::get()
{
  static Includer x;
  return x;
}

const string& Includer::errorMessage() const
{ return errorMessage_; }

bool Includer::addIncludePath(const string &path)
{
  boost::filesystem::path p(path);
  if(boost::filesystem::exists(p)) {
    includePaths_.push_back(p);
    return true;
  } else {
    return false;
  }
}

bool Includer::parseInput(
    const string &key,
    boost::filesystem::path &filePathRet,
    string &fileKeyRet,
    string &effectKeyRet)
{
  list<boost::filesystem::path>::iterator includePath;
  list<string>::iterator token;
  list<string> tokens;
  boost::filesystem::path filePath;

  boost::split(tokens, key, boost::is_any_of("."));

  for(includePath =includePaths_.begin();
      includePath!=includePaths_.end(); ++includePath)
  {
    filePath = *includePath;
    fileKeyRet = "";

    for(token=tokens.begin(); token!=tokens.end(); ++token)
    {
      const string &v = *token;
      filePathRet = filePath / (v+".glsl");

      if(fileKeyRet.empty()) {
        fileKeyRet = v;
      }
      else {
        fileKeyRet = REGEN_STRING(fileKeyRet << "." << v);
      }

      if(boost::filesystem::exists(filePathRet))
      {
        // shader file found
        effectKeyRet = "";
        for(++token; token!=tokens.end(); ++token)
        {
          if(effectKeyRet.empty()) {
            effectKeyRet = (*token);
          }
          else {
            effectKeyRet = REGEN_STRING(effectKeyRet << "." << (*token));
          }
        }
        return true;
      }
      else if(boost::filesystem::exists(filePath / v))
      {
        // token is a valid sub directory, continue with next token
        filePath /= v;
      }
      else
      {
        break;
      }
    }
  }
  return false;
}

bool Includer::isKeyValid(const string &key)
{
  if(boost::contains(key, "\n") ||
     boost::contains(key, "#") ||
     !boost::contains(key, "."))
  { return false; }

  boost::filesystem::path path;
  string fileKey, effectKey;
  if(!parseInput(key, path, fileKey, effectKey)) {
    return false;
  }
  else {
    return true;
  }
}

const string& Includer::include(const string &key)
{
  static const string &emptyString="";
  // matches begin of sections
  static const char* pattern = "^\\s*-- ([a-zA-Z][a-zA-Z0-9_\\-\\.]*)\\s*$";
  static boost::regex regex(pattern);
  errorMessage_.clear();

  // check if section loaded
  map<string,string>::iterator sectionIt = sections_.find(key);
  if(sectionIt != sections_.end())
  { return sectionIt->second; }

  // find file path and section key within shader file
  boost::filesystem::path path;
  string fileKey, effectKey;
  if(!parseInput(key, path, fileKey, effectKey))
  {
    errorMessage_ = REGEN_STRING("Unable to resolve include key '" << key << ".");
    return emptyString;
  }

  if(loadedFiles_.count(path.string())>0)
  {
    errorMessage_ = REGEN_STRING("No section '" << effectKey <<
        "' defined in " << path.string() << ".");
    return emptyString;
  }

  { // read in file content
    string fileContent;
    ifstream file(path.string().c_str());
    fileContent.assign(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>());

    string activeSection = "";
    boost::sregex_iterator it(fileContent.begin(), fileContent.end(), regex);
    boost::sregex_iterator rend, last;
    for ( ; it!=rend; ++it)
    {
      if(!activeSection.empty()) {
        sections_.insert(make_pair(
            REGEN_STRING(fileKey << "." << activeSection),
            it->prefix()));
      }
      activeSection = (*it)[1];
      last = it;
    }
    if(!activeSection.empty()) {
      sections_.insert(make_pair(
          REGEN_STRING(fileKey << "." << activeSection),
          last->suffix()));
    }
    loadedFiles_.insert(path.string());
  }

  return include(key);
}

