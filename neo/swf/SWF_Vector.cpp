/*
===========================================================================

Copyright (C) 2025 HvanGinneken

===========================================================================
*/
#include "precompiled.h"
#pragma hdrstop

#include "SWF_Vector.h"

idSWFScriptObject_VectorPrototype vectorScriptObjectPrototype;
idSWFScriptObject_VectorImplPrototype vectorImplObjectPrototype;
idSWFScriptObject_PointPrototype PointScriptObjectPrototype;

/*
========================
idSWFScriptObject_VectorPrototype
========================
*/

#define SWF_VECTOR_FUNCTION_DEFINE( x ) idSWFScriptVar idSWFScriptObject_VectorPrototype::idSWFScriptFunction_##x::Call( idSWFScriptObject * thisObject, const idSWFParmList & parms )
#define SWF_VECTOR_NATIVE_VAR_DEFINE_GET( x ) idSWFScriptVar idSWFScriptObject_VectorPrototype::idSWFScriptNativeVar_##x::Get( class idSWFScriptObject * object )
#define SWF_VECTOR_NATIVE_VAR_DEFINE_SET( x ) void  idSWFScriptObject_VectorPrototype::idSWFScriptNativeVar_##x::Set( class idSWFScriptObject * object, const idSWFScriptVar & value )

#define SWF_VECTOR_PTHIS_FUNC( x ) idSWFVector * pThis = thisObject ? thisObject : NULL; if ( !verify( pThis != NULL ) ) { idLib::Warning( "SWF: tried to call " x " on NULL object" ); return idSWFScriptVar(); }
#define SWF_VECTOR_PTHIS_GET( x ) idSWFVector * pThis = object ? object : NULL; if ( pThis == NULL ) { return idSWFScriptVar(); }
#define SWF_VECTOR_PTHIS_SET( x ) idSWFVector * pThis = object ? object : NULL; if ( pThis == NULL ) { return; }

#define SWF_VECTOR_FUNCTION_SET( x ) scriptFunction_##x.AddRef(); Set( #x, &scriptFunction_##x );
#define SWF_VECTOR_NATIVE_VAR_SET( x ) SetNative( #x, &swfScriptVar_##x );


idSWFScriptVar idSWFScriptFunction_Vec_Push::Call( idSWFScriptObject * thisObject, const idSWFParmList & parms ) {

	switch ( parms[0].GetType( ) ) {
	case idSWFScriptVar::SWF_VAR_STRING:
		( static_cast< idSWFVector<idStr>* >( thisObject ) )->Push( parms[0].ToString( ) );
		break;
	case idSWFScriptVar::SWF_VAR_FLOAT:
		( static_cast< idSWFVector<float>* >( thisObject ) )->Push( parms[0].ToFloat( ) );
		break;
	case idSWFScriptVar::SWF_VAR_BOOL:
		( static_cast< idSWFVector<bool>* >( thisObject ) )->Push( parms[0].ToBool( ) );
		break;
	case idSWFScriptVar::SWF_VAR_NULL:
		assert( 0 );
		break;
	case idSWFScriptVar::SWF_VAR_UNDEF:
		assert( 0 );
		break;
	case idSWFScriptVar::SWF_VAR_STRINGID:
	case idSWFScriptVar::SWF_VAR_INTEGER:
		( static_cast< idSWFVector<int32>* >( thisObject ) )->Push( parms[0].ToInteger( ) );
		break;
	case idSWFScriptVar::SWF_VAR_FUNCTION:
		assert( 0 );
		//( *static_cast< idSWFVector<>* >( target ) )[stack.B( ).ToInteger( )] = stack.A( ).GetFunction( );
		break;
	case idSWFScriptVar::SWF_VAR_OBJECT:
		assert( 0 );
		//( *static_cast< idSWFVector<>* >( target ) )[stack.B( ).ToInteger( )] = stack.A( ).GetObject( );
		break;
	case idSWFScriptVar::SWF_VAR_RESULT:
		assert( 0 );
		break;
	default:
		break;
	}
	return idSWFScriptVar( );
}


idSWFScriptObject_VectorPrototype::idSWFScriptObject_VectorPrototype( ) {
	//SetPrototype(ALLTYPES_PROTOTYPE -> )
	SWF_VECTOR_NATIVE_VAR_SET( Vector );
	SWF_VECTOR_NATIVE_VAR_SET( length );
}


SWF_VECTOR_NATIVE_VAR_DEFINE_GET( length ) {
	return idSWFScriptVar( ( static_cast< idSWFVectorBase * >( object ) )->Num( ) );
}

SWF_VECTOR_NATIVE_VAR_DEFINE_GET( Vector ) {
	static idSWFScriptObject *vectorProto = nullptr;
	if ( vectorProto == nullptr ) {
		vectorProto = idSWFScriptObject::Alloc();
		vectorProto->SetPrototype(&vectorImplObjectPrototype);
	}
	return vectorProto;

}
//
//SWF_VECTOR_FUNCTION_DEFINE( push ) {
//	SWF_VECTOR_PTHIS_FUNC( "push" );
//	
//	return idSWFScriptVar( );
//}

