/*
 * comment-processor.cpp
 *
 *  Created on: 19.05.2013
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#define NO_REGEX_MATCH boost::sregex_iterator()

#include <regen/utility/string-util.h>

#include "comment-processor.h"
using namespace regen;

CommentProcessor::CommentProcessor()
: commentActive_(false)
{}

void CommentProcessor::clear()
{
  commentActive_ = false;
}

bool CommentProcessor::getline(PreProcessorState &state, string &line)
{
  if(!getlineParent(state, line)) return false;

  static const char* pattern = "(\\/\\/|\\/\\*|\\*\\/|--)";
  static boost::regex regex(pattern);

  boost::sregex_iterator it(line.begin(), line.end(), regex), rend, last;
  if(it==rend) {
    if(commentActive_) {
      // skip comment lines
      return getline(state,line);
    }
    else {
      // no comment active
      return true;
    }
  }
  stringstream lineStream;

  for ( ; it!=rend; ++it)
  {
    const string &comment = (*it)[1];

    if(commentActive_) {
      if(comment == "*/") {
        // multi line comment end
        commentActive_ = false;
      }
    }
    else {
      lineStream << it->prefix();
      if(comment == "/*") {
        // multi line comment start
        commentActive_ = true;
      }
      else if(comment == "//" || comment == "--") {
        // single line comment start
        line = lineStream.str();
        if(line.empty()) {
          return getline(state,line);
        }
        else {
          return true;
        }
      }
    }
    last = it;
  }
  if(!commentActive_) {
    lineStream << last->suffix();
  }
  line = lineStream.str();
  if(line.empty()) {
    return getline(state,line);
  }
  else {
    return true;
  }
}
