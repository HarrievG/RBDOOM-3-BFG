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

CLASS_DECLARATION( idGraphNode, idStateNode )
END_CLASS

idStateNode::idStateNode()
{

}
stateResult_t idStateNode::Exec( stateParms_t* parms )
{
	idGraphNodeSocket& strSocket = inputSockets[1];

	if( inputSockets[1].active )
	{
		input_State = *( idScriptString* )( strSocket.var );
	}

	if( inputSockets[0].active )
	{
		switch( type )
		{
			case idStateNode::Set:
				stateThread->SetState( input_State );
				break;
			case idStateNode::Post:
				stateThread->PostState( input_State );
				break;
			default:
				break;
		}
		output_Result = SRESULT_DONE;
		outputSockets[0].active = true;
	}

	return output_Result;
}

void idStateNode::WriteBinary( idFile* file, ID_TIME_T* _timeStamp /*= NULL*/ )
{
	if( file )
	{
		file->WriteBig( type );
		file->WriteString( input_State );
		idGraphNode::WriteBinary( file, _timeStamp );
	}
}

bool idStateNode::LoadBinary( idFile* file, const ID_TIME_T _timeStamp, idClass* owner )
{
	if( file )
	{
		file->ReadBig( type );
		file->ReadString( input_State );
		Setup( owner );
		return idGraphNode::LoadBinary( file, _timeStamp );
	}
	return false;
}

void idStateNode::Setup( idClass* graphOwner )
{
	output_Result = SRESULT_ERROR;
	stateThread = graph->targetStateThread;

	idGraphNodeSocket* newInput = &CreateInputSocket();
	newInput->name = idTypeInfo::GetEnumTypeInfo( "idStateNode::NodeType", type );

	newInput = &CreateInputSocket();
	newInput->name = "State";
	newInput->var = new idScriptStr( ( void* )&input_State );
	idGraphNodeSocket& newOutput = CreateOutputSocket();
	newOutput.name = "Result";
	newOutput.var = new idScriptInt( ( void* )&output_Result );
}

//////////////////////////////////////////////////////////////////////////
CLASS_DECLARATION( idGraphNode, idGraphOnInitNode )
END_CLASS

idGraphOnInitNode::idGraphOnInitNode()
{
}

stateResult_t idGraphOnInitNode::Exec( stateParms_t* parms )
{
	return SRESULT_DONE;
}

void idGraphOnInitNode::WriteBinary( idFile* file, ID_TIME_T* _timeStamp /*= NULL*/ )
{
	if( file != NULL )
	{
		idGraphNode::WriteBinary( file, _timeStamp );
	}
}

bool idGraphOnInitNode::LoadBinary( idFile* file, const ID_TIME_T _timeStamp, idClass* owner )
{
	if( file != NULL )
	{
		Setup( owner );
		return idGraphNode::LoadBinary( file, _timeStamp );
	}
	return false;
}

void idGraphOnInitNode::Setup( idClass* graphOwner )
{
	idGraphNodeSocket& newOutput = CreateOutputSocket();
	newOutput.active = true;
	newOutput.name = "";
	done = false;
}

idVec4 idGraphOnInitNode::NodeTitleBarColor()
{
	return idVec4( 1, 0, 1, 1 );
}

//////////////////////////////////////////////////////////////////////////
CLASS_DECLARATION( idGraphNode, idGraphInputOutputNode )
END_CLASS

idGraphInputOutputNode::idGraphInputOutputNode() : nodeType( Input )
{
}

stateResult_t idGraphInputOutputNode::Exec( stateParms_t* parms )
{
	return SRESULT_DONE;
}

void idGraphInputOutputNode::WriteBinary( idFile* file, ID_TIME_T* _timeStamp /*= NULL*/ )
{
	if( file != NULL )
	{
		idGraphNode::WriteBinary( file, _timeStamp );
	}
}

