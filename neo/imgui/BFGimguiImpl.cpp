/*
 * ImGui integration into Doom3BFG/OpenTechEngine.
 * Based on ImGui SDL and OpenGL3 examples.
 *  Copyright (c) 2014-2015 Omar Cornut and ImGui contributors
 *
 * Doom3-specific Code (and ImGui::DragXYZ(), based on ImGui::DragFloatN())
 *  Copyright (C) 2015 Daniel Gibson
 *
 * This file is under MIT License, like the original code from ImGui.
 */

#include "precompiled.h"
#pragma hdrstop

#include "BFGimgui.h"
#include "renderer/RenderCommon.h"
#include "renderer/RenderBackend.h"
#include "d3xp/Game_local.h"
# define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

static idCVar imgui_showDemoWindow( "imgui_showDemoWindow", "0", CVAR_GUI | CVAR_BOOL, "show big ImGui demo window" );
static idCVar imgui_showSimpleNodeEditorExample( "imgui_showSimpleNodeEditorExample", "0", CVAR_GUI | CVAR_BOOL, "" );

// our custom ImGui functions from BFGimgui.h

// like DragFloat3(), but with "X: ", "Y: " or "Z: " prepended to each display_format, for vectors
// if !ignoreLabelWidth, it makes sure the label also fits into the current item width.
//    note that this screws up alignment with consecutive "value+label widgets" (like Drag* or ColorEdit*)
bool ImGui::DragVec3( const char* label, idVec3& v, float v_speed,
					  float v_min, float v_max, const char* display_format, float power, bool ignoreLabelWidth )
{
	bool value_changed = false;
	ImGui::BeginGroup();
	ImGui::PushID( label );

	ImGuiStyle& style = ImGui::GetStyle();
	float wholeWidth = ImGui::CalcItemWidth() - 2.0f * style.ItemSpacing.x;
	float spacing = style.ItemInnerSpacing.x;
	float labelWidth = ignoreLabelWidth ? 0.0f : ( ImGui::CalcTextSize( label, NULL, true ).x + spacing );
	float coordWidth = ( wholeWidth - labelWidth - 2.0f * spacing ) * ( 1.0f / 3.0f ); // width of one x/y/z dragfloat

	ImGui::PushItemWidth( coordWidth );
	for( int i = 0; i < 3; i++ )
	{
		ImGui::PushID( i );
		char format[64];
		idStr::snPrintf( format, sizeof( format ), "%c: %s", "XYZ"[i], display_format );
		value_changed |= ImGui::DragFloat( "##v", &v[i], v_speed, v_min, v_max, format, power );

		ImGui::PopID();
		ImGui::SameLine( 0.0f, spacing );
	}
	ImGui::PopItemWidth();
	ImGui::PopID();

	const char* labelEnd = strstr( label, "##" );
	ImGui::TextUnformatted( label, labelEnd );

	ImGui::EndGroup();

	return value_changed;
}

// shortcut for DragXYZ with ignorLabelWidth = false
// very similar, but adjusts width to width of label to make sure it's not cut off
// sometimes useful, but might not align with consecutive "value+label widgets" (like Drag* or ColorEdit*)
bool ImGui::DragVec3fitLabel( const char* label, idVec3& v, float v_speed,
							  float v_min, float v_max, const char* display_format, float power )
{
	return ImGui::DragVec3( label, v, v_speed, v_min, v_max, display_format, power, false );
}

