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
END_CLASS

idStateGraph::idStateGraph()
{
	GetLocalState( "GRAPH_MAIN" );
	localGraphState[0].stateThread = new rvStateThread();
	localGraphState[0].owner = nullptr;
}

idStateGraph::~idStateGraph()
{
	if( !localGraphState[0].owner )
	{
		delete localGraphState[0].stateThread;
	}
}

void idStateGraph::Clear()
{
	//nodes.Clear();
	//links.Clear();
}

int idStateGraph::CreateSubState( const char* name, idList<idScriptVariableInstance_t> inputs, idList< idScriptVariableInstance_t> ouputs )
{
	int stateIndex = GetLocalState( name );
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

void idStateGraph::SharedThink()
{
	auto& graph = localGraphState[0];
	if( !graph.owner )
	{
		graph.stateThread->SetOwner( this );
		if(	graph.stateThread->IsIdle() )
		{
			graph.stateThread->SetState( "State_Update" );
		}
		graph.stateThread->Execute();
	}
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

void idStateGraph::RemoveNode( idGraphNode* node )
{
	DeleteLocalStateNode( "GRAPH_MAIN", node );
}

idGraphNode* idStateGraph::CreateNode( idGraphNode* node )
{
	return CreateLocalStateNode( "GRAPH_MAIN", node );
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
					start->connections[lastIndex]->socketIndex = i;
					start->connections.RemoveIndexFast( i );
				}
			}
			lastIndex = end->connections.Num() - 1;
			for( int i = 0; i < end->connections.Num(); i++ )
			{
				if( end->connections[i] == start )
				{
					end->connections[lastIndex]->socketIndex = i;
					end->connections.RemoveIndexFast( i );
				}
			}
			currentGraphState.links.RemoveIndexFast( idx );
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
		currentGraphState.stateThread->PostState( "State_Exec" );
		return SRESULT_DONE;
	}

	return SRESULT_WAIT;
}