bool idGraphInputOutputNode::LoadBinary( idFile* file, const ID_TIME_T _timeStamp , idClass* owner )
{
	if( file != NULL )
	{
		Setup( owner );
		return idGraphNode::LoadBinary( file, _timeStamp );
	}
	return false;
}

void idGraphInputOutputNode::Setup( idClass* graphOwner )
{
	common->Printf( "idGraphInputOutputNode::Setup() Not Implemented! \n" );
	//cannot be set
	//idGraphNodeSocket& newOutput = CreateOutputSocket();
	//newOutput.active = true;
	//newOutput.name = "Initialize";
}

idVec4 idGraphInputOutputNode::NodeTitleBarColor()
{
	return idVec4( 0.5, 1, 0.5, 1 );
}

idClassNode::~idClassNode()
{
	delete scriptThread;
	scriptThread = nullptr;
}

//////////////////////////////////////////////////////////////////////////
CLASS_DECLARATION( idGraphNode, idClassNode )
END_CLASS

idClassNode::idClassNode()
{
	targetEvent = nullptr;
	targetVariable = nullptr;
	scriptThread = nullptr;

	ownerClass = nullptr;
	nodeOwnerClass = nullptr;

	NodeType type;
	const idEventDef* targetEvent = nullptr;
	idScriptVariableBase* targetVariable = nullptr;
	idThread* scriptThread = nullptr;//for sys events;
}

stateResult_t idClassNode::Exec( stateParms_t* parms )
{
	if( type == Call )
	{
		assert( targetEvent );
		intptr_t			eventData[D_EVENT_MAXARGS];

		auto socketVar =
			[]( idGraphNodeSocket * inp, const idEventDef * targetEvent, GraphState * graph, const char spec, intptr_t& dataPtr ) -> bool
		{
			switch( spec )
			{
				case D_EVENT_FLOAT:
				{
					( *( float* )&dataPtr ) = *( ( idScriptFloat* )inp->var )->GetData();
				}
				break;
				case D_EVENT_INTEGER:
				{
					( *( int* )&dataPtr ) = *( ( idScriptInt* )inp->var )->GetData();
				}
				break;

				case D_EVENT_VECTOR:
				{
					dataPtr = ( uintptr_t )( ( ( idScriptVector* )inp->var )->GetData() )->ToFloatPtr();
				}
				break;

				case D_EVENT_STRING:
					dataPtr = ( uintptr_t )( ( ( idScriptString* )( inp->var ) )->GetData() )->c_str();
					break;

				case D_EVENT_ENTITY:
				case D_EVENT_ENTITY_NULL:
					//hvg_todo : check validity of this.
					dataPtr = ( uintptr_t ) * ( ( idScriptEntity* )inp->var )->GetData();
					break;

				default:
					gameLocal.Warning( "idClassNode::Exec : Invalid arg format '%s' string for '%s' event.", spec, targetEvent->GetName() );
					return false;
			}
			return true;
		};

		const char* formatspec = targetEvent->GetArgFormat();
		for( int i = 1; i <= targetEvent->GetNumArgs(); i++ )
		{
			idGraphNodeSocket* inp = &inputSockets[i];
			if( !socketVar( inp, targetEvent, graph, formatspec[i - 1], eventData[i - 1] ) )
			{
				return SRESULT_ERROR;
			}
		}

		( *nodeOwnerClass )->ProcessEventArgPtr( targetEvent, eventData );

		//Handle return var , if any!
		auto retSocketVar =
			[]( idGraphNodeSocket * inp, const idEventDef * targetEvent, GraphState * graph, const char spec ) -> bool
		{
			switch( spec )
			{
				case D_EVENT_FLOAT:
				{
					*( idScriptFloat* )( inp->var ) = *gameLocal.program.returnDef->value.floatPtr;
				}
				break;
				case D_EVENT_INTEGER:
				{
					*( idScriptInt* )( inp->var ) = *gameLocal.program.returnDef->value.intPtr;
				}
				break;

				case D_EVENT_VECTOR:
				{
					*( idScriptVector* )( inp->var ) = *gameLocal.program.returnDef->value.vectorPtr;
				}
				break;

				case D_EVENT_STRING:
					*( ( idScriptStr* )inp->var )->GetData() = gameLocal.program.returnStringDef->value.stringPtr;
					break;

				case D_EVENT_ENTITY:
				case D_EVENT_ENTITY_NULL:
				{
					int entKey = ( *gameLocal.program.returnDef->value.entityNumberPtr ) - 1;
					if( entKey >= 0 )
					{
						*( ( idScriptEntity* )inp->var )->GetData() = gameLocal.entities[entKey];
					}
				}
				break;
				default:
					gameLocal.Warning( "idClassNode::Setup : Invalid arg format '%s' string for '%s' event.", spec, targetEvent->GetName() );
					return false;
			}
			return true;
		};

		if( targetEvent->GetReturnType() != 0 )
		{
			retSocketVar( &outputSockets[0], targetEvent, graph, targetEvent->GetReturnType() );
		}

		return SRESULT_DONE;
	}
	else if( type == Set )
	{
		return SRESULT_DONE;
	}
	else
	{
		return SRESULT_DONE;
	}
	return SRESULT_ERROR;
}

