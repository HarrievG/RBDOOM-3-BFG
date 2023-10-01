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
#include "script/Script_Program.h"
# define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

CLASS_DECLARATION( idClass, idStateGraph )
EVENT( EV_Activate, idStateGraph::Event_Activate )
EVENT( EV_Thread_Wait, idStateGraph::Event_Wait )
EVENT( EV_Thread_WaitFrame, idStateGraph::Event_WaitFrame )
END_CLASS

idList<int> idStateGraph::GraphThreadEventMap;
bool idStateGraph::GraphThreadEventMapInitDone = false;


void idStateGraph::Event_Wait( float time )
{
	localGraphState[GetLocalStateIndex( MAIN )].waitMS = time;
}

void idStateGraph::Event_WaitFrame()
{
	Event_Wait( 1 );
}

void idStateGraph::Event_Activate( idEntity* activator )
{
	for( auto& state : localGraphState )
	{
		for( auto* node : state.nodes )
		{
			node->ProcessEvent( &EV_Activate, activator );
		}
	}
}

idStateGraph::idStateGraph( idClass* Owner /* = nullptr*/ )
{
	auto& mainState = localGraphState[GetLocalStateIndex( MAIN )];
	mainState.stateThread = new rvStateThread();
	mainState.stateThread->SetOwner( Owner );

	if( !GraphThreadEventMapInitDone )
	{
		GraphThreadEventMap.Alloc() = EV_Thread_Wait.GetEventNum();
		GraphThreadEventMap.Alloc() = EV_Thread_WaitFrame.GetEventNum();
		GraphThreadEventMapInitDone = true;
	}
}

idStateGraph::~idStateGraph()
{
	delete localGraphState[0].stateThread;
}

void idStateGraph::Clear()
{
	//nodes.Clear();
	//links.Clear();
}

int idStateGraph::CreateSubState( const char* name, idList<idScriptVariableInstance_t> inputs, idList< idScriptVariableInstance_t> ouputs )
{
	int stateIndex = GetLocalStateIndex( name );
	auto* graphInputs = CreateStateNode( stateIndex, new idGraphInputOutputNode() );
	graphInputs->nodeType = idGraphInputOutputNode::NodeType::Input;

	for( auto& input : inputs )
	{
		auto& newInput = graphInputs->CreateOutputSocket();
		newInput.name = input.varName;
		newInput.var = input.scriptVariable;
	}

	auto* graphOutputs = CreateNode<idGraphInputOutputNode>( stateIndex );
	graphOutputs->nodeType = idGraphInputOutputNode::NodeType::Output;
	for( auto& input : inputs )
	{
		auto& newInput = graphOutputs->CreateInputSocket();
		newInput.name = input.varName;
		newInput.var = input.scriptVariable;
	}
	return stateIndex;
}

void idStateGraph::Think()
{
	auto& graph = localGraphState[0];

	graph.stateThread->SetOwner( this );
	if( graph.stateThread->IsIdle() )
	{
		graph.stateThread->SetState( "State_Update" );
	}
	graph.stateThread->Execute();
}

void idStateGraph::WriteBinary( idFile* file, ID_TIME_T* _timeStamp /*= NULL*/ )
{
	auto& currentGraphState = localGraphState[0];
	auto& nodes = currentGraphState.nodes;
	auto& links = currentGraphState.links;

	//First write all nodes and sockets.
	//as last all socket connections;
	if( file != nullptr )
	{
		file->WriteBig( nodes.Num() );

		for( idGraphNode* node : nodes )
		{
			idTypeInfo* c = node->GetType();
			file->WriteString( c->classname );
		}

		for( idGraphNode* node : nodes )
		{
			node->WriteBinary( file, _timeStamp );
		}

		file->WriteBig( links.Num() );
		for( idGraphNodeSocket::Link_t& link : links )
		{
			file->WriteBig( link.start->nodeIndex );
			file->WriteBig( link.start->socketIndex );
			file->WriteBig( link.end->nodeIndex );
			file->WriteBig( link.end->socketIndex );
		}
	}
}