stateResult_t idStateGraph::State_Exec( stateParms_t* parms )
{
	auto& currentGraphState = localGraphState[parms->stage];
	auto& currentActiveNodes = currentGraphState.activeNodes;

	stateResult_t result = SRESULT_DONE;
	idList<idGraphNode*> waitingNodes;

	for( idGraphNode* node : currentActiveNodes )
	{
		stateResult_t nodeResult = node->Exec( parms );
		if( nodeResult == SRESULT_WAIT )
		{
			waitingNodes.Alloc() = node;
		}
		node->ActivateOutputConnections();
		node->DeactivateInputs();
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

int idStateGraph::GetLocalState( const char* newStateName )
{
	int i, hash;

	hash = localStateHash.GenerateKey( newStateName );
	for( i = localStateHash.First( hash ); i != -1; i = localStateHash.Next( i ) )
	{
		if( localStates[i].Cmp( newStateName ) == 0 )
		{
			return i;
		}
	}

	i = localStates.Append( newStateName );
	localStateHash.Add( hash, i );
	localGraphState.Alloc();

	return i;

}

void idStateGraph::DeleteLocalStateNode( int stateIndex, idGraphNode* node )
{
	GraphState& graphState = localGraphState[stateIndex];
	assert( !graphState.activeNodes.Num() );
	idGraphNode* lastNode = graphState.nodes[graphState.nodes.Num() - 1];
	int nodeIdx = node->nodeIndex;
	lastNode->nodeIndex = nodeIdx;
	for( auto& socket : lastNode->inputSockets )
	{
		socket.nodeIndex = nodeIdx;
	}
	for( auto& socket : lastNode->outputSockets )
	{
		socket.nodeIndex = nodeIdx;
	}
	graphState.nodes.RemoveIndexFast( nodeIdx );
	delete node;
}

void idStateGraph::DeleteLocalStateNode( const char* stateName, idGraphNode* node )
{
	int stateIndex = GetLocalState( stateName );
	DeleteLocalStateNode( stateIndex, node );
}

idGraphNode* idStateGraph::CreateLocalStateNode( int stateIndex, idGraphNode* node )
{
	GraphState& stateNodes = localGraphState[stateIndex];

	node->graph = &stateNodes;
	auto* retNode = stateNodes.nodes.Alloc() = node;
	node->nodeIndex = stateNodes.nodes.Num() - 1;
	return retNode;
}

idGraphNode* idStateGraph::CreateLocalStateNode( const char* stateName, idGraphNode* node )
{
	int stateIndex = GetLocalState( stateName );
	return CreateLocalStateNode( stateIndex, node );
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
		assert( graph );

		auto writeSockets =
			[]( idFile * file, idList<idGraphNodeSocket>& socketList )
		{
			file->WriteBig( socketList.Num() );
			for( auto& socket : socketList )
			{
				file->WriteBig( socket.connections.Num() );
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
		assert( graph );
		auto readSockets =
			[this]( idFile * file, idList<idGraphNodeSocket>& socketList, bool isInput )
		{
			int numSockets;
			file->ReadBig( numSockets );
			for( auto& socket : socketList )
			{
				socket.owner = this;
				int numConnections;
				file->ReadBig( numConnections );
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
	ed::BeginNode( node.ID );
	ImGui::PushID( node.ID.Get() );
	ImGui::AlignTextToFramePadding();

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
		static ImGuiTableFlags flags = ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingStretchProp ;
		if( ImGui::BeginTable( "NodeContent", 4, flags ) )
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
				if( outputSocketIdx <= maxOutputSockets - 1 )
				{
					ImGui::TableSetColumnIndex( 2 );
					idGraphNodeSocket& ownerSocket = owner->outputSockets[outputSocketIdx];
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
					ImGui::DrawIcon( ImGui::GetWindowDrawList(), cursorPos, cursorPos + ImVec2( 25, 25 ), icon.type, icon.filled, icon.color, icon.innerColor );
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
			ImVec2( cursorScreenPos.x + width + ImGui::GetStyle().ItemInnerSpacing.x, cursorScreenPos.y + TEXT_BASE_HEIGHT ),
			titleBarColor, 12, topRoundCornersFlags );
		ImGui::Dummy( ImVec2( 0, TEXT_BASE_HEIGHT ) );
		ImGui::GetWindowDrawList()->AddText( cursorScreenPos, ImColor( 50.0f, 45.0f, 255.0f, 255.0f ), GetName() );
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
			for( idGraphNodeSocket* input : output.connections )
			{
				input->active = true;
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
	idStr graphFile = spawnArgs.GetString( "graph", "" );

	stateThread.SetOwner( this );
	auto& localState = graph.localGraphState[0];
	localState.targetStateThread = &stateThread;
	auto& localBlackboard = localState.blackBoard;

	//if not linked to a script, use blackboard as storage
	( varFloatTest = *localBlackboard.Alloc<idScriptFloat>( 8 ) ) = 3.1427f;
	( varBoolTest =	localBlackboard.Alloc<idScriptInt>( 8 ) ) = 1;
	( varIntTest =	localBlackboard.Alloc<idScriptInt>( 8 ) ) = 31427;
	varStringTest = ( idScriptStr ) * localBlackboard.Alloc( "StringVariableTest" );
	( varVectorTest = localBlackboard.Alloc<idScriptVector>( 24 ) ) = idVec3( 1.1f, 2.3f, 3.3f );
	( varFloatTestX = localBlackboard.Alloc<idScriptFloat>( 8 ) ) = 3.1427f * 2;
	( varBoolTestX =	localBlackboard.Alloc<idScriptInt>( 8 ) ) = 10;
	( varIntTestX =	localBlackboard.Alloc<idScriptInt>( 8 ) ) = 31427 * 2;
	varStringTestX = ( idScriptStr ) * localBlackboard.Alloc( "StringVariableTestX" );
	( varVectorTestX = localBlackboard.Alloc<idScriptVector>( 24 ) ) = idVec3( 4.1f, 5.3f, 6.3f );
	if( graphFile.IsEmpty() )
	{
		auto* initNode = graph.CreateNode( new idGraphOnInitNode() );
		initNode->Setup( this );

		//auto* bbStr			= graph.blackBoard.Alloc("State_Idle");
		//auto * stateNode		= graph.AddNode(new idStateNode(this, (const char*)(bbStr)));

		auto* stateNode = static_cast<idStateNode*>( graph.CreateNode( new idStateNode() ) );
		stateNode->stateThread = &stateThread;
		stateNode->input_State = "State_Idle";
		stateNode->type = idStateNode::NodeType::Set;
		stateNode->Setup( this );

		auto* classNode = static_cast<idClassNode*>( graph.CreateNode( new idClassNode() ) );
		classNode->type = idClassNode::Call;
		classNode->Setup( this );

		auto* classNodeVarSet = static_cast<idClassNode*>( graph.CreateNode( new idClassNode() ) );
		classNodeVarSet->type = idClassNode::Set;
		classNodeVarSet->Setup( this );

		auto* classNodeVarGet = static_cast<idClassNode*>( graph.CreateNode( new idClassNode() ) );
		classNodeVarGet->type = idClassNode::Get;
		classNodeVarGet->Setup( this );

		//graph.AddLink( initNode->outputSockets[0], stateNode->inputSockets[0] );
	}
	else
	{
		idStr generatedFilename = "graphs/" + graphFile + ".bGrph";
		idFileLocal inputFile( fileSystem->OpenFileRead( generatedFilename, "fs_basepath" ) );
		if( inputFile )
		{
			graph.LoadBinary( inputFile, inputFile->Timestamp(), this );
		}

	}

	BecomeInactive( TH_THINK );
}

void idGraphedEntity::SharedThink()
{
	if( thinkFlags & TH_THINK )
	{
		graph.SharedThink();
	}
}

void idGraphedEntity::Think()
{
	if( thinkFlags & TH_THINK )
	{
		idEntity::Think();
		stateThread.Execute();
	}
}

stateResult_t idGraphedEntity::State_Idle( stateParms_t* parms )
{
	common->Printf( "%s Im Idling! %i\n ", name.c_str(), gameLocal.GetTime() );
	return SRESULT_WAIT;
}

void idGraphedEntity::Event_Activate( idEntity* activator )
{
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
	
	other.owner = nullptr;
	other.var = nullptr;
	
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
				owner->graph->blackBoard.Free( ( idScriptStr* )var );
			}
			else
			{
				owner->graph->blackBoard.Free( var->GetRawData() );
			}
			delete var;
		}
	}
	else if( var )
	{
		common->Warning( "SOCKET VAR LEAKED!!" );
	}

}

idScriptVariableBase* VarFromFormatSpec( const char spec, GraphState* graph /*= nullptr*/ )
{
	idScriptVariableBase* ret = nullptr;

	switch( spec )
	{
		case D_EVENT_FLOAT:
			ret = graph->blackBoard.Alloc<idScriptFloat>( 8 );
			break;
		case D_EVENT_INTEGER:
			ret = graph->blackBoard.Alloc<idScriptInteger>( 8 );
			break;

		case D_EVENT_VECTOR:
			ret = graph->blackBoard.Alloc<idScriptVector>( 3 * 8 );
			break;

		case D_EVENT_STRING:
			ret = graph->blackBoard.Alloc( "" );
			break;

		case D_EVENT_ENTITY:
		case D_EVENT_ENTITY_NULL:
			ret = graph->blackBoard.Alloc<idScriptEntity>( 16 );
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

idScriptVariableBase* VarFromType( etype_t type, GraphState* graph /*= nullptr*/ )
{
	idScriptVariableBase* ret = nullptr;
	switch( type )
	{
		case ev_string:
			ret = graph->blackBoard.Alloc( "" );
			break;
		case ev_float:
			ret = graph->blackBoard.Alloc<idScriptFloat>( 8 );
			break;
		case ev_vector:
			ret = graph->blackBoard.Alloc<idScriptVector>( 3 * 8 ) ;
			break;
		case ev_entity:
			ret = graph->blackBoard.Alloc<idScriptEntity>( 16 );
			break;
		case ev_boolean:
		case ev_int:
			ret = graph->blackBoard.Alloc<idScriptInteger>( 8 );
			break;
		default:
			gameLocal.Error( "idClassNode::Setup : Invalid arg format '%s' for event.", idTypeInfo::GetEnumTypeInfo( "etype_t", type ) );
			break;
	}

	return ret;
}

bool VarCopy( idScriptVariableBase* target, idScriptVariableBase* source )
{
	assert( target->GetType() == source->GetType() );
	switch( target->GetType() )
	{
		case ev_string:
			*( idScriptStr* )target = *( ( idScriptStr* )source )->GetData();
			break;
		case ev_float:
			*( idScriptFloat* )target = *( ( idScriptFloat* )source )->GetData();
			break;
		case ev_vector:
			*( idScriptVector* )target = *( ( idScriptVector* )source )->GetData();
			break;
		case ev_entity:
			gameLocal.Error( "idScriptEntity copy not implemented" );
			//ret = graph->blackBoard.Alloc<idScriptEntity>(16);
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