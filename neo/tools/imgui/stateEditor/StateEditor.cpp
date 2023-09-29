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

#include "../d3xp/Game_local.h"
#include "../extern/imgui-node-editor/imgui_node_editor.h"
#include "../extern/imgui-node-editor/imgui_node_editor_internal.h"
#include "../d3xp/StateGraph.h"

# define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "framework/Common_local.h"

idCVar graph_autoSaveLayout( "graph_autoSaveEditorContext", "30000", CVAR_TOOL | CVAR_INTEGER, "timeout in ms between automatic saves of ed::EditorContext " );

namespace ed = ax::NodeEditor;
namespace ImGuiTools
{
using namespace ImGui;

idList<idGraphNode*> StateGraphEditor::nodeTypes;
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
	ed::DestroyEditor( Context.Editor );
}

StateGraphEditor::StateGraphEditor()
	: isShown( false )
{

}

const int StateGraphEditor::GetLinkIndexByID( ed::LinkId& id, StateEditContext& graphContext )
{
	int index = -1;
	for( auto& link : graphContext.linkList )
	{
		++index;
		if( link.ID == id )
		{
			break;
		}
	}
	return index;
}

idList<GraphLink*> StateGraphEditor::GetAllLinks( const GraphNode& target, StateEditContext& graphContext )
{
	idList<GraphLink*> result;

	if( !graphContext.linkList.Num() )
	{
		return result;
	}

	int foundLinks = 0;
	int linkIdx = 0;
	while( linkIdx < graphContext.linkList.Num() )
	{
		auto& link = graphContext.linkList[linkIdx];

		for( GraphNodePin* pin : target.Inputs )
		{
			if( pin->ID == link.EndPinID )
			{
				result.Alloc() = &link;
				foundLinks++;
				break;
			}
		}

		for( GraphNodePin* pin : target.Outputs )
		{
			if( pin->ID == link.StartPinID )
			{
				result.Alloc() = &link;
				foundLinks++;
				break;
			}
		}
		linkIdx++;
	}

	return result;
}

void StateGraphEditor::DeleteAllPinsAndLinks( GraphNode& target, StateEditContext& graphContext )
{
	auto links = GetAllLinks( target, graphContext );
	for( auto* link : links )
	{
		DeleteLink( *link, graphContext );
	}
	target.Inputs.DeleteContents();
	target.Outputs.DeleteContents();
}

void StateGraphEditor::DeleteLink( GraphLink& link, StateEditContext& graphContext )
{
	int idx = GetLinkIndexByID( link.ID, graphContext );

	idGraphNodeSocket** inputSocketPtr;
	idGraphNodeSocket** outputSocketPtr;
	graphContext.socketHashIdx.Get( link.StartPinID.Get(), &outputSocketPtr );
	graphContext.socketHashIdx.Get( link.EndPinID.Get(), &inputSocketPtr );

	idGraphNodeSocket* inputSocket = *inputSocketPtr;
	idGraphNodeSocket* outputSocket = *outputSocketPtr;

	if( outputSocket->isOutput )
	{
		graphContext.graphObject->RemoveLink( outputSocket, inputSocket );
	}
	else
	{
		graphContext.graphObject->RemoveLink( inputSocket, outputSocket );
	}

	graphContext.linkList.RemoveIndexFast( idx );
}

void StateGraphEditor::DeleteNode( GraphNode* node, StateEditContext& graphContext )
{
	DeleteAllPinsAndLinks( *node, graphContext );
	graphContext.graphObject->RemoveNode( node->Owner );
	graphContext.nodeList.Remove( node );
	delete node;
}

void StateGraphEditor::ReadNode( idGraphNode* node, GraphNode& newNode , StateEditContext& graphContext )
{
	DeleteAllPinsAndLinks( newNode, graphContext );

	for( auto& socket : node->inputSockets )
	{
		newNode.Inputs.Alloc() = new GraphNodePin( graphContext, NextPinID( graphContext ), socket.name, PinType::Flow, PinKind::Input, &socket, &newNode );
	}

	for( auto& socket : node->outputSockets )
	{
		newNode.Outputs.Alloc() = new GraphNodePin( graphContext, NextPinID( graphContext ), socket.name, PinType::Flow, PinKind::Output, &socket, &newNode );
	}

	newNode.Graph = this;
}

