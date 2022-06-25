#ifndef __rosemary_project_util_hooks_h_
#define __rosemary_project_util_hooks_h_

#include <vector>
#include "utils.hpp"
namespace rsm {
	/// A hook to be run at an event
	struct GenericHook {
		bool enabled = true;
		static inline std::vector<GenericHook*> __hooks_vec{};

		// Key pressed on physical or virtual keyboard
		virtual void KeyPressed(int scancode) {}
		// Runs whenever a key is held down every frame
		virtual void KeyHeld(int scancode) {}
		// Key released on physical or virtual keyboard
		virtual void KeyReleased(int scancode) {}
		// Touchscreen touched
		virtual void TouchSent(int x, int y) {}
		// Touchscreen touch moved
		virtual void TouchMove(int delta_x, int delta_y, int last_x, int last_y) {}
		// Code to run on first frame
		virtual void Start() { return; }
		// Code to run after Start()
		virtual void PostStart() { return; }
		// Code to run before update cycle is run
		virtual void PreUpdate() { return; }
		// Code to update during update cycle
		virtual void Update() { return; }
		// Code to run after update cycle
		virtual void PostUpdate() { return; }
		// Code to run before render cycle
		virtual void PreRender() { return; }
		// Code to run during render cycle
		virtual void Render() { return; }
		// Code to run before swapping buffers via OpenGL
		virtual void PreSwap() { return; }
		// Code to run after render cycle
		virtual void PostRender() { return; }

		// Util: OpenGL window destroyed (lost focus, or went out of screenspace)
		virtual void WindowUnavailable() { return; }
		// Util: OpenGL window recreated or able to be drawn to
		virtual void WindowAvailable() { return; }


		static void enable_static(GenericHook* __t) {
			GenericHook::__hooks_vec.push_back(__t);
		}
		static void disable_static(GenericHook* __t) {
			alib_remove_any_of(__hooks_vec, __t);
		}
		GenericHook() {
			if (!alib_contains_any_of(__hooks_vec, this)) {
				__hooks_vec.push_back(this);
				this->enabled = false;
			}
		}
		virtual void operator ~() { alib_remove_any_of(__hooks_vec, this); }
	};
	struct HookManager : GenericHook {
		static void RunWindowAvailable() {
			for (int i = 0; i < __hooks_vec.size(); i++) {
				if (__hooks_vec.at(i)->enabled)
				__hooks_vec.at(i)->WindowAvailable();
			}
		}
		static void RunWindowUnavailable() {
			for (int i = 0; i < __hooks_vec.size(); i++) {
				if (__hooks_vec.at(i)->enabled)
				__hooks_vec.at(i)->WindowUnavailable();
			}
		}
		static void RunStart() {
			for (int i = 0; i < __hooks_vec.size(); i++) {
				if (__hooks_vec.at(i)->enabled)
				__hooks_vec.at(i)->Start();
			}
		}
		static void RunPostStart() {
			for (int i = 0; i < __hooks_vec.size(); i++) {
				if (__hooks_vec.at(i)->enabled)
				__hooks_vec.at(i)->PostStart();
			}
		}
		static void RunUpdate() {
			for (int i = 0; i < __hooks_vec.size(); i++) {
				if (__hooks_vec.at(i)->enabled)
				__hooks_vec.at(i)->Update();
			}
		}
		static void RunPreUpdate() {
			for (int i = 0; i < __hooks_vec.size(); i++) {
				if (__hooks_vec.at(i)->enabled)
				__hooks_vec.at(i)->PreUpdate();
			}
		}
		static void RunPostUpdate() {
			for (int i = 0; i < __hooks_vec.size(); i++) {
				if (__hooks_vec.at(i)->enabled)
				__hooks_vec.at(i)->PostUpdate();
			}
		}
		static void RunRender() {
			for (int i = 0; i < __hooks_vec.size(); i++) {
				if (__hooks_vec.at(i)->enabled)
				__hooks_vec.at(i)->Render();
			}
		}
		static void RunPreRender() {
			for (int i = 0; i < __hooks_vec.size(); i++) {
				if (__hooks_vec.at(i)->enabled)
				__hooks_vec.at(i)->PreRender();
			}
		}
		static void RunPostRender() {
			for (int i = 0; i < __hooks_vec.size(); i++) {
				if (__hooks_vec.at(i)->enabled)
				__hooks_vec.at(i)->PostRender();
			}
		}
		static void RunPreSwap() {
			for (int i = 0; i < __hooks_vec.size(); i++) {
				if (__hooks_vec.at(i)->enabled)
				__hooks_vec.at(i)->PreSwap();
			}
		}
		static void RunKeyPressed(int scancode) {
			for (int i = 0; i < __hooks_vec.size(); i++) {
				if (__hooks_vec.at(i)->enabled)
				__hooks_vec.at(i)->KeyPressed(scancode);
			}
		}
		static void RunKeyReleased(int scancode) {
			for (int i = 0; i < __hooks_vec.size(); i++) {
				if (__hooks_vec.at(i)->enabled)
				__hooks_vec.at(i)->KeyReleased(scancode);
			}
		}
		static void RunKeyHeld(int scancode) {
			for (int i = 0; i < __hooks_vec.size(); i++) {
				if (__hooks_vec.at(i)->enabled)
				__hooks_vec.at(i)->KeyHeld(scancode);
			}
		}

	};
	struct RenderHook : private GenericHook {
		virtual void PreRender() {}
		virtual void Render() {}
		virtual void PostRender() {}
		RenderHook() {}
	};
// Enable a hook using this function.
// Note: all it does is create an instance of the hook with _runner appended to the back of the class name
#define RSM_HOOK_ENABLE(hook_cls) hook_cls* hook_cls##_runner = new hook_cls();
#define RSM_HOOK_DISABLE(hook_cls) delete hook_cls##_runner;
};


#endif