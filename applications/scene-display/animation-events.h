/*
 * animation.h
 *
 *  Created on: Oct 19, 2014
 *      Author: daniel
 */

#ifndef SCENE_DISPLAY_ANIMATION_H_
#define SCENE_DISPLAY_ANIMATION_H_

#include <list>
#include <algorithm>

class RandomAnimationRangeUpdater : public EventHandler
{
public:
  RandomAnimationRangeUpdater(
      const ref_ptr<NodeAnimation> &anim,
      const vector<AnimRange> &animRanges)
  : EventHandler(),
    anim_(anim),
    animRanges_(animRanges) {}

  void call(EventObject *ev, EventData *data)
  {
    NodeAnimation *anim = (NodeAnimation*)ev;
    int index = rand() % animRanges_.size();
    anim->setAnimationIndexActive(0, animRanges_[index].range);
  }

protected:
  ref_ptr<NodeAnimation> anim_;
  vector<AnimRange> animRanges_;
};

struct KeyAnimationMapping {
  string key;
  string press;
  string idle;
  GLboolean toggle;
  GLboolean backwards;
  GLboolean interrupt;
  GLboolean releaseInterrupt;
};

class KeyAnimationRangeUpdater : public EventHandler
{
public:
  KeyAnimationRangeUpdater(
      const ref_ptr<NodeAnimation> &anim,
      const vector<AnimRange> &animRanges,
      const map<string,KeyAnimationMapping> &mappings,
      const string &idleAnimation)
  : EventHandler(),
    anim_(anim),
    mappings_(mappings)
  {
    for(vector<AnimRange>::const_iterator it=animRanges.begin(); it!=animRanges.end(); ++it) {
      animRanges_[it->name] = *it;
    }
    active_ = "";
    idleAnimation_ = idleAnimation;
    startIdleAnimation();
  }

  void startIdleAnimation()
  {
    active_ = "";
    if(toggles_.empty()) {
      if(!idleAnimation_.empty()) {
        anim_->setAnimationIndexActive(0, animRanges_[idleAnimation_].range);
      }
    }
    else {
      KeyAnimationMapping &m0 = mappings_[*toggles_.begin()];
      if(!m0.idle.empty()) {
        anim_->setAnimationIndexActive(0, animRanges_[m0.idle].range);
      }
    }
  }

  void startAnimation()
  {
    // Turn the toggle 'off' before starting another animation.
    if(toggles_.empty()) {
      active_ = pressed_.front();
    }
    else {
      active_ = *toggles_.begin();
    }

    KeyAnimationMapping &m = mappings_[active_];
    Vec2d animRange = animRanges_[m.press].range;
    // For toggle animations, the animation range is processed backwards.
    if(m.toggle) {
      // Turn off toggle
      if(toggles_.count(active_)>0) {
        toggles_.erase(active_);
        animRange = Vec2d(animRange.y, animRange.x);
      }
      // Turn on toggle
      else {
        toggles_.insert(active_);
      }
    }
    else if(m.backwards) {
      animRange = Vec2d(animRange.y, animRange.x);
    }
    anim_->setAnimationIndexActive(0, animRange);
  }

  void nextAnimation()
  {
    // If there is a key pressed start the animation...
    if(!pressed_.empty()) {
      // Start animation associated to last pressed key
      startAnimation();
    }
    else {
      startIdleAnimation();
    }
  }

  void call(EventObject *evObject, EventData *data)
  {
    if(data->eventID == Animation::ANIMATION_STOPPED) {
      nextAnimation();
    }
    else if(data->eventID == Application::KEY_EVENT) {
      Application::KeyEvent *ev = (Application::KeyEvent*)data;
      // TODO: mapping of non-asci keys
      string key = REGEN_STRING((char)ev->key);

      if(mappings_.count(key)==0) return;
      KeyAnimationMapping &m = mappings_[key];

      if(ev->isUp) {
        // Remember that the key was released
        pressed_.remove(key);

        // Interrupt current animation
        if(m.releaseInterrupt && active_ == key) {
          nextAnimation();
        }
      }
      else {
        // Remember that the key was pressed
        pressed_.remove(key);
        pressed_.push_front(key);

        if(active_.empty()) {
          // Start if no animation is active
          startAnimation();
        }
        else { // Interrupt current animation if allowed
          KeyAnimationMapping &a = mappings_[active_];
          if(a.interrupt) {
            startAnimation();
          }
        }
        // Avoid that the toggle turns off automatically
        if(m.toggle) {
          pressed_.remove(key);
        }
      }

    }
  }


protected:
  ref_ptr<NodeAnimation> anim_;
  map<string,KeyAnimationMapping> mappings_;
  map<string,AnimRange> animRanges_;
  list<string> pressed_;
  set<string> toggles_;
  string active_;
  string idleAnimation_;
};


#endif /* SCENE_DISPLAY_ANIMATION_H_ */
