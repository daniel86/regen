/*
 * value-generator.h
 *
 *  Created on: Nov 4, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_VALUE_GENERATOR_H_
#define REGEN_SCENE_VALUE_GENERATOR_H_

namespace regen {
	namespace scene {
		/**
		 * Value distribution function.
		 * Useful to generate attribute or instanced attribute
		 * values by simple expressions.
		 */
		template<class T>
		class ValueGenerator {
		public:
			/**
			 * Default constructor.
			 * @param n The input node.
			 * @param numValues Number of values to generate.
			 * @param defaultValue The default value.
			 */
			ValueGenerator(SceneInputNode *n,
						   const GLuint numValues,
						   const T &defaultValue = T(0))
					: n_(n),
					  numValues_(numValues),
					  counter_(Vec4ui(0u)),
					  value_(defaultValue) {
				mode_ = n_->getValue<std::string>("mode", "constant");
			}

			/**
			 * Compute next distribution value.
			 * @return The generated value.
			 */
			T next() {
				if (mode_ == "row") return nextRow();
				else if (mode_ == "circle") return nextCircle();
				else if (mode_ == "fade") return nextFade();
				else if (mode_ == "random") return nextRandom();
				else if (mode_ == "constant") return value_;
				REGEN_WARN("Unknown distribute mode '" << mode_ << "'.");
				return value_;
			}

		protected:
			SceneInputNode *n_;
			GLuint numValues_;
			Vec4ui counter_;
			T value_;
			std::string mode_;

			T nextRow() {
				const T stepX = n_->getValue<T>("x-step", T(1));
				const T stepY = n_->getValue<T>("y-step", T(1));
				const T stepZ = n_->getValue<T>("z-step", T(1));
				const auto varianceX = n_->getValue<float>("x-variance", 0.0f);
				const auto varianceY = n_->getValue<float>("y-variance", 0.0f);
				const auto varianceZ = n_->getValue<float>("z-variance", 0.0f);
				value_ =
					stepX * (counter_.x + random()*varianceX) +
					stepY * (counter_.y + random()*varianceY) +
					stepZ * (counter_.z + random()*varianceZ);

				auto xCount = n_->getValue<GLuint>("x-count", numValues_);
				auto yCount = n_->getValue<GLuint>("y-count", 1);
				counter_.x += 1;
				if (counter_.x >= xCount) {
					counter_.x = 0;
					counter_.y += 1;
					if (counter_.y >= yCount) {
						counter_.y = 0;
						counter_.z += 1;
					}
				}
				return value_;
			}

			T nextCircle() {
				const T dirX = n_->getValue<T>("dir-x", T(1));
				const T dirY = n_->getValue<T>("dir-z", T(1));
				const auto radius = n_->getValue<float>("radius", 1.0f);
				const auto variance = n_->getValue<float>("variance", 0.0f);
				// evenly distribute points on a circle
				double position = counter_.x / (double) numValues_;
				// scale to 0..2PI
				position = position * 2 * M_PI;
				const double x = radius * cos(position);
				const double y = radius * sin(position);
				value_ =
					dirX * (x + random()*variance) +
					dirY * (y + random()*variance);
				counter_.x += 1;
				return value_;
			}

			T nextRandom() {
				const T min = n_->getValue<T>("min", T(0));
				const T max = n_->getValue<T>("max", T(1));
				value_ = min + (max - min) * random();
				counter_.x += 1;
				return value_;
			}

			T nextFade() {
				const T start = n_->getValue<T>("start", T(1.0f));
				const T stop = n_->getValue<T>("stop", T(2.0f));
				const GLfloat progress = ((GLfloat) counter_.x) / ((GLfloat) numValues_);
				value_ = start + (stop - start) * progress;
				counter_.x += 1;
				return value_;
			}

			auto random() {
				return (rand() % 10000) / 10000.0;
			}
		};
	}
}

#endif /* REGEN_SCENE_VALUE_GENERATOR_H_ */
