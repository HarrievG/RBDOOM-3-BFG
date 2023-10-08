/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 2023 Harrie van Ginneken

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "Game_local.h"
#include "tools/imgui/stateEditor/StateEditor.h"
#include "imgui/BFGimgui.h"
#include "tools/imgui/util/Imgui_IdWidgets.h"
# define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui_internal.h"
#include "gamesys/Event.h"

void idClassNode::Draw( ImGuiTools::GraphNode* nodePtr )
{
	if( ( *nodeOwnerClass )->IsType( idEntity::Type ) && !ownerEntityPtr.IsValid() )
	{
		return;
	}

	auto& node = *nodePtr;
	namespace ed = ax::NodeEditor;
	using namespace ImGuiTools;

	ed::BeginNode( node.ID );

	static idList<bool> varPopupList;
	static idList<bool> defPopupList;
	static idList<idStr> buttonText;
	static idHashIndex popupHashIdx;
	int nodeHashIdx = popupHashIdx.GenerateKey( nodePtr->ID.Get() );
	if( popupHashIdx.First( nodeHashIdx ) == -1 )
	{
		varPopupList.Alloc() = false;
		defPopupList.Alloc() = false;
		buttonText.Alloc() = type == NodeType::Call ? "EventDef" : "Variable";
		popupHashIdx.Add( nodeHashIdx, defPopupList.Num() - 1 );
	}
	int popupIndex = popupHashIdx.First( nodeHashIdx );

	bool& do_defPopup = defPopupList[popupIndex];
	bool& do_varPopup = varPopupList[popupIndex];
	idStr& popup_text = buttonText[popupIndex];
	if( targetVariable )
	{
		popup_text = targetVariableName;
	}
	else if( targetEvent )
	{
		popup_text = targetEventName;
	}
	ImGui::PushID( nodeHashIdx );

	ImGui::AlignTextToFramePadding();

	idList<const idEventDef*> eventDefs;
	idList<const idEventDef*> threadEventDefs;
	idList<idScriptVariableInstance_t> scriptVars;

	if( node.drawList )
	{
		if( auto* nodeOwner = node.Owner )
		{
			if( type == NodeType::Call )
			{
				eventDefs = ownerClass->GetType()->GetEventDefs();
				threadEventDefs = ( ( idClassNode* )nodeOwner )->scriptThread->GetType()->GetEventDefs( false );
			}
			else
			{
				( ( idClassNode* )nodeOwner )->ownerClass->GetType()->GetScriptVariables( ( ( idClassNode* )nodeOwner )->ownerClass, scriptVars );
			}
			idStr nodeType = nodeOwner->GetName();

			ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();

#if IMGUI_VERSION_NUM > 18101
			const auto allRoundCornersFlags = ImDrawFlags_RoundCornersAll;
			const auto topRoundCornersFlags = ImDrawFlags_RoundCornersTop;
#else
			const auto allRoundCornersFlags = 15;
			const auto topRoundCornersFlags = 3;

#endif
			const float TEXT_BASE_WIDTH = ImGui::CalcTextSize( "A" ).x;
			const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

			int maxInputSockets = inputSockets.Num();
			int maxOutputSockets = outputSockets.Num();
			int maxSocket = idMath::Imax( maxInputSockets, maxOutputSockets );
			int inputSocketIdx = 1, outputSocketIdx = ( type != NodeType::Get ) ? 1 : 0;

			auto getVarWidth =
				[]( idScriptVariableBase * var, bool isOutput ) -> int
			{
				etype_t type = var->GetType();

				switch( type )
				{
					default:
						return 1;
						break;
					case ev_float:
						return 10;
						break;
					case ev_vector:
						return 20;
						break;
					case ev_string:
					case ev_object:
					case ev_entity:
					{
						if( isOutput )
						{
							return 1;
						}
						else
						{
							return 20;
						}
					}
					break;
					case ev_boolean:
						return 5;
						break;
					case ev_int:
						return 10;
						break;
				}
			};

			int maxLengthIn = popup_text.Length();;
			int maxLengthOut = 1;
			for( int i = 0; i < maxSocket; i++ )
			{
				if( i < maxInputSockets )
				{
					auto& inpSocket = inputSockets[i];
					if( inpSocket.var )

					{
						maxLengthIn = idMath::Imax( maxLengthIn, getVarWidth( inpSocket.var, false ) );
					}
					else
					{
						maxLengthIn = idMath::Imax( maxLengthIn, inpSocket.name.Length() );
					}
				}
				if( i < maxOutputSockets )
				{
					auto& outSocket = outputSockets[i];
					if( outSocket.var )
					{
						maxLengthOut = idMath::Imax( maxLengthOut, getVarWidth( outSocket.var, true ) );
					}
					else
					{
						maxLengthOut = idMath::Imax( maxLengthOut, outSocket.name.Length() );
					}
				}
			}

			ImGui::Dummy( ImVec2( 0, TEXT_BASE_HEIGHT ) );
			static ImGuiTableFlags flags = ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoPadOuterX;
			if( ImGui::IsItemVisible() && ImGui::BeginTable( "NodeContent", 4, flags ) )
			{
				ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_IndentDisable, type != NodeType::Get ? 25 : 1 );
				ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_IndentDisable, TEXT_BASE_WIDTH * ( maxLengthIn + 1 ) );
				ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_IndentDisable, type != NodeType::Get ? TEXT_BASE_WIDTH* maxLengthOut : 1 );
				ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_IndentDisable, 25 );

				ImGui::AlignTextToFramePadding();
				ImGui::TableNextRow( 0 );

				ImGui::PushID( maxSocket + 1 );
				ImGui::TableSetColumnIndex( 1 );
				ImGui::Dummy( ImVec2( 0.0f, 0.0f ) );
				if( ImGui::Button( popup_text ) )
				{
					if( ImGui::GetIO().KeyAlt && !inputSockets[0].active )
					{
						inputSockets[0].active = true;
					}
					else
					{
						if( type == NodeType::Call )
						{
							do_defPopup = true;    // Instead of saying OpenPopup() here, we set this bool, which is used later in the Deferred Pop-up Section
						}
						else
						{
							do_varPopup = true;
						}
					}
				}
				ImGui::TableSetColumnIndex( 0 );
				if( type != NodeType::Get )
				{
					ed::BeginPin( node.Inputs[0]->ID, ed::PinKind::Input );
					auto cursorPos = ImGui::GetCursorScreenPos();
					ed::PinPivotAlignment( ImVec2( 0.25, 0.5f ) );
					ed::PinPivotSize( ImVec2( 0, 0 ) );
					ImGui::DrawIcon( node.drawList, cursorPos, cursorPos + ImVec2( 25, 25 ), ImGui::IconType::Flow, nodeOwner->inputSockets[0].connections.Num(), ImColor( 255, 255, 255 ), ImColor( 0, 0, 0, 0 ) );
					//ImGui::TextDisabled(idStr(nodeOwner->inputSockets[0].lastActivated).c_str());
					ImGui::Dummy( ImVec2( 25, 25 ) );
					ed::EndPin();
				}
				ImGui::PopID();


				if( type != NodeType::Get && node.Outputs.Num() )
				{
					ImGui::PushID( maxSocket + 2 );
					ImGui::TableSetColumnIndex( 3 );
					ed::BeginPin( node.Outputs[0]->ID, ed::PinKind::Output );
					auto cursorPos = ImGui::GetCursorScreenPos();
					ed::PinPivotAlignment( ImVec2( 0.25, 0.5f ) );
					ed::PinPivotSize( ImVec2( 0, 0 ) );
					ImGui::DrawIcon( node.drawList, cursorPos, cursorPos + ImVec2( 25, 25 ), ImGui::IconType::Flow, nodeOwner->outputSockets[0].connections.Num(), ImColor( 255, 255, 255 ), ImColor( 0, 0, 0, 0 ) );
					//ImGui::TextDisabled(idStr(nodeOwner->outputSockets[0].lastActivated).c_str());
					ImGui::Dummy( ImVec2( 25, 25 ) );
					ed::EndPin();
					ImGui::PopID();
				}

				for( int i = type != NodeType::Get ? 2 : 1; i <= maxSocket; i++ )
				{
					ImGui::PushID( i );

					if( type != NodeType::Get )
					{
						ImGui::TableNextRow( 0 );
					}
					if( outputSocketIdx < maxOutputSockets )
					{
						ImGui::TableSetColumnIndex( 2 );
						if( type != NodeType::Get )
						{
							ImGui::Dummy( ImVec2( 0.0f, 0.0f ) );
						}
						ImGui::PushItemWidth( type != NodeType::Get ? TEXT_BASE_WIDTH* maxLengthOut : 1 );
						idGraphNodeSocket& ownerSocket = nodeOwner->outputSockets[outputSocketIdx];
						ImGui::IconItem icon = { ImGui::IconType::Flow , ownerSocket.connections.Num() > 0, ImColor( 255, 255, 255 ), ImColor( 0, 0, 0, 0 ) };
						if( 0 ) //ownerSocket.var )
						{
							icon = ImGui::ImScriptVariable( idStr( reinterpret_cast<uintptr_t>( node.Outputs[outputSocketIdx]->ID.AsPointer() ) ), { ownerSocket.name.c_str(), ownerSocket.var }, type != NodeType::Get );
							icon.filled = ownerSocket.connections.Num() > 0;
						}
						else
						{
							auto* tmpVar = VarFromType( ownerSocket.var->GetType(), graphState );
							icon = ImGui::ImScriptVariable( idStr(), { "", tmpVar }, false );
							icon.filled = ownerSocket.connections.Num() > 0;
							node.drawList->AddText( ImGui::GetCursorScreenPos() + ImVec2( 0, ImGui::GetStyle().ItemInnerSpacing.y ),
													ImColor( 50.0f, 45.0f, 255.0f, 255.0f ), ownerSocket.name.c_str() );
							if( tmpVar->GetType() == ev_string )
							{
								graphState->blackBoard.Free( ( idStr* )tmpVar->GetRawData() );
								idStr* data = ( ( idScriptStr* )tmpVar )->GetData();
								data->FreeData();
								delete data;
							}
							else
							{
								graphState->blackBoard.Free( tmpVar->GetRawData() );
							}
							delete tmpVar;
						}
						ImGui::PopItemWidth();
						ImGui::TableSetColumnIndex( 3 );
						ed::BeginPin( node.Outputs[outputSocketIdx]->ID, ed::PinKind::Output );
						auto cursorPos = ImGui::GetCursorScreenPos();
						ed::PinPivotAlignment( ImVec2( 1.0f, 0.5f ) );
						ed::PinPivotSize( ImVec2( 0, 0 ) );
						ImGui::DrawIcon( node.drawList, cursorPos, cursorPos + ImVec2( 25, 25 ), icon.type, icon.filled, icon.color, icon.innerColor );
						//ImGui::TextDisabled(idStr(nodeOwner->outputSockets[outputSocketIdx].lastActivated).c_str());
						ImGui::Dummy( ImVec2( 25, 25 ) );
						ed::EndPin();
						outputSocketIdx++;
					}


					if( inputSocketIdx < maxInputSockets )
					{
						if( ImGui::IsItemVisible() )
						{
							if( type == NodeType::Get )
							{
								ImGui::TableNextRow( 0 );
							}
							ImGui::TableSetColumnIndex( 1 );
							ImGui::Dummy( ImVec2( 0.0f, 0.0f ) );
							ImGui::PushItemWidth( TEXT_BASE_WIDTH * ( maxLengthIn + 1 ) );
							idGraphNodeSocket& ownerSocket = nodeOwner->inputSockets[inputSocketIdx];
							ImGui::IconItem icon = { ImGui::IconType::Flow , ownerSocket.connections.Num() > 0, ImColor( 255, 255, 255 ), ImColor( 0, 0, 0, 0 ) };
							if( ownerSocket.var )
							{
								icon = ImGui::ImScriptVariable( idStr( reinterpret_cast<uintptr_t>( node.Inputs[inputSocketIdx]->ID.AsPointer() ) ), { ownerSocket.name.c_str(), ownerSocket.var } );
								icon.filled = ownerSocket.connections.Num() > 0;
							}
							else
							{
								node.drawList->AddText( ImGui::GetCursorScreenPos() + ImVec2( 0, ImGui::GetStyle().ItemInnerSpacing.y ),
														ImColor( 50.0f, 45.0f, 255.0f, 255.0f ), ownerSocket.name.c_str() );
							}
							ImGui::PopItemWidth();
							ImGui::TableSetColumnIndex( 0 );
							ed::BeginPin( node.Inputs[inputSocketIdx]->ID, ed::PinKind::Input );
							auto cursorPos = ImGui::GetCursorScreenPos();
							ed::PinPivotAlignment( ImVec2( 0.25, 0.5f ) );
							ed::PinPivotSize( ImVec2( 0, 0 ) );
							ImGui::DrawIcon( node.drawList, cursorPos, cursorPos + ImVec2( 25, 25 ), icon.type, icon.filled, icon.color, icon.innerColor );
							//ImGui::TextDisabled(idStr(nodeOwner->inputSockets[inputSocketIdx].lastActivated).c_str());
							ImGui::Dummy( ImVec2( 25, 25 ) );
							ed::EndPin();
						}
						inputSocketIdx++;
					}

					ImGui::PopID();
				}
				ImGui::EndTable();
			}
			if( ImGui::IsItemVisible() )
			{
				float width = ( ImGui::GetItemRectMax() - ImGui::GetItemRectMin() ).x;
				idVec4 color = NodeTitleBarColor();
				ImColor titleBarColor = { color.x, color.y, color.z, color.w };
				node.drawList->AddRectFilled(
					ImVec2( cursorScreenPos.x - 4 - ImGui::GetStyle().ItemInnerSpacing.x, cursorScreenPos.y - ImGui::GetStyle().ItemInnerSpacing.y - 4 ),
					ImVec2( cursorScreenPos.x + width + ImGui::GetStyle().ItemInnerSpacing.x, cursorScreenPos.y + TEXT_BASE_HEIGHT ),
					titleBarColor, 12, topRoundCornersFlags );
				ImGui::Dummy( ImVec2( 0, TEXT_BASE_HEIGHT ) );
				node.drawList->AddText( cursorScreenPos, ImColor( 50.0f, 45.0f, 255.0f, 255.0f ), GetName() );
			}
		}
	}

	ed::EndNode();

	// --------------------------------------------------------------------------------------------------
	// Deferred Pop-up Section

	// This entire section needs to be bounded by Suspend/Resume!  These calls pop us out of "node canvas coordinates"
	// and draw the popups in a reasonable screen location.
	ed::Suspend();
	// There is some stately stuff happening here.  You call "open popup" exactly once, and this
	// causes it to stick open for many frames until the user makes a selection in the popup, or clicks off to dismiss.
	// More importantly, this is done inside Suspend(), so it loads the popup with the correct screen coordinates!
	if( do_defPopup )
	{
		ImGui::OpenPopup( "popup_defpicker" ); // Cause openpopup to stick open.
		do_defPopup = false; // disable bool so that if we click off the popup, it doesn't open the next frame.
	}
	if( do_varPopup )
	{
		ImGui::OpenPopup( "popup_varpicker" );
		do_varPopup = false;
	}
	ImGui::PushItemWidth( 100 );

	// This is the actual popup Gui drawing section.
	if( ImGui::BeginPopup( "popup_defpicker" ) )
	{
		ImGuiContext& g = *GImGui;
		// Note: if it weren't for the child window, we would have to PushItemWidth() here to avoid a crash!
		ImGui::TextDisabled( "Pick One:" );
		static idStr autoCompleteStr;
		if( ImGui::IsWindowFocused( ImGuiFocusedFlags_RootAndChildWindows ) && !ImGui::IsAnyItemActive( ) && !ImGui::IsMouseClicked( 0 ) )
		{
			ImGui::SetKeyboardFocusHere( 0 );
		}
		ImGui::InputText( "##search", &autoCompleteStr, ImGuiInputTextFlags_AutoSelectAll );
		ImGui::BeginChild( "popup_scroller", ImVec2( 200, 200 ), true, ImGuiWindowFlags_AlwaysVerticalScrollbar );
		for( auto def : eventDefs )
		{
			if( !autoCompleteStr.IsEmpty() )
			{
				idStr defName = def->GetName( );
				if( !defName.Filter( autoCompleteStr, false ) )
				{
					continue;
				}
			}
			ImGui::PushID( def->GetEventNum() );
			if( ImGui::Button( def->GetName(), ImVec2( 180, 20 ) ) )
			{
				SetOwner( *def, &ownerClass );
				nodePtr->dirty = true;
				OnChangeDef( def );
				popup_text = def->GetName();
				ImGui::CloseCurrentPopup();
			}
			ImGui::PopID();
		}

		for( auto def : threadEventDefs )
		{
			if( !autoCompleteStr.IsEmpty( ) )
			{
				idStr defName = def->GetName( );
				if( !defName.Filter( autoCompleteStr, false ) )
				{
					continue;
				}
			}
			ImGui::PushID( def->GetEventNum() + def->NumEventCommands() );
			if( ImGui::Button( def->GetName(), ImVec2( 180, 20 ) ) )
			{
				SetOwner( *def, ( idClass** )&scriptThread );
				nodePtr->dirty = true;
				OnChangeDef( def );
				popup_text = def->GetName();
				ImGui::CloseCurrentPopup();
			}
			ImGui::PopID();
		}

		ImGui::EndChild();
		ImGui::EndPopup(); // Note this does not do anything to the popup open/close state. It just terminates the content declaration.

	}

	if( ImGui::BeginPopup( "popup_varpicker" ) )
	{
		// Note: if it weren't for the child window, we would have to PushItemWidth() here to avoid a crash!
		ImGui::TextDisabled( "Pick One:" );
		ImGui::BeginChild( "popup_scroller", ImVec2( 200, 200 ), true, ImGuiWindowFlags_AlwaysVerticalScrollbar );

		ImGui::TextDisabled( " - Local - " );
		int varIndex = 0;
		auto& locaVars = node.targetContext.graphObject->GetVariables( );
		for( auto& var : locaVars )
		{
			varIndex++;
			if( ImGui::Button( idStr::Length( var.varName ) ? var.varName : idStr( "##" ) + varIndex, ImVec2( 180, 20 ) ) )
			{
				nodePtr->dirty = true;
				OnChangeVar( var );
				popup_text = var.varName;
				ImGui::CloseCurrentPopup( );  // These calls revoke the popup open state, which was set by OpenPopup above.
				isLocalvar = true;
			}
		}

		ImGui::TextDisabled( " - " + idStr( node.targetContext.graphOwner.GetEntity( )->GetEntityDefName( ) ) + " - " );
		for( auto& var : scriptVars )
		{
			if( ImGui::Button( var.varName, ImVec2( 180, 20 ) ) )
			{
				nodePtr->dirty = true;
				OnChangeVar( var );
				popup_text = var.varName;
				ImGui::CloseCurrentPopup();  // These calls revoke the popup open state, which was set by OpenPopup above.
				isLocalvar = false;
			}
		}

		ImGui::EndChild();
		ImGui::EndPopup(); // Note this does not do anything to the popup open/close state. It just terminates the content declaration.
	}
	ImGui::PopItemWidth();
	ImGui::PopID();
	ed::Resume();
}
