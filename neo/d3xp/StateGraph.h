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

#ifndef __SYS_STATE_GRAPH_H__
#define __SYS_STATE_GRAPH_H__

#include "d3xp/Game_local.h"

class idBlackBoard
{
public:
	template<class scriptType>
	scriptType* Alloc()
	{
		return  new scriptType( data.Alloc() );
	}

	template<class scriptType>
	void Free( scriptType* var )
	{
		data.Free( ( byte* )var );
	}

	const idScriptString* Alloc( const char* str )
	{
		return  new idScriptString( ( void* )strPool.AllocString( str ) );
	}

	template<>
	void Free( const idScriptString* var )
	{
		strPool.FreeString( ( idPoolStr* )( var->GetData() ) );
	}

private:
	//BLOCK_ALLOC_ALIGNMENT == 16b
	idBlockAlloc<byte, 4, TAG_BLACKBOARD> data;
	idStrPool	strPool;
};

class idGraphNode;
class idGraphNodeSocket
{
public:

	//////////////////////////////////////////////////////////////////////////
	//should only be used in editor
	typedef struct
	{
		idGraphNodeSocket* start;
		idGraphNodeSocket* end;
	} Link_t;
	//////////////////////////////////////////////////////////////////////////
	idGraphNodeSocket() : owner( nullptr ), var( nullptr ), active( false ), name( "" ), socketIndex(-1){}
	idList<idGraphNodeSocket*> connections;
	idGraphNode* owner;
	idScriptVariableBase* var;
	bool active;
	idStr name;
	int socketIndex;
	int nodeIndex;
};

class idStateGraph;
class idGraphNode : public idClass
{
public:

	ABSTRACT_PROTOTYPE(idGraphNode);

	virtual stateResult_t Exec( stateParms_t* parms ) = 0;
	virtual	void WriteBinary( idFile* file, ID_TIME_T* _timeStamp = NULL );
	virtual	bool LoadBinary( idFile* file, const ID_TIME_T _timeStamp);
	virtual const char* GetName() = 0;
	virtual void Setup() = 0;

	//should only be used in editor.
	virtual void Draw(const ImGuiTools::GraphNode* node);
	
	idGraphNodeSocket& CreateInputSocket();
	idGraphNodeSocket& CreateOutputSocket();
	bool HasActiveSocket();
	void ActivateOutputConnections();
	void DeactivateInputs();
	idList<idGraphNodeSocket> inputSockets;
	idList<idGraphNodeSocket> outputSockets;
	idStateGraph* graph;
	int nodeIndex;

private:
	idGraphNodeSocket& CreateSocket(idList<idGraphNodeSocket>& socketList);
};

class idStateGraph : public idClass
{
public:
	friend class idStateEditor;

	CLASS_PROTOTYPE( idStateGraph );

	idStateGraph( idClass* targetClass = nullptr, rvStateThread* targetState = nullptr )
		: stateThread( ( targetState && targetClass ) ? targetState : new rvStateThread() )
		, owner( ( targetState && targetClass ) ? targetClass : nullptr )
	{
		if( owner )
		{
			stateThread->SetOwner( owner );
		}
		else if( targetState )
		{
			stateThread->SetOwner( this );
		}
	};

	~idStateGraph()
	{
		if( owner )
		{
			delete stateThread;
		}
	}
	virtual void	SharedThink();

	void			ConvertScriptObject( idScriptObject* scriptObject );
	void			WriteBinary( idFile* file, ID_TIME_T* _timeStamp = NULL );
	bool			LoadBinary( idFile* file, const ID_TIME_T _timeStamp);

	idGraphNode*						CreateNode( idGraphNode* node );
	idGraphNodeSocket::Link_t&			AddLink( idGraphNodeSocket& input, idGraphNodeSocket& output );
	stateResult_t						State_Update( stateParms_t* parms );
	stateResult_t						State_Exec( stateParms_t* parms );

	idClass* 		owner;
	rvStateThread*  targetStateThread;
	idBlackBoard	blackBoard;

	idList<idGraphNode*> nodes;
	idList<idGraphNode*> activeNodes;
	//should only be used in editor.
	idList<idGraphNodeSocket::Link_t>  links;

private:
	rvStateThread* stateThread;
};
//Testcase for idStateGraph[editor]
class idGraphedEntity : public idEntity
{
public:
	CLASS_PROTOTYPE( idGraphedEntity );
	idGraphedEntity()
	{

	}

	void			Spawn();

	stateResult_t	State_Idle( stateParms_t* parms );
	void			Event_Activate( idEntity* activator );

	idStateGraph	graph;
	rvStateThread	stateThread;
private:
	virtual void	SharedThink();
	virtual void	Think();
protected:

};

#include "d3xp/StateGraphNodes.h"

#endif	__SYS_STATE_GRAPH_H__