void StateGraphEditor::LoadGraph( StateEditContext& graphContext )
{
	if( graphContext.Loaded )
	{
		auto& graphState = graphContext.graphObject->localGraphState[0];

		idStr posInfoFilename = graphContext.file;
		posInfoFilename.SetFileExtension( "bnpos" );
		idFileLocal posInfoFile( fileSystem->OpenFileRead( posInfoFilename, "fs_basepath" ) );
		idList<idVec2> nodePositions;
		if( posInfoFile )
		{
			for( int i = 0; i < graphState.nodes.Num(); i++ )
			{
				posInfoFile->ReadVec2( nodePositions.Alloc() );
			}
		}

		int posIndex = 0;
		for( auto node : graphState.nodes )
		{
			auto& newNode = *( graphContext.nodeList.Alloc() = new GraphNode( graphContext, NextNodeID( graphContext ), node->GetName(), node ) );
			ReadNode( node, newNode, graphContext );

			if( posInfoFile )
			{
				newNode.Position = nodePositions[posIndex].ToFloatPtr();
				newNode.FirstDraw = true;
				posIndex++;
			}
		}
		for( idGraphNodeSocket::Link_t& link : graphState.links )
		{
			auto& inputNode = *graphContext.nodeList[link.end->nodeIndex];
			auto& outputNode = *graphContext.nodeList[link.start->nodeIndex];
			GraphLink& newLink = graphContext.linkList.Alloc();
			newLink.ID			= ed::LinkId( NextLinkID( graphContext ) );
			newLink.StartPinID	= outputNode.Outputs[link.start->socketIndex]->ID;
			newLink.EndPinID	= inputNode.Inputs[link.end->socketIndex]->ID;
			auto* var = inputNode.Owner->inputSockets[link.end->socketIndex].var;
			if( var )
			{
				ImGui::IconItem icon = ImGui::ImScriptVariable( "", { "", var }, false );
				newLink.Color = icon.color;
			}
		};
	}
}

void StateGraphEditor::Clear( StateEditContext& graphContext )
{
	graphContext.nodeList.Clear();
	graphContext.linkList.Clear();
	nextElementId = 1;
}

bool SaveConfig( const char* data, size_t size, ed::SaveReasonFlags reason, void* userPointer )
{
	auto* editorContext = static_cast< StateEditContext* >( userPointer );

	if( editorContext->lastSave + graph_autoSaveLayout.GetInteger() < gameLocal.GetTime() || !editorContext->lastSave )
	{
		//common->Printf("SaveConfig %s\n", editorContext->File.c_str());
		//common->Printf("\t%s\n", data);

		if( !editorContext->file.IsEmpty( ) )
		{
			editorContext->saveState = data;
			idFileLocal outputFile( fileSystem->OpenFileWrite( editorContext->file + ".json", "fs_basepath" ) );

			outputFile->Printf( editorContext->saveState );
		}
		editorContext->lastSave = gameLocal.GetTime();
	}

	return true;
}

size_t LoadConfig( char* data, void* userPointer )
{
	auto* editorContext = static_cast<StateEditContext*>( userPointer );
	//common->Printf("LoadConfig %s\n", static_cast<StateEditContext*>(userPointer)->File.c_str());
	idFileLocal inFile( fileSystem->OpenFileReadMemory( editorContext->file + ".json" ) );
	if( inFile != NULL && inFile->Length() > 0 )
	{
		if( data )
		{
			inFile->Read( data, inFile->Length() );
		}
		return inFile->Length();
	}

	return 0;
}

bool SaveNode( ed::NodeId nodeId, const char* data, size_t size, ed::SaveReasonFlags reason, void* userPointer )
{
	//common->Printf("SaveNode %s\n", static_cast<StateEditContext*>(userPointer)->File.c_str());
	//common->Printf("\t%s\n", data);
	return true;
}

size_t LoadNode( ed::NodeId nodeId, char* data, void* userPointer )
{
	//common->Printf("LoadNode %s\n", static_cast<StateEditContext*>(userPointer)->File.c_str());
	//common->Printf("\t%s\n", data);
	return 0;
}

void  ConfigSession( void* userPointer )
{
	//common->Printf("ConfigEditorSession %s\n" , static_cast<StateEditContext*>(userPointer)->File.c_str());
}

