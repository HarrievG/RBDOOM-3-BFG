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

CLASS_DECLARATION( idClass, idStateGraph )
END_CLASS


idStateGraph::idStateGraph() 
{	
	GetLocalState("GRAPH_MAIN");
	localGraphState[0].stateThread = new rvStateThread();
	localGraphState[0].owner = nullptr;
}

idStateGraph::~idStateGraph()
{
	if (!localGraphState[0].owner)
	{
		delete localGraphState[0].stateThread;
	}
}

void idStateGraph::Clear()
{
	//nodes.Clear();
	//links.Clear();
}
void idStateGraph::SharedThink()
{
	auto& graph = localGraphState[0];
	if( !graph.owner)
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
	if( file != nullptr)
	{
		file->WriteBig( nodes.Num() );

		for (idGraphNode * node : nodes)
		{
			idTypeInfo* c = node->GetType();
			file->WriteBig(c->typeNum);
		}
		
		for (idGraphNode* node : nodes)
		{
			node->WriteBinary(file, _timeStamp);
		}

		file->WriteBig(links.Num());
		for (idGraphNodeSocket::Link_t& link : links)
		{
			file->WriteBig(link.start->nodeIndex);
			file->WriteBig(link.start->socketIndex);
			file->WriteBig(link.end->nodeIndex);
			file->WriteBig(link.end->socketIndex);
		}
	}
}

bool idStateGraph::LoadBinary( idFile* file, const ID_TIME_T _timeStamp)
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
		for (int i = 0; i < totalNodes; i++)
		{
			int typeNum;
			file->ReadBig(typeNum);
			auto* typeInfo = idClass::GetType(typeNum);
			auto* newNode = CreateNode(static_cast<idGraphNode*>(typeInfo->CreateInstance()));
			newNode->Setup();
		}
		//load data
		for (auto* node : nodes)
		{
			if (!node->LoadBinary(file, _timeStamp))
				return false;
		}

		int totalLinks;
		file->ReadBig(totalLinks);
		for (int i = 0; i < totalLinks; i++)
		{
			int startIdx, endIdx;
			int startSocketIdx, endSocketIdx;
			file->ReadBig(startIdx);
			file->ReadBig(startSocketIdx);
			file->ReadBig(endIdx);
			file->ReadBig(endSocketIdx);
			links.Alloc() = { &nodes[startIdx]->outputSockets[startSocketIdx],&nodes[endIdx]->inputSockets[endSocketIdx] };
		}
		return true;
	}
	return false;
}

idGraphNode* idStateGraph::CreateNode(idGraphNode* node)
{
	return AddLocalStateNode("GRAPH_MAIN", node);
	//
	//node->graph = this;
	//auto * retNode = nodes.Alloc() = node;
	//node->nodeIndex = nodes.Num() - 1;
	//return retNode;
}

idGraphNodeSocket::Link_t& idStateGraph::AddLink( idGraphNodeSocket& input, idGraphNodeSocket& output )
{
	auto& currentGraphState = localGraphState[0];

	return currentGraphState.links.Alloc() = {
		output.connections.Alloc() = &input,
		input.connections.Alloc() = &output
	};
}

stateResult_t idStateGraph::State_Update( stateParms_t* parms )
{
	auto& currentGraphState = localGraphState[parms->stage];

	auto& currentNodes = currentGraphState.nodes;
	auto& currentActiveNodes = currentGraphState.activeNodes;

	for( idGraphNode* node : currentNodes)
	{
		if( node->HasActiveSocket() )
		{
			currentActiveNodes.Alloc() = node;
		}
	}

	if(currentActiveNodes.Num() )
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

	for( idGraphNode* node : currentActiveNodes)
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
		currentGraphState.stateThread->PostState("State_Update");
		return SRESULT_DONE;
	}

	return SRESULT_WAIT;
}

stateResult_t idStateGraph::State_LocalExec(stateParms_t* parms)
{
	return SRESULT_ERROR;
}

int idStateGraph::GetLocalState(const char* newStateName)
{
	 int i, hash;

	 hash = localStateHash.GenerateKey(newStateName);
	 for (i = localStateHash.First(hash); i != -1; i = localStateHash.Next(i))
	 {
		 if (localStates[i].Cmp(newStateName) == 0)
		 {
			 return i;
		 }
	 }

	 i = localStates.Append(newStateName);
	 localStateHash.Add(hash, i);
	 localGraphState.Alloc();

	 return i;

}