ImGui::IconItem ImGui::ImScriptVariable( const char* strId, const idScriptVariableInstance_t& scriptVar, bool enabled /*= true*/ )
{
	etype_t t = scriptVar.scriptVariable->GetType();

	auto& io = ImGui::GetIO();
	auto getColor = [t]() -> ImU32
	{
		switch( t )
		{
			default:
			case ev_void:
				return ImColor( 255, 255, 255 );
			case ev_boolean:
				return ImColor( 220, 48, 48 );
			case ev_int:
				return ImColor( 68, 201, 156 );
			case ev_float:
				return ImColor( 147, 226, 74 );
			case ev_string:
				return ImColor( 124, 21, 153 );
			case ev_vector:
				return ImColor( 236, 180, 48 );
			case ev_entity:
				return ImColor( 51, 150, 215 );
			case ev_object:
				return ImColor( 51, 150, 215 );
			case ev_function:
				return ImColor( 218, 0, 183 );
			case ev_scriptevent:
				return ImColor( 255, 48, 48 );
		}
	};
	auto getType = [t]() -> IconType
	{
		switch( t )
		{
			default:
			case ev_void:
				return ImGui::IconType::Flow;
			case ev_boolean:
				return ImGui::IconType::Circle;
			case ev_int:
				return ImGui::IconType::Circle;
			case ev_float:
				return ImGui::IconType::Circle;
			case ev_string:
				return ImGui::IconType::Circle;
			case ev_vector:
				return ImGui::IconType::Circle;
			case ev_entity:
				return ImGui::IconType::RoundSquare;
			case ev_object:
				return ImGui::IconType::Square;
			case ev_function:
				return ImGui::IconType::Square;
			case ev_scriptevent:
				return ImGui::IconType::Square;
		}
	};

	ImGui::IconItem icon = { getType(), false, getColor(), ImColor( 0, 0, 0, 0 ) };
	if( enabled )
	{
		switch( t )
		{
			case ev_vector:
				ImGui::DragFloat3( idStr( "##" ) + strId, ( float* )( ( idScriptVector* )( scriptVar.scriptVariable ) )->GetData() );
				break;
			case ev_string:
			{
				idStr* txt = ( ( idScriptStr* )( scriptVar.scriptVariable ) )->GetData();
				ImGui::InputText( idStr( "##" ) + strId, txt );
			}
			break;
			case ev_int:
				ImGui::InputScalar( idStr( "##" ) + strId, ImGuiDataType_S32, ( int* )( ( idScriptInteger* )( scriptVar.scriptVariable ) )->GetData() );
				break;
			case ev_float:
				ImGui::InputFloat( idStr( "##" ) + strId, ( float* )( ( idScriptFloat* )( scriptVar.scriptVariable ) )->GetData() );
				break;
			case ev_boolean:
				ImGui::Checkbox( idStr( "##" ) + strId, ( bool* )( ( idScriptBool* )( scriptVar.scriptVariable ) )->GetData() );
				break;
			case ev_entity:
			{
				if( *( idScriptEntity** )( scriptVar.scriptVariable )->GetRawData() )
				{

					idStr txt = ( *( ( idScriptEntity* )scriptVar.scriptVariable )->GetData() )->GetName();
					ImGui::Text( txt );
				}
				else
				{
					ImGui::LabelText( idStr( "##" ) + strId, "" );
				}

			}
			break;
			default:
				icon.type = ImGui::IconType::Flow;
				ImGui::LabelText( idStr( "##" ) + strId, "" );
				break;
		}
	}
	return icon;
}

