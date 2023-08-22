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

#ifndef __GAME_STATE_GRAPH_NODES_H__
#define __GAME_STATE_GRAPH_NODES_H__

#include "d3xp/Game_local.h"
#include "d3xp/StateGraph.h"

class idStateNode : public idGraphNode
{
public:
	CLASS_PROTOTYPE(idStateNode);

	enum NodeType
	{
		Set,
		Post,
		Interrupt
	};
	idStateNode();
	//idStateNode( rvStateThread* owner );
	//idStateNode( rvStateThread* owner, const char* stateString, NodeType type = NodeType::Set );
	//An exec can
	stateResult_t Exec( stateParms_t* parms ) override;
	const char* GetName() override
	{
		return "idStateNode";
	}
	void WriteBinary( idFile* file, ID_TIME_T* _timeStamp = NULL ) override;
	bool LoadBinary( idFile* file, const ID_TIME_T _timeStamp) override;
	void Setup() override;

	idStr			input_State;
	NodeType		type;
	rvStateThread*	stateThread;
	stateResult_t	output_Result;
};

class idClassNode : public idStateNode
{
public:
	enum NodeType
	{
		Call,//Event
		Set,//Scriptvariable
		Get//Scriptvariable
	};
	idClassNode( idClass* owner, NodeType type );
	stateResult_t Exec( stateParms_t* parms ) override;
	const char* GetName() override
	{
		return "idClassNode";
	}
	void WriteBinary( idFile* file, ID_TIME_T* _timeStamp = NULL ) override {}
	bool LoadBinary( idFile* file, const ID_TIME_T _timeStamp) override
	{
		return true;
	}
	void Setup() override {}
private:
	idBlackBoard blackBoard;
};

class idGraphOnInitNode : public idGraphNode
{
public:
	CLASS_PROTOTYPE( idGraphOnInitNode );

	idGraphOnInitNode();
	stateResult_t Exec( stateParms_t* parms ) override;
	const char* GetName() override
	{
		return "idGraphOnInitNode";
	}
	void WriteBinary( idFile* file, ID_TIME_T* _timeStamp = NULL ) override;
	bool LoadBinary( idFile* file, const ID_TIME_T _timeStamp) override;
	void Setup() override;

	idVec4 NodeTitleBarColor() override;

private:
	bool done;
};


#endif //__SYS_STATE_GRAPH_NODES_H__