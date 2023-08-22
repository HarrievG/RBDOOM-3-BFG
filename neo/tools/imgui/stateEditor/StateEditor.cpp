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

#include "StateEditor.h"

#include "../imgui/BFGimgui.h"
#include "../imgui/imgui_internal.h"

#include "../d3xp/Game_local.h"
#include "../extern/imgui-node-editor/imgui_node_editor.h"
#include "../d3xp/StateGraph.h"

namespace ed = ax::NodeEditor;
namespace ImGuiTools
{
using namespace ImGui;

static bool Splitter( bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f )
{
	using namespace ImGui;
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	ImGuiID id = window->GetID( "##Splitter" );
	ImRect bb;
	ImVec2 size = ( split_vertically ? ImVec2( *size1, 0.0f ) : ImVec2( 0.0f, *size1 ) );
	bb.Min = ImVec2( window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y );
	ImVec2 itemSize = CalcItemSize( split_vertically ? ImVec2( thickness, splitter_long_axis_size ) : ImVec2( splitter_long_axis_size, thickness ), 0.0f, 0.0f );
	bb.Max = ImVec2( bb.Min.x + itemSize.x, bb.Min.y + itemSize.y );
	return SplitterBehavior( bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f );
}

StateGraphEditor::~StateGraphEditor()
{
	ed::DestroyEditor( EditorContext );
}

StateGraphEditor::StateGraphEditor()
	: isShown( false ),
	  graphEnt( nullptr )
{

}

ImGuiTools::GraphNode* StateGraphEditor::SpawnTreeSequenceNode()
{
	GraphNode& newNode = nodeList.Alloc() = { nextElementId++, "Sequence" };
	newNode.Type = NodeType::Tree;

	GraphNodePin& newInput = newNode.Inputs.Alloc() = { nextElementId++, "IN", PinType::Flow , PinKind::Input , &newNode };
	GraphNodePin& newOutput = newNode.Outputs.Alloc() = { nextElementId++, "OUT", PinType::Flow , PinKind::Output, &newNode };

	//BuildNode(&m_Nodes.back());
	return &newNode;
}

const int StateGraphEditor::GetLinkIndexByID( ed::LinkId& id )
{
	int index = -1;
	for( auto& link : linkList )
	{
		++index;
		if( link.ID == id )
		{
			break;
		}
	}
	return index;
}

void StateGraphEditor::ReadGraph(const idStateGraph* graph)
{
	for (auto node : graph->nodes)
	{
		auto& newNode = nodeList.Alloc() = { nextElementId++, node->GetName(), node };
		newNode.Type = NodeType::AnimState;
		for (auto& socket : node->inputSockets)
		{
			auto& newPin = newNode.Inputs.Alloc() = { nextElementId++, socket.name, PinType::Flow , PinKind::Input , &newNode };
		}
		for (auto& socket : node->outputSockets)
		{
			newNode.Outputs.Alloc() = { nextElementId++, socket.name, PinType::Flow , PinKind::Output, &newNode };
		}
	}
	for (idGraphNodeSocket::Link_t& link : graph->links)
	{
		auto& outputNode = nodeList[link.start->nodeIndex];
		auto& inputNode = nodeList[link.end->nodeIndex];
		Link& newLink = linkList.Alloc() = { ed::LinkId(nextElementId++), outputNode.Outputs[link.start->socketIndex].ID, inputNode.Inputs[link.end->socketIndex].ID };
	}
}

void StateGraphEditor::Clear()
{
	nodeList.Clear();
	linkList.Clear();
}

const ImGuiTools::StateGraphEditor::Link& StateGraphEditor::GetLinkByID( ed::LinkId id )
{
	for( auto& link : linkList )
	{
		if( link.ID == id )
		{
			return link;
		}
	}
}

void StateGraphEditor::Init()
{
	ed::Config config;
	config.SettingsFile = "Simple.json";
	EditorContext = ed::CreateEditor();
}

void StateGraphEditor::DrawPlayer()
{
	auto& io = ImGui::GetIO();
	{
		ImVec2 currentWidowSize = ImGui::GetWindowSize();

		ImGui::Text( "FPS: %.2f (%.2gms) %s ", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f, io.KeyCtrl ? "CTRL" : "" );

		ImGui::Separator();

		ed::SetCurrentEditor( EditorContext );

		if( ImGui::Button( "Zoom to Content" ) )
		{
			ed::NavigateToContent();
		}
		ImGui::SameLine();
		if( ImGui::Button( "Zoom to Selection" ) )
		{
			ed::NavigateToSelection( true );
		}

		ImGui::SameLine();
		if( ImGui::Button( "Test Anim Node" ) )
		{
			if( !graphEnt )
			{
				graphEnt = static_cast<idGraphedEntity*>( gameLocal.SpawnEntityType( idGraphedEntity::Type, NULL ) );
				graphEnt->PostEventMS( &EV_Activate, 0, graphEnt );
				ReadGraph(&graphEnt->graph);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Write Graph Bin"))
		{
			if ( graphEnt )
			{
				idFileLocal outputFile(fileSystem->OpenFileWrite("graphs/graphBin.bGrph", "fs_basepath"));
				graphEnt->graph.WriteBinary(outputFile);
			}
		}
		if (ImGui::Button("Load Graph Bin"))
		{
			if (!graphEnt)
			{
				idDict args;
				args.Set("graph", "graphBin");
				graphEnt = static_cast<idGraphedEntity*>(gameLocal.SpawnEntityType(idGraphedEntity::Type, &args));
				graphEnt->PostEventMS(&EV_Activate, 0, graphEnt);
				ReadGraph(&graphEnt->graph);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear Graph"))
		{
			Clear();
		}
		static float leftPaneWidth = 400.0f;
		static float rightPaneWidth = 800.0f;
		Splitter( true, 4.0f, &leftPaneWidth, &rightPaneWidth, 50.0f, 50.0f );

		DrawLeftPane( leftPaneWidth - 4.0f );
		ImGui::BeginChild( "CONTENT" );
		//ImGui::SameLine(0.0f, 12.0f);
		ed::Begin( "My Editor" );

		for( auto& node : nodeList )
		{
			node.Owner->Draw(&node);
		}

		for( auto& link : linkList )
		{
			ed::Link( link.ID, link.StartPinID, link.EndPinID, link.Color );
		}

		Handle_NodeEvents();

		ed::End();
		ImGui::EndChild();
		ed::SetCurrentEditor( nullptr );
	}
	//blackboard test
	{
		idScriptBool& A = *bb.Alloc<idScriptBool>();
		idScriptBool& B = *bb.Alloc<idScriptBool>();
		A = false;
		B = true;

		idScriptInt& iA = *bb.Alloc<idScriptInt>();
		idScriptInt& iB = *bb.Alloc<idScriptInt>();
		iA = MAXINT;
		iB = -MAXINT;

		idScriptFloat& fA = *bb.Alloc<idScriptFloat>();
		idScriptFloat& fB = *bb.Alloc<idScriptFloat>();
		fA = FLT_MAX;
		fB = -FLT_MAX;

		//const idScriptString& sA = *bb.Alloc("ScriptString!!!");;

		//gameLocal.Printf("%s", (const char*)(sA));

		bb.Free( &A );
		bb.Free( &fA );
		//bb.Free(&sA);
	}


}
void StateGraphEditor::Draw()
{
	bool showTool = isShown;
	auto& io = ImGui::GetIO();
	if( isShown )
	{
		impl::SetReleaseToolMouse( true );
	}

	if( ImGui::Begin( "State Editor", &showTool, ImGuiWindowFlags_MenuBar ) )
	{
		nextElementId = 1;
		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		if( ImGui::BeginTabBar( "StateEditors", tab_bar_flags ) )
		{
			if (ImGui::BeginTabItem("idGraphedEntity [ TEST ]"))
			{
				DrawPlayer();

				ImGui::EndTabItem();
			}

			if( ImGui::BeginTabItem( "idPlayer" ) )
			{
				ImGui::Text("Should create 4 idGraph's; 1 for the actor rvStateThread and 1 for each animState rvStateThread");
				//DrawPlayer();

				ImGui::EndTabItem();
			}
			if( ImGui::BeginTabItem( "idWeapon" ) )
			{
				ImGui::Text( "Should create 2 idGraph's; 1 for the weapon rvStateThread and 1 for the animState rvStateThread" );
				ImGui::EndTabItem();
			}
			if( ImGui::BeginTabItem( "idAI" ) )
			{
				ImGui::Text("Should create 4 idGraph's; 1 for the actor rvStateThread and 1 for each animState rvStateThread");
				ImGui::Text( "This is the Cucumber tab!\nblah blah blah blah blah" );
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

	}
	ImGui::End();

	if( isShown && !showTool )
	{
		// TODO: do the same as when pressing cancel?
		isShown = showTool;
		impl::SetReleaseToolMouse( false );
	}
}

ImGuiTools::StateGraphEditor& StateGraphEditor::Instance()
{
	static StateGraphEditor instance;
	return instance;

}

void StateGraphEditor::Enable( const idCmdArgs& args )
{
	StateGraphEditor::Instance().ShowIt( true );
}

void StateGraphEditor::GetAllStateThreads()
{

	idTypeInfo* a = idClass::GetClass( "idAnimated" );
	//auto* b = idTypeInfo::GetClassTypeInfo("idAnimated");
	DebugBreak();
}

void StateGraphEditor::DrawLeftPane( float paneWidth )
{
	auto& io = ImGui::GetIO();

	ImGui::BeginChild( "Selection", ImVec2( paneWidth, 0 ) );

	paneWidth = ImGui::GetContentRegionAvail().x;

	static bool showStyleEditor = false;
	if( ImGui::Button( "Show Flow" ) )
	{
		for( auto& link : linkList )
		{
			ed::Flow( link.ID );
		}
	}

	std::vector<ed::NodeId> selectedNodes;
	std::vector<ed::LinkId> selectedLinks;
	int selectedObjectCount = ed::GetSelectedObjectCount();
	if( selectedObjectCount )
	{
		selectedNodes.resize( selectedObjectCount );
		selectedLinks.resize( selectedObjectCount );
	}

	int nodeCount = ed::GetSelectedNodes( selectedNodes.data(), static_cast<int>( selectedNodes.size() ) );
	int linkCount = ed::GetSelectedLinks( selectedLinks.data(), static_cast<int>( selectedLinks.size() ) );

	selectedNodes.resize( nodeCount );
	selectedLinks.resize( linkCount );

	int saveIconWidth = 0;
	int saveIconHeight = 0;
	int restoreIconWidth = 0;
	int restoreIconHeight = 0;

	ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();

	ImGui::GetWindowDrawList()->AddRectFilled(
		cursorScreenPos,
		ImVec2( cursorScreenPos.x + paneWidth, cursorScreenPos.y + ImGui::GetTextLineHeight() ),
		ImColor( ImGui::GetStyle().Colors[ImGuiCol_HeaderActive] ), ImGui::GetTextLineHeight() * 0.25f );

	ImGui::TextUnformatted( "Nodes" );
	ImGui::Indent();
	for( auto& node : nodeList )
	{
		ImGui::PushID( node.ID.AsPointer() );
		auto start = ImGui::GetCursorScreenPos();

		bool isSelected = std::find( selectedNodes.begin(), selectedNodes.end(), node.ID ) != selectedNodes.end();
		if( ImGui::Selectable( ( node.Name + "##" + idStr( reinterpret_cast<unsigned int>( node.ID.AsPointer() ) ) ).c_str(), &isSelected ) )
		{
			if( io.KeyCtrl )
			{
				if( isSelected )
				{
					ed::SelectNode( node.ID, true );
				}
				else
				{
					ed::DeselectNode( node.ID );
				}
			}
			else
			{
				ed::SelectNode( node.ID, false );
			}

			ed::NavigateToSelection();
		}
		if( ImGui::IsItemHovered() && !node.State.IsEmpty() )
		{
			ImGui::SetTooltip( "State: %s", node.State.c_str() );
		}

		auto id = std::string( "(" ) + std::to_string( reinterpret_cast<uintptr_t>( node.ID.AsPointer() ) ) + ")";
		auto textSize = ImGui::CalcTextSize( id.c_str(), nullptr );
		auto iconPanelPos = ImVec2(
								start.x + paneWidth - ImGui::GetStyle().FramePadding.x - ImGui::GetStyle().IndentSpacing - saveIconWidth - restoreIconWidth - ImGui::GetStyle().ItemInnerSpacing.x * 1,
								start.y + ( ImGui::GetTextLineHeight() - saveIconHeight ) / 2 );
		ImGui::GetWindowDrawList()->AddText(
			ImVec2( iconPanelPos.x - textSize.x - ImGui::GetStyle().ItemInnerSpacing.x, start.y ),
			IM_COL32( 255, 255, 255, 255 ), id.c_str(), nullptr );

		auto drawList = ImGui::GetWindowDrawList();
		ImGui::SetCursorScreenPos( iconPanelPos );
		ImGui::SetItemAllowOverlap();
		if( node.SavedState.IsEmpty() )
		{
			//if (ImGui::InvisibleButton("save", ImVec2((float)saveIconWidth, (float)saveIconHeight)))
			//    node.SavedState = node.State;

			//if (ImGui::IsItemActive())
			//    drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
			//else if (ImGui::IsItemHovered())
			//    drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
			//else
			//    drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
		}
		else
		{
			//ImGui::Dummy(ImVec2((float)saveIconWidth, (float)saveIconHeight));
			// drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
		}

		ImGui::SameLine( 0, ImGui::GetStyle().ItemInnerSpacing.x );
		ImGui::SetItemAllowOverlap();
		if( !node.SavedState.IsEmpty() )
		{
			if( ImGui::InvisibleButton( "restore", ImVec2( ( float )restoreIconWidth, ( float )restoreIconHeight ) ) )
			{
				node.State = node.SavedState;
				ed::RestoreNodeState( node.ID );
				node.SavedState.IsEmpty();
			}

			//  if (ImGui::IsItemActive())
			//      drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
			//  else if (ImGui::IsItemHovered())
			//      drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
			//  else
			//      drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
		}
		else
		{
			//ImGui::Dummy(ImVec2((float)restoreIconWidth, (float)restoreIconHeight));
			//drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
		}

		ImGui::SameLine( 0, 0 );
		ImGui::SetItemAllowOverlap();
		ImGui::Dummy( ImVec2( 0, ( float )restoreIconHeight ) );

		ImGui::PopID();
	}
	ImGui::Unindent();

	static int changeCount = 0;

	ImGui::GetWindowDrawList()->AddRectFilled(
		cursorScreenPos,
		ImVec2( cursorScreenPos.x + paneWidth, cursorScreenPos.y + ImGui::GetTextLineHeight() ),
		ImColor( ImGui::GetStyle().Colors[ImGuiCol_HeaderActive] ), ImGui::GetTextLineHeight() * 0.25f );

	ImGui::TextUnformatted( "Selection" );

	ImGui::Text( "Changed %d time%s", changeCount, changeCount > 1 ? "s" : "" );
	if( ImGui::Button( "Deselect All" ) )
	{
		ed::ClearSelection();
	}
	ImGui::Indent();
	for( int i = 0; i < nodeCount; ++i )
	{
		ImGui::Text( "Node (%p)", selectedNodes[i].AsPointer() );
	}
	for( int i = 0; i < linkCount; ++i )
	{
		ImGui::Text( "Link (%p)", selectedLinks[i].AsPointer() );
	}
	ImGui::Unindent();

	if( ImGui::IsKeyPressed( ImGui::GetKeyIndex( ImGuiKey_Z ) ) )
		for( auto& link : linkList )
		{
			ed::Flow( link.ID );
		}

	if( ed::HasSelectionChanged() )
	{
		++changeCount;
	}

	ImGui::Separator();
	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player )
	{
		auto ret = idClass::GetClass( "idPlayer" )->GetScriptVariables( ( void* )player );
		for( auto& scriptVar : ret )
		{
			ImScriptVariable( scriptVar );
		}
	}
	ImGui::EndChild();
	SameLine();
}

void StateGraphEditor::Handle_NodeEvents()
{
	// Handle creation action, returns true if editor want to create new object (node or link)
	if( ed::BeginCreate() )
	{
		ed::PinId inputPinId, outputPinId;
		if( ed::QueryNewLink( &inputPinId, &outputPinId ) )
		{
			// QueryNewLink returns true if editor want to create new link between pins.
			//
			// Link can be created only for two valid pins, it is up to you to
			// validate if connection make sense. Editor is happy to make any.
			//
			// Link always goes from input to output. User may choose to drag
			// link from output pin or input pin. This determine which pin ids
			// are valid and which are not:
			//   * input valid, output invalid - user started to drag new ling from input pin
			//   * input invalid, output valid - user started to drag new ling from output pin
			//   * input valid, output valid   - user dragged link over other pin, can be validated

			if( inputPinId && outputPinId ) // both are valid, let's accept link
			{
				// ed::AcceptNewItem() return true when user release mouse button.
				if( ed::AcceptNewItem() )
				{
					// Since we accepted new link, lets add one to our list of links.
					Link& newLink = linkList.Alloc() = { ed::LinkId( nextElementId++ ), inputPinId, outputPinId };
				}

				// You may choose to reject connection between these nodes
				// by calling ed::RejectNewItem(). This will allow editor to give
				// visual feedback by changing link thickness and color.
			}
		}
	}
	ed::EndCreate(); // Wraps up object creation action handling.


	// Handle deletion action
	if( ed::BeginDelete() )
	{
		// There may be many links marked for deletion, let's loop over them.
		ed::LinkId deletedLinkId;
		while( ed::QueryDeletedLink( &deletedLinkId ) )
		{
			// If you agree that link can be deleted, accept deletion.
			if( ed::AcceptDeletedItem() )
			{
				// Then remove link from your data.
				linkList.RemoveIndexFast( GetLinkIndexByID( deletedLinkId ) );
			}

			// You may reject link deletion by calling:
			// ed::RejectDeletedItem();
		}
	}
	ed::EndDelete(); // Wrap up deletion action
}
}