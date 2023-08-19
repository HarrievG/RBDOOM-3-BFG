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
#include "d3xp/Game_local.h"

//namespace ax { namespace NodeEditor { class EditorContext; } }

namespace ImGuiTools
{
namespace ed = ax::NodeEditor;

template <class T>
void DrawNode( T* graphNode );

/**
* NodeEditor
*/
class StateGraphEditor
{
public:
	struct Node;
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
	struct Pin
	{
		ed::PinId   ID;
		Node*		NodePtr;
		std::string Name;
		PinType     Type;
		PinKind     Kind;

		Pin() :
			ID( -1 ), NodePtr( nullptr ), Name( "newPin" ), Type( PinType::Flow ), Kind( PinKind::Input )
		{
		}

		Pin( int id, const char* name, PinType type, PinKind kind, Node* parent ) :
			ID( id ), NodePtr( parent ), Name( name ), Type( type ), Kind( kind )
		{
		}
	};
	struct Node
	{
	public:
		ed::NodeId ID;
		idStr Name;
		idList<Pin> Inputs;
		idList<Pin> Outputs;
		ImColor Color;
		NodeType Type;
		ImVec2 Size;

		idStr State;
		idStr SavedState;

		idGraphNode* Owner;

		bool FirstDraw;
		Node() :
			ID( -1 ), Name( "newNode" ), Inputs(), Outputs(), Color( ImColor( 255, 255, 255 ) ), Type( NodeType::Tree ), Size( 0, 0 ), FirstDraw( true )
		{
		}
		Node( int id, const char* name, idGraphNode* owner = nullptr, ImColor color = ImColor( 255, 255, 255 ) ) :
			ID( id ), Name( name ), Owner( owner ), Color( color ), Type( NodeType::Tree ), Size( 0, 0 ), FirstDraw( true )
		{
		}
	};
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

	Node*		SpawnTreeSequenceNode();
	const Link& GetLinkByID( ed::LinkId id );
	const int	GetLinkIndexByID( ed::LinkId& id );
private:

	void Handle_PlayerNodeEvents();
	void Handle_PlayerNodeDraw( const Node& node );
	void DrawPlayer();
	void GetAllStateThreads();

	void DrawLeftPane( float paneWidth );

	idBlackBoard bb;

	ed::EditorContext*	EditorContext;
	int					nextElementId = 1;


	idList<Node>		nodeList;
	idList<Link>		linkList;

	bool				isShown;

	idGraphedEntity* graphEnt;

	void DrawAnimNode( Node& node );
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