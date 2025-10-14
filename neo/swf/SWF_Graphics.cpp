/*
===========================================================================

Copyright (C) 2022 HvanGinneken

===========================================================================
*/
#include "precompiled.h"
#pragma hdrstop

#include "SWF_Graphics.h"

idSWFScriptObject_GraphicsPathPrototype graphicsPathScriptObjectPrototype;
idSWFScriptObject_GraphicsPrototype graphicsScriptObjectPrototype;
idSWFScriptObject_IGraphicsDataPrototype IGraphicsDataScriptObjectPrototype;

#define SWF_GRAPHICS_FUNCTION_DEFINE( x ) idSWFScriptVar idSWFScriptObject_GraphicsPrototype::idSWFScriptFunction_##x::Call( idSWFScriptObject * thisObject, const idSWFParmList & parms )
#define SWF_GRAPHICS_NATIVE_VAR_DEFINE_GET( x ) idSWFScriptVar idSWFScriptObject_GraphicsPrototype::idSWFScriptNativeVar_##x::Get( class idSWFScriptObject * object )
#define SWF_GRAPHICS_NATIVE_VAR_DEFINE_SET( x ) void  idSWFScriptObject_GraphicsPrototype::idSWFScriptNativeVar_##x::Set( class idSWFScriptObject * object, const idSWFScriptVar & value )

#define SWF_GRAPHICS_PTHIS_FUNC( x ) idSWFScriptObject * pThis = thisObject ? thisObject : NULL; if ( !verify( pThis != NULL ) ) { idLib::Warning( "SWF: tried to call " x " on NULL graphics" ); return idSWFScriptVar(); }
#define SWF_GRAPHICS_PTHIS_GET( x ) idSWFScriptObject * pThis = object ? object : NULL; if ( pThis == NULL ) { return idSWFScriptVar(); }
#define SWF_GRAPHICS_PTHIS_SET( x ) idSWFScriptObject * pThis = object ? object : NULL; if ( pThis == NULL ) { return; }

#define SWF_GRAPHICS_FUNCTION_SET( x ) scriptFunction_##x.AddRef(); Set( #x, &scriptFunction_##x );
#define SWF_GRAPHICS_NATIVE_VAR_SET( x ) SetNative( #x, &swfScriptVar_##x );

idSWFScriptObject_GraphicsPrototype::idSWFScriptObject_GraphicsPrototype( ) {
	SWF_GRAPHICS_FUNCTION_SET( readGraphicsData )
}

void idSWFScriptObject_GraphicsPath::SetGraphicsPathCommandGlobals( idSWFScriptObject * globalObject ) {
	static idSWFScriptObject *gfxPathCmdEnumObj = nullptr;
	idSWFScriptVar ret = globalObject->Get( "flash.display.GraphicsPathCommand" );
	if ( ret.IsUndefined( ) ) {
		if ( gfxPathCmdEnumObj == nullptr ) {
			gfxPathCmdEnumObj = idSWFScriptObject::Alloc( );
			gfxPathCmdEnumObj->Set( "NO_OP", swfGraphics_Path_Command_t::NO_OP );
			gfxPathCmdEnumObj->Set( "MOVE_TO", swfGraphics_Path_Command_t::MOVE_TO );
			gfxPathCmdEnumObj->Set( "LINE_TO", swfGraphics_Path_Command_t::LINE_TO );
			gfxPathCmdEnumObj->Set( "CURVE_TO", swfGraphics_Path_Command_t::CURVE_TO );
			gfxPathCmdEnumObj->Set( "WIDE_MOVE_TO", swfGraphics_Path_Command_t::WIDE_MOVE_TO );
			gfxPathCmdEnumObj->Set( "WIDE_LINE_TO", swfGraphics_Path_Command_t::WIDE_LINE_TO );
			gfxPathCmdEnumObj->Set( "CUBIC_CURVE_TO", swfGraphics_Path_Command_t::CUBIC_CURVE_TO );
		}
		globalObject->Set( "flash.display.GraphicsPathCommand", gfxPathCmdEnumObj );
	}
}

SWF_GRAPHICS_FUNCTION_DEFINE( readGraphicsData ) {
	SWF_GRAPHICS_PTHIS_FUNC( "readGraphicsData" );
	idSWFScriptVar shapeObj = pThis->Get("_shape" );
	auto * result = new( TAG_SWF ) idSWFVector<idSWFScriptVar>();
	if ( shapeObj.IsValid( ) ) {
		auto gfxPath =new( TAG_SWF ) idSWFScriptObject_GraphicsPath;

		gfxPath->commands.elements = static_cast<idSWFScriptObject_Shape*>(shapeObj.GetObject())->Shape->GraphicsCommands;
		gfxPath->data.elements = static_cast<idSWFScriptObject_Shape*>(shapeObj.GetObject())->Shape->GraphicsData;
		result->elements.AssureSize(1);
		(*result)[0] = gfxPath;
		return result;
	}
	return idSWFScriptVar( );
}