bool idStateGraph::LoadBinary( idFile* file, const ID_TIME_T _timeStamp, idClass* owner )
{
	Clear();
	if( file != nullptr )
	{
		auto& currentGraphState = localGraphState[0];
		auto& nodes = currentGraphState.nodes;
		auto& links = currentGraphState.links;

		int totalNodes;
		file->ReadBig( totalNodes );
		//create nodes
		for( int i = 0; i < totalNodes; i++ )
		{
			idStr className;
			file->ReadString( className );
			auto* typeInfo = idClass::GetClass( className );
			auto* newNode = CreateNode( static_cast<idGraphNode*>( typeInfo->CreateInstance() ) );
		}
		//load data
		for( auto* node : nodes )
		{
			if( !node->LoadBinary( file, _timeStamp, owner ) )
			{
				return false;
			}
		}

		int totalLinks;
		file->ReadBig( totalLinks );
		for( int i = 0; i < totalLinks; i++ )
		{
			int startIdx, endIdx;
			int startSocketIdx, endSocketIdx;
			file->ReadBig( startIdx );
			file->ReadBig( startSocketIdx );
			file->ReadBig( endIdx );
			file->ReadBig( endSocketIdx );
			AddLink( nodes[startIdx]->outputSockets[startSocketIdx], nodes[endIdx]->inputSockets[endSocketIdx] );
		}
		return true;
	}
	return false;
}

idList<idScriptVariableInstance_t>& idStateGraph::GetVariables()
{
	return GetLocalVariables();
}

idScriptVariableInstance_t& idStateGraph::CreateVariable( const char* variableName, etype_t type )
{
	int stateIndex = GetLocalStateIndex( MAIN );
	GraphState& stateNodes = localGraphState[stateIndex];

	auto& newInstance = stateNodes.localVariables.Alloc();

	newInstance.varName = variableName;
	newInstance.scriptVariable = VarFromType( type, &stateNodes );
	return newInstance;
}

void idStateGraph::RemoveNode( idGraphNode* node )
{
	DeleteLocalStateNode( MAIN , node );
}

idGraphNode* idStateGraph::CreateNode( idGraphNode* node )
{
	return CreateLocalStateNode( MAIN , node );
}

void idStateGraph::RemoveLink( idGraphNodeSocket* start, idGraphNodeSocket* end )
{
	auto& currentGraphState = localGraphState[0];
	int idx = 0;
	for( auto& link : currentGraphState.links )
	{
		if( link.start == start && link.end == end )
		{
			int lastIndex = start->connections.Num() - 1;
			for( int i = 0; i < start->connections.Num(); i++ )
			{
				if( start->connections[i] == end )
				{
					start->connections.RemoveIndexFast( i );
				}
			}
			lastIndex = end->connections.Num() - 1;
			for( int i = 0; i < end->connections.Num(); i++ )
			{
				if( end->connections[i] == start )
				{
					end->connections.RemoveIndexFast( i );
				}
			}
			currentGraphState.links.RemoveIndexFast( idx );
			return;
		}
		idx++;
	}
}

idGraphNodeSocket::Link_t& idStateGraph::AddLink( idGraphNodeSocket& start, idGraphNodeSocket& end )
{
	auto& currentGraphState = localGraphState[0];

	return currentGraphState.links.Alloc() =
	{
		end.connections.Alloc() = &start,
		start.connections.Alloc() = &end
	};
}

stateResult_t idStateGraph::State_Update( stateParms_t* parms )
{
	auto& currentGraphState = localGraphState[parms->stage];

	auto& currentNodes = currentGraphState.nodes;
	auto& currentActiveNodes = currentGraphState.activeNodes;

	for( idGraphNode* node : currentNodes )
	{
		if( node->HasActiveSocket() )
		{
			currentActiveNodes.Alloc() = node;
		}
	}

	if( currentActiveNodes.Num() )
	{
		currentGraphState.stateThread->PostState( "State_Exec", 0, currentGraphState.waitMS );
		return SRESULT_DONE;
	}

	return SRESULT_WAIT;
}

