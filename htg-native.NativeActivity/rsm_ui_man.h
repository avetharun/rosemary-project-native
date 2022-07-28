#ifndef __rosemary_project_ui_mgr_h_
#define __rosemary_project_ui_mgr_h_
#include "imgui/imgui_uielement.h"
#include "rsm_hooks.h"
#include <set>
#include "imgui/imgui_format.h"

struct GlobalState {
	// Keyboard height in pixels
	static inline int KeyboardHeight;
	// Height of the status (notification) bar in pixels
	static inline int StatusBarHeight;
	static inline bool statusbar_visible = false;
	static inline int DisplayHeightTotal = 0;
	static inline bool keyboard_visible = false;
	static inline ImVec2 screen_size = {};
	static inline ImVec2 ui_begin = {};
	static inline bool ShowSettingsWindow = false;
	static inline bool ShowMainWindow = true;
	static inline bool ShowDebugWindow = false;
	static inline bool ShowCameraWindow = false;
	static inline bool ShowCharSelectWindow = false;
	static inline bool ShowCharAddWindow = false;
	static inline bool ShowLicenseWindow = false;
	static inline bool ShowCreditsWindow = false;
	// Settings
	static inline int SerialWebsocketPort = 802;
	static inline bool UseSerialWebsocket = false;
	static inline bool UseSerialUSB = false;
	static inline char DeviceFriendlyName[128]{"RSM Mobile Camera Instance"};
};


namespace rsm {
	struct Fonts { 
		static inline ImFont* symbols = nullptr;
		static inline ImFont* default_font = nullptr;
		static inline ImFont* arial = nullptr;
		static inline ImFont* proggy = nullptr;
	};
	// Layout for a given scrne
	struct Layout {
		// Disable projecting the globalState::ui_begin vec onto the element
		bool DisableGlobalUIBegin;
		bool top, bottom, left, right, center;
		ImVec2 CalcElement(ImVec2 elem_size) {
			ImVec2 pos;
			if (top) {
				pos.y = 0;
			}
			if (bottom) {
				pos.y = GlobalState::screen_size.y - elem_size.y - ImGui::GetStyle().ItemSpacing.x;
			}
			if (left) {
				pos.x = 0;
			}
			if (right) {
				pos.x = GlobalState::screen_size.x - elem_size.x - ImGui::GetStyle().ItemSpacing.x;
			}
			if (!DisableGlobalUIBegin) {
				pos = ImVec2{ pos.x + GlobalState::ui_begin.x, pos.y + GlobalState::ui_begin.y };
			}
			return {};
		}
		bool Button(const char* label) {
			ImVec2 cspos = CalcElement(ImGui::CalcTextSize(label));
			ImGui::SetCursorScreenPos(cspos);
			return ImGui::Button(label);
		}
		void TextV(const char* fmt, va_list args) {
			const char* text = alib_strfmtv(fmt, args);
			ImVec2 cspos = CalcElement(ImGui::CalcTextSize(text));
			ImGui::SetCursorScreenPos(cspos);
			ImGui::TextV(fmt, args);
		}
		void Text(const char* fmt, ...) {
			va_list args;
			va_start(args, fmt);
			ImGui::SameLine();
			this->TextV(fmt, args);
			va_end(args);
			ImGui::NewLine();
		}
	};
};

#endif