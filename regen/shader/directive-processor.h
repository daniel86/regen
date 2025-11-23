/*
 * directive-processor.h
 *
 *  Created on: 29.10.2012
 *      Author: daniel
 */

#ifndef __GLSL_DIRECTOVE_PROCESSOR_H_
#define __GLSL_DIRECTOVE_PROCESSOR_H_

#include <GL/glew.h>
#include "glsl-processor.h"

#include <iostream>

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
    bool process(PreProcessorState &state, std::string &line);
    void clear();

  protected:
    /**
     * Models the nested nature of #ifdef/#if/#else/#endif statements.
     */
    struct MacroBranch {
      bool isDefined_;
      bool isAnyChildDefined_;
      std::list<MacroBranch> childs_;
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
      std::map<std::string,std::string> defines_;
      MacroBranch root_;

      void clear();

      bool isDefined(const std::string &arg) const;
      std::string define(const std::string &arg);

      bool evaluateInner(const std::string &expression);
      bool evaluate(const std::string &expression);

      void _define(const std::string &s);
      void _undef(const std::string &s);
      void _ifdef(const std::string &s);
      void _ifndef(const std::string &s);
      void _if(const std::string &s);
      void _elif(const std::string &s);
      void _else();
      void _endif();
      bool isDefined();
    };
    struct ForBranch {
    	std::string variableName;
    	std::string upToValue;
    	std::string lines;
    	void clear() {
			variableName = "";
			upToValue = "";
			lines = "";
		}
    };

    std::list< ref_ptr<GLSLProcessor> > inputs_;
    MacroTree tree_;
    std::string continuedLine_;

    ForBranch forBranch_;
    uint32_t forLoopCounter_ = 0;

    bool wasEmpty_;
    GLenum lastStage_;

    void parseVariables(std::string &line);
  };
} // namespace

#endif /* GLSL_DIRECTOVE_PROCESSOR_H_ */