stateResult_t idStateGraph::State_Exec( stateParms_t* parms )
{
	auto& currentGraphState = localGraphState[parms->stage];
	auto& currentActiveNodes = currentGraphState.activeNodes;
	auto lastActiveNodes = currentActiveNodes;

	stateResult_t result = SRESULT_DONE;
	idList<idGraphNode*> waitingNodes;

	for( idGraphNode* node : currentActiveNodes )
	{
		if( !parms->substage )
		{
			stateResult_t nodeResult = node->Exec( parms );
			if( nodeResult == SRESULT_WAIT )
			{
				waitingNodes.Alloc() = node;
			}
			node->DeactivateInputs();
		}
		else
		{
			node->outputSockets[0].active = true;
			node->outputSockets[0].lastActivated = gameLocal.GetTime();
			parms->substage = 0;
		}
		if( currentGraphState.waitMS )
		{
			currentActiveNodes = lastActiveNodes;
			parms->Wait( ( 1.0f / 1000.0f ) * currentGraphState.waitMS );
			currentGraphState.waitMS = 0.0f;
			parms->substage = 1;
			node->outputSockets[0].active = false;
			return SRESULT_WAIT;
		}
		node->ActivateOutputConnections();
		lastActiveNodes.RemoveIndex( 0 );
	}

	currentActiveNodes = waitingNodes;

	if( !currentActiveNodes.Num() )
	{
		currentGraphState.stateThread->PostState( "State_Update" );
		return SRESULT_DONE;
	}

	return SRESULT_WAIT;
}

stateResult_t idStateGraph::State_LocalExec( stateParms_t* parms )
{
	return SRESULT_ERROR;
}

int idStateGraph::GetLocalStateIndex( const char* stateName )
{
	int i, hash;

	hash = localStateHash.GenerateKey( stateName );
	for( i = localStateHash.First( hash ); i != -1; i = localStateHash.Next( i ) )
	{
		if( localStates[i].Cmp( stateName ) == 0 )
		{
			return i;
		}
	}

	i = localStates.Append( stateName );
	localStateHash.Add( hash, i );
	auto& newState = localGraphState.Alloc();
	newState.graph = this;
	newState.waitMS = 0.0f;

	return i;

}

GraphState* idStateGraph::GetLocalState( const char* stateName )
{
	int hashIndex, hash;

	hash = localStateHash.GenerateKey( stateName );
	hashIndex = localStateHash.First( hash );
	if( hashIndex == -1 )
	{
		return nullptr;
	}

	return &localGraphState[hashIndex];
}

void idStateGraph::DeleteLocalStateNode( int stateIndex, idGraphNode* node )
{
	GraphState& graphState = localGraphState[stateIndex];
	assert( !graphState.activeNodes.Num() );
	idGraphNode* lastNode = graphState.nodes[graphState.nodes.Num() - 1];
	int nodeIdx = node->nodeIndex;
	node->nodeIndex = lastNode->nodeIndex;
	lastNode->nodeIndex = nodeIdx;
	for( auto& socket : lastNode->inputSockets )
	{
		socket.nodeIndex = nodeIdx;
	}
	for( auto& socket : lastNode->outputSockets )
	{
		socket.nodeIndex = nodeIdx;
	}
	for( auto& socket : node->outputSockets )
	{
		socket.nodeIndex = node->nodeIndex;
	}
	for( auto& socket : node->inputSockets )
	{
		socket.nodeIndex = node->nodeIndex;
	}
	delete node;
	graphState.nodes.RemoveIndexFast( nodeIdx );
}

void idStateGraph::DeleteLocalStateNode( const char* stateName, idGraphNode* node )
{
	int stateIndex = GetLocalStateIndex( stateName );
	DeleteLocalStateNode( stateIndex, node );
}

idGraphNode* idStateGraph::CreateLocalStateNode( int stateIndex, idGraphNode* node )
{
	GraphState& localState = localGraphState[stateIndex];

	node->graphState = &localState;
	auto* retNode = localState.nodes.Alloc() = node;
	node->nodeIndex = localState.nodes.Num() - 1;
	return retNode;
}

idGraphNode* idStateGraph::CreateLocalStateNode( const char* stateName, idGraphNode* node )
{
	int stateIndex = GetLocalStateIndex( stateName );
	return CreateLocalStateNode( stateIndex, node );
}

idList<idScriptVariableInstance_t>& idStateGraph::GetLocalVariables()
{
	int stateIndex = GetLocalStateIndex( MAIN );
	return localGraphState[stateIndex].localVariables;
}