void idClassNode::Setup( idClass* graphOwner )
{
	inputSockets.Clear();
	outputSockets.Clear();
	ownerClass = graphOwner;
	nodeOwnerClass = &ownerClass;

	idGraphNodeSocket* newInput = &CreateInputSocket();
	newInput->name = "Call"; // hidden ; used as button with event or var name , alt click to activate this input

	if( targetEvent )
	{
		int numargs = targetEvent->GetNumArgs();
		const char* formatspec = targetEvent->GetArgFormat();

		auto socketVar =
			[]( idGraphNodeSocket * inp, const idEventDef * targetEvent, GraphState * graph, const char spec ) -> bool
		{
			switch( spec )
			{
				case D_EVENT_FLOAT:
				{
					inp->var = graph->blackBoard.Alloc<idScriptFloat>( 8 );
				}
				break;
				case D_EVENT_INTEGER:
				{
					inp->var = graph->blackBoard.Alloc<idScriptInteger>( 8 );
				}
				break;

				case D_EVENT_VECTOR:
				{
					inp->var = graph->blackBoard.Alloc<idScriptVector>( 3 * 8 );
				}
				break;

				case D_EVENT_STRING:
					inp->var = graph->blackBoard.Alloc( "" );
					break;

				case D_EVENT_ENTITY:
				case D_EVENT_ENTITY_NULL:
					inp->var = graph->blackBoard.Alloc<idScriptEntity>( 16 );
					break;

				default:
					gameLocal.Warning( "idClassNode::Setup : Invalid arg format '%s' string for event.", spec, targetEvent->GetName() );
					return false;
			}
			return true;
		};

		for( int i = 0; i < numargs; i++ )
		{
			idGraphNodeSocket* inp = &CreateInputSocket();
			if( !socketVar( inp, targetEvent, graph, formatspec[i] ) )
			{
				inputSockets.RemoveIndexFast( inputSockets.Num() - 1 );
			}
		}

		idGraphNodeSocket* out = &CreateOutputSocket();
		if( !socketVar( out, targetEvent, graph, targetEvent->GetReturnType() ) )
		{
			outputSockets.RemoveIndexFast( outputSockets.Num() - 1 );
		}
	}
	else if( targetVariable )
	{
		if( type == NodeType::Get )
		{
			idGraphNodeSocket* out = &CreateOutputSocket();
			out->var = targetVariable;
			out->freeData = false;
		}
		else
		{
			idGraphNodeSocket* inp = &CreateInputSocket();
			inp->var = targetVariable;
			inp->freeData = false;
		}
	}

	if( !scriptThread )
	{
		scriptThread = new idThread();
		scriptThread->ManualDelete();
	}
	scriptThread->EndThread();
	scriptThread->ManualControl();

	if( targetEvent )
	{
		idTypeInfo* c = scriptThread->GetType();
		int	num = targetEvent->GetEventNum();
		if( c->eventMap[num] )
		{
			nodeOwnerClass = ( ( idClass** )&scriptThread );
		}
	}

}

