#ifndef __rosemary_project_ui_mgr_h_
#define __rosemary_project_ui_mgr_h_
#include "imgui/imgui_uielement.h"
#include "rsm_hooks.h"
#include <set>
#include "imgui/imgui_format.h"

struct GlobalState {
	static inline bool ShowSettingsWindow = false;
	static inline bool ShowMainWindow = true;
	static inline bool ShowDebugWindow = false;
	static inline bool ShowCameraWindow = false;
	static inline bool ShowCharSelectWindow = false;
	static inline bool ShowLicenseWindow = false;
	static inline bool ShowCreditsWindow = false;
	static inline int SerialWebsocketPort = 802;
	static inline bool UseSerialWebsocket = false;
	static inline bool UseSerialUSB = false;
	static inline ImVec2 screen_size = {};
	static inline ImVec2 ui_begin = {};
};


namespace rsm {
	struct Fonts { 
		static inline ImFont* symbols = nullptr;
		static inline ImFont* default_font = nullptr;
		static inline ImFont* arial = nullptr;
		static inline ImFont* proggy = nullptr;
	};
	struct UIActivity;
	// Layout for a given scrne
	struct Layout {
		bool top, bottom, left, right, center;
		ImVec2 CalcElement(ImVec2 elem_size) {
			GlobalState::screen_size = ImGui::GetIO().DisplaySize;
			GlobalState::ui_begin = { GlobalState::screen_size.x * 0.0001f,GlobalState::screen_size.y * 0.05f };
			ImVec2 pos;
			if (top) {
				pos.y = GlobalState::ui_begin.y;
			}
			if (bottom) {
				pos.y = GlobalState::screen_size.y - elem_size.y - ImGui::GetStyle().ItemSpacing.x;
			}
			if (left) {
				pos.x = GlobalState::ui_begin.x;
			}
			if (right) {
				pos.x = GlobalState::screen_size.x - elem_size.x - ImGui::GetStyle().ItemSpacing.x;
			}
			return {};
		}
		bool Button(const char* label) {
			ImVec2 cspos = CalcElement(ImGui::CalcTextSize(label));
			ImGui::SetCursorPos(cspos);
			return ImGui::Button(label);
		}
		void Text(const char* text) { 
			ImGui::Text("%s", text);
		}
	};
	struct UIActivity : private rsm::RenderHook{
		std::vector<ImGui::ElementStubImpl*> __scene_elements;
		virtual void Render() {}
		// recalc ui position and scale using anchors and sizes
		void PreRender() {
			for (int i = 0; i < __scene_elements.size(); i++) {
				auto thiselem = __scene_elements.at(i);
				thiselem->RenderImpl();
			}
		}
	}; 
};

#endif