template<class T>
T* idStateGraph::CreateStateNode( int stateIndex, T* node )
{
	return static_cast<T*>( CreateLocalStateNode( stateIndex, node ) );
}

template<class T>
T* idStateGraph::CreateNode( int stateIndex )
{
	return static_cast<T*>( CreateLocalStateNode( stateIndex, new T() ) );
}

void idGraphNode::WriteBinary( idFile* file, ID_TIME_T* _timeStamp /*= NULL */ )
{
	if( file )
	{
		assert( graphState );

		auto writeSockets =
			[]( idFile * file, idList<idGraphNodeSocket>& socketList )
		{
			file->WriteBig( socketList.Num() );
			for( auto& socket : socketList )
			{
				file->WriteInt( socket.var ? socket.var->GetType() : ev_void );
				bool writeSocketData = socket.var && ( socket.freeData && !socket.connections.Num() );
				file->WriteBool( writeSocketData );
				if( writeSocketData )
				{
					switch( socket.var->GetType() )
					{
						case ev_string:
							file->WriteString( *( ( idScriptStr* )socket.var )->GetData() );
							break;
						case ev_float:
							file->WriteFloat( *( ( idScriptFloat* )socket.var )->GetData() );
							break;
						case ev_vector:
							file->WriteVec3( *( ( idScriptVector* )socket.var )->GetData() );
							break;
						case ev_entity:
						{
							gameLocal.Warning( "idClassNode::WriteBinary : ev_entity not implemented" );
							//idEntityPtr<idEntity> tmpEntPtr;
							//tmpEntPtr = (*((idScriptEntity*)socket.var)->GetData());
							//file->WriteInt(tmpEntPtr.GetSpawnId());
						}
						break;
						case ev_int:
							file->WriteInt( *( ( idScriptInteger* )socket.var )->GetData() );
							break;
						case ev_boolean:
							file->WriteBool( *( ( idScriptBool* )socket.var )->GetData() );
						default:
							gameLocal.Warning( "idClassNode::WriteBinary : Invalid arg format '%s' ", idTypeInfo::GetEnumTypeInfo( "etype_t", socket.var->GetType() ) );
							break;
					}
				}
			}
		};

		writeSockets( file, inputSockets );
		writeSockets( file, outputSockets );
	}
}

bool idGraphNode::LoadBinary( idFile* file, const ID_TIME_T _timeStamp, idClass* owner )
{
	if( file )
	{
		assert( graphState );
		auto readSockets =
			[this]( idFile * file, idList<idGraphNodeSocket>& socketList, bool isInput )
		{
			int numSockets;
			file->ReadBig( numSockets );
			for( auto& socket : socketList )
			{
				socket.owner = this;
				int typeNum;
				file->ReadInt( typeNum );
				bool readData;
				file->ReadBool( readData );
				if( typeNum != ev_void )
				{
					switch( typeNum )
					{
						case ev_string:
						{
							if( readData )
							{
								file->ReadString( *static_cast<idScriptStr*>( socket.var )->GetData() );
							}
						}
						break;
						case ev_float:
						{
							if( readData )
							{
								file->ReadFloat( *static_cast<idScriptFloat*>( socket.var )->GetData() );
							}
						}
						break;
						case ev_vector:
						{
							if( readData )
							{
								file->ReadVec3( *static_cast<idScriptVector*>( socket.var )->GetData() );
							}
						}
						break;
						//case ev_object:
						//	//classtype?
						//	break;
						//case ev_entity:
						//	//entity nr?
						//	//ret = graph->blackBoard.Alloc<idScriptEntity>(16);
						//	break;
						case ev_boolean:
						{
							if( readData )
							{
								bool readVar;
								file->ReadBool( readVar );
								*static_cast<idScriptBool*>( socket.var ) = readVar;
							}
						}
						break;
						case ev_int:
						{
							if( readData )
							{
								file->ReadInt( *static_cast<idScriptInteger*>( socket.var )->GetData() );
							}
						}
						break;
						default:
							gameLocal.Warning( "idGraphNode::LoadBinary : Invalid arg format '%s' ", idTypeInfo::GetEnumTypeInfo( "etype_t", typeNum ) );
							break;
					};
				}
			}
		};

		readSockets( file, inputSockets, true );
		readSockets( file, outputSockets, false );
	}
	return true;
}

