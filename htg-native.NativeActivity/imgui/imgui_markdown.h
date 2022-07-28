#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_uielement.h"
#include <istream>
namespace ImGui {
	// A button with only text visible.
	bool TextButton(const char* str_id, const char* txt) {
		bool out = false;
		ImVec2 pos = GetCursorPos();
		ImVec2 txt_sz = CalcTextSize(txt);
		ImGui::Text(txt);
		ImGui::SetCursorPos(pos);
		ImGui::InvisibleButton(str_id, txt_sz);
		if (IsItemClicked()) { out = true; }
		return out;
	}
	inline void DrawCharItalic(char chr) {
		ImRotateStart();
		const char* s = alib_strfmt("%c", chr);
		ImVec2 __s_sz = CalcTextSize(s);
		//ImGui::SetCursorPosY(GetCursorPos().y + __s_sz.y * 0.005f );
		ImGui::Text(s);
		free((void*)s);
		ImRotateEnd(-3);

	}
	void ItalicText(const char* text) {
		//SetCursorPosX(GetCursorPosX())
		float font_sz = GetCurrentContext()->Font->FontSize;
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { font_sz * 0.025f, font_sz * 0.025f });
		while (*(text)) {
			DrawCharItalic(*text);
			if (*((text++) + 1)) {
				ImGui::SameLine();
			}
		}
		ImGui::PopStyleVar();
	}
	// May not work with all fonts! Optimized to work with the default ImGui font!
	void BoldText(const char* text) {
		ImVec2 begin = ImGui::GetCursorPos();
		ImGui::Text(text);
		ImGui::SetCursorPos({ begin.x + ImGui::GetFontSize() * 0.035f, begin.y + ImGui::GetFontSize() * 0.035f });
		ImGui::Text(text);
	}
	void CodeBlockLast() {
		ImVec2 min = GetItemRectMin() - ImGui::GetStyle().ItemSpacing * ImVec2{0.15f, 0.15f};
		ImVec2 max = GetItemRectMax() + ImGui::GetStyle().ItemSpacing * ImVec2{0.15f, 0.15f};
		ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
		float _w = col.w;
		col.w *= 0.25f;
		ImGui::GetWindowDrawList()->AddRectFilled(min, max, ImGui::GetColorU32(col), ImGui::GetStyle().ItemSpacing.y * 0.75f);
		col.w = _w;
		ImGui::GetWindowDrawList()->AddRect(min, max, ImGui::GetColorU32(col), ImGui::GetStyle().ItemSpacing.y * 0.75f, 0, ImGui::GetStyle().ItemSpacing.y * 0.05f);
	}
	void CodeBlockText(const char* text) {
		ImGui::PushStyleColor(ImGuiCol_Text, { 0,0,0,0 });
		ImVec2 cspos = ImGui::GetCursorPos();
		ImGui::Text(text);
		ImGui::PopStyleColor();
		ImGui::CodeBlockLast();
		ImGui::SetCursorPos(cspos);
		ImVec4 _c = ImGui::GetStyleColorVec4(ImGuiCol_Text);
		_c.w *= 0.5f;
		ImGui::PushStyleColor(ImGuiCol_Text, _c);
		ImGui::Text(text);
		ImGui::PopStyleColor();
		
	}
	// Quote bar inherits color from ImGuiCol_Button
	// Puts bar before last instance of an item
	void QuoteLast() {
		float bar_off = GetStyle().ItemSpacing.x * 0.5f;
		ImVec2 min = GetItemRectMin();
		ImVec2 max = GetItemRectMax();
		DrawLine({min.x - bar_off, min.y}, {min.x - bar_off, max.y}, GetColorU32(ImGuiCol_Button), bar_off * 0.75f);
	}
	// Underlines last instance of an item, but is mostly used to underline text.
	void UnderlineLast()
	{
		ImVec2 min = GetItemRectMin();
		ImVec2 max = GetItemRectMax();
		min.y = max.y;
		float underline_width = GetFontSize() * 0.1f;
		GetWindowDrawList()->AddLine(
			min, max, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]), underline_width);
	}
	void StrikethroughLast()
	{
		ImVec2 min = GetItemRectMin();
		ImVec2 max = GetItemRectMax();
		min.y = max.y = max.y - GetItemRectSize().y * 0.5f;
		float strikethrough_width = GetFontSize() * 0.05f;
		GetWindowDrawList()->AddLine(
			min, max, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]), strikethrough_width);
	}
	// Quote bar inherits color from ImGuiCol_Button
	void QuoteText(const char* text) {
		ImGui::TextWrapped(text);
		ImGui::QuoteLast();
	}
	void UnderlineText(const char* text)
	{
		std::string line; 
		std::stringstream ss(text);
		while (std::getline(ss, line)) {
			if (line == "") {
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y );
			}
			if (line != "")
				ImGui::TextWrapped(line.c_str());
				ImGui::UnderlineLast();
		}
	}
	void StrikethroughText(const char* text)
	{
		std::string line;
		std::stringstream ss(text);
		while (std::getline(ss, line)) {
			if (line == "") {
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y);
			}
			if (line != "")
				ImGui::TextWrapped(line.c_str());
			ImGui::StrikethroughLast();
		}
	}
	static inline void UrlHTTPText(const char* url, std::function<void(const char* url)> url_opener, const char* url_text = "\0", bool underlined = true) {
		ImVec2 pos = ImGui::GetCursorPos();
		if (*url_text == '\0') {
			url_text = url;
		}
		ImGui::PushStyleColor(ImGuiCol_Button, { 0,0,0,0 });
		ImVec4 _c = ImGui::GetStyleColorVec4(ImGuiCol_Text);
		_c.x *= 0.75;
		_c.y *= 0.75;
		ImGui::PushStyleColor(ImGuiCol_Text, _c);
		bool clicked = ImGui::TextButton("##__impl_ImGui_UtlHttpTextBtn", url_text);
		if (underlined) {
			ImGui::UnderlineLast();
		}
		ImGui::PopStyleColor(2);
		if (clicked) {
			url_opener(url);
		}
	}
};