void StateGraphEditor::Init()
{
	ed::Config config;
	config.SettingsFile = "Simple.json";

	config.BeginSaveSession = ConfigSession;
	config.EndSaveSession = ConfigSession;
	config.SaveSettings = SaveConfig;
	config.LoadSettings = LoadConfig;
	config.SaveNodeSettings = SaveNode;
	config.LoadNodeSettings = LoadNode;
	config.UserPointer = &Context;

	Context.Editor = ed::CreateEditor( &config );
	if( !nodeTypes.Num() )
	{
		auto* childNode = idGraphNode::Type.node.GetChild();
		if( childNode )
		{
			nodeTypes.Append( static_cast<idGraphNode*>( childNode->CreateInstance() ) );
			for( const idTypeInfo* c = childNode->node.GetSibling(); c != NULL; c = c->node.GetSibling() )
			{
				nodeTypes.Append( static_cast<idGraphNode*>( c->CreateInstance() ) );
			}
		}
		else
		{
			assert( 0 );
		}
	}


}

void StateGraphEditor::DrawGraphEntityTest()
{
	if( ImGui::Button( "Load Graph Bin" ) )
	{
		if( !graphEnt )
		{
			idDict args;
			args.Set( "graphObject", "graphBin" );
			graphEnt = static_cast< idGraphedEntity* >( gameLocal.SpawnEntityType( idGraphedEntity::Type, &args ) );
			graphEnt->PostEventMS( &EV_Activate, 0, graphEnt.GetEntity( ) );
			Context.graphObject = graphEnt->graphObject;
			Context.graphOwner = graphEnt;
			Context.Loaded = true;
			Context.file = "graphs/graphBin.bGrph";
			Context.name = "GraphEntityTest";
			LoadGraph( Context );
		}
	}

	ImGui::SameLine( );
	if( ImGui::Button( "SpawnGraphEntity" ) )
	{
		if( !graphEnt.IsValid( ) )
		{
			idDict	args;
			args.Set( "entkey", "idGraphedEntity__80" );
			args.Set( "intkey", "1234" );
			args.Set( "graphObject", "" );
			graphEnt = static_cast< idGraphedEntity* >( gameLocal.SpawnEntityType( idGraphedEntity::Type, &args ) );
			graphEnt->PostEventMS( &EV_Activate, 0, graphEnt.GetEntity( ) );
			LoadGraph( Context );
		}
	}

	if( graphEnt.IsValid( ) )
	{
		Draw( Context );
	}
}