idGraphNodeSocket& idGraphNode::CreateSocket( idList<idGraphNodeSocket>& socketList )
{
	idGraphNodeSocket& ret = socketList.Alloc();
	ret.socketIndex = socketList.Num() - 1;
	ret.nodeIndex = this->nodeIndex;
	ret.owner = this;
	return ret;
}

//Move out of this file and only compile with editor tools.
void idGraphNode::Draw( ImGuiTools::GraphNode* nodePtr )
{
	auto& node = *nodePtr;
	namespace ed = ax::NodeEditor;
	using namespace ImGuiTools;
	ImGui::PushID( node.ID.Get() );
	ed::BeginNode( node.ID );
	ImGui::AlignTextToFramePadding();
	if( ImGui::IsItemVisible() )
	{
		if( auto* owner = node.Owner )
		{
			idStr nodeType = owner->GetName();

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
			int inputSocketIdx = 0, outputSocketIdx = 0;

			auto getVarWidth =
				[]( idScriptVariableBase * var ) -> int
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
						return 18;
						break;
					case ev_string:
					case ev_object:
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

			int maxLengthIn = 1;
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
			if( ImGui::IsItemVisible() && ImGui::BeginTable( "NodeContent", 4, flags ) )
			{
				ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_IndentDisable, 25 );
				ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_IndentDisable, TEXT_BASE_WIDTH * maxLengthIn );
				ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_IndentDisable, TEXT_BASE_WIDTH * maxLengthOut );
				ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_IndentDisable, 25 );

				for( int i = 0; i < maxSocket; i++ )
				{
					ImGui::TableNextRow( 0 );
					ImGui::PushID( i );
					if( inputSocketIdx <= maxInputSockets - 1 )
					{
						ImGui::TableSetColumnIndex( 1 );
						idGraphNodeSocket& ownerSocket = owner->inputSockets[inputSocketIdx];
						ImGui::IconItem icon = { ImGui::IconType::Flow , ownerSocket.connections.Num() > 0, ImColor( 255, 255, 255 ), ImColor( 0, 0, 0, 0 ) };
						if( ownerSocket.var )
						{
							icon = ImGui::ImScriptVariable( idStr( reinterpret_cast<uintptr_t>( node.Inputs[inputSocketIdx]->ID.AsPointer() ) ), { ownerSocket.name.c_str(), ownerSocket.var } );
							icon.filled = ownerSocket.connections.Num() > 0;
						}
						else
						{
							ImGui::GetWindowDrawList()->AddText( ImGui::GetCursorScreenPos() + ImVec2( 0, ImGui::GetStyle().ItemInnerSpacing.y ),
																 ImColor( 50.0f, 45.0f, 255.0f, 255.0f ), ownerSocket.name.c_str() );
						}

						ImGui::PushID( node.Inputs[inputSocketIdx]->ID.Get() );
						ImGui::TableSetColumnIndex( 0 );
						ed::BeginPin( node.Inputs[inputSocketIdx]->ID, ed::PinKind::Input );
						auto cursorPos = ImGui::GetCursorScreenPos();
						ed::PinPivotAlignment( ImVec2( 0.25, 0.5f ) );
						ed::PinPivotSize( ImVec2( 0, 0 ) );
						if( node.drawList )
						{
							ImGui::DrawIcon( node.drawList, cursorPos, cursorPos + ImVec2( 25, 25 ), icon.type, icon.filled, icon.color, icon.innerColor );
						}
						ImGui::Dummy( ImVec2( 25, 25 ) );
						ed::EndPin();
						ImGui::PopID();
						inputSocketIdx++;
					}
					if( outputSocketIdx <= maxOutputSockets - 1 )
					{
						ImGui::TableSetColumnIndex( 2 );
						idGraphNodeSocket& ownerSocket = owner->outputSockets[outputSocketIdx];
						ImGui::IconItem icon = { ImGui::IconType::Flow , ownerSocket.connections.Num() > 0, ImColor( 255, 255, 255 ), ImColor( 0, 0, 0, 0 ) };
						if( ownerSocket.var )
						{
							icon = ImGui::ImScriptVariable( idStr( reinterpret_cast<uintptr_t>( node.Outputs[outputSocketIdx]->ID.AsPointer() ) ), { ownerSocket.name.c_str(), ownerSocket.var } );
							icon.filled = ownerSocket.connections.Num() > 0;
						}
						else
						{
							ImGui::GetWindowDrawList()->AddText( ImGui::GetCursorScreenPos() + ImVec2( 0, ImGui::GetStyle().ItemInnerSpacing.y ),
																 ImColor( 50.0f, 45.0f, 255.0f, 255.0f ), ownerSocket.name.c_str() );
						}
						ImGui::PushID( node.Outputs[outputSocketIdx]->ID.Get() );
						ImGui::TableSetColumnIndex( 3 );
						ed::BeginPin( node.Outputs[outputSocketIdx]->ID, ed::PinKind::Output );
						auto cursorPos = ImGui::GetCursorScreenPos();
						ed::PinPivotAlignment( ImVec2( 1.0f, 0.5f ) );
						ed::PinPivotSize( ImVec2( 0, 0 ) );

						if( node.drawList )
						{
							ImGui::DrawIcon( node.drawList, cursorPos, cursorPos + ImVec2( 25, 25 ), icon.type, icon.filled, icon.color, icon.innerColor );
						}
						ImGui::Dummy( ImVec2( 25, 25 ) );
						ed::EndPin();
						ImGui::PopID();
						outputSocketIdx++;
					}
					ImGui::PopID();
				}
				ImGui::EndTable();
			}
			//auto drawListX = ed::GetNodeBackgroundDrawList(nodePtr->ID.Get());
			float width = ( ImGui::GetItemRectMax() - ImGui::GetItemRectMin() ).x;
			idVec4 color = NodeTitleBarColor();
			ImColor titleBarColor = { color.x, color.y, color.z, color.w };
			if( ImGui::IsItemVisible() )
			{
				if( node.drawList )
					node.drawList->AddRectFilled(
						ImVec2( cursorScreenPos.x - 4 - ImGui::GetStyle().ItemInnerSpacing.x, cursorScreenPos.y - ImGui::GetStyle().ItemInnerSpacing.y - 4 ),
						ImVec2( cursorScreenPos.x + width + ImGui::GetStyle().ItemInnerSpacing.x, cursorScreenPos.y + TEXT_BASE_HEIGHT ),
						titleBarColor, 12, topRoundCornersFlags );
				ImGui::Dummy( ImVec2( 0, TEXT_BASE_HEIGHT ) );
				ImGui::GetWindowDrawList()->AddText( cursorScreenPos, ImColor( 50.0f, 45.0f, 255.0f, 255.0f ), GetName() );
			}
		}
	}
	ImGui::PopID();
	ed::EndNode();
}

