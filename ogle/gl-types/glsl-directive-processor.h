/*
 * glsl-directive-processor.h
 *
 *  Created on: 29.10.2012
 *      Author: daniel
 */

#ifndef GLSL_DIRECTOVE_PROCESSOR_H_
#define GLSL_DIRECTOVE_PROCESSOR_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <iostream>
using namespace std;

struct MacroTree;

/**
 * Offline Processing of Shader PreProcessor directives.
 * All undefined code is dropped, #ifdef/#if/#else/#endif
 * is also dropped. #define's are left for Online evaluating
 * what this processor left.
 * #define's are not used to replace the actual code, thats
 * done by GL after uploading the code for now.
 * The directives would be evaluated by the GL anyway
 * but doing this offline allows us some more fancy
 * processing then just evaluating the directives.
 * Also it is nice because less code is uploaded to the GPU.
 *
 * Another fancy thing is that we can easily evaluate #include
 * directives using GLSW.
 *
 * GLSL inserts #line's but using include and generated code
 * it starts getting complicated to use this. For now the
 * #line directives are dropped.
 */
class GLSLDirectiveProcessor {
public:
  static GLboolean canInclude(const string &effectKey);
  /**
   * GLSL style path to include.
   * For example "A.B" would load section "B"
   * from file named "A.glsl".
   */
  static string include(const string &effectKey);

  GLSLDirectiveProcessor(istream &in,
      const map<string,string> &functions);
  ~GLSLDirectiveProcessor();

  /**
   * Read a single line from input stream.
   */
  bool getline(string &line);

  /**
   * Read input stream until EOF reached.
   */
  void preProcess(ostream &out);

protected:
  list<istream*> inputs_;
  istream &in_;
  MacroTree *tree_;
  string continuedLine_;

  string forArg_;
  string forLines_;

  GLboolean wasEmpty_;

  const map<string,string> &functions_;

  void parseVariables(string &line);
};

#endif /* GLSL_DIRECTOVE_PROCESSOR_H_ */
