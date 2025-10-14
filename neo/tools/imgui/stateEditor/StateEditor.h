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

#ifndef EDITOR_TOOLS_STATEEDITOR_H_
#define EDITOR_TOOLS_STATEEDITOR_H_

#include "../../edit_public.h"
#include "d3xp/script/Script_Program.h"
#include "../extern/imgui-node-editor/imgui_node_editor.h"
#include "../extern/imgui-node-editor/imgui_node_editor_internal.h"

#include "d3xp/StateGraph.h"

namespace ImGuiTools
{
namespace ed = ax::NodeEditor;

template <class T>
void DrawNode( T* graphNode );

/**
* NodeEditor
*/

enum class PinType
{
	Flow,
	Bool,
	Int,
	Float,
	String,
	Object,
	Function,
	Delegate,
};

enum class PinKind
{
	Output,
	Input
};

enum class NodeType
{
	AnimState,
	Simple,
	Tree,
	Comment,
	Houdini
};


struct GraphLink
{
	ed::LinkId ID;

	ed::PinId StartPinID;
	ed::PinId EndPinID;

	ImColor Color;

	GraphLink() :
		ID( -1 ), StartPinID( -1 ), EndPinID( -1 ), Color( 255, 255, 255 )
	{
	}
	GraphLink( ed::LinkId id, ed::PinId startPinId, ed::PinId endPinId ) :
		ID( id ), StartPinID( startPinId ), EndPinID( endPinId ), Color( 255, 255, 255 )
	{
	}
};

struct GraphEditContext
{

};

struct StateEditContext
{
	ed::EditorContext* Editor = nullptr;
	ed::Config Config;

	idStr name;
	idStateGraph* graphObject = nullptr;
	idEntityPtr<idEntity>	graphOwner;
	idList<GraphNode*>		nodeList;
	idList<GraphLink>		linkList;
	bool Loaded = false;
	idStr file;
	int nextElementId = 1;
	idHashTableT<int, idGraphNodeSocket*> socketHashIdx;
	idHashTableT<int, GraphNode*> nodeHashIdx;
	idStr saveState;
	int lastSave = 0;
	idList<idScriptStr*> localVarNames;
	ed::PinId lastDragSocket = ed::PinId::Invalid;
};


struct GraphNodePin
{
	ed::PinId			ID;
	GraphNode*			ParentNode;
	std::string			Name;
	PinType				Type;
	PinKind				Kind;
	StateEditContext& targetContext;

	GraphNodePin( StateEditContext& graphContext, int id, const char* name, PinType type, PinKind kind, idGraphNodeSocket* ownerSocket, GraphNode* parentNode ) :
		ID( id ), ParentNode( parentNode ), Name( name ), Type( type ), Kind( kind ), targetContext( graphContext )
	{
		idGraphNodeSocket** value;
		if( targetContext.socketHashIdx.Get( id, &value ) && *value != NULL )
		{
			assert( 0 );
		}
		else
		{
			targetContext.socketHashIdx.Set( id, ownerSocket );
		}
	}
	~GraphNodePin()
	{
		idGraphNodeSocket** value;
		if( targetContext.socketHashIdx.Get( ID.Get(), &value ) )
		{
			targetContext.socketHashIdx.Remove( ID.Get() );
		}
		else
		{
			assert( 0 );
		}

	}
};

struct GraphNode
{
public:
	bool dirty;
	ed::NodeId ID;
	idStr Name;
	idList<GraphNodePin*> Inputs;
	idList<GraphNodePin*> Outputs;

	ImColor Color;
	NodeType Type;
	ImVec2 Size;

	idStr State;
	idStr SavedState;
	bool FirstDraw;
	ImVec2 Position;

	idGraphNode* Owner;
	StateGraphEditor* Graph;
	StateEditContext& targetContext;
	ImDrawList* drawList;
	GraphNode( StateEditContext& graphContext, int id, const char* name, idGraphNode* owner = nullptr, ImColor color = ImColor( 255, 255, 255 ) ) :
		dirty( false ), ID( id ), Name( name ), Color( color ), Type( NodeType::Tree ), Size( 0, 0 ), FirstDraw( false ), Position( ImVec2( 0, 0 ) ), Owner( owner ), Graph( nullptr )
		, targetContext( graphContext ), drawList( nullptr )
	{
		GraphNode** value;
		if( targetContext.nodeHashIdx.Get( id, &value ) && *value != NULL )
		{
			assert( 0 );
		}
		else
		{
			targetContext.nodeHashIdx.Set( id, this );
		}
	}
	~GraphNode()
	{
		GraphNode** value;
		if( targetContext.nodeHashIdx.Get( ID.Get(), &value ) )
		{
			targetContext.nodeHashIdx.Remove( ID.Get() );
		}
		else
		{
			assert( 0 );
		}

	}
};


class StateGraphEditor
{
public:
	StateGraphEditor();
	virtual	~StateGraphEditor();
	StateGraphEditor( StateGraphEditor const& ) = delete;
	void operator=( StateGraphEditor const& ) = delete;

	void							Init();
	void							ShowIt( bool show );
	bool							IsShown() const;
	void							Draw();

	void							Draw( StateEditContext& graphContext );
	static StateGraphEditor&		Instance();
	static void						Enable( const idCmdArgs& args );

	const int						GetLinkIndexByID( ed::LinkId& i, StateEditContext& graphContext );
	idList<GraphLink*>				GetAllLinks( const GraphNode& target, StateEditContext& graphContext );
	void							DeleteAllPinsAndLinks( GraphNode& target, StateEditContext& graphContext );
	void							DeleteLink( GraphLink& id, StateEditContext& graphContext );
	void							DeleteNode( GraphNode* node , StateEditContext& graphContext );
	void							DeleteLocalVar( int index, StateEditContext& graphContext );
	void							ReadNode( idGraphNode* node, GraphNode& newNode, StateEditContext& graphContext );
	void							LoadGraph( StateEditContext& graphContext );

	void							Clear( StateEditContext& graphContext );

	int								NextNodeID( StateEditContext& graphContext )
	{
		return graphContext.nextElementId++;
	}
	int								NextPinID( StateEditContext& graphContext )
	{
		return graphContext.nextElementId++;
	}
	int								NextLinkID( StateEditContext& graphContext )
	{
		return graphContext.nextElementId++;
	}
	int activeStateIndex = -1;
	int contextStateIndex = -1;
	idStr contextStateName = idStateGraph::MAIN;
private:

	void Handle_ContextMenus( StateEditContext& graphContext );
	void Handle_NodeEvents( StateEditContext& graphContext );
	void DrawGraphEntityTest();
	void DrawMapGraph();

	void DrawIdPlayer();

	void DrawLeftPane( float paneWidth, StateEditContext& graphContext );
	void DrawEditorButtonBar( StateEditContext& graphContext );

	StateEditContext	Context;
	int					nextElementId = 1;

	bool				isShown;




	//replace this with only the graph, editor does care about entity but not as parent of the graph.
	idEntityPtr<idGraphedEntity> graphEnt;

	static idList<idGraphNode*> nodeTypes;
};

inline void StateGraphEditor::ShowIt( bool show )
{
	isShown = show;
}

inline bool StateGraphEditor::IsShown() const
{
	return isShown;
}


}

#endif