idVec4 idGraphNode::NodeTitleBarColor()
{
	ImVec4 color = ImGui::GetStyle().Colors[ImGuiCol_HeaderActive];
	return idVec4( color.x, color.y, color.z, color.w );
	//return idVec4(color.Value.x, color.Value.y, color.Value.z, color.Value.w) / 255.0f;
}

idGraphNodeSocket& idGraphNode::CreateInputSocket()
{
	return CreateSocket( inputSockets );
}

idGraphNodeSocket& idGraphNode::CreateOutputSocket()
{
	auto& ret = CreateSocket( outputSockets );
	ret.isOutput = true;
	return ret;
}

//////////////////////////////////////////////////////////////////////////

ABSTRACT_DECLARATION( idClass, idGraphNode )
END_CLASS

bool idGraphNode::HasActiveSocket()
{
	for( idGraphNodeSocket& input : inputSockets )
	{
		if( input.active )
		{
			return true;
		}
	}

	for( idGraphNodeSocket& output : outputSockets )
	{
		if( output.active && output.connections.Num() )
		{
			return true;
		}
	}

	return false;
}

void idGraphNode::ActivateOutputConnections()
{
	for( idGraphNodeSocket& output : outputSockets )
	{
		if( output.active )
		{
			output.lastActivated = gameLocal.GetTime();
			for( idGraphNodeSocket* input : output.connections )
			{
				input->active = true;
				input->lastActivated = gameLocal.GetTime();
			}
			output.active = false;
		}
	}
}

