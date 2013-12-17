/*
 * directive-processor.h
 *
 *  Created on: 29.10.2012
 *      Author: daniel
 */

#ifndef __GLSL_DIRECTOVE_PROCESSOR_H_
#define __GLSL_DIRECTOVE_PROCESSOR_H_

#include <GL/glew.h>
#include <regen/gl-types/glsl/glsl-processor.h>

#include <iostream>
using namespace std;

namespace regen {
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
   * Another fancy thing is that we can easily evaluate include
   * directives using GLSW.
   *
   * GLSW inserts line statements but using include and generated code
   * it starts getting complicated to use this. For now the
   * line directives are dropped.
   */
  class DirectiveProcessor : public GLSLProcessor {
  public:
    /**
     * Default-Constructor.
     */
    DirectiveProcessor();

    // override
    bool process(PreProcessorState &state, string &line);
    void clear();

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

      void clear();

      GLboolean isDefined(const string &arg);
      string define(const string &arg);

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

    list< ref_ptr<GLSLProcessor> > inputs_;
    MacroTree tree_;
    string continuedLine_;

    list<ForBranch> forBranches_;
    GLboolean wasEmpty_;
    GLenum lastStage_;

    void parseVariables(string &line);
  };
} // namespace

#endif /* GLSL_DIRECTOVE_PROCESSOR_H_ */