void StateGraphEditor::DrawMapGraph()
{
	static StateEditContext MapGraphContext;
	if( MapGraphContext.Editor == nullptr )
	{
		idStr currentMap  = commonLocal.GetCurrentMapName();
		if( !currentMap.IsEmpty() )
		{
			MapGraphContext.file = "maps/" + currentMap.SetFileExtension( ".bgrph" );
		}

		MapGraphContext.Config.BeginSaveSession = ConfigSession;
		MapGraphContext.Config.EndSaveSession = ConfigSession;
		MapGraphContext.Config.SaveSettings = SaveConfig;
		MapGraphContext.Config.LoadSettings = LoadConfig;
		MapGraphContext.Config.SaveNodeSettings = SaveNode;
		MapGraphContext.Config.LoadNodeSettings = LoadNode;
		MapGraphContext.Config.UserPointer = &MapGraphContext;
		MapGraphContext.name = "MapGraph";

		MapGraphContext.Config.SettingsFile = MapGraphContext.file;
		MapGraphContext.Editor = ed::CreateEditor( &MapGraphContext.Config );

		idEntityPtr<idEntity> worldEnt;
		worldEnt = gameLocal.entities[ENTITYNUM_WORLD];

		if( worldEnt && worldEnt->graphObject )
		{
			MapGraphContext.graphOwner = worldEnt;
			MapGraphContext.graphObject = worldEnt->graphObject;
			MapGraphContext.Loaded = true;
			LoadGraph( MapGraphContext );
		}
		return;
	}

	Draw( MapGraphContext );
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
		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		if( ImGui::BeginTabBar( "StateEditors", tab_bar_flags ) )
		{
			if( ImGui::BeginTabItem( "idGraphedEntity [ TEST ]" ) )
			{
				DrawGraphEntityTest();
				ImGui::EndTabItem();
			}
			if( ImGui::BeginTabItem( "MapGraph" ) )
			{
				DrawMapGraph();
				ImGui::EndTabItem();
			}
			if( ImGui::BeginTabItem( "idPlayer" ) )
			{
				ImGui::Text( "Should create 4 idGraph's; 1 for the actor rvStateThread and 1 for each animState rvStateThread" );
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
				ImGui::Text( "Should create 4 idGraph's; 1 for the actor rvStateThread and 1 for each animState rvStateThread" );
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

void StateGraphEditor::Draw( StateEditContext& graphContext )
{
	ed::SetCurrentEditor( graphContext.Editor );

	DrawEditorButtonBar( graphContext );

	ImGui::BeginChild( graphContext.name );
	static float leftPaneWidth = 400.0f;
	static float rightPaneWidth = 800.0f;
	Splitter( true, 4.0f, &leftPaneWidth, &rightPaneWidth, 50.0f, 50.0f );

	DrawLeftPane( leftPaneWidth - 4.0f , graphContext );

	//ImGui::SameLine(0.0f, 12.0f);
	ed::Begin( graphContext.name + "_editor" );

	if( graphContext.graphOwner.IsValid() )
	{
		for( auto* nodePtr : graphContext.nodeList )
		{
			auto& node = *nodePtr;
			ImGui::PushID( node.ID.Get() );
			if( node.dirty )
			{
				ReadNode( node.Owner, node, graphContext );
				node.dirty = false;
			}

			node.Owner->Draw( &node );
			if( node.FirstDraw )
			{
				ed::SetNodePosition( node.ID, node.Position );
				node.FirstDraw = false;
			}
			if( ImGui::IsItemVisible() )
			{
				node.drawList = ed::GetNodeBackgroundDrawList( node.ID );
			}
			else
			{
				node.drawList = nullptr;
			}
			ImGui::PopID();
		}

		for( auto& link : graphContext.linkList )
		{
			ImGui::PushID( link.ID.Get() );
			ed::Link( link.ID, link.StartPinID, link.EndPinID, link.Color, 2.0f );
			ImGui::PopID();

			idGraphNodeSocket** inputSocketPtr;
			graphContext.socketHashIdx.Get( link.EndPinID.Get(), &inputSocketPtr );
			idGraphNodeSocket* inputSocket = *inputSocketPtr;
			idGraphNodeSocket** outputSocketPtr;
			graphContext.socketHashIdx.Get( link.StartPinID.Get(), &outputSocketPtr );
			idGraphNodeSocket* outputSocket = *outputSocketPtr;

			int last = gameLocal.previousTime;

			if( ( outputSocket->lastActivated > last ) && inputSocket->lastActivated > last )
			{
				ed::Flow( link.ID );
			}
		}
	}
	else
	{
		Clear( graphContext );
	}

	Handle_ContextMenus( graphContext );
	Handle_NodeEvents( graphContext );

	ed::End();
	ImGui::EndChild();

	ed::SetCurrentEditor( nullptr );
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

void StateGraphEditor::DrawLeftPane( float paneWidth, StateEditContext& graphContext )
{
	auto& io = ImGui::GetIO();
	const float TEXT_BASE_WIDTH = ImGui::CalcTextSize( "A" ).x;
	const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

	ImGui::BeginChild( "Selection", ImVec2( paneWidth, 0 ) );

	paneWidth = ImGui::GetContentRegionAvail().x;

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
		ImColor( ImGui::GetStyle().Colors[ImGuiCol_HeaderActive] ), 1.0f );
	ImGui::Dummy( ImVec2( 0, ImGui::GetTextLineHeight() ) );
	ImGui::GetWindowDrawList()->AddText( cursorScreenPos, ImColor( 50.0f, 45.0f, 255.0f, 255.0f ), "Nodes" );

	ImGui::Indent();
	for( auto* nodePtr : graphContext.nodeList )
	{
		auto& node = *nodePtr;

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
		for( auto& link : graphContext.linkList )
		{
			ed::Flow( link.ID );
		}

	if( ed::HasSelectionChanged() )
	{
		++changeCount;
	}

	ImGui::Separator();


	if( graphContext.graphOwner.IsValid() )
	{
		if( ImGui::CollapsingHeader( "States" ) )
		{
			if( ImGui::Button( "Create New" ) )
			{
				graphContext.graphObject->CreateSubState( "New Function", {}, {} );
			}
			auto& graphState = graphContext.graphObject->localGraphState;
			auto& graphNames = graphContext.graphObject->localStates;
			if( graphState.Num() > 1 )
			{
				for( int i = 1; i < graphState.Num(); i++ )
				{
					ImGui::Text( graphNames[i] );
				}
			}
		}
		if( ImGui::CollapsingHeader( "Events" ) )
		{
			if( ImGui::Button( "Create New" ) )
			{
				graphContext.graphObject->CreateSubState( "New Function", {}, {} );
			}
			auto& graphState = graphContext.graphObject->localGraphState;
			auto& graphNames = graphContext.graphObject->localStates;
			if( graphState.Num() > 1 )
			{
				for( int i = 1; i < graphState.Num(); i++ )
				{
					ImGui::Text( graphNames[i] );
				}
			}
		}

		if( ImGui::CollapsingHeader( "Variables" ) )
		{
			ImGui::TextDisabled( "Self ( %s )", graphContext.graphOwner->GetClassname() );
			ImGui::Separator();
			idList<idScriptVariableInstance_t> thisVars;
			graphContext.graphOwner->GetType()->GetScriptVariables( graphContext.graphOwner , thisVars );

			static ImGuiTableFlags flags = ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingStretchProp;
			if( ImGui::BeginTable( "selfVars", 3, flags ) )
			{
				ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_IndentDisable, 25 );
				ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_IndentDisable, TEXT_BASE_WIDTH * 20 );
				ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_IndentDisable );

				for( auto& var : thisVars )
				{
					ImGui::TableNextRow( 0 );
					ImGui::TableSetColumnIndex( 0 );
					ImGui::Dummy( ImVec2( 25, 25 ) );
					ImGui::TableSetColumnIndex( 1 );
					ImGui::Text( var.varName );
					ImGui::TableSetColumnIndex( 2 );
					ImGui::ImScriptVariable( var.varName, var );
				}

				ImGui::EndTable();
			}

			ImGui::TextDisabled( "Local" );
			ImGui::Separator();

			static ImGuiTableFlags flags2 = ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingStretchProp;
			if( ImGui::BeginTable( "localVars", 3, flags2 ) )
			{
				ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_IndentDisable, 25 );
				ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_IndentDisable, TEXT_BASE_WIDTH * 20 );
				ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_IndentDisable );

				const idList<idScriptVariableInstance_t>& vars = graphContext.graphObject->GetVariables();
				for( auto& var : vars )
				{
					ImGui::TableNextRow( 0 );
					ImGui::TableSetColumnIndex( 0 );
					ImGui::Dummy( ImVec2( 25, 25 ) );
					ImGui::TableSetColumnIndex( 1 );
					ImGui::Text( var.varName );
					ImGui::TableSetColumnIndex( 2 );
					ImGui::ImScriptVariable( var.varName, var );
				}

				ImGui::EndTable();
			}

			if( ImGui::Button( "Create New" ) )
			{
				ImGui::OpenPopup( "CreateLocalVar" );

				//graphEnt->graph.CreateSubState("New Function", {}, {});
			}
			if( ImGui::BeginPopup( "CreateLocalVar" ) )
			{
				ImGui::TextDisabled( "Pick Type:" );
				ImGui::BeginChild( "popup_scroller", ImVec2( 200, 150 ), true );
				//extern idScriptVariableBase * VarFromType(etype_t type, GraphState * graphState);
				if( ImGui::MenuItem( "Boolean" ) )
				{
					graphContext.graphObject->CreateVariable( "newBoolean", ev_boolean );
					//localVars. VarFromType(ev_boolean, &graphEnt->graph.CreateVar);
					//return ;
				}
				if( ImGui::MenuItem( "Float" ) )
				{
					//return ev_float;
				}
				if( ImGui::MenuItem( "Integer" ) )
				{
					//return ev_int;
				}
				if( ImGui::MenuItem( "3d Vector" ) )
				{
					//return ev_vector;
				}
				if( ImGui::MenuItem( "String" ) )
				{
					//return ev_string;
				}
				if( ImGui::MenuItem( "Entity" ) )
				{
				}

				ImGui::EndChild();
				ImGui::EndPopup();
			}
		}
	}

	ImGui::EndChild();
	SameLine();
}

