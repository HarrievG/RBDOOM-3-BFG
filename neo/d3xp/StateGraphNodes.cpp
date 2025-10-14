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
		__debugbreak();
		//input_State = *( idScriptString* )( strSocket.var );
	}

	if( inputSockets[0].active )
	{
		switch( type )
		{
			case idStateNode::Set:
				( *stateThread )->SetState( input_State );
				break;
			case idStateNode::Post:
				( *stateThread )->PostState( input_State );
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
	stateThread = &graphState->stateThread;

	idGraphNodeSocket* newInput = &CreateInputSocket();
	newInput->name = idTypeInfo::GetEnumTypeInfo( "idStateNode::NodeType", type );

	newInput = &CreateInputSocket();
	newInput->name = "State";
	newInput->var = new idScriptStr( ( void* )&input_State );
	newInput->freeData = false;
	idGraphNodeSocket& newOutput = CreateOutputSocket();
	newOutput.name = "Result";
	newOutput.var = new idScriptInt( ( void* )&output_Result );
	newOutput.freeData = false;

}

idGraphNode *idStateNode::QueryNodeConstruction( idStateGraph *targetGraph, idClass *graphOwner, idStr contextName )
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CLASS_DECLARATION( idGraphNode, idGraphOnInitNode )
EVENT( EV_Activate, idGraphOnInitNode::OnActivate )
END_CLASS

idGraphOnInitNode::idGraphOnInitNode()
{
	type = NodeType::Activate;
}

stateResult_t idGraphOnInitNode::Exec( stateParms_t* parms )
{
	return SRESULT_DONE;
}

const char* idGraphOnInitNode::GetName( )
{
	if( type == NodeType::Activate )
	{
		return "On Activate";
	}
	else
	{
		return "On Construct";
	}
}

void idGraphOnInitNode::WriteBinary( idFile* file, ID_TIME_T* _timeStamp /*= NULL*/ )
{
	if( file != NULL )
	{
		file->WriteBig( ( int )type );
		idGraphNode::WriteBinary( file, _timeStamp );
	}
}

bool idGraphOnInitNode::LoadBinary( idFile* file, const ID_TIME_T _timeStamp, idClass* owner )
{
	if( file != NULL )
	{
		int newType = 0;
		file->ReadBig( newType );
		type = ( NodeType ) newType;

		Setup( owner );
		return idGraphNode::LoadBinary( file, _timeStamp, owner );
	}
	return false;
}

void idGraphOnInitNode::Setup( idClass* graphOwner )
{
	idGraphNodeSocket& newOutput = CreateOutputSocket();
	newOutput.name = "";
	if( type == NodeType::Construct )
	{
		newOutput.active = true;
	}
	else
	{
		idGraphNodeSocket& newEntOutput = CreateOutputSocket( );
		newEntOutput.name = "Activator";
		newEntOutput.var = VarFromType( ev_entity, graphState );
	}
	done = false;
}

idGraphNode* idGraphOnInitNode::QueryNodeConstruction( idStateGraph* targetGraph, idClass* graphOwner,idStr contextName )
{
	auto& graph = *targetGraph;

	bool foundOnActivate = false;
	bool foundOnConstruct = false;
	auto nodes = graph.GetLocalState( contextName )->nodes;
	for( auto node : nodes )
	{
		if( auto onInitNode = node->Cast<idGraphOnInitNode>() )
		{
			if( onInitNode->type == NodeType::Activate )
			{
				foundOnActivate = true;
			}
			if( onInitNode->type == NodeType::Construct )
			{
				foundOnConstruct = true;
			}
		}
		if( foundOnActivate && foundOnConstruct )
		{
			break;
		}
	}

	if( !foundOnActivate && ImGui::MenuItem( "OnActivate" ) )
	{

		auto* logicNode = static_cast< idGraphOnInitNode* >( graph.CreateNode( new idGraphOnInitNode( ) ) );
		logicNode->type = NodeType::Activate;
		logicNode->Setup( graphOwner );
		return logicNode;
	}


	if( !foundOnConstruct && ImGui::MenuItem( "OnConstruct" ) )
	{

		auto* logicNode = static_cast< idGraphOnInitNode* >( graph.CreateNode( new idGraphOnInitNode( ) ) );
		logicNode->type = NodeType::Construct;
		logicNode->Setup( graphOwner );
		return logicNode;
	}


	return nullptr;
}

idVec4 idGraphOnInitNode::NodeTitleBarColor()
{
	if( type == NodeType::Activate )
	{
		return idVec4( 1.0f, 0.0f, 1.0f, 1.0f );
	}
	else
	{
		return idVec4( 0.7f, 0.3f, 1.0f, 1.0f );
	}

}

void idGraphOnInitNode::OnActivate( idEntity* activator )
{
	if( type == NodeType::Activate )
	{
		outputSockets[0].active = true;
		*( ( idScriptEntity* ) outputSockets[1].var )->GetData( ) = activator;
		done = true;
	}

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
	isLocalvar = false;
	isStaticVar = false;

	scriptThread = nullptr;

	ownerClass = nullptr;
	nodeOwnerClass = nullptr;

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
			if( inp->connections.Num() && inp->connections[0]->var )
			{
				VarCopy( inp->var, inp->connections[0]->var );
			}

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
			idGraphNodeSocket* inp = &inputSockets[i + 1];
			if( !socketVar( inp, targetEvent, graphState, formatspec[i - 1], eventData[i - 1] ) )
			{
				return SRESULT_ERROR;
			}
		}

		idGraphNodeSocket& targetSocket = inputSockets[1];

		if( targetSocket.connections.Num() == 1 && *targetSocket.connections[0]->var->GetRawData() )
		{
			VarCopy( targetSocket.var, targetSocket.connections[0]->var );

			if( ( *nodeOwnerClass ) != *( ( idScriptClass* )targetSocket.var )->GetData() )
			{
				SetOwner( &*( ( idScriptClass* )targetSocket.var )->GetData() );
			}
		}

		if( *nodeOwnerClass )
		{
			if( ( *nodeOwnerClass )->IsType( idEntity::Type ) )
			{
				if( ownerEntityPtr.IsValid() )
				{
					ownerEntityPtr.GetEntity()->ProcessEventArgPtr( targetEvent, eventData );
				}
			}
			else
			{
				//UNSAFE
				( *nodeOwnerClass )->ProcessEventArgPtr( targetEvent, eventData );
			}
		}

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
			retSocketVar( &outputSockets[1], targetEvent, graphState, targetEvent->GetReturnType() );
		}

		outputSockets[0].active = true;

		return SRESULT_DONE;
	}
	else if( type == Set )
	{
		if( inputSockets.Num() && inputSockets[1].connections.Num()
				&& inputSockets[1].connections[0]->var && targetVariable )
		{
			VarCopy( targetVariable, inputSockets[1].connections[0]->var );
			outputSockets[0].active = true;
		}
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
	SetOwner( &ownerClass );

	idGraphNodeSocket* newInput = &CreateInputSocket();
	newInput->name = "Call"; // hidden ; used as button with event or var name , alt click to activate this input

	if( targetEvent )
	{
		idGraphNodeSocket& ownerInput = CreateInputSocket();
		ownerInput.name = "idClass ( self )";
		ownerInput.var = VarFromType( ev_object, graphState );

		*( ( idScriptClass* )ownerInput.var )->GetData() = ownerClass;

		int numargs = targetEvent->GetNumArgs();
		const char* formatspec = targetEvent->GetArgFormat();

		for( int i = 0; i < numargs; i++ )
		{
			idGraphNodeSocket* inp = &CreateInputSocket();
			inp->var = VarFromFormatSpec( formatspec[i], graphState );
			if( !inp->var )
			{
				inputSockets.RemoveIndexFast( inputSockets.Num() - 1 );
			}
		}

		idGraphNodeSocket* outFlow = &CreateOutputSocket();
		outFlow->name = "Out";

		idGraphNodeSocket* out = &CreateOutputSocket();
		out->var = VarFromFormatSpec( targetEvent->GetReturnType(), graphState );
		if( !out->var )
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
			idGraphNodeSocket* outFlow = &CreateOutputSocket();
			outFlow->name = "Out";
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
			SetOwner( *targetEvent, ( idClass** )&scriptThread );
		}
	}
}