void idGraphNode::DeactivateInputs()
{
	for( idGraphNodeSocket& input : inputSockets )
	{
		input.active = false;
	}
}

//////////////////////////////////////////////////////////////////////////
#include "d3xp/StateGraphNodes.h"
//////////////////////////////////////////////////////////////////////////
/// TEST ENTITY
CLASS_DECLARATION( idEntity, idGraphedEntity )
EVENT( EV_Activate, idGraphedEntity::Event_Activate )
END_CLASS

void idGraphedEntity::Spawn()
{
	idStr graphFile = spawnArgs.GetString( "graphObject", "" );

	if( graphObject )
	{
		auto& localState = graphObject->localGraphState[0];
		auto& localBlackboard = localState.blackBoard;

		idScriptVariableBase* test = &varStringTest;
		idScriptVariableBase* test2 = &varStringTestX;
		//if not linked to a script, use blackboard as storage

		varFloatTest.SetRawData( localBlackboard.Alloc<idScriptFloat>( 8 )->GetRawData() );
		varFloatTest = 3.1427f;
		varBoolTest.SetRawData( localBlackboard.Alloc<idScriptInt>( 8 )->GetRawData() );
		varBoolTest = 1;
		varIntTest.SetRawData( localBlackboard.Alloc<idScriptInt>( 8 )->GetRawData() );
		varIntTest = 31427;
		varStringTest.SetRawData( localBlackboard.Alloc( "StringVariableTest" )->GetRawData() );
		( varVectorTest = localBlackboard.Alloc<idScriptVector>( 24 ) ) = idVec3( 1.1f, 2.3f, 3.3f );

		varFloatTestX.SetRawData( localBlackboard.Alloc<idScriptFloat>( 8 )->GetRawData() );
		varFloatTestX = 3.1427f * 2;
		varBoolTestX.SetRawData( localBlackboard.Alloc<idScriptInt>( 8 )->GetRawData() );
		varBoolTestX = 0;
		varIntTestX.SetRawData( localBlackboard.Alloc<idScriptInt>( 8 )->GetRawData() );
		varIntTestX = 31427 * 2;
		varStringTestX.SetRawData( localBlackboard.Alloc( "StringVariableTestX" )->GetRawData() );
		varVectorTestX.SetRawData( localBlackboard.Alloc<idScriptVector>( 24 )->GetRawData() );
		varVectorTestX = idVec3( 1.1f, 2.3f, 3.3f );


		if( graphFile.IsEmpty() )
		{
			auto* initNode = graphObject->CreateNode( new idGraphOnInitNode() );
			initNode->Setup( this );

			//auto* stateNode = static_cast<idStateNode*>(graphObject->CreateNode(new idStateNode()));
			//stateNode->stateThread = &graphStateThread;
			//stateNode->input_State = "State_Idle";
			//stateNode->type = idStateNode::NodeType::Set;
			//stateNode->Setup(this);

			auto* classNode = static_cast<idClassNode*>( graphObject->CreateNode( new idClassNode() ) );
			classNode->type = idClassNode::Call;
			classNode->Setup( this );

			auto* classNodeVarSet = static_cast<idClassNode*>( graphObject->CreateNode( new idClassNode() ) );
			classNodeVarSet->type = idClassNode::Set;
			classNodeVarSet->Setup( this );

			auto* classNodeVarGet = static_cast<idClassNode*>( graphObject->CreateNode( new idClassNode() ) );
			classNodeVarGet->type = idClassNode::Get;
			classNodeVarGet->Setup( this );
		}
	}


	BecomeInactive( TH_THINK );
}

void idGraphedEntity::Think()
{
	if( thinkFlags & TH_THINK )
	{
		idEntity::Think();
	}
}

stateResult_t idGraphedEntity::State_Idle( stateParms_t* parms )
{
	common->Printf( "%s Im Idling! %i\n ", name.c_str(), gameLocal.GetTime() );
	return SRESULT_WAIT;
}

void idGraphedEntity::Event_Activate( idEntity* activator )
{
	if( graphObject )
	{
		graphObject->Event_Activate( activator );
	}
	BecomeActive( TH_THINK );
}