void StateGraphEditor::DrawEditorButtonBar( StateEditContext& graphContext )
{
	auto& io = ImGui::GetIO();
	ImGui::Text( "FPS: %.2f (%.2gms) %s ", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f, io.KeyCtrl ? "CTRL" : "" );

	ImGui::Separator();

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
	if( ImGui::Button( "Clear Graph" ) )
	{
		Clear( graphContext );
	}
	ImGui::SameLine();
	if( ImGui::Button( "Write Graph Bin" ) )
	{
		if( graphContext.graphObject )
		{
			idFileLocal outputFile( fileSystem->OpenFileWrite( graphContext.file, "fs_basepath" ) );
			graphContext.graphObject->WriteBinary( outputFile );
			idStr posOutFilename = graphContext.file;
			posOutFilename.SetFileExtension( "bnpos" );
			idFileLocal posOutputFile( fileSystem->OpenFileWrite( posOutFilename, "fs_basepath" ) );
			for( auto* node : graphContext.nodeList )
			{
				auto imVec = ed::GetNodePosition( node->ID );
				posOutputFile->WriteVec2( idVec2( imVec.x, imVec.y ) );
			}
			//HVG Todo:
			//Zoomlevel is not correctly set in the data!
			graphContext.lastSave = 0;
			SaveConfig( graphContext.saveState, graphContext.saveState.Allocated( ), ed::SaveReasonFlags::None, &graphContext );
		}
	}

}