void ImGui::DrawIcon( ImDrawList* drawList, const ImVec2& a, const ImVec2& b, ImGui::IconType type, bool filled, ImU32 color, ImU32 innerColor )
{
	auto rect = ImRect( a, b );
	auto rect_x = rect.Min.x;
	auto rect_y = rect.Min.y;
	auto rect_w = rect.Max.x - rect.Min.x;
	auto rect_h = rect.Max.y - rect.Min.y;
	auto rect_center_x = ( rect.Min.x + rect.Max.x ) * 0.5f;
	auto rect_center_y = ( rect.Min.y + rect.Max.y ) * 0.5f;
	auto rect_center = ImVec2( rect_center_x, rect_center_y );
	const auto outline_scale = rect_w / 24.0f;
	const auto extra_segments = static_cast<int>( 2 * outline_scale ); // for full circle

	if( type == ImGui::IconType::Flow )
	{
		const auto origin_scale = rect_w / 24.0f;

		const auto offset_x = 1.0f * origin_scale;
		const auto offset_y = 0.0f * origin_scale;
		const auto margin = ( filled ? 2.0f : 2.0f ) * origin_scale;
		const auto rounding = 0.1f * origin_scale;
		const auto tip_round = 0.7f; // percentage of triangle edge (for tip)
		//const auto edge_round = 0.7f; // percentage of triangle edge (for corner)
		const auto canvas = ImRect(
								rect.Min.x + margin + offset_x,
								rect.Min.y + margin + offset_y,
								rect.Max.x - margin + offset_x,
								rect.Max.y - margin + offset_y );
		const auto canvas_x = canvas.Min.x;
		const auto canvas_y = canvas.Min.y;
		const auto canvas_w = canvas.Max.x - canvas.Min.x;
		const auto canvas_h = canvas.Max.y - canvas.Min.y;

		const auto left = canvas_x + canvas_w * 0.5f * 0.3f;
		const auto right = canvas_x + canvas_w - canvas_w * 0.5f * 0.3f;
		const auto top = canvas_y + canvas_h * 0.5f * 0.2f;
		const auto bottom = canvas_y + canvas_h - canvas_h * 0.5f * 0.2f;
		const auto center_y = ( top + bottom ) * 0.5f;
		//const auto angle = AX_PI * 0.5f * 0.5f * 0.5f;

		const auto tip_top = ImVec2( canvas_x + canvas_w * 0.5f, top );
		const auto tip_right = ImVec2( right, center_y );
		const auto tip_bottom = ImVec2( canvas_x + canvas_w * 0.5f, bottom );

		drawList->PathLineTo( ImVec2( left, top ) + ImVec2( 0, rounding ) );
		drawList->PathBezierCubicCurveTo(
			ImVec2( left, top ),
			ImVec2( left, top ),
			ImVec2( left, top ) + ImVec2( rounding, 0 ) );
		drawList->PathLineTo( tip_top );
		drawList->PathLineTo( tip_top + ( tip_right - tip_top ) * tip_round );
		drawList->PathBezierCubicCurveTo(
			tip_right,
			tip_right,
			tip_bottom + ( tip_right - tip_bottom ) * tip_round );
		drawList->PathLineTo( tip_bottom );
		drawList->PathLineTo( ImVec2( left, bottom ) + ImVec2( rounding, 0 ) );
		drawList->PathBezierCubicCurveTo(
			ImVec2( left, bottom ),
			ImVec2( left, bottom ),
			ImVec2( left, bottom ) - ImVec2( 0, rounding ) );

		if( !filled )
		{
			if( innerColor & 0xFF000000 )
			{
				drawList->AddConvexPolyFilled( drawList->_Path.Data, drawList->_Path.Size, innerColor );
			}

			drawList->PathStroke( color, true, 2.0f * outline_scale );
		}
		else
		{
			drawList->PathFillConvex( color );
		}
	}
	else
	{
		auto triangleStart = rect_center_x + 0.32f * rect_w;

		auto rect_offset = -static_cast<int>( rect_w * 0.25f * 0.25f );

		rect.Min.x += rect_offset;
		rect.Max.x += rect_offset;
		rect_x += rect_offset;
		rect_center_x += rect_offset * 0.5f;
		rect_center.x += rect_offset * 0.5f;

		if( type == ImGui::IconType::Circle )
		{
			const auto c = rect_center;

			if( !filled )
			{
				const auto r = 0.5f * rect_w / 2.0f - 0.5f;

				if( innerColor & 0xFF000000 )
				{
					drawList->AddCircleFilled( c, r, innerColor, 12 + extra_segments );
				}
				drawList->AddCircle( c, r, color, 12 + extra_segments, 2.0f * outline_scale );
			}
			else
			{
				drawList->AddCircleFilled( c, 0.5f * rect_w / 2.0f, color, 12 + extra_segments );
			}
		}

		if( type == ImGui::IconType::Square )
		{
			if( filled )
			{
				const auto r = 0.5f * rect_w / 2.0f;
				const auto p0 = rect_center - ImVec2( r, r );
				const auto p1 = rect_center + ImVec2( r, r );

#if IMGUI_VERSION_NUM > 18101
				drawList->AddRectFilled( p0, p1, color, 0, ImDrawFlags_RoundCornersAll );
#else
				drawList->AddRectFilled( p0, p1, color, 0, 15 );
#endif
			}
			else
			{
				const auto r = 0.5f * rect_w / 2.0f - 0.5f;
				const auto p0 = rect_center - ImVec2( r, r );
				const auto p1 = rect_center + ImVec2( r, r );

				if( innerColor & 0xFF000000 )
				{
#if IMGUI_VERSION_NUM > 18101
					drawList->AddRectFilled( p0, p1, innerColor, 0, ImDrawFlags_RoundCornersAll );
#else
					drawList->AddRectFilled( p0, p1, innerColor, 0, 15 );
#endif
				}

#if IMGUI_VERSION_NUM > 18101
				drawList->AddRect( p0, p1, color, 0, ImDrawFlags_RoundCornersAll, 2.0f * outline_scale );
#else
				drawList->AddRect( p0, p1, color, 0, 15, 2.0f * outline_scale );
#endif
			}
		}

		if( type == ImGui::IconType::Grid )
		{
			const auto r = 0.5f * rect_w / 2.0f;
			const auto w = ceilf( r / 3.0f );

			const auto baseTl = ImVec2( floorf( rect_center_x - w * 2.5f ), floorf( rect_center_y - w * 2.5f ) );
			const auto baseBr = ImVec2( floorf( baseTl.x + w ), floorf( baseTl.y + w ) );

			auto tl = baseTl;
			auto br = baseBr;
			for( int i = 0; i < 3; ++i )
			{
				tl.x = baseTl.x;
				br.x = baseBr.x;
				drawList->AddRectFilled( tl, br, color );
				tl.x += w * 2;
				br.x += w * 2;
				if( i != 1 || filled )
				{
					drawList->AddRectFilled( tl, br, color );
				}
				tl.x += w * 2;
				br.x += w * 2;
				drawList->AddRectFilled( tl, br, color );

				tl.y += w * 2;
				br.y += w * 2;
			}

			triangleStart = br.x + w + 1.0f / 24.0f * rect_w;
		}

		if( type == ImGui::IconType::RoundSquare )
		{
			if( filled )
			{
				const auto r = 0.5f * rect_w / 2.0f;
				const auto cr = r * 0.5f;
				const auto p0 = rect_center - ImVec2( r, r );
				const auto p1 = rect_center + ImVec2( r, r );

#if IMGUI_VERSION_NUM > 18101
				drawList->AddRectFilled( p0, p1, color, cr, ImDrawFlags_RoundCornersAll );
#else
				drawList->AddRectFilled( p0, p1, color, cr, 15 );
#endif
			}
			else
			{
				const auto r = 0.5f * rect_w / 2.0f - 0.5f;
				const auto cr = r * 0.5f;
				const auto p0 = rect_center - ImVec2( r, r );
				const auto p1 = rect_center + ImVec2( r, r );

				if( innerColor & 0xFF000000 )
				{
#if IMGUI_VERSION_NUM > 18101
					drawList->AddRectFilled( p0, p1, innerColor, cr, ImDrawFlags_RoundCornersAll );
#else
					drawList->AddRectFilled( p0, p1, innerColor, cr, 15 );
#endif
				}

#if IMGUI_VERSION_NUM > 18101
				drawList->AddRect( p0, p1, color, cr, ImDrawFlags_RoundCornersAll, 2.0f * outline_scale );
#else
				drawList->AddRect( p0, p1, color, cr, 15, 2.0f * outline_scale );
#endif
			}
		}
		else if( type == ImGui::IconType::Diamond )
		{
			if( filled )
			{
				const auto r = 0.607f * rect_w / 2.0f;
				const auto c = rect_center;

				drawList->PathLineTo( c + ImVec2( 0, -r ) );
				drawList->PathLineTo( c + ImVec2( r, 0 ) );
				drawList->PathLineTo( c + ImVec2( 0, r ) );
				drawList->PathLineTo( c + ImVec2( -r, 0 ) );
				drawList->PathFillConvex( color );
			}
			else
			{
				const auto r = 0.607f * rect_w / 2.0f - 0.5f;
				const auto c = rect_center;

				drawList->PathLineTo( c + ImVec2( 0, -r ) );
				drawList->PathLineTo( c + ImVec2( r, 0 ) );
				drawList->PathLineTo( c + ImVec2( 0, r ) );
				drawList->PathLineTo( c + ImVec2( -r, 0 ) );

				if( innerColor & 0xFF000000 )
				{
					drawList->AddConvexPolyFilled( drawList->_Path.Data, drawList->_Path.Size, innerColor );
				}

				drawList->PathStroke( color, true, 2.0f * outline_scale );
			}
		}
		else
		{
			const auto triangleTip = triangleStart + rect_w * ( 0.45f - 0.32f );

			drawList->AddTriangleFilled(
				ImVec2( ceilf( triangleTip ), rect_y + rect_h * 0.5f ),
				ImVec2( triangleStart, rect_center_y + 0.15f * rect_h ),
				ImVec2( triangleStart, rect_center_y - 0.15f * rect_h ),
				color );
		}
	}
}


