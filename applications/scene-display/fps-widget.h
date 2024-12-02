/*
 * fps-widget.h
 *
 *  Created on: Oct 19, 2014
 *      Author: daniel
 */

#ifndef SCENE_DISPLAY_FPS_WIDGET_H_
#define SCENE_DISPLAY_FPS_WIDGET_H_

class UpdateFPS : public Animation {
public:
	UpdateFPS(const ref_ptr<TextureMappedText> &widget)
			: Animation(GL_TRUE, GL_FALSE),
			  widget_(widget),
			  frameCounter_(0),
			  fps_(0),
			  sumDtMiliseconds_(0.0f) {
		setAnimationName("fps-widget");
	}

	void glAnimate(RenderState *rs, GLdouble dt) {
		frameCounter_ += 1;
		sumDtMiliseconds_ += dt;

		if (sumDtMiliseconds_ > 1000.0) {
			fps_ = (GLint) (frameCounter_ * 1000.0 / sumDtMiliseconds_);
			sumDtMiliseconds_ = 0;
			frameCounter_ = 0;

			wstringstream ss;
			ss << fps_ << " FPS";
			widget_->set_value(ss.str());
		}
	}

private:
	ref_ptr<TextureMappedText> widget_;
	GLuint frameCounter_;
	GLint fps_;
	GLdouble sumDtMiliseconds_;
};

#endif /* SCENE_DISPLAY_FPS_WIDGET_H_ */
