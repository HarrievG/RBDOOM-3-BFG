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


CLASS_DECLARATION( idClass, idStateGraph )
END_CLASS

void idStateGraph::SharedThink()
{
	if( !owner )
	{
		stateThread->SetOwner( this );
		if( stateThread->IsIdle() )
		{
			stateThread->SetState( "State_Update" );
		}
		stateThread->Execute();
	}
}

void idStateGraph::WriteBinary( idFile* file, ID_TIME_T* _timeStamp /*= NULL*/ )
{
	//First write all nodes and sockets.
	//as last all socket connections;
	if( file != NULL )
	{
		file->WriteInt( nodes.Num() );

		for (idGraphNode * node : nodes)
		{
			node->WriteBinary(file, _timeStamp);
		}
	}
}

idGraphNode* idStateGraph::AddNode(idGraphNode* node)
{
	node->graph = this;
	return (nodes.Alloc() = node);
}

idGraphNodeSocket::Link_t& idStateGraph::AddLink( idGraphNodeSocket& input, idGraphNodeSocket& output )
{
	return links.Alloc() = {
		output.connections.Alloc() = &input,
		input.connections.Alloc() = &output
	};
}

stateResult_t idStateGraph::State_Update( stateParms_t* parms )
{
	for( idGraphNode* node : nodes )
	{
		if( node->HasActiveSocket() )
		{
			activeNodes.Alloc() = node;
		}
	}

	if( activeNodes.Num() )
	{
		stateThread->PostState( "State_Exec" );
		return SRESULT_DONE;
	}

	return SRESULT_WAIT;
}

stateResult_t idStateGraph::State_Exec( stateParms_t* parms )
{
	stateResult_t result = SRESULT_DONE;
	idList<idGraphNode*> waitingNodes;

	for( idGraphNode* node : activeNodes )
	{
		stateResult_t nodeResult = node->Exec( parms );
		if( nodeResult == SRESULT_WAIT )
		{
			waitingNodes.Alloc() = node;
		}
		node->ActivateOutputConnections();
		node->DeactivateInputs();
	}

	activeNodes = waitingNodes;

	if( !activeNodes.Num() )
	{
		stateThread->PostState("State_Update");
		return SRESULT_DONE;
	}

	return SRESULT_WAIT;
}

void idGraphNode::WriteBinary(idFile* file, ID_TIME_T* _timeStamp /*= NULL */)
{
	if ( file )
	{
		assert(graph);
		file->WriteBig(inputSockets.Num());
		for (auto& socket : inputSockets)
		{
			//file->WriteBig(socket.connections.Num());

			//for (auto* connSocket : socket.connections)
			//{

			//	//output socket index from parent node
			//	connSocket->owner->outputSockets.IndexOf(connSocket);
			//	//graph->nodes.IndexOf()
			//}
		}
		file->WriteBig(outputSockets.Num());
		for (int i = 0; i < outputSockets.Num(); i++)
		{
			
		}
		file->WriteBigArray(inputSockets.Ptr(), inputSockets.Num());
		file->WriteBig(outputSockets.Num());
		file->WriteBigArray(outputSockets.Ptr(), outputSockets.Num());
	}	
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
	stateThread.SetOwner( this );
	auto* initNode		= graph.AddNode( new idGraphOnInitNode() );
	//auto* bbStr			= graph.blackBoard.Alloc("State_Idle");
	//auto * stateNode		= graph.AddNode(new idStateNode(this, (const char*)(bbStr)));
	auto* stateNode		= graph.AddNode( new idStateNode( &stateThread, "State_Idle", idStateNode::NodeType::Set ) );

	graph.AddLink( initNode->outputSockets[0], stateNode->inputSockets[0] );

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
	idEntity::Think();
	stateThread.Execute();
}

stateResult_t idGraphedEntity::State_Idle( stateParms_t* parms )
{
	common->Printf( "Im Idling! %i\n ", gameLocal.GetTime() );
	return SRESULT_WAIT;
}

void idGraphedEntity::Event_Activate( idEntity* activator )
{
	BecomeActive( TH_THINK );
}