idGraphNodeSocket& idGraphNodeSocket::operator=( idGraphNodeSocket&& other )
{
	connections = std::move( other.connections );
	owner = other.owner;
	var = other.var;
	active = other.active;
	name = std::move( other.name );
	socketIndex = other.socketIndex;
	nodeIndex = other.nodeIndex;
	freeData = other.freeData;
	isOutput = other.isOutput;
	lastActivated = other.lastActivated;

	other.owner = nullptr;
	other.var = nullptr;
	other.lastActivated = -1;

	return *this;
}

idGraphNodeSocket::~idGraphNodeSocket()
{
	if( var && !connections.Num() )
	{
		if( freeData )
		{
			etype_t t = var->GetType();

			if( t == ev_string )
			{
				owner->graphState->blackBoard.Free( ( idStr* )var->GetRawData() );
				idStr* data = ( ( idScriptStr* )var )->GetData();
				data->FreeData();
				delete data;
			}
			else if( t != ev_entity || t == ev_object )
			{
				owner->graphState->blackBoard.Free( var->GetRawData() );
			}
			delete var;
		}
	}
	else if( var )
	{
		common->Warning( "SOCKET VAR LEAKED!!" );
	}

}

idScriptVariableBase* VarFromFormatSpec( const char spec, GraphState* graphState )
{
	idScriptVariableBase* ret = nullptr;

	switch( spec )
	{
		case D_EVENT_FLOAT:
			ret = graphState->blackBoard.Alloc<idScriptFloat>( 8 );
			break;
		case D_EVENT_INTEGER:
			ret = graphState->blackBoard.Alloc<idScriptInteger>( 8 );
			break;

		case D_EVENT_VECTOR:
			ret = graphState->blackBoard.Alloc<idScriptVector>( 3 * 8 );
			break;

		case D_EVENT_STRING:
			ret = graphState->blackBoard.Alloc( "" );
			break;

		case D_EVENT_ENTITY:
		case D_EVENT_ENTITY_NULL:
			ret = graphState->blackBoard.Alloc<idScriptEntity>( 8 );
			break;
		case D_EVENT_VOID:
			//no var but valid
			break;
		default:
			gameLocal.Error( "idClassNode::Setup : Invalid arg format '%s' string for event.", spec );
			break;
	}
	return ret;
}

idScriptVariableBase* VarFromType( etype_t type, GraphState* graphState )
{
	idScriptVariableBase* ret = nullptr;
	switch( type )
	{
		case ev_string:
			ret = graphState->blackBoard.Alloc( "" );
			break;
		case ev_float:
			ret = graphState->blackBoard.Alloc<idScriptFloat>( 8 );
			break;
		case ev_vector:
			ret = graphState->blackBoard.Alloc<idScriptVector>( 3 * 8 ) ;
			break;
		case ev_object:
		case ev_entity:
			ret = graphState->blackBoard.Alloc<idScriptEntity>( 8 );
			break;
		case ev_boolean:
		case ev_int:
			ret = graphState->blackBoard.Alloc<idScriptInteger>( 8 );
			break;
		default:
			gameLocal.Error( "idClassNode::Setup : Invalid arg format '%s' for event.", idTypeInfo::GetEnumTypeInfo( "etype_t", type ) );
			break;
	}

	return ret;
}

bool VarCopy( idScriptVariableBase* target, idScriptVariableBase* source )
{
	switch( target->GetType() )
	{
		case ev_string:
			*( ( idScriptStr* )target )->GetData() = ( ( idScriptStr* )source )->GetData()->c_str();
			break;
		case ev_float:
			*( idScriptFloat* )target = *( ( idScriptFloat* )source )->GetData();
			break;
		case ev_vector:
			*( idScriptVector* )target = *( ( idScriptVector* )source )->GetData();
			break;
		case ev_object:
		case ev_entity:
		{
			target->SetRawData( source->GetRawData() );
		}
		break;
		case ev_int:
			*( idScriptInteger* )target = *( ( idScriptInteger* )source )->GetData();
			break;
		case ev_boolean:
			*( idScriptBool* )target = *( ( idScriptBool* )source )->GetData();
		default:
			gameLocal.Warning( "idClassNode::Setup : Invalid arg format '%s' string for event.", idTypeInfo::GetEnumTypeInfo( "etype_t", target->GetType() ) );
			return false;
			break;
	};
	return true;
}