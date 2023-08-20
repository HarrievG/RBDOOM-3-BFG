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
#include "d3xp/StateGraphNodes.h"


CLASS_DECLARATION(idGraphNode, idStateNode)
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
	if ( file ) 
	{
		file->WriteBig(type);
		file->WriteString(input_State);
		idGraphNode::WriteBinary(file, _timeStamp);
	}
}

bool idStateNode::LoadBinary( idFile* file, const ID_TIME_T _timeStamp)
{
	if (file)
	{
		file->ReadBig(type);
		file->ReadString(input_State);
		return idGraphNode::LoadBinary(file, _timeStamp);
	}
}

void idStateNode::Setup()
{
	output_Result = SRESULT_ERROR;
	stateThread = graph->targetStateThread;

	idGraphNodeSocket* newInput = &CreateInputSocket();
	newInput->name = idTypeInfo::GetEnumTypeInfo("idStateNode::NodeType", type);

	newInput = &CreateInputSocket();
	newInput->name = "State";
	newInput->var = new idScriptString((void*)input_State.c_str());

	idGraphNodeSocket& newOutput = CreateOutputSocket();
	newOutput.name = "Result";
	newOutput.var = new idScriptInt((void*)output_Result);
}

//////////////////////////////////////////////////////////////////////////
CLASS_DECLARATION(idGraphNode, idGraphOnInitNode)
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
		idGraphNode::WriteBinary(file, _timeStamp);
	}
}

bool idGraphOnInitNode::LoadBinary( idFile* file, const ID_TIME_T _timeStamp)
{
	if (file != NULL)
	{
		return idGraphNode::LoadBinary(file, _timeStamp);
	}
}

void idGraphOnInitNode::Setup()
{
	idGraphNodeSocket& newOutput = CreateOutputSocket();
	newOutput.active = true;
	newOutput.name = "Initialize";
	done = false;
}
