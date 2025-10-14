#ifndef __SWF_VECTOR_H__
#define __SWF_VECTOR_H__

#include "SWF_Sprites.h"
#include "swf.h"
#include "SWF_Abc.h"
#include "SWF_ScriptObject.h"
#include "containers/List.h"

extern class idSWFScriptObject_VectorPrototype vectorScriptObjectPrototype;
extern class idSWFScriptObject_VectorImplPrototype vectorImplObjectPrototype;


class idSWFVectorBase : public idSWFScriptObject {
public:
	static void setproperty(int32 index, idSWFScriptVar &val,idSWFScriptObject * target ) ; 
	virtual idStr ToString() = 0;
	virtual void SetNum (int num) = 0;
	virtual idSWFScriptVar GetVal(int32 index) = 0;
	virtual int Num()= 0;
	//virtual void PushVal(idSWFScriptVar val) = 0;
};

class idSWFScriptFunction_Vec_Push: public idSWFScriptFunction_RefCounted {
public:
	idSWFScriptFunction_Vec_Push( ) { }
	idSWFScriptVar Call( idSWFScriptObject *thisObject, const idSWFParmList &parms );
};


template < class T >
class idSWFVector : public idSWFVectorBase {
public:
	idSWFScriptFunction_Vec_Push scriptFunction_push;

	idSWFVector( idSWFScriptObject *proto ) {
		SetPrototype( proto );
	}
	idSWFVector( ) {
		SetPrototype(&vectorScriptObjectPrototype);
		scriptFunction_push.AddRef( );
		Set( "push", &scriptFunction_push );
		
	}

	~idSWFVector( ) {
		common->DPrintf( "~idSWFVector()\n" );
	}

	void Push(T && val) { elements.AddGrow(val); }

	int GetSize( ) const { return elements.Size( ); };
	
	idSWFScriptVar GetVal( int32 index ) {
		assert( index >= 0 && index < elements.Num( ) );
		return idSWFScriptVar( elements[index] );
	}

	void SetVal(int index, T && val) {
		assert( index >= 0 && index < elements.Num() );
		elements[index] = val;
	}
	int Num() final { return elements.Num(); }
	void SetNum ( int num ) final { elements.SetNum(num); }

	idStr ToString() {
		idStr ret = "Vector { ";
		for(int i = 0; i < elements.Num(); i++) 
		{
			ret += GetVal(i).ToString(); 
			if ((i + 1) != elements.Num())
				ret+=", "; 
		}
		ret += " }";
		return ret; 
	}
 	T &operator []( unsigned int i ) {
		assert( (int)i < elements.Num() );
		return elements[i];
	}
	const T &operator []( unsigned int i ) const {
		assert( i < elements.Size() );
		return elements[i];
	}
	idList<T> elements;
};


class idSWFScriptObject_VectorPrototype : public idSWFScriptObject {
public:
	idSWFScriptObject_VectorPrototype( );
	SWF_NATIVE_VAR_DECLARE_READONLY( Vector );
	SWF_NATIVE_VAR_DECLARE_READONLY( length );
};


class idSWFScriptObject_VectorImplPrototype : public idSWFScriptObject {
public:
	idSWFScriptObject_VectorImplPrototype( );
	SWF_NATIVE_VAR_DECLARE_READONLY( int );
	SWF_NATIVE_VAR_DECLARE_READONLY( String );
	SWF_NATIVE_VAR_DECLARE_READONLY( Number );
	SWF_NATIVE_VAR_DECLARE_READONLY( Boolean );
};

class idSWFScriptObject_PointPrototype : public idSWFScriptObject {
public:
	idSWFScriptObject_PointPrototype( );

#define SWF_POINT_FUNCTION_DECLARE( x ) \
	class idSWFScriptFunction_##x : public idSWFScriptFunction_Script { \
	public: \
		void			AddRef() {} \
		void			Release() {} \
		idSWFScriptVar Call( idSWFScriptObject * thisObject, const idSWFParmList & parms ); \
	} scriptFunction_##x;

	SWF_POINT_FUNCTION_DECLARE( distance );
	SWF_POINT_FUNCTION_DECLARE( constructor );
#undef SWF_POINT_FUNCTION_DECLARE

	//SWF_NATIVE_VAR_DECLARE( x );
	//SWF_NATIVE_VAR_DECLARE( y );
	~idSWFScriptObject_PointPrototype( ) {
		common->DPrintf( "~idSWFScriptObject_PointPrototype()\n" );
	}
};
extern idSWFScriptObject_PointPrototype PointScriptObjectPrototype;

#endif // __SWF_VECTOR_H__
