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

struct GraphNodePin
{
	ed::PinId			ID;
	GraphNode*			ParentNode;
	std::string			Name;
	PinType				Type;
	PinKind				Kind;

	GraphNodePin( int id, const char* name, PinType type, PinKind kind, idGraphNodeSocket* ownerSocket, GraphNode* parentNode ) :
		ID( id ), ParentNode( parentNode ), Name( name ), Type( type ), Kind( kind )
	{
		idGraphNodeSocket** value;
		if( socketHashIdx.Get( id, &value ) && *value != NULL )
		{
			assert( 1 );
		}
		else
		{
			socketHashIdx.Set( id, ownerSocket );
		}
	}
	~GraphNodePin()
	{
		idGraphNodeSocket** value;
		if( socketHashIdx.Get( ID.Get(), &value ) )
		{
			socketHashIdx.Remove( ID.Get() );
		}
		else
		{
			assert( 1 );
		}

	}
	static idHashTableT<int, idGraphNodeSocket*> socketHashIdx;
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

	idGraphNode* Owner;
	StateGraphEditor* Graph;
	GraphNode( int id, const char* name, idGraphNode* owner = nullptr, ImColor color = ImColor( 255, 255, 255 ) ) :
		dirty( false ), ID( id ), Name( name ), Color( color ), Type( NodeType::Tree ), Size( 0, 0 ), FirstDraw( true ), Owner( owner ), Graph( nullptr )
	{
		GraphNode** value;
		if( nodeHashIdx.Get( id, &value ) && *value != NULL )
		{
			assert( 1 );
		}
		else
		{
			nodeHashIdx.Set( id, this );
		}
	}
	~GraphNode()
	{
		GraphNode** value;
		if( nodeHashIdx.Get( ID.Get(), &value ) )
		{
			nodeHashIdx.Remove( ID.Get() );
		}
		else
		{
			assert( 1 );
		}

	}
	static idHashTableT<int, GraphNode*> nodeHashIdx;
};

class StateGraphEditor
{
public:

	struct Link
	{
		ed::LinkId ID;

		ed::PinId StartPinID;
		ed::PinId EndPinID;

		ImColor Color;

		Link() :
			ID( -1 ), StartPinID( -1 ), EndPinID( -1 ), Color( 255, 255, 255 )
		{
		}
		Link( ed::LinkId id, ed::PinId startPinId, ed::PinId endPinId ) :
			ID( id ), StartPinID( startPinId ), EndPinID( endPinId ), Color( 255, 255, 255 )
		{
		}
	};

	StateGraphEditor();
	virtual	~StateGraphEditor();
	StateGraphEditor( StateGraphEditor const& ) = delete;
	void operator=( StateGraphEditor const& ) = delete;

	void							Init();
	void							ShowIt( bool show );
	bool							IsShown() const;
	void							Draw();
	static StateGraphEditor&		Instance();
	static void						Enable( const idCmdArgs& args );

	const Link&						GetLinkByID( ed::LinkId& id );
	const int						GetLinkIndexByID( ed::LinkId& id );
	idList<Link*>					GetAllLinks( const GraphNode& target );
	void							DeleteAllPinsAndLinks( GraphNode& target );
	void							DeleteLink( StateGraphEditor::Link& id );
	void							DeleteNode( GraphNode* node );
	void							ReadNode( idGraphNode* node, GraphNode& newNode );
	void							ReadGraph( const GraphState* graph );
	void							Clear();
private:

	void Handle_ContextMenus();
	void Handle_NodeEvents();
	void DrawGraphEntityTest();
	void DrawMapGraph();
	void GetAllStateThreads();

	void DrawLeftPane( float paneWidth );

	idBlackBoard bb;

	ed::EditorContext*	EditorContext;
	int					nextElementId = 1;


	idList<GraphNode*>	nodeList;
	idList<Link>		linkList;

	bool				isShown;

	idGraphedEntity* graphEnt;
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