
#ifndef NEO_IMGUI_BFGIMGUI_H_
#define NEO_IMGUI_BFGIMGUI_H_

#include "imgui.h"

#include "../idlib/math/Vector.h"
#include "../d3xp/script/Script_Program.h"
#include "./d3xp/StateGraph.h"

// add custom functions for imgui
namespace ImGui
{

bool DragVec3( const char* label, idVec3& v, float v_speed = 1.0f,
			   float v_min = 0.0f, float v_max = 0.0f,
			   const char* display_format = "%.1f",
			   float power = 1.0f, bool ignoreLabelWidth = true );

bool DragVec3fitLabel( const char* label, idVec3& v, float v_speed = 1.0f,
					   float v_min = 0.0f, float v_max = 0.0f,
					   const char* display_format = "%.1f", float power = 1.0f );

enum class IconType : ImU32 { Flow, Circle, Square, Grid, RoundSquare, Diamond };

struct IconItem
{
	IconType type;
	bool filled;
	ImU32 color;
	ImU32 innerColor;
};

IconItem ImScriptVariable( const char* label, const idScriptVariableInstance_t& scriptVar, bool enabled = true );

void DrawIcon( ImDrawList* drawList, const ImVec2& a, const ImVec2& b, IconType type, bool filled, ImU32 color, ImU32 innerColor );

int GetItemWidth( idScriptVariableBase* var, bool isOutput, int minLength );

int GetMaxWidth( idList<idGraphNodeSocket>& socketList , bool isOutput, int minLength );

}

#endif /* NEO_IMGUI_BFGIMGUI_H_ */
