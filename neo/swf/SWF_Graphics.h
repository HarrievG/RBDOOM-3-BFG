#ifndef __SWF_GRAPHICS_H__
#define __SWF_GRAPHICS_H__

#include "SWF_Sprites.h"
#include "swf.h"
#include "SWF_Abc.h"
#include "SWF_ScriptObject.h"
#include "containers/List.h"
#include "SWF_Vector.h"


typedef enum {
	NO_OP = 0,
	MOVE_TO = 1,
	LINE_TO = 2,
	CURVE_TO = 3,
	WIDE_MOVE_TO = 4,
	WIDE_LINE_TO = 5,
	CUBIC_CURVE_TO = 6
} swfGraphics_Path_Command_t;

typedef enum {
	EVEN_ODD,
	NON_ZERO
} GraphicsPathWinding;


class idSWFScriptObject_GraphicsPrototype : public idSWFScriptObject {
public:

	idSWFScriptObject_GraphicsPrototype( );
#define SWF_SPRITE_FUNCTION_DECLARE( x ) \
	class idSWFScriptFunction_##x : public idSWFScriptFunction { \
	public: \
		void			AddRef() {} \
		void			Release() {} \
		idSWFScriptVar Call( idSWFScriptObject * thisObject, const idSWFParmList & parms ); \
	} scriptFunction_##x

	SWF_SPRITE_FUNCTION_DECLARE( readGraphicsData );
};

extern idSWFScriptObject_GraphicsPrototype graphicsScriptObjectPrototype;

class idSWFScriptObject_IGraphicsDataPrototype : public idSWFScriptObject {
public:
	idSWFScriptObject_IGraphicsDataPrototype( ) { };
};

extern idSWFScriptObject_IGraphicsDataPrototype IGraphicsDataScriptObjectPrototype;

class idSWFScriptObject_GraphicsPath : public idSWFScriptObject {
public:

	static void SetGraphicsPathCommandGlobals( idSWFScriptObject * globalObject );
	idSWFScriptObject_GraphicsPath( );
	idSWFVector<int> commands;
	idSWFVector<float> data;
	GraphicsPathWinding winding;

	void cubicCurveTo( float controlX1, float controlY1, float controlX2, float controlY2, float anchorX, float anchorY );
	void curveTo( float controlX, float controlY, float anchorX, float anchorY );
	void lineTo( float x, float y );
	void moveTo( float x, float y );
	void wideLineTo( float x, float y );
	void wideMoveTo( float x, float y );
};

class idSWFScriptObject_GraphicsPathPrototype : public idSWFScriptObject {
public:
	idSWFScriptObject_GraphicsPathPrototype( );

	SWF_NATIVE_VAR_DECLARE_READONLY( commands );
	SWF_NATIVE_VAR_DECLARE_READONLY( data );
};


extern idSWFScriptObject_GraphicsPathPrototype graphicsPathScriptObjectPrototype;

class idSWFScriptObject_ShapePrototype : public idSWFScriptObject_SpriteInstancePrototype {
public:
	idSWFScriptObject_ShapePrototype( );
	SWF_NATIVE_VAR_DECLARE_READONLY( graphics );
};

extern idSWFScriptObject_ShapePrototype shapeScriptObjectPrototype;

class idSWFScriptObject_Shape : public idSWFScriptObject {
public:
	idSWFScriptObject_Shape( ) {
		SetPrototype( &shapeScriptObjectPrototype );
	};
	idSWFShape * Shape = nullptr;
};



#undef SWF_SPRITE_FUNCTION_DECLARE

#endif // __SWF_GRAPICS_H__