void StateGraphEditor::Handle_ContextMenus( StateEditContext& graphContext )
{
	static ed::NodeId contextNodeId = 0;
	static ed::LinkId contextLinkId = 0;
	static ed::PinId  contextPinId = 0;
	auto openPopupPosition = ImGui::GetMousePos();
	ed::Suspend();
	if( ed::ShowNodeContextMenu( &contextNodeId ) )
	{
		ImGui::OpenPopup( "Node Context Menu" );
	}
	else if( ed::ShowPinContextMenu( &contextPinId ) )
	{
		ImGui::OpenPopup( "Pin Context Menu" );
	}
	else if( ed::ShowLinkContextMenu( &contextLinkId ) )
	{
		ImGui::OpenPopup( "Link Context Menu" );
	}
	else if( ed::ShowBackgroundContextMenu() )
	{
		ImGui::OpenPopup( "Create New Node" );
	}

	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 8, 8 ) );
	if( ImGui::BeginPopup( "Create New Node" ) )
	{
		idList<int> classNums;
		for( auto* nodePtr : nodeTypes )
		{
			idGraphNode* createdNode = nodePtr->QueryNodeConstruction( graphContext.graphObject, graphContext.graphOwner );
			if( createdNode != nullptr )
			{
				auto& newNode = *( graphContext.nodeList.Alloc() = new GraphNode( graphContext, NextNodeID( graphContext ), createdNode->GetName(), createdNode ) );
				ReadNode( createdNode, newNode , graphContext );
				ed::SetNodePosition( newNode.ID, openPopupPosition );
			}
		}
		ImGui::EndPopup();
	}
	ImGui::PopStyleVar();

	ed::Resume();
}