idSWFScriptObject_VectorImplPrototype::idSWFScriptObject_VectorImplPrototype( ) {
	SetNative( "int", &swfScriptVar_int );
	SetNative( "String", &swfScriptVar_String );
	SetNative( "Number", &swfScriptVar_Number );
	SetNative( "Boolean", &swfScriptVar_Boolean );
}

idSWFScriptVar idSWFScriptObject_VectorImplPrototype::idSWFScriptNativeVar_int::Get( class idSWFScriptObject * object ) {
	return new( TAG_SWF ) idSWFVector<int32>;
}

idSWFScriptVar idSWFScriptObject_VectorImplPrototype::idSWFScriptNativeVar_String::Get( class idSWFScriptObject *object ) {
	return new( TAG_SWF ) idSWFVector<idStr>;
}
idSWFScriptVar idSWFScriptObject_VectorImplPrototype::idSWFScriptNativeVar_Number::Get( class idSWFScriptObject *object ) {

	return new( TAG_SWF ) idSWFVector<float>;
}
idSWFScriptVar idSWFScriptObject_VectorImplPrototype::idSWFScriptNativeVar_Boolean::Get( class idSWFScriptObject *object ) {

	return new( TAG_SWF ) idSWFVector<bool>;
}

void idSWFVectorBase::setproperty( int32 index, idSWFScriptVar &val, idSWFScriptObject * target ) {
	//idSWFScriptVar &val = stack.A( );
	//int32 index = stack.B( ).ToInteger( );
	switch ( val.GetType( ) ) {
	case idSWFScriptVar::SWF_VAR_STRING:
		( *static_cast< idSWFVector<idStr>* >( target ) )[index] = val.ToString( ) ;
		break;
	case idSWFScriptVar::SWF_VAR_FLOAT:
		( *static_cast< idSWFVector<float>* >( target ) )[index] = val.ToFloat( ) ;
		break;
	case idSWFScriptVar::SWF_VAR_BOOL:
		( *static_cast< idSWFVector<bool>* >( target ) )[index] = val.ToBool( );
		break;
	case idSWFScriptVar::SWF_VAR_NULL:
		assert( 0 );
		break;
	case idSWFScriptVar::SWF_VAR_UNDEF:
		assert( 0 );
		break;
	case idSWFScriptVar::SWF_VAR_STRINGID:
	case idSWFScriptVar::SWF_VAR_INTEGER:
		( *static_cast< idSWFVector<int32>* >( target ) )[index] = val.ToInteger( );
		break;
	case idSWFScriptVar::SWF_VAR_FUNCTION:
		assert( 0 );
		//( *static_cast< idSWFVector<>* >( target ) )[stack.B( ).ToInteger( )] = stack.A( ).GetFunction( );
		break;
	case idSWFScriptVar::SWF_VAR_OBJECT:
		assert( 0 );
		//( *static_cast< idSWFVector<>* >( target ) )[stack.B( ).ToInteger( )] = stack.A( ).GetObject( );
		break;
	case idSWFScriptVar::SWF_VAR_RESULT:
		assert( 0 );
		break;
	default:
		assert( 0 );
		break;
	}
	//stack.Pop( 3 );	
}

idSWFScriptVar idSWFVectorBase::GetVal( int32 index ) {
	return idSWFScriptString("getVaaaal" );
}

idSWFScriptObject_PointPrototype::idSWFScriptObject_PointPrototype( ) {
	//SetNative( "x", &swfScriptVar_x );
	//SetNative( "y", &swfScriptVar_y );
	scriptFunction_distance.AddRef( );	
	Set( "distance", &scriptFunction_distance );
	scriptFunction_distance.AddRef( );
	Set( "__constructor__", &scriptFunction_constructor );
	scriptFunction_constructor.AddRef( );
}

idSWFScriptVar idSWFScriptObject_PointPrototype::idSWFScriptFunction_distance::Call( idSWFScriptObject *thisObject, const idSWFParmList &parms ) {
	idVec2 lh = idVec2( parms[0].GetObject()->Get( "x" ).ToFloat( ),   parms[0].GetObject()->Get( "y" ).ToFloat( ) );
	idVec2 rh = idVec2( parms[1].GetObject()->Get( "x" ).ToFloat( ),   parms[1].GetObject()->Get( "y" ).ToFloat( ) );
	return (rh - lh).Length();
}

idSWFScriptVar idSWFScriptObject_PointPrototype::idSWFScriptFunction_constructor::Call( idSWFScriptObject *thisObject, const idSWFParmList &parms ) {
	auto * newObj= idSWFScriptObject::Alloc();
	newObj->Set( "x", parms[0].ToFloat( ) );
	newObj->Set( "y", parms[1].ToFloat( ) );
	newObj->SetPrototype( &PointScriptObjectPrototype );
	return newObj;
}