#undef SWF_GRAPHICS_FUNCTION_DEFINE
#undef SWF_GRAPHICS_NATIVE_VAR_DEFINE_GET
#undef SWF_GRAPHICS_NATIVE_VAR_DEFINE_SET

#undef SWF_GRAPHICS_PTHIS_FUNC
#undef SWF_GRAPHICS_PTHIS_GET
#undef SWF_GRAPHICS_PTHIS_SET

#undef SWF_GRAPHICS_FUNCTION_SET
#undef SWF_GRAPHICS_NATIVE_VAR_SET

idSWFScriptObject_ShapePrototype shapeScriptObjectPrototype;

#define SWF_Shape_FUNCTION_DEFINE( x ) idSWFScriptVar idSWFScriptObject_ShapePrototype::idSWFScriptFunction_##x::Call( idSWFScriptObject * thisObject, const idSWFParmList & parms )
#define SWF_Shape_NATIVE_VAR_DEFINE_GET( x ) idSWFScriptVar idSWFScriptObject_ShapePrototype::idSWFScriptNativeVar_##x::Get( class idSWFScriptObject * object )
#define SWF_Shape_NATIVE_VAR_DEFINE_SET( x ) void  idSWFScriptObject_GraphicsPrototype::idSWFScriptNativeVar_##x::Set( class idSWFScriptObject * object, const idSWFScriptVar & value )

//fix next line before use
#define SWF_Shape_PTHIS_FUNC( x ) idSWFScriptObject_Shape * pThis = thisObject ? thisObject : NULL; if ( !verify( pThis != NULL ) ) { idLib::Warning( "SWF: tried to call " x " on NULL Object" ); return idSWFScriptVar(); }
#define SWF_Shape_PTHIS_GET( x ) idSWFScriptObject_Shape * pThis = object ? static_cast<idSWFScriptObject_Shape*>(object): NULL; if ( pThis == NULL ) { return idSWFScriptVar(); }
#define SWF_Shape_PTHIS_SET( x ) idSWFScriptObject_Shape * pThis = object ? static_cast<idSWFScriptObject_Shape*>(object): NULL; if ( pThis == NULL ) { return; }

#define SWF_Shape_FUNCTION_SET( x ) scriptFunction_##x.AddRef(); Set( #x, &scriptFunction_##x );
#define SWF_Shape_NATIVE_VAR_SET( x ) SetNative( #x, &swfScriptVar_##x );

idSWFScriptObject_ShapePrototype::idSWFScriptObject_ShapePrototype( ) {
	
	SWF_Shape_NATIVE_VAR_SET( graphics );
	//SetPrototype( &spriteInstanceScriptObjectPrototype );
	//auto gxObj = idSWFScriptObject::Alloc( );
	//gxObj->SetPrototype( &graphicsScriptObjectPrototype );
	//Set( "graphics", gxObj );
	SetPrototype( &spriteInstanceScriptObjectPrototype );
}

SWF_Shape_NATIVE_VAR_DEFINE_GET( graphics ) {
	SWF_Shape_PTHIS_GET( "graphics" );
	auto grpx = pThis->Get( "_graphics" );
	if (grpx.IsUndefined())
	{
		auto gxObj = idSWFScriptObject::Alloc( );
		gxObj->SetPrototype( &graphicsScriptObjectPrototype );
		gxObj->SetSprite(pThis->GetSprite());
		gxObj->Set( "_shape", pThis );
		pThis->Set( "_graphics", gxObj );
		return gxObj;
	}
	return grpx;
}

#undef SWF_SHAPE_FUNCTION_DEFINE
#undef SWF_SHAPE_NATIVE_VAR_DEFINE_GET
#undef SWF_SHAPE_NATIVE_VAR_DEFINE_SET

#undef SWF_SHAPE_PTHIS_FUNC
#undef SWF_SHAPE_PTHIS_GET
#undef SWF_SHAPE_PTHIS_SET

#undef SWF_SHAPE_FUNCTION_SET
#undef SWF_SHAPE_NATIVE_VAR_SET


#define SWF_gfxPath_FUNCTION_DEFINE( x ) idSWFScriptVar idSWFScriptObject_GraphicsPathPrototype::idSWFScriptFunction_##x::Call( idSWFScriptObject * thisObject, const idSWFParmList & parms )
#define SWF_gfxPath_NATIVE_VAR_DEFINE_GET( x ) idSWFScriptVar idSWFScriptObject_GraphicsPathPrototype::idSWFScriptNativeVar_##x::Get( class idSWFScriptObject * object )
#define SWF_gfxPath_NATIVE_VAR_DEFINE_SET( x ) void  idSWFScriptObject_GraphicsPrototype::idSWFScriptNativeVar_##x::Set( class idSWFScriptObject * object, const idSWFScriptVar & value )

