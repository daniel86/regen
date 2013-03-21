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

namespace ogle {
/**
 * \brief Offline Processing of Shader PreProcessor directives.
 *
 * All undefined code is dropped, ifdef/if/else/endif
 * is also dropped. define's are left for Online evaluating
 * what this processor left.
 * define's are not used to replace the actual code, thats
 * done by GL after uploading the code for now.
 * The directives would be evaluated by the GL anyway
 * but doing this offline allows us some more fancy
 * processing then just evaluating the directives.
 * Also it is nice because less code is uploaded to the GPU.
 *
 * Another fancy thing is that we can easily evaluate #include
 * directives using GLSW.
 *
 * GLSW inserts line statements but using include and generated code
 * it starts getting complicated to use this. For now the
 * line directives are dropped.
 */
class GLSLDirectiveProcessor {
public:
  /**
   * @param effectKey the shader key.
   * @return GL_TRUE if the key is valid.
   */
  static GLboolean canInclude(const string &effectKey);

  /**
   * GLSL style path to include.
   * For example "A.B" would load section "B"
   * from file named "A.glsl".
   */
  static string include(const string &effectKey);

  /**
   * @param in input stream.
   * @param functions user defined GLSL functions.
   */
  GLSLDirectiveProcessor(istream &in,
      const map<string,string> &functions);
  ~GLSLDirectiveProcessor();

  /**
   * @return the version as collected in the source code.
   */
  GLint version() const;

  /**
   * Read a single line from input stream.
   */
  bool getline(string &line);

  /**
   * Read input stream until EOF reached.
   */
  void preProcess(ostream &out);

protected:
  /**
   * Models the nested nature of #ifdef/#if/#else/#endif statements.
   */
  struct MacroBranch {
    bool isDefined_;
    bool isAnyChildDefined_;
    list<MacroBranch> childs_;
    MacroBranch *parent_;

    MacroBranch& getActive();
    void open(bool isDefined);
    void add(bool isDefined);
    void close();
    int depth();

    MacroBranch();
    MacroBranch(bool isDefined, MacroBranch *parent);
    MacroBranch(const MacroBranch &other);
  };
  /**
   * Keeps track of definitions, evaluates expressions
   * and uses MacroBranch to keep track of the context.
   */
  struct MacroTree {
    map<string,string> defines_;
    MacroBranch root_;

    GLboolean isDefined(const string &arg);
    const string& define(const string &arg);

    bool evaluateInner(const string &expression);
    bool evaluate(const string &expression);

    void _define(const string &s);
    void _undef(const string &s);
    void _ifdef(const string &s);
    void _ifndef(const string &s);
    void _if(const string &s);
    void _elif(const string &s);
    void _else();
    void _endif();
    bool isDefined();
  };
  struct ForBranch {
    string variableName;
    string upToValue;
    string lines;
  };

  list<istream*> inputs_;
  istream &in_;
  MacroTree *tree_;
  string continuedLine_;

  list<ForBranch> forBranches_;
  GLboolean wasEmpty_;
  GLint version_;

  const map<string,string> &functions_;

  void parseVariables(string &line);
};

} // end ogle namespace

#endif /* GLSL_DIRECTOVE_PROCESSOR_H_ */