idGraphNode* idClassNode::QueryNodeConstruction( idStateGraph* targetGraph, idClass* graphOwner,idStr contextName )
{
	auto& graph = *targetGraph;
	if( ImGui::MenuItem( "Call Event" ) )
	{
		auto* classNode = static_cast<idClassNode*>( graph.CreateNode( new idClassNode() ) );
		classNode->type = idClassNode::Call;
		classNode->Setup( graphOwner );
		return classNode;
	}
	if( ImGui::MenuItem( "Set Variable" ) )
	{
		auto* classNodeVarSet = static_cast<idClassNode*>( graph.CreateNode( new idClassNode() ) );
		classNodeVarSet->type = idClassNode::Set;
		classNodeVarSet->Setup( graphOwner );
		return classNodeVarSet;
	}
	if( ImGui::MenuItem( "Get Variable" ) )
	{
		auto* classNodeVarGet = static_cast<idClassNode*>( graph.CreateNode( new idClassNode() ) );
		classNodeVarGet->type = idClassNode::Get;
		classNodeVarGet->Setup( graphOwner );
		return classNodeVarGet;
	}
	return nullptr;
}

void idClassNode::SetOwner( idClass** target )
{
	nodeOwnerClass = target;

	if( ( *target )->IsType( idEntity::Type ) )
	{
		ownerEntityPtr = ( *target )->Cast<idEntity>();
	}
	else
	{
		ownerEntityPtr = nullptr;
	}
}

