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

#ifndef __GAME_STATE_GRAPH_H__
#define __GAME_STATE_GRAPH_H__

#include "d3xp/Game_local.h"


class GraphState;
idScriptVariableBase* VarFromFormatSpec( const char spec, GraphState* graphState );
idScriptVariableBase* VarFromType( etype_t type, GraphState* graphState );
bool VarCopy( idScriptVariableBase* target, idScriptVariableBase* source );

class idBlackBoard
{
public:

	template<class scriptType>
	scriptType* Alloc( int size )
	{
		byte* b = data.Alloc( size );
		memset( b, 0, size );
		return  new scriptType( b );
	}

	//const for a reason!
	//const idScriptString* Alloc( const char* str )
	//{
	//	return  new idScriptString( ( void* )strPool.AllocString( str ) );
	//}

	idScriptStr* Alloc( const char* str )
	{
		strList.AddUnique( new idStr( str ) );
		return new idScriptStr( ( void* )strList[strList.Num() - 1] );
	}

	idStrPtr GetAllocatedStr( const char* str )
	{
		for( auto& strPtr : strList )
		{
			if( *strPtr == str )
			{
				return strPtr;
			}
		}
		return nullptr;
	}

	void Free( idStr* var )
	{
		strList.Remove( var );
	}

	void Free( byte* var )
	{
		data.Free( var );
	}

	idBlackBoard()
	{
		data.Init();
	}
	~idBlackBoard()
	{
		data.Shutdown();
	}
private:
	idDynamicBlockAlloc< byte , 16 * 1024, 256 , TAG_BLACKBOARD> data;

	//use for static node strings
	//idStrPool		strPool;
	idStrPtrList		strList;
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

	idGraphNodeSocket& operator=( idGraphNodeSocket&& );
	~idGraphNodeSocket();
	idGraphNodeSocket() :
		owner( nullptr ), var( nullptr ), active( false ), lastActivated( -1 ), name( "" ), socketIndex( -1 ),
		nodeIndex( -1 ), freeData( true ), isOutput( false ) {}
	idList<idGraphNodeSocket*> connections;
	idGraphNode* owner;
	idScriptVariableBase* var;
	bool active;
	int lastActivated;
	idStr name;
	int socketIndex;
	int nodeIndex;
	bool freeData;
	bool isOutput;
};
class idStateGraph;
class GraphState
{
public:
	idList<idGraphNode*> nodes;
	idList<idGraphNode*> activeNodes;
	//should only be used in editor.
	idList<idGraphNodeSocket::Link_t>  links;
	idBlackBoard blackBoard;
	rvStateThread* stateThread;
	idStateGraph* graph;
	int waitMS;
	idList<idScriptVariableInstance_t> localVariables;
	idStr name;
};

class idGraphNode : public idClass
{
public:
	ABSTRACT_PROTOTYPE( idGraphNode );

	virtual stateResult_t Exec( stateParms_t* parms ) = 0;
	virtual const char* GetName() = 0;
	virtual void Setup( idClass* graphOwner ) = 0;

	//////////////////////////////////////////////////////////////////////////
	//Should only be used in editor.
	virtual idGraphNode* QueryNodeConstruction( idStateGraph* targetGraph, idClass* graphOwner ) = 0;
	virtual void Draw( ImGuiTools::GraphNode* nodePtr );
	virtual idVec4 NodeTitleBarColor();
	//////////////////////////////////////////////////////////////////////////

	virtual	void WriteBinary( idFile* file, ID_TIME_T* _timeStamp = NULL );
	virtual	bool LoadBinary( idFile* file, const ID_TIME_T _timeStamp, idClass* owner = nullptr );

	idGraphNodeSocket& CreateInputSocket();
	idGraphNodeSocket& CreateOutputSocket();
	bool HasActiveSocket();
	void ActivateOutputConnections();
	void DeactivateInputs();

	idList<idGraphNodeSocket> inputSockets;
	idList<idGraphNodeSocket> outputSockets;
	GraphState* graphState;
	int nodeIndex;
private:
	idGraphNodeSocket& CreateSocket( idList<idGraphNodeSocket>& socketList );
};


class idStateGraph : public idClass
{
public:
	static constexpr const char* MAIN = "GRAPH_MAIN";
	friend class idStateEditor;

	CLASS_PROTOTYPE( idStateGraph );


	//////////////////////////////////////////////////////////////////////////
	//Taking over from idThread
	void								Event_Wait( float time );
	void								Event_WaitFrame();
	//////////////////////////////////////////////////////////////////////////

	void								Event_Activate( idEntity* activator );
	idStateGraph( idClass* Owner = nullptr );

	~idStateGraph();
	virtual void						Think();

	void								ConvertScriptObject( idScriptObject* scriptObject );
	void								WriteBinary( idFile* file, ID_TIME_T* _timeStamp = NULL );
	bool								LoadBinary( idFile* file, const ID_TIME_T _timeStamp , idClass* owner = nullptr );

	idList<idScriptVariableInstance_t>& GetVariables();
	idScriptVariableInstance_t&			CreateVariable( idScriptStr** variableName, etype_t type );
	void								RemoveNode( idGraphNode* node );
	idGraphNode*						CreateNode( idGraphNode* node );
	void								RemoveLink( idGraphNodeSocket* start, idGraphNodeSocket* end );
	idGraphNodeSocket::Link_t&			AddLink( idGraphNodeSocket& start, idGraphNodeSocket& end );
	stateResult_t						State_Update( stateParms_t* parms );
	stateResult_t						State_Exec( stateParms_t* parms );

	stateResult_t						State_LocalExec( stateParms_t* parms );
	int									GetLocalStateIndex( const char* stateName );
	GraphState* 						GetLocalState( const char* stateName );

	template<class T>
	T* CreateStateNode( int stateIndex, T* node );
	template<class T>
	T* CreateNode( int stateIndex );

	void								Clear();

	int									CreateSubState( const char* name, idList<idScriptVariableInstance_t> inputs, idList< idScriptVariableInstance_t> ouputs );

	idStrList							localStates;
	idHashIndex							localStateHash;
	idList<GraphState>					localGraphState;

	static idList<int>	GraphThreadEventMap;
	static bool			GraphThreadEventMapInitDone;

private:
	void								DeleteLocalStateNode( int stateIndex, idGraphNode* node );
	void								DeleteLocalStateNode( const char* stateName, idGraphNode* node );
	idGraphNode*						CreateLocalStateNode( int stateIndex, idGraphNode* node );
	idGraphNode*						CreateLocalStateNode( const char* stateName, idGraphNode* node );

	idList<idScriptVariableInstance_t>&  GetLocalVariables();
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

	idScriptBool	varBoolTest;
	idScriptInteger	varIntTest;
	idScriptFloat	varFloatTest;
	idScriptStr		varStringTest;
	idScriptVector	varVectorTest;

	idScriptBool	varBoolTestX;
	idScriptInteger	varIntTestX;
	idScriptFloat	varFloatTestX;
	idScriptStr		varStringTestX;
	idScriptVector	varVectorTestX;
private:
	virtual void	Think();
protected:

};

#include "d3xp/StateGraphNodes.h"

#endif	__SYS_STATE_GRAPH_H__