void StateGraphEditor::Handle_NodeEvents( StateEditContext& graphContext )
{
	auto showLabel = []( const char* label, ImColor color )
	{
		ImGui::SetCursorPosY( ImGui::GetCursorPosY() - ImGui::GetTextLineHeight() );
		auto size = ImGui::CalcTextSize( label );

		auto padding = ImGui::GetStyle().FramePadding;
		auto spacing = ImGui::GetStyle().ItemSpacing;

		ImGui::SetCursorPos( ImGui::GetCursorPos() + ImVec2( spacing.x, -spacing.y ) );

		auto rectMin = ImGui::GetCursorScreenPos() - padding;
		auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

		auto drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled( rectMin, rectMax, color, size.y * 0.15f );
		ImGui::TextUnformatted( label );
	};

	// Handle creation action, returns true if editor want to create new object (node or link)
	if( ed::BeginCreate() )
	{
		ed::PinId startPinId, endPinId;
		if( ed::QueryNewLink( &startPinId, &endPinId ) )
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

			if( startPinId && endPinId ) // both are valid, can we accept link?
			{
				idGraphNodeSocket** inputSocketPtr;
				idGraphNodeSocket** outputSocketPtr;
				graphContext.socketHashIdx.Get( startPinId.Get(), &outputSocketPtr );
				graphContext.socketHashIdx.Get( endPinId.Get(), &inputSocketPtr );

				idGraphNodeSocket* inputSocket = *inputSocketPtr;
				idGraphNodeSocket* outputSocket = *outputSocketPtr;

				// ed::AcceptNewItem() return true when user release mouse button.
				if( ed::AcceptNewItem() )
				{
					if( outputSocket->isOutput )
					{
						graphContext.graphObject->AddLink( *outputSocket, *inputSocket );
					}
					else
					{
						graphContext.graphObject->AddLink( *inputSocket, *outputSocket );
					}

					GraphLink& newLink = graphContext.linkList.Alloc();
					newLink.ID = ed::LinkId( NextLinkID( graphContext ) );
					newLink.StartPinID = startPinId;
					newLink.EndPinID = endPinId;

					if( inputSocket->var )
					{
						ImGui::IconItem icon = ImGui::ImScriptVariable( "", { "", inputSocket->var }, false );
						newLink.Color = icon.color;
					}

				}
				if( startPinId != endPinId )
				{
					if( ( inputSocket->isOutput == outputSocket->isOutput ) || ( !inputSocket->isOutput == !outputSocket->isOutput ) )
					{
						ed::RejectNewItem( ImColor( 255, 0, 0 ), 2.0f );
					}
					else if( !( inputSocket->var == nullptr && outputSocket->var == nullptr ) )
					{
						if( ( inputSocket->var && !outputSocket->var || !inputSocket->var && outputSocket->var )
								|| inputSocket->var->GetType() != outputSocket->var->GetType() )
						{
							ed::RejectNewItem( ImColor( 255, 0, 0 ), 2.0f );
						}
						else
						{
							for( auto* connection : outputSocket->connections )
							{
								if( connection == inputSocket )
								{
									ed::RejectNewItem( ImColor( 255, 0, 0 ), 2.0f );
									break;
								}
							}
						}
					}
					else
					{
						for( auto* connection : outputSocket->connections )
						{
							if( connection == inputSocket )
							{
								ed::RejectNewItem( ImColor( 255, 0, 0 ), 2.0f );
								break;
							}
						}
					}
					if( inputSocket->var && outputSocket->var
							&& ( inputSocket->var->GetType() == ev_entity || outputSocket->var->GetType() == ev_entity )
							&& ( inputSocket->var->GetType() == ev_object || outputSocket->var->GetType() == ev_object ) )
					{
						//insert cast node?
					}
				}
			}
		}

		ed::PinId pinId = 0;
		if( ed::QueryNewNode( &pinId ) )
		{
			idGraphNodeSocket** newSocket;
			graphContext.socketHashIdx.Get( pinId.Get(), &newSocket );
			idGraphNodeSocket* newSocketPtr = *newSocket;
			if( newSocketPtr )
			{
				showLabel( "+ Create Node", ImColor( 32, 45, 32, 180 ) );
			}

			if( ed::AcceptNewItem() )
			{
				//createNewNode = true;
				//newNodeLinkPin = FindPin(pinId);
				//newLinkPin = nullptr;
				ed::Suspend();
				ImGui::OpenPopup( "Create New Node" );
				ed::Resume();
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
				int idx = GetLinkIndexByID( deletedLinkId , graphContext );
				if( idx >= 0 )
				{
					DeleteLink( graphContext.linkList[idx] , graphContext );
				}
			}

			// You may reject link deletion by calling:
			// ed::RejectDeletedItem();
		}

		ed::NodeId nodeId = 0;
		while( ed::QueryDeletedNode( &nodeId ) )
		{
			if( ed::AcceptDeletedItem() )
			{

				GraphNode** targetNode;
				graphContext.nodeHashIdx.Get( nodeId.Get(), &targetNode );
				DeleteNode( *targetNode , graphContext );
			}
		}
	}
	ed::EndDelete(); // Wrap up deletion action
}
}