void idClassNode::OnChangeDef( const idEventDef* eventDef )
{
	targetEvent = eventDef;
	targetEventName = targetEvent->GetName();
	Setup( ownerClass );
}

void idClassNode::OnChangeVar( idScriptVariableInstance_t& varInstance )
{
	targetVariableName = varInstance.varName;
	targetVariable = varInstance.scriptVariable;
	Setup( ownerClass );
}

void idClassNode::WriteBinary( idFile* file, ID_TIME_T* _timeStamp /*= NULL */ )
{
	file->WriteBig( type );
	file->WriteString( targetEventName );
	file->WriteString( targetVariableName );

	idGraphNode::WriteBinary( file, _timeStamp );
}

bool idClassNode::LoadBinary( idFile* file, const ID_TIME_T _timeStamp, idClass* owner )
{
	if( file != NULL )
	{
		file->ReadBig( type );

		file->ReadString( targetEventName );
		if( !targetEventName.IsEmpty() )
		{
			targetEvent = idEventDef::FindEvent( targetEventName );
		}

		file->ReadString( targetVariableName );
		if( !targetVariableName.IsEmpty() )
		{
			idList<idScriptVariableInstance_t> searchVar;
			idScriptVariableInstance_t& var = searchVar.Alloc();
			var.varName = targetVariableName.c_str();
			var.scriptVariable = nullptr;
			owner->GetType()->GetScriptVariables( owner, searchVar );
			targetVariable = var.scriptVariable;
		}


		Setup( owner );
		return idGraphNode::LoadBinary( file, _timeStamp, owner );
	}
	return false;
}

