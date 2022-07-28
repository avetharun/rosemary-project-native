#ifndef __rosemary_project_util_hooks_h_
#define __rosemary_project_util_hooks_h_

#include <vector>
#include <stack>
#include "utils.hpp"
namespace rsm {
	/// A hook to be run at an event
	struct GenericHook {
		bool ShowStatusBar = true;
		bool enabled = true;
		static inline std::vector<GenericHook*> __globally_enabled_hooks{};
		static inline std::stack<GenericHook*> __hook_stack;
		static void push(GenericHook* hook) {
			__hook_stack.push(hook);
		}
		static void pop() {
			if (__hook_stack.size() == 0) { return; }
			__hook_stack.pop();
		}
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
		// Code to run before rendering ImGui to OpenGL
		virtual void PreUI() { return; }
		// Code to run after render cycle
		virtual void PostRender() { return; }

		// Code to run after updating, regardless of if this hook is enabled or not.
		virtual void Tick() { return; }

		// Util: OpenGL & ImGui initialized, set up fonts/settings
		virtual void InitPost() { return; }
		// Util: OpenGL initialized. Do things to set up VBOs, VAOs, etc.
		virtual void InitGL() {}


		static void enable_static(GenericHook* __t) {
			GenericHook::__globally_enabled_hooks.push_back(__t);
		}
		static void disable_static(GenericHook* __t) {
			alib_remove_any_of(__globally_enabled_hooks, __t);
		}
		GenericHook() {
			if (!alib_contains_any_of(__globally_enabled_hooks, this)) {
				__globally_enabled_hooks.push_back(this);
				this->enabled = false;
			}
		}
		virtual void operator ~() { alib_remove_any_of(__globally_enabled_hooks, this); }
	};
	struct HookManager {
		// Runs for all hooks
		static void RunInitPost() {
			for (int i = 0; i < GenericHook::__globally_enabled_hooks.size(); i++) {
				GenericHook::__globally_enabled_hooks.at(i)->InitPost();
			}
		}
		// Runs for all hooks
		static void RunInitGL() {
			for (int i = 0; i < GenericHook::__globally_enabled_hooks.size(); i++) {
				GenericHook::__globally_enabled_hooks.at(i)->InitGL();
			}
		}
		// Runs for all hooks
		static void RunStart() {
			for (int i = 0; i < GenericHook::__globally_enabled_hooks.size(); i++) {
				GenericHook::__globally_enabled_hooks.at(i)->Start();
			}
		}
		// Runs for only the current pushed() hook
		static void RunPostStart() {
			if (GenericHook::__hook_stack.size()) {
				GenericHook::__hook_stack.top()->PostStart();
			}
		}
		// Runs for only the current pushed() hook
		static void RunUpdate() {
			if (GenericHook::__hook_stack.size()) {
				GenericHook::__hook_stack.top()->Update();
			}
		}
		// Runs for all hooks, regardless if enabled or not.
		static void RunTick() {
			for (int i = 0; i < GenericHook::__globally_enabled_hooks.size(); i++) {
				GenericHook::__globally_enabled_hooks.at(i)->Tick();
			}
		}
		// Runs for only the current pushed() hook
		static void RunPreUpdate() {
			if (GenericHook::__hook_stack.size()) {
				GenericHook::__hook_stack.top()->PreUpdate();
			}
		}
		// Runs for only the current pushed() hook
		static void RunPostUpdate() {
			if (GenericHook::__hook_stack.size()) {
				GenericHook::__hook_stack.top()->PostUpdate();
			}
		}
		// Runs for only the current pushed() hook
		static void RunRender() {
			if (GenericHook::__hook_stack.size()) {
				GenericHook::__hook_stack.top()->Render();
			}
		}
		// Runs for only the current pushed() hook
		static void RunPreRender() {
			if (GenericHook::__hook_stack.size()) {
				GenericHook::__hook_stack.top()->PreRender();
			}
		}
		// Runs for only the current pushed() hook
		static void RunPostRender() {
			if (GenericHook::__hook_stack.size()) {
				GenericHook::__hook_stack.top()->PostRender();
			}
		}
		// Runs for only the current pushed() hook
		static void RunPreSwap() {
			if (GenericHook::__hook_stack.size()) {
				GenericHook::__hook_stack.top()->PreSwap();
			}
		}
		// Runs for only the current pushed() hook
		static void RunPreUI() {
			if (GenericHook::__hook_stack.size()) {
				GenericHook::__hook_stack.top()->PreUI();
			}
		}
		// Runs for only the current pushed() hook
		static void RunKeyPressed(int scancode) {
			if (GenericHook::__hook_stack.size()) {
				GenericHook::__hook_stack.top()->KeyPressed(scancode);
			}
		}
		// Runs for only the current pushed() hook
		static void RunKeyReleased(int scancode) {
			if (GenericHook::__hook_stack.size()) {
				GenericHook::__hook_stack.top()->KeyReleased(scancode);
			}
		}
		// Runs for only the current pushed() hook
		static void RunKeyHeld(int scancode) {
			if (GenericHook::__hook_stack.size()) {
				GenericHook::__hook_stack.top()->KeyHeld(scancode);
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