//returns false if def was in GraphThreadEventMap
bool idClassNode::SetOwner( const idEventDef& def, idClass** target )
{
	if( idStateGraph::GraphThreadEventMap.Find( def.GetEventNum() ) )
	{
		SetOwner( ( idClass** ) & ( graphState->graph ) );
		return false;
	}
	else
	{
		SetOwner( target );
		return true;
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

const char* idClassNode::GetName()
{
	currentTitle = "";
	switch( type )
	{
		case idClassNode::Call:
			return "Call Event";
			break;
		case idClassNode::Set:
			if( targetVariable )
			{
				currentTitle = "Set ";
			}
			break;
		case idClassNode::Get:
			if( targetVariable )
			{
				currentTitle = "Get ";
			}
			break;
	}

	if( targetVariable )
	{
		switch( targetVariable->GetType() )
		{
			case ev_string:
				currentTitle += "String";
				break;
			case ev_float:
				currentTitle += "Float";
				break;
			case ev_vector:
				currentTitle += "3D Vector";
				break;
			case ev_entity:
				currentTitle += "Entity";
				break;
			case ev_boolean:
				currentTitle += "Boolean";
				break;
			case ev_int:
				currentTitle += "Integer";
				break;
		}
		return currentTitle.c_str();
	};
	return "idClassNode";
}

void idClassNode::WriteBinary( idFile* file, ID_TIME_T* _timeStamp /*= NULL */ )
{
	file->WriteBig( ( int )type );
	file->WriteString( targetEventName );

	int index = -1;
	idStr outVarName = targetVariableName;

	if( isLocalvar )
	{
		idEntity* entPtr = ownerClass->Cast<idEntity>( );
		assert( entPtr );
		for( auto& localVar : graphState->localVariables )
		{
			index++;
			if( localVar.varName == targetVariableName )
			{
				outVarName.Format( "%s_%s_localVar_%i", entPtr->GetEntityDefName( ), graphState->name.c_str( ), index );
				break;
			}

		}
	}
	else if( isStaticVar )
	{
		for( auto& staticVar : idStateGraph::StaticVars )
		{
			index++;
			if( staticVar.varName == outVarName )
			{
				break;
			}
		}
	}

	file->WriteString( outVarName );
	file->WriteBig( index );
	file->WriteBool( isLocalvar );
	file->WriteBool( isStaticVar );

	idGraphNode::WriteBinary( file, _timeStamp );
}

bool idClassNode::LoadBinary( idFile* file, const ID_TIME_T _timeStamp, idClass* owner )
{
	if( file != NULL )
	{
		int index = -1;
		int newType = 0;
		file->ReadBig( newType );
		type = ( NodeType )newType;

		file->ReadString( targetEventName );
		if( !targetEventName.IsEmpty() )
		{
			targetEvent = idEventDef::FindEvent( targetEventName );
		}

		file->ReadString( targetVariableName );
		file->ReadBig( index );
		file->ReadBool( isLocalvar );
		file->ReadBool( isStaticVar );
		if( !targetVariableName.IsEmpty() )
		{
			if( isLocalvar )
			{
				targetVariable = graphState->localVariables[index].scriptVariable;
			}
			else if( !isStaticVar )
			{
				idList<idScriptVariableInstance_t> searchVar;
				idScriptVariableInstance_t& var = searchVar.Alloc( );
				var.varName = targetVariableName.c_str( );
				var.scriptVariable = nullptr;
				owner->GetType( )->GetScriptVariables( owner, searchVar );
				targetVariable = var.scriptVariable;
			}
			else
			{
				targetVariable = idStateGraph::StaticVars[index].scriptVariable;
			}
		}

		Setup( owner );
		return idGraphNode::LoadBinary( file, _timeStamp, owner );
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
CLASS_DECLARATION( idGraphNode, idGraphLogicNode )
END_CLASS

idGraphLogicNode::idGraphLogicNode( )
{
	nodeType = idGraphLogicNode::IF_EQU;
}

stateResult_t idGraphLogicNode::Exec( stateParms_t* parms )
{
	if( inputSockets[1].connections.Num() && inputSockets[2].connections.Num() )
	{
		idGraphNodeSocket& lhs = *inputSockets[1].connections[0];
		idGraphNodeSocket& rhs = *inputSockets[2].connections[0];

		switch( nodeType )
		{
			case idGraphLogicNode::IF_EQU:
				*boolOutput = lhs == rhs;
				break;
			case idGraphLogicNode::IF_NOTEQ:
				*boolOutput = lhs != rhs;
				break;
			case idGraphLogicNode::IF_GT:
				*boolOutput = lhs > rhs;
				break;
			case idGraphLogicNode::IF_LT:
				*boolOutput = lhs < rhs;
				break;
			case idGraphLogicNode::IF_GTE:
				*boolOutput = lhs >= rhs;
				break;
			case idGraphLogicNode::IF_LTE:
				*boolOutput = lhs <= rhs;
				break;
			default:
				*boolOutput = false;
				break;
		}

		if( *boolOutput )
		{
			outputSockets[0].active = true;
		}
		else
		{
			outputSockets[1].active = true;
		}
	}
	return SRESULT_DONE;
}

void idGraphLogicNode::WriteBinary( idFile* file, ID_TIME_T* _timeStamp /*= NULL */ )
{
	file->WriteBig( ( int )nodeType );
	file->WriteBig( numInputs );
	idGraphNode::WriteBinary( file, _timeStamp );
}

bool idGraphLogicNode::LoadBinary( idFile* file, const ID_TIME_T _timeStamp, idClass* owner /*= nullptr */ )
{
	file->ReadBig( ( int& )nodeType );
	file->ReadBig( numInputs );
	Setup( owner );
	idGraphNode::LoadBinary( file, _timeStamp, owner );
	return true;
}

void idGraphLogicNode::Setup( idClass* graphOwner )
{
	numInputs = 2;
	idGraphNodeSocket& newInput = CreateInputSocket();
	newInput.name = "  ";
	idGraphNodeSocket& newOutputTrue = CreateOutputSocket( );
	idGraphNodeSocket& newOutputFalse = CreateOutputSocket( );

	idGraphNodeSocket& resultOutput = CreateOutputSocket( );
	resultOutput.name = "Result";
	boolOutput = ( idScriptBool* ) VarFromType( ev_boolean, graphState );
	resultOutput.var = boolOutput;

	for( int i = 0; i < numInputs; i++ )
	{
		idGraphNodeSocket* inp = &CreateInputSocket( );
		inp->var = nullptr;
	}

}

idVec4 idGraphLogicNode::NodeTitleBarColor( )
{
	return idVec4( 0.7f, 0.3f, 3.0f, 1.0f );
}