void idClassNode::Draw( ImGuiTools::GraphNode* nodePtr )
{
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

	if( auto* nodeOwner = node.Owner )
	{
		if( type == NodeType::Call )
		{
			eventDefs = ( ( idClassNode* )nodeOwner )->ownerClass->GetType()->GetEventDefs();
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
		int inputSocketIdx = 1, outputSocketIdx = 0;

		auto getVarWidth =
			[]( idScriptVariableBase * var ) -> int
		{
			etype_t type = var->GetType();

			switch( type )
			{
				default:
					return 1;
					break;
				case ev_string:
					return 20;
					break;
				case ev_float:
					return 10;
					break;
				case ev_vector:
					return 18;
					break;
				case ev_entity:
					return 20;
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
					maxLengthIn = idMath::Imax( maxLengthIn, getVarWidth( inpSocket.var ) );
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
					maxLengthOut = idMath::Imax( maxLengthOut, getVarWidth( outSocket.var ) );
				}
				else
				{
					maxLengthOut = idMath::Imax( maxLengthOut, outSocket.name.Length() );
				}
			}
		}

		ImGui::Dummy( ImVec2( 0, TEXT_BASE_HEIGHT ) );
		static ImGuiTableFlags flags = ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingStretchProp;
		if( ImGui::BeginTable( "NodeContent", 4, flags ) )
		{
			ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_IndentDisable, 25 );
			ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_IndentDisable, TEXT_BASE_WIDTH * maxLengthIn );
			ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_IndentDisable, TEXT_BASE_WIDTH * maxLengthOut );
			ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_IndentDisable, 25 );

			ImGui::TableNextRow( 0 );
			ImGui::PushID( maxSocket + 1 );
			ImGui::TableSetColumnIndex( 1 );
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
			ed::BeginPin( node.Inputs[0]->ID, ed::PinKind::Input );
			auto cursorPos = ImGui::GetCursorScreenPos();
			auto drawList = ImGui::GetWindowDrawList();
			ed::PinPivotAlignment( ImVec2( 0.25, 0.5f ) );
			ed::PinPivotSize( ImVec2( 0, 0 ) );
			ImGui::DrawIcon( ImGui::GetWindowDrawList(), cursorPos, cursorPos + ImVec2( 25, 25 ), ImGui::IconType::Flow, nodeOwner->inputSockets[0].connections.Num(), ImColor( 255, 255, 255 ), ImColor( 0, 0, 0, 0 ) );
			ImGui::Dummy( ImVec2( 25, 25 ) );
			ed::EndPin();
			ImGui::PopID();

			for( int i = 1; i <= maxSocket; i++ )
			{
				ImGui::TableNextRow( 0 );
				ImGui::PushID( i );
				if( inputSocketIdx < maxInputSockets )
				{

					ImGui::TableSetColumnIndex( 1 );
					idGraphNodeSocket& ownerSocket = nodeOwner->inputSockets[inputSocketIdx];
					ImGui::IconItem icon = { ImGui::IconType::Flow , ownerSocket.connections.Num() > 0, ImColor( 255, 255, 255 ), ImColor( 0, 0, 0, 0 ) };
					if( ownerSocket.var )
					{
						icon = ImGui::ImScriptVariable( idStr( reinterpret_cast<uintptr_t>( node.Inputs[inputSocketIdx]->ID.AsPointer() ) ), { ownerSocket.name.c_str(), "", ownerSocket.var } );
						icon.filled = ownerSocket.connections.Num() > 0;
					}
					else
					{
						ImGui::GetWindowDrawList()->AddText( ImGui::GetCursorScreenPos() + ImVec2( 0, ImGui::GetStyle().ItemInnerSpacing.y ),
															 ImColor( 50.0f, 45.0f, 255.0f, 255.0f ), ownerSocket.name.c_str() );
					}

					ImGui::TableSetColumnIndex( 0 );
					ed::BeginPin( node.Inputs[inputSocketIdx]->ID, ed::PinKind::Input );
					auto cursorPos = ImGui::GetCursorScreenPos();
					auto drawList = ImGui::GetWindowDrawList();
					ed::PinPivotAlignment( ImVec2( 0.25, 0.5f ) );
					ed::PinPivotSize( ImVec2( 0, 0 ) );
					ImGui::DrawIcon( ImGui::GetWindowDrawList(), cursorPos, cursorPos + ImVec2( 25, 25 ), icon.type, icon.filled, icon.color, icon.innerColor );
					ImGui::Dummy( ImVec2( 25, 25 ) );
					ed::EndPin();
					inputSocketIdx++;
				}
				if( outputSocketIdx < maxOutputSockets )
				{
					ImGui::TableSetColumnIndex( 2 );
					idGraphNodeSocket& ownerSocket = nodeOwner->outputSockets[outputSocketIdx];
					ImGui::IconItem icon = { ImGui::IconType::Flow , ownerSocket.connections.Num() > 0, ImColor( 255, 255, 255 ), ImColor( 0, 0, 0, 0 ) };
					if( ownerSocket.var )
					{
						icon = ImGui::ImScriptVariable( idStr( reinterpret_cast<uintptr_t>( node.Outputs[outputSocketIdx]->ID.AsPointer() ) ), { ownerSocket.name.c_str(), "", ownerSocket.var } );
						icon.filled = ownerSocket.connections.Num() > 0;
					}
					else
					{
						ImGui::GetWindowDrawList()->AddText( ImGui::GetCursorScreenPos() + ImVec2( 0, ImGui::GetStyle().ItemInnerSpacing.y ),
															 ImColor( 50.0f, 45.0f, 255.0f, 255.0f ), ownerSocket.name.c_str() );
					}

					ImGui::TableSetColumnIndex( 3 );
					ed::BeginPin( node.Outputs[outputSocketIdx]->ID, ed::PinKind::Output );
					auto cursorPos = ImGui::GetCursorScreenPos();
					auto drawList = ImGui::GetWindowDrawList();
					ed::PinPivotAlignment( ImVec2( 1.0f, 0.5f ) );
					ed::PinPivotSize( ImVec2( 0, 0 ) );
					ImGui::DrawIcon( ImGui::GetWindowDrawList(), cursorPos, cursorPos + ImVec2( 25, 25 ), icon.type,  icon.filled, icon.color, icon.innerColor );
					ImGui::Dummy( ImVec2( 25, 25 ) );
					ed::EndPin();
					outputSocketIdx++;
				}
				ImGui::PopID();
			}
			ImGui::EndTable();
		}

		float width = ( ImGui::GetItemRectMax() - ImGui::GetItemRectMin() ).x;
		idVec4 color = NodeTitleBarColor();
		ImColor titleBarColor = { color.x, color.y, color.z, color.w };
		ImGui::GetWindowDrawList()->AddRectFilled(
			ImVec2( cursorScreenPos.x - 4 - ImGui::GetStyle().ItemInnerSpacing.x, cursorScreenPos.y - ImGui::GetStyle().ItemInnerSpacing.y - 4 ),
			ImVec2( cursorScreenPos.x + width +  ImGui::GetStyle().ItemInnerSpacing.x, cursorScreenPos.y + TEXT_BASE_HEIGHT ),
			titleBarColor, 12, topRoundCornersFlags );
		ImGui::Dummy( ImVec2( 0, TEXT_BASE_HEIGHT ) );
		ImGui::GetWindowDrawList()->AddText( cursorScreenPos, ImColor( 50.0f, 45.0f, 255.0f, 255.0f ), GetName() );
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
		ImGui::BeginChild( "popup_scroller", ImVec2( 200, 200 ), true, ImGuiWindowFlags_AlwaysVerticalScrollbar );

		for( auto def : eventDefs )
		{
			if( ImGui::Button( def->GetName(), ImVec2( 180, 20 ) ) )
			{
				nodeOwnerClass = &ownerClass;
				nodePtr->dirty = true;
				OnChangeDef( def );
				popup_text = def->GetName();
				ImGui::CloseCurrentPopup();
			}
		}

		for( auto def : threadEventDefs )
		{
			if( ImGui::Button( def->GetName(), ImVec2( 180, 20 ) ) )
			{
				nodeOwnerClass = ( idClass** )&scriptThread;
				nodePtr->dirty = true;
				OnChangeDef( def );
				popup_text = def->GetName();
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::EndChild();
		ImGui::EndPopup(); // Note this does not do anything to the popup open/close state. It just terminates the content declaration.

	}

	if( ImGui::BeginPopup( "popup_varpicker" ) )
	{
		// Note: if it weren't for the child window, we would have to PushItemWidth() here to avoid a crash!
		ImGui::TextDisabled( "Pick One:" );
		ImGui::BeginChild( "popup_scroller", ImVec2( 200, 200 ), true, ImGuiWindowFlags_AlwaysVerticalScrollbar );

		for( auto& var : scriptVars )
		{
			if( ImGui::Button( var.varName, ImVec2( 180, 20 ) ) )
			{
				nodePtr->dirty = true;
				OnChangeVar( var );
				popup_text = var.varName;
				ImGui::CloseCurrentPopup();  // These calls revoke the popup open state, which was set by OpenPopup above.
			}
		}

		ImGui::EndChild();
		ImGui::EndPopup(); // Note this does not do anything to the popup open/close state. It just terminates the content declaration.
	}
	ImGui::PopItemWidth();
	ImGui::PopID();
	ed::Resume();
}