idGraphNode* idStateGraph::AddLocalStateNode(const char* stateName, idGraphNode* node)
{
	int stateIndex = GetLocalState(stateName);
	GraphState& stateNodes = localGraphState[stateIndex];

	node->graph = &stateNodes;
	auto* retNode = stateNodes.nodes.Alloc() = node;
	node->nodeIndex = stateNodes.nodes.Num() - 1;
	return retNode;
}


void idGraphNode::WriteBinary(idFile* file, ID_TIME_T* _timeStamp /*= NULL */)
{
	if ( file )
	{
		assert(graph);

		auto writeSockets =
			[](idFile* file,idList<idGraphNodeSocket>& socketList)
		{
			file->WriteBig(socketList.Num());
			for (auto& socket : socketList)
			{
				file->WriteBig(socket.connections.Num());

				for (auto* connSocket : socket.connections)
				{
					file->WriteString(connSocket->name);
					assert(connSocket->nodeIndex >= 0);
					assert(connSocket->socketIndex >= 0);					
					file->WriteBig(connSocket->nodeIndex);
					file->WriteBig(connSocket->socketIndex);
				}
			}
		};

		writeSockets(file,inputSockets);
		writeSockets(file,outputSockets);
	}	
}

bool idGraphNode::LoadBinary(idFile* file, const ID_TIME_T _timeStamp)
{
	if (file)
	{
		assert(graph);
		auto readSockets =
			[this](idFile* file,idList<idGraphNodeSocket>& socketList,bool isInput)
		{
			int numSockets;
			file->ReadBig(numSockets);
			for (auto& socket : socketList)
			{
				socket.owner = this;
				int numConnections;
				file->ReadBig(numConnections);
				socket.connections.AssureSize(numConnections);
				for (auto& connSocket : socket.connections)
				{
					idStr name;
					file->ReadString(name);
					int nodeIndex;
					file->ReadBig(nodeIndex);
					assert(nodeIndex >= 0);
					int socketIndex;
					file->ReadBig(socketIndex);
					assert(socketIndex >= 0);
					if (isInput)
						connSocket = &graph->nodes[nodeIndex]->outputSockets[socketIndex];
					else
						connSocket = &graph->nodes[nodeIndex]->inputSockets[socketIndex];
					connSocket->name = name;
				}
			}
		};

		readSockets(file,inputSockets, true);
		readSockets(file,outputSockets, false);
	}
	return true;
}

idGraphNodeSocket& idGraphNode::CreateSocket(idList<idGraphNodeSocket>& socketList)
{
	idGraphNodeSocket& ret = socketList.Alloc();
	ret.socketIndex = socketList.Num() - 1;
	ret.nodeIndex = this->nodeIndex;
	ret.owner = this;
	return ret;
}