//fix next lgfxPathefore use
#define SWF_gfxPath_PTHIS_FUNC( x ) idSWFScriptObject_GraphicsPath * pThis = thisObject ? thisObject : NULL; if ( !verify( pThis != NULL ) ) { idLib::Warning( "SWF: tried to call " x " on NULL Object" ); return idSWFScriptVar(); }
#define SWF_gfxPath_PTHIS_GET( x ) idSWFScriptObject_GraphicsPath * pThis = object ? static_cast<idSWFScriptObject_GraphicsPath*>(object): NULL; if ( pThis == NULL ) { return idSWFScriptVar(); }
#define SWF_gfxPath_PTHIS_SET( x ) idSWFScriptObject_GraphicsPath * pThis = object ? static_cast<idSWFScriptObject_GraphicsPath*>(object): NULL; if ( pThis == NULL ) { return; }

#define SWF_gfxPath_FUNCTION_SET( x ) scriptFunction_##x.AddRef(); Set( #x, &scriptFunction_##x );
#define SWF_gfxPath_NATIVE_VAR_SET( x ) SetNative( #x, &swfScriptVar_##x );

idSWFScriptObject_GraphicsPathPrototype::idSWFScriptObject_GraphicsPathPrototype( ) {

	SWF_gfxPath_NATIVE_VAR_SET( commands );
	SWF_gfxPath_NATIVE_VAR_SET( data );

	SetPrototype( &IGraphicsDataScriptObjectPrototype );
}

SWF_gfxPath_NATIVE_VAR_DEFINE_GET( commands ) {
	SWF_gfxPath_PTHIS_GET( "commands" );
	return &pThis->commands;
}

SWF_gfxPath_NATIVE_VAR_DEFINE_GET( data ) {
	SWF_gfxPath_PTHIS_GET( "data" );
	return &pThis->data;
}

idSWFScriptObject_GraphicsPath::idSWFScriptObject_GraphicsPath( ) {
	SetPrototype( &graphicsPathScriptObjectPrototype );
}

#undef SWF_gfxPath_FUNCTION_DEFINE
#undef SWF_gfxPath_NATIVE_VAR_DEFINE_GET
#undef SWF_gfxPath_NATIVE_VAR_DEFINE_SET

#undef SWF_gfxPath_PTHIS_FUNC
#undef SWF_gfxPath_PTHIS_GET
#undef SWF_gfxPath_PTHIS_SET

#undef SWF_gfxPath_FUNCTION_SET
#undef SWF_gfxPath_NATIVE_VAR_SET

void idSWFScriptObject_GraphicsPath::cubicCurveTo( float controlX1, float controlY1, float controlX2, float controlY2, float anchorX, float anchorY ) {
	commands.Push( static_cast< int >( swfGraphics_Path_Command_t::CUBIC_CURVE_TO ) );
	data.elements.AssureSize( data.Num() + 6 );
	data[data.Num()    ]= controlX1;
	data[data.Num() + 1]= controlY1 ;
	data[data.Num() + 2]= controlX2 ;
	data[data.Num() + 3]= controlY2 ;
	data[data.Num() + 4]= anchorX ;
	data[data.Num() + 5]= anchorY ;
}

void idSWFScriptObject_GraphicsPath::curveTo( float controlX, float controlY, float anchorX, float anchorY ) {
	commands.Push( static_cast< int >( swfGraphics_Path_Command_t::CURVE_TO ) );
	data.elements.AssureSize( data.Num() + 4 );
	data[data.Num( )    ] = controlX;
	data[data.Num( ) + 1] = controlY;
	data[data.Num( ) + 2] = anchorX;
	data[data.Num( ) + 3] = anchorY;
}

void idSWFScriptObject_GraphicsPath::lineTo( float x, float y ) {
	commands.Push( static_cast< int >( swfGraphics_Path_Command_t::LINE_TO ) );
	data.elements.AssureSize( data.Num() + 2 );
	data[data.Num( )    ] = x;
	data[data.Num( ) + 1] = y;
}

void idSWFScriptObject_GraphicsPath::moveTo( float x, float y ) {
	commands.Push( static_cast< int >( swfGraphics_Path_Command_t::MOVE_TO ) );

	data.elements.AssureSize( data.Num( ) + 2 );
	data[data.Num( )] = x;
	data[data.Num( ) + 1] = y;
}

void idSWFScriptObject_GraphicsPath::wideLineTo( float x, float y ) {
	commands.Push( static_cast< int >( swfGraphics_Path_Command_t::WIDE_LINE_TO ) );
	data.elements.AssureSize( data.Num( ) + 2 );
	data[data.Num( )] = x;
	data[data.Num( ) + 1] = y;
}

void idSWFScriptObject_GraphicsPath::wideMoveTo( float x, float y ) {
	commands.Push( static_cast< int >( swfGraphics_Path_Command_t::WIDE_MOVE_TO ) );
	data.elements.AssureSize( data.Num( ) + 2 );
	data[data.Num( )] = x;
	data[data.Num( ) + 1] = y;
}