// the ImGui hooks to integrate it into the engine


// NODE EDITOR EXAMPLE AND TEST
#include "../extern/imgui-node-editor/imgui_node_editor.h"
namespace ed = ax::NodeEditor;

static ax::NodeEditor::EditorContext* g_EDcontext = nullptr;

void ED_Simple_OnStart()
{
	ed::Config config;
	config.SettingsFile = "Simple.json";
	g_EDcontext = ed::CreateEditor( &config );
}

void ED_Simple_OnStop( ax::NodeEditor::EditorContext* context )
{
	ed::DestroyEditor( context );
}

void ED_OnFrame( float deltaTime )
{
	auto& io = ImGui::GetIO();

	ImGui::Text( "FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f );

	ImGui::Separator();

	ed::SetCurrentEditor( g_EDcontext );
	ed::Begin( "My Editor", ImVec2( 0.0, 0.0f ) );
	int uniqueId = 1;
	// Start drawing nodes.
	ed::BeginNode( uniqueId++ );
	ImGui::Text( "Node A" );
	ed::BeginPin( uniqueId++, ed::PinKind::Input );
	ImGui::Text( "-> In" );
	ed::EndPin();
	ImGui::SameLine();
	ed::BeginPin( uniqueId++, ed::PinKind::Output );
	ImGui::Text( "Out ->" );
	ed::EndPin();
	ed::EndNode();
	ed::End();
	ed::SetCurrentEditor( nullptr );

	//ImGui::ShowMetricsWindow();
}



namespace ImGuiHook
{

namespace
{

bool	g_IsInit = false;
double	g_Time = 0.0f;
bool	g_MousePressed[5] = { false, false, false, false, false };
float	g_MouseWheel = 0.0f;
ImVec2	g_MousePos = ImVec2( -1.0f, -1.0f ); //{-1.0f, -1.0f};
ImVec2	g_DisplaySize = ImVec2( 0.0f, 0.0f ); //{0.0f, 0.0f};



bool g_haveNewFrame = false;

bool HandleKeyEvent( const sysEvent_t& keyEvent )
{
	assert( keyEvent.evType == SE_KEY );

	keyNum_t keyNum = static_cast<keyNum_t>( keyEvent.evValue );
	bool pressed = keyEvent.evValue2 > 0;

	ImGuiIO& io = ImGui::GetIO();

	if( keyNum < K_JOY1 )
	{
		// keyboard input as direct input scancodes
		io.KeysDown[keyNum] = pressed;

		io.KeyAlt = usercmdGen->KeyState( K_LALT ) == 1 || usercmdGen->KeyState( K_RALT ) == 1;
		io.KeyCtrl = usercmdGen->KeyState( K_LCTRL ) == 1 || usercmdGen->KeyState( K_RCTRL ) == 1;
		io.KeyShift = usercmdGen->KeyState( K_LSHIFT ) == 1 || usercmdGen->KeyState( K_RSHIFT ) == 1;

		return true;
	}
	else if( keyNum >= K_MOUSE1 && keyNum <= K_MOUSE5 )
	{
		int buttonIdx = keyNum - K_MOUSE1;

		// K_MOUSE* are contiguous, so they can be used as indexes into imgui's
		// g_MousePressed[] - imgui even uses the same order (left, right, middle, X1, X2)
		g_MousePressed[buttonIdx] = pressed;

		return true; // let's pretend we also handle mouse up events
	}
	else if( keyNum >= K_MWHEELDOWN && keyNum <= K_MWHEELUP )
	{
		return InjectMouseWheel( keyNum == K_MWHEELUP ? 1 : -1 );
	}

	return false;
}

// Gross hack. I'm sorry.
// sets the kb-layout specific keys in the keymap
void FillCharKeys( int* keyMap )
{
	// set scancodes as default values in case the real keys aren't found below
	keyMap[ImGuiKey_A] = K_A;
	keyMap[ImGuiKey_C] = K_C;
	keyMap[ImGuiKey_V] = K_V;
	keyMap[ImGuiKey_X] = K_X;
	keyMap[ImGuiKey_Y] = K_Y;
	keyMap[ImGuiKey_Z] = K_Z;

	// try all probable keys for whether they're ImGuiKey_A/C/V/X/Y/Z
	for( int k = K_1; k < K_RSHIFT; ++k )
	{
		const char* kn = idKeyInput::LocalizedKeyName( ( keyNum_t )k );
		if( kn[0] == '\0' || kn[1] != '\0' || kn[0] == '?' )
		{
			// if the key wasn't found or the name has more than one char,
			// it's not what we're looking for.
			continue;
		}
		switch( kn[0] )
		{
			case 'a': // fall-through
			case 'A':
				keyMap [ImGuiKey_A] = k;
				break;
			case 'c': // fall-through
			case 'C':
				keyMap [ImGuiKey_C] = k;
				break;

			case 'v': // fall-through
			case 'V':
				keyMap [ImGuiKey_V] = k;
				break;

			case 'x': // fall-through
			case 'X':
				keyMap [ImGuiKey_X] = k;
				break;

			case 'y': // fall-through
			case 'Y':
				keyMap [ImGuiKey_Y] = k;
				break;

			case 'z': // fall-through
			case 'Z':
				keyMap [ImGuiKey_Z] = k;
				break;
		}
	}
}

// Sys_GetClipboardData() expects that you Mem_Free() its returned data
// ImGui can't do that, of course, so copy it into a static buffer here,
// Mem_Free() and return the copy
const char* GetClipboardText( void* )
{
	char* txt = Sys_GetClipboardData();
	if( txt == NULL )
	{
		return NULL;
	}

	static idStr clipboardBuf;
	clipboardBuf = txt;

	Mem_Free( txt );

	return clipboardBuf.c_str();
}

void SetClipboardText( void*, const char* text )
{
	Sys_SetClipboardData( text );
}


bool ShowWindows()
{
	return ( ImGuiTools::AreEditorsActive() || imgui_showDemoWindow.GetBool() || imgui_showSimpleNodeEditorExample.GetBool() || com_showFPS.GetInteger() > 1 );
}

bool UseInput()
{
	return ImGuiTools::ReleaseMouseForTools();
}

} //anon namespace

bool Init( int windowWidth, int windowHeight )
{
	if( IsInitialized() )
	{
		Destroy();
	}

	IMGUI_CHECKVERSION();

	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	// Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
	io.KeyMap[ImGuiKey_Tab] = K_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = K_LEFTARROW;
	io.KeyMap[ImGuiKey_RightArrow] = K_RIGHTARROW;
	io.KeyMap[ImGuiKey_UpArrow] = K_UPARROW;
	io.KeyMap[ImGuiKey_DownArrow] = K_DOWNARROW;
	io.KeyMap[ImGuiKey_PageUp] = K_PGUP;
	io.KeyMap[ImGuiKey_PageDown] = K_PGDN;
	io.KeyMap[ImGuiKey_Home] = K_HOME;
	io.KeyMap[ImGuiKey_End] = K_END;
	io.KeyMap[ImGuiKey_Delete] = K_DEL;
	io.KeyMap[ImGuiKey_Backspace] = K_BACKSPACE;
	io.KeyMap[ImGuiKey_Enter] = K_ENTER;
	io.KeyMap[ImGuiKey_Escape] = K_ESCAPE;

	FillCharKeys( io.KeyMap );

	g_DisplaySize.x = windowWidth;
	g_DisplaySize.y = windowHeight;
	io.DisplaySize = g_DisplaySize;

	// RB: FIXME double check
	io.SetClipboardTextFn = SetClipboardText;
	io.GetClipboardTextFn = GetClipboardText;
	io.ClipboardUserData = NULL;

	// SRS - store imgui.ini file in fs_savepath (not in cwd please!)
	static idStr BFG_IniFilename = fileSystem->BuildOSPath( cvarSystem->GetCVarString( "fs_savepath" ), io.IniFilename );
	io.IniFilename = BFG_IniFilename;

	// make it a bit prettier with rounded edges
	ImGuiStyle& style = ImGui::GetStyle();
	//style.ChildWindowRounding = 9.0f;
	//style.FrameRounding = 4.0f;
	//style.ScrollbarRounding = 4.0f;
	//style.GrabRounding = 4.0f;

	// Setup style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	g_IsInit = true;

	return true;
}

void NotifyDisplaySizeChanged( int width, int height )
{
	if( g_DisplaySize.x != width || g_DisplaySize.y != height )
	{
		g_DisplaySize = ImVec2( ( float )width, ( float )height );

		if( IsInitialized() )
		{
			Destroy();
			Init( width, height );

			// reuse the default ImGui font
			const idMaterial* image = declManager->FindMaterial( "_imguiFont" );

			ImGuiIO& io = ImGui::GetIO();

			byte* pixels = NULL;
			io.Fonts->GetTexDataAsRGBA32( &pixels, &width, &height );

			io.Fonts->TexID = ( void* )image;
		}
	}
}

// inject a sys event
bool InjectSysEvent( const sysEvent_t* event )
{
	if( IsInitialized() && UseInput() )
	{
		if( event == NULL )
		{
			assert( 0 ); // I think this shouldn't happen
			return false;
		}

		const sysEvent_t& ev = *event;

		switch( ev.evType )
		{
			case SE_KEY:
				return HandleKeyEvent( ev );

			case SE_MOUSE_ABSOLUTE:
				g_MousePos.x = ev.evValue;
				g_MousePos.y = ev.evValue2;
				return true;

			case SE_CHAR:
				if( ev.evValue < 0x10000 )
				{
					ImGui::GetIO().AddInputCharacter( ev.evValue );
					return true;
				}
				break;

			case SE_MOUSE_LEAVE:
				g_MousePos = ImVec2( -1.0f, -1.0f );
				return true;

			default:
				break;
		}
	}
	return false;
}

bool InjectMouseWheel( int delta )
{
	if( IsInitialized() && UseInput() && delta != 0 )
	{
		g_MouseWheel = ( delta > 0 ) ? 1 : -1;
		return true;
	}
	return false;
}

void NewFrame()
{
	if( !g_haveNewFrame && IsInitialized() && ShowWindows() )
	{
		ImGuiIO& io = ImGui::GetIO();

		// Setup display size (every frame to accommodate for window resizing)
		io.DisplaySize = g_DisplaySize;

		// Setup time step
		int	time = Sys_Milliseconds();
		double current_time = time * 0.001;
		io.DeltaTime = g_Time > 0.0 ? ( float )( current_time - g_Time ) : ( float )( 1.0f / 60.0f );

		if( io.DeltaTime <= 0.0F )
		{
			io.DeltaTime = ( 1.0f / 60.0f );
		}

		g_Time = current_time;

		// Setup inputs
		io.MousePos = g_MousePos;

		// If a mouse press event came, always pass it as "mouse held this frame",
		// so we don't miss click-release events that are shorter than 1 frame.
		for( int i = 0; i < 5; ++i )
		{
			io.MouseDown[i] = g_MousePressed[i] || usercmdGen->KeyState( K_MOUSE1 + i ) == 1;
			//g_MousePressed[i] = false;
		}

		io.MouseWheel = g_MouseWheel;
		g_MouseWheel = 0.0f;

		// Hide OS mouse cursor if ImGui is drawing it TODO: hide mousecursor?
		// ShowCursor(io.MouseDrawCursor ? 0 : 1);

		ImGui::GetIO().MouseDrawCursor = UseInput();

		// Start the frame
		ImGui::NewFrame();
		g_haveNewFrame = true;

		if( imgui_showDemoWindow.GetBool() && !ImGuiTools::ReleaseMouseForTools() )
		{
			ImGuiTools::impl::SetReleaseToolMouse( true );
		}
	}
}

bool IsReadyToRender()
{
	if( IsInitialized() && ShowWindows() )
	{
		if( !g_haveNewFrame )
		{
			// for screenshots etc, where we didn't go through idCommonLocal::Frame()
			// before idRenderSystemLocal::SwapCommandBuffers_FinishRendering()
			NewFrame();
		}

		return true;
	}

	return false;
}

void Render()
{
	if( IsInitialized() && ShowWindows() )
	{
		if( !g_haveNewFrame )
		{
			// for screenshots etc, where we didn't go through idCommonLocal::Frame()
			// before idRenderSystemLocal::SwapCommandBuffers_FinishRendering()
			NewFrame();
		}

		ImGuiTools::DrawToolWindows();

		// do standard demo window test
		if( imgui_showDemoWindow.GetBool() )
		{
			ImGui::ShowDemoWindow();
		}

		// do simple node editor test
		if( imgui_showSimpleNodeEditorExample.GetBool() )
		{
			if( !g_EDcontext )
			{
				ED_Simple_OnStart();
			}

			ImGuiIO& io = ImGui::GetIO();

			ED_OnFrame( io.DeltaTime );
		}
		else
		{
			if( g_EDcontext )
			{
				ED_Simple_OnStop( g_EDcontext );
				g_EDcontext = nullptr;
			}
		}

		ImGui::Render();
		idRenderBackend::ImGui_RenderDrawLists( ImGui::GetDrawData() );
		g_haveNewFrame = false;
	}
}

void Destroy()
{
	if( IsInitialized() )
	{
		ImGui::DestroyContext();
		g_IsInit = false;
		g_haveNewFrame = false;
	}
}

bool IsInitialized()
{
	// checks if imgui is up and running
	return g_IsInit;
}

} //namespace ImGuiHook