//Move out of this file and only compile with editor tools.
void idGraphNode::Draw(const ImGuiTools::GraphNode* nodePtr)
{
	auto& node = *nodePtr;
	namespace ed = ax::NodeEditor;
	using namespace ImGuiTools;


	ed::BeginNode(node.ID);


	int nodeWidth = 100;
	ImGui::PushItemWidth(nodeWidth);
	//ImGui::DragInt("drag int", &nodeWidth, 1);

	ImGui::AlignTextToFramePadding();

	if (auto* owner = node.Owner)
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
		idVec4 color = NodeTitleBarColor();
		ImColor titleBarColor = { color.x,color.y,color.z,color.w };
		const char* titleText = GetName();
		ImVec2 textbb = ImGui::CalcTextSize(titleText, NULL, true);
		float barWidth = idMath::Imax(ImGui::GetStyle().ItemInnerSpacing.x + textbb.x + ImGui::GetStyle().ItemInnerSpacing.x, nodeWidth);
		ImGui::GetWindowDrawList()->AddRectFilled(
			ImVec2(cursorScreenPos.x - 4 - ImGui::GetStyle().ItemInnerSpacing.x, cursorScreenPos.y - ImGui::GetStyle().ItemInnerSpacing.y - 4),
			ImVec2(cursorScreenPos.x + barWidth + 24, cursorScreenPos.y + ImGui::GetTextLineHeight() + 4),
			titleBarColor, 12, topRoundCornersFlags);

		ImGui::Dummy(ImVec2(barWidth - 4 - ImGui::GetStyle().ItemInnerSpacing.x, 0));
		ImGui::GetWindowDrawList()->AddText(cursorScreenPos, ImColor(50.0f, 45.0f, 255.0f, 255.0f), titleText);

		int maxInputSockets = inputSockets.Num();
		int maxOutputSockets = outputSockets.Num();
		int maxSocket = idMath::Imax(maxInputSockets, maxOutputSockets);
		int inputSocketIdx = 0, outputSocketIdx = 0;

		const char* inputText = nullptr;
		const char* outputText = nullptr;
		for (int i = 0; i < maxSocket; i++)
		{
			//check for input;


			if (inputSocketIdx <= maxInputSockets - 1)
			{
				idGraphNodeSocket& ownerSocket = owner->inputSockets[inputSocketIdx];
				ed::BeginPin(node.Inputs[inputSocketIdx].ID, ed::PinKind::Input);
				if (ownerSocket.var)
				{
					ImGui::ImScriptVariable({ ownerSocket.name.c_str(),"",ownerSocket.var });
				}
				else
				{
					ImGui::Text(ownerSocket.name.c_str());
				}
				ed::EndPin();
				inputSocketIdx++;
			}
			else
			{
				ImGui::Text("");
				inputText = "";
			}
			if (outputSocketIdx <= maxOutputSockets - 1)
			{
				outputText = owner->outputSockets[outputSocketIdx].name.c_str();
				ImGui::SameLine();
				ImGui::Dummy(ImVec2(idMath::Imax(barWidth - ImGui::CalcTextSize(outputText).x - ImGui::CalcTextSize(inputText).x, 0), 0));
				ImGui::SameLine();
				ed::BeginPin(node.Outputs[outputSocketIdx].ID, ed::PinKind::Output);
				ImGui::Text(outputText);
				ed::EndPin();
				outputSocketIdx++;
			}
			else
			{
				outputText = nullptr;
			}
		}
	}

	ImGui::PopItemWidth();
	ed::EndNode();

}

idVec4 idGraphNode::NodeTitleBarColor()
{
	ImVec4 color = ImGui::GetStyle().Colors[ImGuiCol_HeaderActive];
	return idVec4(color.x, color.y, color.z, color.w);
	//return idVec4(color.Value.x, color.Value.y, color.Value.z, color.Value.w) / 255.0f;
}

idGraphNodeSocket& idGraphNode::CreateInputSocket()
{
	return CreateSocket(inputSockets);
}

idGraphNodeSocket& idGraphNode::CreateOutputSocket()
{
	return CreateSocket(outputSockets);
}

//////////////////////////////////////////////////////////////////////////


ABSTRACT_DECLARATION(idClass, idGraphNode)
END_CLASS

/// <summary>
/// idGraphNode
/// </summary>
/// <returns>bool</returns>
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
	idStr graphFile = spawnArgs.GetString("graph", "");

	stateThread.SetOwner( this );
	graph.localGraphState[0].targetStateThread = &stateThread;

	if (graphFile.IsEmpty())
	{
		auto* initNode = graph.CreateNode(new idGraphOnInitNode());
		initNode->Setup();

		//auto* bbStr			= graph.blackBoard.Alloc("State_Idle");
		//auto * stateNode		= graph.AddNode(new idStateNode(this, (const char*)(bbStr)));

		auto* stateNode = static_cast<idStateNode*>(graph.CreateNode(new idStateNode()));
		stateNode->stateThread = &stateThread;
		stateNode->input_State = "State_Idle";
		stateNode->type = idStateNode::NodeType::Set;
		stateNode->Setup();

		graph.AddLink(initNode->outputSockets[0], stateNode->inputSockets[0]);
	}
	else
	{
		idStr generatedFilename = "graphs/" + graphFile + ".bGrph";
		idFileLocal inputFile(fileSystem->OpenFileRead(generatedFilename, "fs_basepath"));
		if (inputFile)
		{
			graph.LoadBinary(inputFile, inputFile->Timestamp());
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
	if (thinkFlags & TH_THINK)
	{
		idEntity::Think();
		stateThread.Execute();
	}
}

stateResult_t idGraphedEntity::State_Idle( stateParms_t* parms )
{
	common->Printf( "%s Im Idling! %i\n ", name.c_str(),gameLocal.GetTime() );
	return SRESULT_WAIT;
}

void idGraphedEntity::Event_Activate( idEntity* activator )
{
	BecomeActive( TH_THINK );
}

