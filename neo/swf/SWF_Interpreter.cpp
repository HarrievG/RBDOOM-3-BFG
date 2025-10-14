#include "precompiled.h"
#include "SWF_Vector.h"
#pragma hdrstop

//#include "SWF_Bitstream.h"
//#include "SWF_Enums.h"
//#include "SWF_Abc.h"
//#include "SWF_ScriptObject.h"
//#include "SWF_ScriptFunction.h"
//#include "framework/Common.h"

inline int32 readS24( const byte* pc )
{
	return ( ( uint16_t* )pc )[0] | ( ( int8_t* )pc )[2] << 16;
}

inline uint32 readU30( const uint8_t*& pc )
{
	uint32 result = 0;
	for( int i = 0; i < 5; i++ )
	{
		byte b = *pc++;
		result |= ( b & 0x7F ) << ( 7 * i );
		if( ( b & 0x80 ) == 0 )
		{
			return result;
		}
	}
	return result;
}

const AbcOpcodeInfo opcodeInfo[] =
{
	// For stack movement ("stk") only constant movement is accounted for; variable movement,
	// as for arguments to CALL, CONSTRUCT, APPLYTYPE, et al, and for run-time parts of
	// names, must be handled separately.

#define ABC_OP(operandCount, canThrow, stack, internalOnly, nameToken)        { operandCount, canThrow, stack , OP_##nameToken	, #nameToken },
#define ABC_UNUSED_OP(operandCount, canThrow, stack, internalOnly, nameToken) { operandCount, canThrow, stack ,	0				, #nameToken },

#include "opcodes.tbl"

#undef ABC_OP
#undef ABC_UNUSED_OP
#undef ABC_OP_F
};

extern idCVar swf_debug;

void debug( SWF_AbcFile *file, idSWFBitStream &bitstream ) {
	common->Printf( " debug %s\n", file->constant_pool.utf8Strings[bitstream.ReadEncodedU32( )].c_str( ) );
}
void debugfile( SWF_AbcFile* file,  idSWFBitStream& bitstream )
{
	common->Printf( " debugfile %s\n", file->constant_pool.utf8Strings[bitstream.ReadEncodedU32()].c_str() );
}

void debugline( SWF_AbcFile* file,  idSWFBitStream& bitstream )
{
	common->Printf( " debugline %i\n", ( int )bitstream.ReadEncodedU32() );
}

void swf_OptimizeStream( SWF_AbcFile *file, idSWFBitStream &bitstream ) {
	//(case.*OP_)([A-Za-z0-9]*_?[A-Za-z0-9]*)(.*\n)(.*)
	idSWFStack stack;
	static idList<AbcOpcodeInfo *> codeMap;
	idStr type;
	const AbcOpcodeInfo *info = nullptr;
	idSWFBitStream optStream;

	while ( bitstream.Tell( ) < bitstream.Length( ) ) {
#define DoWordCode( n ) case OP_##n:  break;
		SWFAbcOpcode opCode = ( SWFAbcOpcode ) bitstream.ReadU8( );
		switch ( opCode ) {
			DoWordCode( bkpt );
			DoWordCode( nop );
			DoWordCode( throw );
			DoWordCode( getsuper );
			DoWordCode( setsuper );
			DoWordCode( dxns );
			DoWordCode( dxnslate );
			DoWordCode( kill );
			DoWordCode( label );
			DoWordCode( ifnlt );
			DoWordCode( ifnle );
			DoWordCode( ifngt );
			DoWordCode( ifnge );
			DoWordCode( jump );
			DoWordCode( iftrue );
			DoWordCode( iffalse );
			DoWordCode( ifeq );
			DoWordCode( ifne );
			DoWordCode( iflt );
			DoWordCode( ifle );
			DoWordCode( ifgt );
			DoWordCode( ifge );
			DoWordCode( ifstricteq );
			DoWordCode( ifstrictne );
			DoWordCode( lookupswitch );
			DoWordCode( pushwith );
			DoWordCode( popscope );
			DoWordCode( nextname );
			DoWordCode( hasnext );
			DoWordCode( pushnull );
			DoWordCode( pushundefined );
			DoWordCode( DISABLED_pushfloat );
			DoWordCode( nextvalue );
			DoWordCode( pushbyte );
			DoWordCode( pushshort );
			DoWordCode( pushtrue );
			DoWordCode( pushfalse );
			DoWordCode( pushnan );
			DoWordCode( pop );
			DoWordCode( dup );
			DoWordCode( swap );
			DoWordCode( pushstring );
			DoWordCode( pushint );
			DoWordCode( pushuint );
			DoWordCode( pushdouble );
			DoWordCode( pushscope );
			DoWordCode( pushnamespace );
			DoWordCode( hasnext2 );
			DoWordCode( lix8 );
			DoWordCode( lix16 );
			DoWordCode( li8 );
			DoWordCode( li16 );
			DoWordCode( li32 );
			DoWordCode( lf32 );
			DoWordCode( lf64 );
			DoWordCode( si8 );
			DoWordCode( si16 );
			DoWordCode( si32 );
			DoWordCode( sf32 );
			DoWordCode( sf64 );
			DoWordCode( newfunction );
			DoWordCode( call );
			DoWordCode( construct );
			DoWordCode( callmethod );
			DoWordCode( callstatic );
			DoWordCode( callsuper );
			DoWordCode( callproperty );
			DoWordCode( returnvoid );
			DoWordCode( returnvalue );
			DoWordCode( constructsuper );
			DoWordCode( constructprop );
			DoWordCode( callsuperid );
			DoWordCode( callproplex );
			DoWordCode( callinterface );
			DoWordCode( callsupervoid );
			DoWordCode( callpropvoid );
			DoWordCode( sxi1 );
			DoWordCode( sxi8 );
			DoWordCode( sxi16 );
			DoWordCode( applytype );
			DoWordCode( DISABLED_pushfloat4 );
			DoWordCode( newobject );
			DoWordCode( newarray );
			DoWordCode( newactivation );
			DoWordCode( newclass );
			DoWordCode( getdescendants );
			DoWordCode( newcatch );
			DoWordCode( findpropglobalstrict );
			DoWordCode( findpropglobal );
			DoWordCode( findpropstrict );
			DoWordCode( findproperty );
			DoWordCode( finddef );
			DoWordCode( getlex );
			DoWordCode( setproperty );
			DoWordCode( getlocal );
			DoWordCode( setlocal );
			DoWordCode( getglobalscope );
			DoWordCode( getscopeobject );
			DoWordCode( getproperty );
			DoWordCode( getouterscope );
			DoWordCode( initproperty );
			DoWordCode( deleteproperty );
			DoWordCode( getslot );
			DoWordCode( setslot );
			DoWordCode( getglobalslot );
			DoWordCode( setglobalslot );
			DoWordCode( convert_s );
			DoWordCode( esc_xelem );
			DoWordCode( esc_xattr );
			DoWordCode( convert_i );
			DoWordCode( convert_u );
			DoWordCode( convert_d );
			DoWordCode( convert_b );
			DoWordCode( convert_o );
			DoWordCode( checkfilter );
			//DoWordCode ( DISABLED_convert );
			//DoWordCode ( DISABLED_unplus );
			//DoWordCode ( DISABLED_convert );
			DoWordCode( coerce );
			DoWordCode( coerce_b );
			DoWordCode( coerce_a );
			DoWordCode( coerce_i );
			DoWordCode( coerce_d );
			DoWordCode( coerce_s );
			DoWordCode( astype );
			DoWordCode( astypelate );
			DoWordCode( coerce_u );
			DoWordCode( coerce_o );
			DoWordCode( negate );
			DoWordCode( increment );
			DoWordCode( inclocal );
			DoWordCode( decrement );
			DoWordCode( declocal );
			DoWordCode( typeof );
			DoWordCode( not );
			DoWordCode( bitnot );
			DoWordCode( add );
			DoWordCode( subtract );
			DoWordCode( multiply );
			DoWordCode( divide );
			DoWordCode( modulo );
			DoWordCode( lshift );
			DoWordCode( rshift );
			DoWordCode( urshift );
			DoWordCode( bitand );
			DoWordCode( bitor );
			DoWordCode( bitxor );
			DoWordCode( equals );
			DoWordCode( strictequals );
			DoWordCode( lessthan );
			DoWordCode( lessequals );
			DoWordCode( greaterthan );
			DoWordCode( greaterequals );
			DoWordCode( instanceof );
			DoWordCode( istype );
			DoWordCode( istypelate );
			DoWordCode( in );
			DoWordCode( increment_i );
			DoWordCode( decrement_i );
			DoWordCode( inclocal_i );
			DoWordCode( declocal_i );
			DoWordCode( negate_i );
			DoWordCode( add_i );
			DoWordCode( subtract_i );
			DoWordCode( multiply_i );
			DoWordCode( getlocal0 );
			DoWordCode( getlocal1 );
			DoWordCode( getlocal2 );
			DoWordCode( getlocal3 );
			DoWordCode( setlocal0 );
			DoWordCode( setlocal1 );
			DoWordCode( setlocal2 );
			DoWordCode( setlocal3 );
			DoWordCode( debug );
			DoWordCode( debugline );
			DoWordCode( debugfile );
			DoWordCode( bkptline );
			DoWordCode( timestamp );
			DoWordCode( restargc );
			DoWordCode( restarg );
		default:
			common->Printf( "default %s %s\n", type.c_str( ), info ? info->name : "Empty" );
		}
		static const char *tabs[] = { " ", "  ", "   ", "    ", "      ", "       ", "        ", "         ", "          ", "           ", "            ", "             ", "              ", "               ", "                " };
		if ( info && info->operandCount > 0 ) {
			bitstream.ReadData( info->operandCount );
		}

		common->Printf( " %s %s o %s%i  \t s %s%i \n",
			info ? info->name : type.c_str( ),
			tabs[int( 18 - ( int( idStr::Length( info->name ) ) ) )],
			info->operandCount > 0 ? "^2" : "^1",
			info->operandCount,
			info->stack < 0 ? "^2" : "^1",
			info->stack
		);
	}
	bitstream.Rewind( );
#undef DoWordCode
#undef ExecWordCode
}

void swf_PrintStream( SWF_AbcFile* file, idSWFBitStream& bitstream )
{
	//(case.*OP_)([A-Za-z0-9]*_?[A-Za-z0-9]*)(.*\n)(.*)
	idSWFStack stack;
	static idList<AbcOpcodeInfo*> codeMap;
	idStr type;
	const AbcOpcodeInfo* info = nullptr;
	while( bitstream.Tell() < bitstream.Length() )
	{
#define DoWordCode( n ) case OP_##n: type = #n; info = &opcodeInfo[opCode]; break;
#define ExecWordCode( n ) case OP_##n: type = #n; info = &opcodeInfo[opCode]; n(file,bitstream); continue;
		SWFAbcOpcode opCode = ( SWFAbcOpcode ) bitstream.ReadU8();
		switch( opCode )
		{
				DoWordCode( bkpt );
				DoWordCode( nop );
				DoWordCode( throw );
				DoWordCode( getsuper );
				DoWordCode( setsuper );
				DoWordCode( dxns );
				DoWordCode( dxnslate );
				DoWordCode( kill );
				DoWordCode( label );
				DoWordCode( ifnlt );
				DoWordCode( ifnle );
				DoWordCode( ifngt );
				DoWordCode( ifnge );
				DoWordCode( jump );
				DoWordCode( iftrue );
				DoWordCode( iffalse );
				DoWordCode( ifeq );
				DoWordCode( ifne );
				DoWordCode( iflt );
				DoWordCode( ifle );
				DoWordCode( ifgt );
				DoWordCode( ifge );
				DoWordCode( ifstricteq );
				DoWordCode( ifstrictne );
				DoWordCode( lookupswitch );
				DoWordCode( pushwith );
				DoWordCode( popscope );
				DoWordCode( nextname );
				DoWordCode( hasnext );
				DoWordCode( pushnull );
				DoWordCode( pushundefined );
				DoWordCode( DISABLED_pushfloat );
				DoWordCode( nextvalue );
				DoWordCode( pushbyte );
				DoWordCode( pushshort );
				DoWordCode( pushtrue );
				DoWordCode( pushfalse );
				DoWordCode( pushnan );
				DoWordCode( pop );
				DoWordCode( dup );
				DoWordCode( swap );
				DoWordCode( pushstring );
				DoWordCode( pushint );
				DoWordCode( pushuint );
				DoWordCode( pushdouble );
				DoWordCode( pushscope );
				DoWordCode( pushnamespace );
				DoWordCode( hasnext2 );
				DoWordCode( lix8 );
				DoWordCode( lix16 );
				DoWordCode( li8 );
				DoWordCode( li16 );
				DoWordCode( li32 );
				DoWordCode( lf32 );
				DoWordCode( lf64 );
				DoWordCode( si8 );
				DoWordCode( si16 );
				DoWordCode( si32 );
				DoWordCode( sf32 );
				DoWordCode( sf64 );
				DoWordCode( newfunction );
				DoWordCode( call );
				DoWordCode( construct );
				DoWordCode( callmethod );
				DoWordCode( callstatic );
				DoWordCode( callsuper );
				DoWordCode( callproperty );
				DoWordCode( returnvoid );
				DoWordCode( returnvalue );
				DoWordCode( constructsuper );
				DoWordCode( constructprop );
				DoWordCode( callsuperid );
				DoWordCode( callproplex );
				DoWordCode( callinterface );
				DoWordCode( callsupervoid );
				DoWordCode( callpropvoid );
				DoWordCode( sxi1 );
				DoWordCode( sxi8 );
				DoWordCode( sxi16 );
				DoWordCode( applytype );
				DoWordCode( DISABLED_pushfloat4 );
				DoWordCode( newobject );
				DoWordCode( newarray );
				DoWordCode( newactivation );
				DoWordCode( newclass );
				DoWordCode( getdescendants );
				DoWordCode( newcatch );
				DoWordCode( findpropglobalstrict );
				DoWordCode( findpropglobal );
				DoWordCode( findpropstrict );
				DoWordCode( findproperty );
				DoWordCode( finddef );
				DoWordCode( getlex );
				DoWordCode( setproperty );
				DoWordCode( getlocal );
				DoWordCode( setlocal );
				DoWordCode( getglobalscope );
				DoWordCode( getscopeobject );
				DoWordCode( getproperty );
				DoWordCode( getouterscope );
				DoWordCode( initproperty );
				DoWordCode( deleteproperty );
				DoWordCode( getslot );
				DoWordCode( setslot );
				DoWordCode( getglobalslot );
				DoWordCode( setglobalslot );
				DoWordCode( convert_s );
				DoWordCode( esc_xelem );
				DoWordCode( esc_xattr );
				DoWordCode( convert_i );
				DoWordCode( convert_u );
				DoWordCode( convert_d );
				DoWordCode( convert_b );
				DoWordCode( convert_o );
				DoWordCode( checkfilter );
				//DoWordCode ( DISABLED_convert );
				//DoWordCode ( DISABLED_unplus );
				//DoWordCode ( DISABLED_convert );
				DoWordCode( coerce );
				DoWordCode( coerce_b );
				DoWordCode( coerce_a );
				DoWordCode( coerce_i );
				DoWordCode( coerce_d );
				DoWordCode( coerce_s );
				DoWordCode( astype );
				DoWordCode( astypelate );
				DoWordCode( coerce_u );
				DoWordCode( coerce_o );
				DoWordCode( negate );
				DoWordCode( increment );
				DoWordCode( inclocal );
				DoWordCode( decrement );
				DoWordCode( declocal );
				DoWordCode( typeof );
				DoWordCode( not );
				DoWordCode( bitnot );
				DoWordCode( add );
				DoWordCode( subtract );
				DoWordCode( multiply );
				DoWordCode( divide );
				DoWordCode( modulo );
				DoWordCode( lshift );
				DoWordCode( rshift );
				DoWordCode( urshift );
				DoWordCode( bitand );
				DoWordCode( bitor );
				DoWordCode( bitxor );
				DoWordCode( equals );
				DoWordCode( strictequals );
				DoWordCode( lessthan );
				DoWordCode( lessequals );
				DoWordCode( greaterthan );
				DoWordCode( greaterequals );
				DoWordCode( instanceof );
				DoWordCode( istype );
				DoWordCode( istypelate );
				DoWordCode( in );
				DoWordCode( increment_i );
				DoWordCode( decrement_i );
				DoWordCode( inclocal_i );
				DoWordCode( declocal_i );
				DoWordCode( negate_i );
				DoWordCode( add_i );
				DoWordCode( subtract_i );
				DoWordCode( multiply_i );
				DoWordCode( getlocal0 );
				DoWordCode( getlocal1 );
				DoWordCode( getlocal2 );
				DoWordCode( getlocal3 );
				DoWordCode( setlocal0 );
				DoWordCode( setlocal1 );
				DoWordCode( setlocal2 );
				DoWordCode( setlocal3 );
				DoWordCode( debug );
				ExecWordCode( debugline );
				ExecWordCode( debugfile );
				DoWordCode( bkptline );
				DoWordCode( timestamp );
				DoWordCode( restargc );
				DoWordCode( restarg );
			default:
				common->Printf( "default %s %s\n", type.c_str( ) , info ? info->name : "Empty" );
		}
		static const char* tabs[] = { " ", "  ", "   ", "    ", "      ", "       ", "        ", "         ", "          ", "           ", "            ", "             ", "              ", "               ", "                "};
		if( info && info->operandCount > 0 )
		{
			bitstream.ReadData( info->operandCount );
		}
		common->Printf( " %s %s o %s%i  \t s %s%i \n" ,
						info ? info->name : type.c_str( ),
						tabs[int( 18 - ( int( idStr::Length( info->name ) ) ) )],
						info->operandCount > 0 ? "^2" : "^1" ,
						info->operandCount,
						info->stack < 0 ? "^2" : "^1",
						info->stack
					  );
	}
	bitstream.Rewind();
#undef DoWordCode
#undef ExecWordCode
}

idStr idSWFScriptFunction_Script::ResolveMultiname( SWF_AbcFile *file, idSWFStack& stack,
	idSWFBitStream& bitstream,swfMultiname & ResolvedMultiName, swfMultiname & ResolvedTypeName) {
	
	auto &cp = file->constant_pool;
	ResolvedMultiName = cp.multinameInfos[bitstream.ReadEncodedU32( )];

	idStr fullPropName;
	switch ( ResolvedMultiName.type ) {
	case Qname:
	case Multiname:
	{
		fullPropName = *cp.namespaceNames[ResolvedMultiName.index];
		if ( !fullPropName.IsEmpty( ) ) {
			fullPropName += ".";
		}
		fullPropName += cp.utf8Strings[ResolvedMultiName.nameIndex];
	}
		break;

	case RTQname:
		fullPropName = cp.utf8Strings[ResolvedMultiName.nameIndex];
		break;

	case RTQnameL:
		fullPropName = "runtime_multiname";
		break;

	case MultinameL:
	{
		//uint32 a = bitstream.ReadEncodedU32( );
		auto &ns = cp.namespaceSets[ResolvedMultiName.index];
		fullPropName = stack.A( ).ToString( );
		//stack.Pop( 1 );
	}
		break;

	case TypeName:
	{
		ResolvedTypeName = cp.multinameInfos[ResolvedMultiName.indexT];
		while ( ResolvedMultiName.type != Qname ) {
			if ( ResolvedMultiName.type == TypeName )
				ResolvedTypeName = cp.multinameInfos[ResolvedMultiName.indexT];
			ResolvedMultiName = cp.multinameInfos[ResolvedMultiName.nameIndex];
		}
		fullPropName = cp.utf8Strings[ResolvedMultiName.nameIndex];// + "<" + cp.utf8Strings[typeName.nameIndex] + ">";
	}
	break;

	default:
		fullPropName = "unexpected_multiname_type";
		break;
	}

	return fullPropName;	
	
	//const swfConstant_pool_info &cp = abcFile->constant_pool;
	//const auto *mn = &cp.multinameInfos[bitstream.ReadEncodedU32( )];
	//const swfMultiname* typeName = nullptr;
	//while (mn->type != Qname)
	//{
	//	if ( mn->type == TypeName )
	//		typeName = &cp.multinameInfos[mn->indexT];
	//	mn =  &cp.multinameInfos[mn->nameIndex];
	//}
	//return cp.utf8Strings[mn->nameIndex];
}

void idSWFScriptFunction_Script::findproperty( SWF_AbcFile* file, idSWFStack& stack, idSWFBitStream& bitstream )
{
	const auto& cp = file->constant_pool;
	const auto& mn = file->constant_pool.multinameInfos[bitstream.ReadEncodedU32()];
	const idStrPtr propName = ( idStrPtr )&cp.utf8Strings[mn.nameIndex];
	//search up scope stack.
	for( int i = scope.Num() - 1; i >= 0; i-- )
	{
		auto* s = scope[i];
		while( s )
			if( s->HasProperty( propName->c_str() ) )
			{
				stack.Alloc() = s->Get( propName->c_str() );
				return;
			}
			else if( s->GetPrototype() && s->GetPrototype()->GetPrototype() )
			{
				s = s->GetPrototype()->GetPrototype();
			}
			else
			{
				s = NULL;
			}
	}
	auto prop = scope[0]->GetVariable( *propName, true );
	stack.Alloc() = prop->value;
}

void idSWFScriptFunction_Script::findpropstrict( SWF_AbcFile* file, idSWFStack& stack, idSWFBitStream& bitstream )
{
	const auto& cp = file->constant_pool;
	const auto& mn = file->constant_pool.multinameInfos[bitstream.ReadEncodedU32( )];

	idStr fullPropName = *cp.namespaceNames[mn.index];
	if ( !fullPropName.IsEmpty( ) ) {
		fullPropName += ".";
	}
	fullPropName += cp.utf8Strings[mn.nameIndex];

	//search current scope stack
	for( int i = scope.Num( ) - 1; i >= 0; i-- )
	{
		auto* s = scope[i];
		while( s )
			if( s->HasProperty( fullPropName ) )
			{
				stack.Alloc( ) = s->Get( fullPropName );
				return;
			}
			else if( s->GetPrototype( ) && s->GetPrototype( )->GetPrototype( ) )
			{
				s = s->GetPrototype( )->GetPrototype( );
			}
			else
			{
				s = NULL;
			}
	}

	common->Warning( "ReferenceError : property %s could not be found in any object on the scope stack!", fullPropName.c_str( ) );
	bitstream.SeekToEnd( );

	common->Warning( "idSWFScriptFunction_Script::findpropstrict cant find %s", fullPropName.c_str() );
	stack.Alloc().SetObject( idSWFScriptObject::Alloc() );
	stack.A().GetObject()->Release();
}

void idSWFScriptFunction_Script::getlex( SWF_AbcFile* file, idSWFStack& stack, idSWFBitStream& bitstream )
{
	const auto& cp = file->constant_pool;
	const auto& mn = file->constant_pool.multinameInfos[bitstream.ReadEncodedU32( )];

	idStr fullPropName = *cp.namespaceNames[mn.index];
	if ( !fullPropName.IsEmpty( ) ) {
		fullPropName += ".";
	}
	fullPropName += cp.utf8Strings[mn.nameIndex];

	//search current stack scope 
	if ( stack.A( ).IsObject( ) && stack.A( ).GetObject( )->HasProperty( fullPropName ) ) {
		stack.Alloc( ) = stack.A( ).GetObject( )->Get( fullPropName );
		return;
	}

	bool found = false;
	//search current scope stack
	for ( int i = scope.Num( ) - 1; i >= 0; i-- ) {
		auto *s = scope[i];
		while ( s )
			if ( s->HasProperty( fullPropName ) ) {
				stack.Alloc( ) = s->Get( fullPropName );

				if ( stack.A( ).IsObject( ) && stack.A( ).GetObject( )->HasProperty( fullPropName ) ) {
					idSWFScriptVar A = stack.A( );
					stack.Pop(1);
					stack.Alloc( ) = A.GetObject( )->Get( fullPropName );	
				}

				return;
			} else if ( s->GetPrototype( ) && s->GetPrototype( )->GetPrototype( ) ) {
				s = s->GetPrototype( )->GetPrototype( );
			} else {
				s = NULL;
			}
	}

	common->Warning( "ReferenceError : property %s could not be resolved in any object on the scope stack!", fullPropName.c_str( ) );
	bitstream.SeekToEnd( );
}

void idSWFScriptFunction_Script::getscopeobject( SWF_AbcFile* file, idSWFStack& stack, idSWFBitStream& bitstream )
{
	uint8 index = bitstream.ReadEncodedU32();
	stack.Alloc() = scope[( scope.Num() - 1 ) - index];
}

void idSWFScriptFunction_Script::pushscope( SWF_AbcFile* file, idSWFStack& stack, idSWFBitStream& bitstream )
{
	if( stack.Num() > 0 )
	{
		if( stack.A().IsObject() )
		{
			auto stackOBj = stack.A().GetObject();
			stackOBj->AddRef();
			scope.Alloc() = stackOBj;

		}
		else
		{
			common->DWarning( "tried to push a non object onto scope" );
		}
		stack.Pop( 1 );
	}
}

void idSWFScriptFunction_Script::getlocal0( SWF_AbcFile* file, idSWFStack& stack, idSWFBitStream& bitstream )
{
	stack.Alloc() = registers[0];
}

//Classes are constructed implicitly
void idSWFScriptFunction_Script::newclass( SWF_AbcFile* file, idSWFStack& stack, idSWFBitStream& bitstream )
{
	const auto& ci = file->classes[bitstream.ReadEncodedU32( )];
	auto& base = stack.A();
	stack.Pop( 1 );
	idSWFScriptVar* thisObj = &registers[0];
	common->FatalError( "Bytestream corrupted? Classes should already be created in CreateAbcObjects()!" );
}

void abcFunctionCall(const idStrPtr funcName, idSWFParmList& parms,
	idList< idSWFScriptVar, TAG_SWF > & registers, idList< idSWFScriptObject*, TAG_SWF > & scope,idSWFStack& stack )
{
	idSWFScriptFunction *funcCallPtr = nullptr;
	idSWFScriptFunction_Script *funcPtr = nullptr;
	idSWFScriptVar &item = stack.A( );
	if ( item.IsFunction( ) ) {
		funcCallPtr = item.GetFunction( );
		funcPtr = dynamic_cast< idSWFScriptFunction_Script * >( funcCallPtr );
	} else if ( item.IsObject( ) ) {
		funcCallPtr = item.GetObject( )->Get( funcName->c_str( ) ).GetFunction( );
		funcPtr = dynamic_cast< idSWFScriptFunction_Script * >( funcCallPtr );
	}
	if ( funcPtr ) {
		//idSWFScriptFunction_Script func = *funcPtr;
		//if ( !func.GetScope( )->Num( ) ) {
		//	func.SetScope( scope );
		//}

		//func.Call( registers[0].GetObject( ), parms );

		if ( !funcPtr->GetScope( )->Num( ) ) {
			funcPtr->SetScope( scope );
		}
		funcPtr->Call( registers[0].GetObject( ), parms );
	} else {

		funcCallPtr->Call( registers[0].GetObject( ), parms );
	}
}

void idSWFScriptFunction_Script::callpropvoid( SWF_AbcFile* file, idSWFStack& stack, idSWFBitStream& bitstream )
{
	const auto& cp = file->constant_pool;
	const auto& mn = file->constant_pool.multinameInfos[bitstream.ReadEncodedU32( )];
	const idStrPtr funcName = ( idStrPtr ) &cp.utf8Strings[mn.nameIndex];

	uint32 arg_count = bitstream.ReadEncodedU32();

	//Todo Optimize: search for addFrameScript string index in constantpool!
	if( *funcName == "addFrameScript" )
	{
		stack.Pop( arg_count );
		arg_count = 0;
	}
	idSWFParmList parms( arg_count );

	for( int i = 0; i < parms.Num( ); i++ )
	{
		parms[parms.Num() - 1 - i] = stack.A();
		stack.Pop( 1 );
	}
	assert(stack.Num());
	if( stack.Num() )
	{

		abcFunctionCall(funcName,parms,registers,scope,stack);

		//idSWFScriptFunction *funcCallPtr = nullptr;
		//idSWFScriptFunction_Script *funcPtr = nullptr;
		//idSWFScriptVar& item = stack.A();
		//if( item.IsFunction() )
		//{
		//	funcCallPtr = item.GetFunction();
		//	funcPtr = dynamic_cast< idSWFScriptFunction_Script * >( funcCallPtr );
		//}else if( item.IsObject() )
		//{
		//	funcCallPtr = item.GetObject()->Get( funcName->c_str() ).GetFunction();
		//	funcPtr = dynamic_cast< idSWFScriptFunction_Script * >( funcCallPtr );
		//}
		//if ( funcPtr ) {
		//	idSWFScriptFunction_Script func = *funcPtr;
		//	if ( !func.GetScope( )->Num( ) ) {
		//		func.SetScope( scope );
		//	}
		//
		//	func.Call( registers[0].GetObject(), parms );
		//} else 	{
		//	
		//	funcCallPtr->Call( registers[0].GetObject(), parms );
		//}

		////auto func = *( ( idSWFScriptFunction_Script* )item.GetFunction() );
		////if ( !func.GetScope( )->Num( ) ) {
		////	func.SetScope( *GetScope( ) );
		////}
		////auto func = item.GetFunction() ;
		////func->Call( registers[0].GetObject(), parms );
		//
		// if( item.IsObject() )
		//
		//
		//idSWFScriptFunction_Script *funcPtr = dynamic_cast< idSWFScriptFunction_Script * >(  item.GetObject()->Get( funcName->c_str() ));
		//if ( funcPtr ) {
		//	idSWFScriptFunction_Script func = *funcPtr;
		//	if ( !func.GetScope( )->Num( ) ) {
		//		func.SetScope( scope );
		//	}
		//
		//	func.Call( registers[0].GetObject( ), parms );
		//} else {
		//	item.GetFunction( )->Call( registers[0].GetObject( ), parms );
		//}
		//
		//auto func = item.GetObject()->Get( funcName->c_str() );
		//idSWFScriptFunction_Script *funcPtr = dynamic_cast< idSWFScriptFunction_Script * >(func.GetFunction());
		//if ( funcPtr ) {
		//	idSWFScriptFunction_Script func = *funcPtr;
		//	if ( !func.GetScope( )->Num( ) ) {
		//		func.SetScope( scope );
		//	}
		//
		//	func.Call( registers[0].GetObject( ), parms );
		//} else {
		//	item.GetFunction( )->Call( registers[0].GetObject( ), parms );
		//}
			//if( func.IsFunction() )
			//{
			//	if( !( ( idSWFScriptFunction_Script* )func.GetFunction() )->GetScope()->Num() )
			//	{
			//		( ( idSWFScriptFunction_Script* )func.GetFunction() )->SetScope( *GetScope() );
			//	}
			//	func.GetFunction()->Call( item.GetObject(), parms );
			//}
		//}
		stack.Pop( 1 );
	}
}

/*
========================
idSWFScriptFunction_Script::RunAbc bytecode
========================
*/
idSWFScriptVar idSWFScriptFunction_Script::RunAbc( idSWFScriptObject* thisObject, idSWFStack& stack, idSWFBitStream& bitstream )
{
	static int abcCallstackLevel = -1;
	assert( abcFile );
	idSWFScriptVar returnValue;

	idSWFSpriteInstance* thisSprite = thisObject->GetSprite();
	idSWFSpriteInstance* currentTarget = thisSprite;

	if( currentTarget == NULL )
	{
		thisSprite = currentTarget = defaultSprite;
	}

	abcCallstackLevel++;
	while( bitstream.Tell( ) < bitstream.Length( ) )
	{
#define ExecWordCode( n ) case OP_##n: n(abcFile,stack,bitstream); continue;
#define InlineWordCode( n ) case OP_##n:
		SWFAbcOpcode opCode = ( SWFAbcOpcode ) bitstream.ReadU8();
		switch( opCode )
		{
				//ExecWordCode( bkpt );
				//ExecWordCode( throw );
				//ExecWordCode( getsuper );
				//ExecWordCode( setsuper );
				//ExecWordCode( dxns );
				InlineWordCode( dxnslate )
				{
					stack.Pop( 1 );
					continue;
				}
				InlineWordCode( kill )
				{
					uint32 idx = bitstream.ReadEncodedU32();
					registers[idx].SetUndefined();
					registers.RemoveIndex(idx);
					if (registers.Num() < 4)
					{
						assert(registers.Num() == 3);
						registers.Alloc();
					}
					continue;
				}
				InlineWordCode( nop );
				InlineWordCode( label )
				{
					continue;
				}
				InlineWordCode( ifnlt )
				{
					int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							condition = !( lH.ToString() < rH.ToString() );
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							condition = !( lH.ToFloat() < rH.ToFloat() );
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							condition = !( lH.ToInteger() < rH.ToInteger() );
							break;
						default:
							common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( ifnle )
				{
					int offset = bitstream.ReadS24();
					const auto& lH = stack.B();
					const auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							condition = !( lH.ToString() <= rH.ToString() );
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							condition = !( lH.ToFloat() <= rH.ToFloat() );
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							condition = !( lH.ToInteger() <= rH.ToInteger() );
							break;
						default:
							common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( ifngt )
				{
					int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							condition = !( lH.ToString() > rH.ToString() );
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							condition = !( lH.ToFloat() > rH.ToFloat() );
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							condition = !( lH.ToInteger() > rH.ToInteger() );
							break;
						default:
							common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( ifnge )
				{
					int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							condition = !( lH.ToString() >= rH.ToString() );
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							condition = !( lH.ToFloat() >= rH.ToFloat() );
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							condition = !( lH.ToInteger() >= rH.ToInteger() );
							break;
						default:
							common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( jump )
				{
					int offset = bitstream.ReadS24();
					bitstream.Seek( offset );
					continue;
				}
				InlineWordCode( iftrue )
				{
					int offset = bitstream.ReadS24();
					auto& value = stack.A();
					stack.Pop( 1 );
					bool condition = value.ToBool();
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( iffalse )
				{
					int offset = bitstream.ReadS24();
					if( !stack.A().ToBool() )
					{
						bitstream.Seek( offset );
					}
					stack.Pop( 1 );
					continue;
				}
				InlineWordCode( ifeq )
				{
					int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							condition = lH.ToString() == rH.ToString();
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							condition = lH.ToFloat() == rH.ToFloat();
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							condition = lH.ToInteger() == rH.ToInteger();
							break;
						default:
							common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( ifne )
				{
					int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							condition = lH.ToString() != rH.ToString();
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							condition = lH.ToFloat() != rH.ToFloat();
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							condition = lH.ToInteger() != rH.ToInteger();
							break;
						default:
							common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( iflt )
				{
					int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							condition = lH.ToString() < rH.ToString();
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							condition = lH.ToFloat() < rH.ToFloat();
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							condition = lH.ToInteger() < rH.ToInteger();
							break;
						default:
							common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( ifle )
				{
					int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							condition = lH.ToString() <= rH.ToString();
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							condition = lH.ToFloat() <= rH.ToFloat();
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							condition = lH.ToInteger() <= rH.ToInteger();
							break;
						default:
							common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( ifgt )
				{
					int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							condition = lH.ToString() > rH.ToString();
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							condition = lH.ToFloat() > rH.ToFloat();
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							condition = lH.ToInteger() > rH.ToInteger();
							break;
						default:
							common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( ifge )
				{
					int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							condition = lH.ToString() >= rH.ToString();
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							condition = lH.ToFloat() >= rH.ToFloat();
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							condition = lH.ToInteger() >= rH.ToInteger();
							break;
						default:
							common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( ifstricteq )
				{
					int offset = bitstream.ReadS24( );
					auto &lH = stack.B( );
					auto &rH = stack.A( );
					stack.Pop( 2 );

					if ( lH.StrictEquals( rH ) ) {
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( ifstrictne )
				{
					int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );

					if (!lH.StrictEquals(rH))
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( lookupswitch )
				{
					//this can be optimized massivly. 
					//unroling the case offsets and storing them in a list would be a good start.
					int32 baseLocation = bitstream.Tell() - 1;

					int32 defaultOffset = bitstream.ReadS24( );
					uint32 caseCount = bitstream.ReadEncodedU32( );
					idList<int32> caseOffsets;
					caseOffsets.SetNum( caseCount + 1 );
					for ( uint32 i = 0; i <= caseCount; i++ ) {
						caseOffsets[i] = bitstream.ReadS24( );
					}

					int32 index = stack.A( ).ToInteger( );
					stack.Pop(1);
					int32 baseOffset = baseLocation - bitstream.Tell();
					if ( index < 0 || index > caseCount ) {
						bitstream.Seek( baseOffset + defaultOffset );
					} else {
						bitstream.Seek( baseOffset + caseOffsets[index] );
					}
					continue;
				}
				//ExecWordCode( pushwith );
				InlineWordCode( popscope )
				{
					scope[scope.Num() - 1]->Release();
					scope.SetNum( scope.Num() - 1 );
					continue;
				}
				//ExecWordCode( nextname );
				//ExecWordCode( hasnext );
				InlineWordCode( pushnull )
				{
					stack.Append( idSWFScriptVar( ) );
					continue;
				}
				InlineWordCode( pushundefined )
				{
					stack.Append( idSWFScriptVar() );
					stack.A().SetUndefined();
					continue;
				}
				InlineWordCode( nextvalue )
				{
					int index = stack.A().ToInteger();
					idSWFScriptObject *scriptObj = stack.B().GetObject( );
					stack.Pop(2);
					if ( scriptObj->GetPrototype( ) == &vectorScriptObjectPrototype ) {
						if ( ( static_cast< idSWFVectorBase * >( scriptObj ) )->Num( ) >= index )
							stack.Alloc() =  ( static_cast< idSWFVectorBase * >( scriptObj ) )->GetVal(index - 1 ); 
					} else {
							stack.Alloc() = scriptObj->Get(index - 1);
					}
					continue;
				}
				InlineWordCode( pushbyte )
				{
					stack.Append( idSWFScriptVar( ( int )bitstream.ReadU8() ) );
					continue;
				}
				InlineWordCode( pushshort )
				{
					stack.Append( idSWFScriptVar( ( int )bitstream.ReadEncodedU32() ) );
					continue;
				}
				InlineWordCode( pushtrue )
				{
					stack.Append( idSWFScriptVar( true ) );
					continue;
				}
				InlineWordCode( pushfalse )
				{
					stack.Append( idSWFScriptVar( false ) );
					continue;
				}
				InlineWordCode ( pushnan );
				{
					stack.Append( idSWFScriptVar( ) );
				}
				InlineWordCode( pop )
				{
					stack.Pop( 1 );
					continue;
				}
				InlineWordCode( dup )
				{
					stack.Alloc() = idSWFScriptVar( stack.A() );
					continue;
				}
				InlineWordCode( swap )
				{
					common->FatalError( "swap not implemented" );
					continue;
				}
				InlineWordCode( pushstring )
				{
					const auto& cp = abcFile->constant_pool.utf8Strings;
					const auto& mn = cp[bitstream.ReadEncodedU32( )];
					stack.Append( idSWFScriptString( mn ) );
					continue;
				}
				InlineWordCode( pushint )
				{
					const auto& cp = abcFile->constant_pool.integers;
					const auto& val = cp[bitstream.ReadEncodedU32( )];
					stack.Append( idSWFScriptVar( val ) );
					continue;
				}
				InlineWordCode( pushuint )
				{
					const auto& cp = abcFile->constant_pool.uIntegers;
					const auto& val = cp[bitstream.ReadEncodedU32( )];
					stack.Append( idSWFScriptVar( ( int )val ) );
					continue;
				}
				InlineWordCode( pushdouble )
				{
					const auto& cp = abcFile->constant_pool.doubles;
					const auto& val = cp[bitstream.ReadEncodedU32( )];
					stack.Append( idSWFScriptVar( ( float )val ) );
					continue;
				}
				ExecWordCode( pushscope )
				//ExecWordCode( pushnamespace );
				InlineWordCode( hasnext2 ) 
				{
					uint32_t object_reg = bitstream.ReadEncodedU32( );
					uint32_t index_reg = bitstream.ReadEncodedU32( );

					idSWFScriptVar &obj = registers[object_reg];
					idSWFScriptVar &cur_index = registers[index_reg];

					if ( obj.IsObject( ) ) {
						idSWFScriptObject *scriptObj = obj.GetObject( );
						if ( cur_index.GetType ( ) == idSWFScriptVar::SWF_VAR_INTEGER ) {
							int index = cur_index.ToInteger( );
							idSWFScriptObject *nextObj = scriptObj;
							while ( nextObj ) {

								if ( scriptObj->GetPrototype( ) == &vectorScriptObjectPrototype ) {
									if (( static_cast< idSWFVectorBase * >( scriptObj ) )->Num() > index )
									{ 
										cur_index = idSWFScriptVar( index + 1 );
										stack.Alloc( ) = idSWFScriptVar( true );
										nextObj = nullptr;
										continue;
									} else
									{
										nextObj = nullptr;
										continue;
									}
								}else
								{
									if ( nextObj->NumEnumerateable( ) > index ) {
										cur_index = idSWFScriptVar( index + 1 );
										stack.Alloc( ) = idSWFScriptVar( true );
										nextObj = nullptr;
										continue;
									}
								}

								if ( !nextObj ) {
									obj = idSWFScriptVar( );
									cur_index = idSWFScriptVar( 0 );
									stack.Alloc( ) = idSWFScriptVar( false );
								}else
								{
									nextObj = nextObj->GetPrototype( );
									index = 0;
								}
							
							}
						}
					}
					continue;
				}
				//ExecWordCode( lix8 );
				//ExecWordCode( lix16 );
				//ExecWordCode( li8 );
				//ExecWordCode( li16 );
				//ExecWordCode( li32 );
				//ExecWordCode( lf32 );
				//ExecWordCode( lf64 );
				//ExecWordCode( si8 );
				//ExecWordCode( si8 );
				//ExecWordCode( si16 );
				//ExecWordCode( si32 );
				//ExecWordCode( sf32 );
				//ExecWordCode( sf64 );
				InlineWordCode( newfunction )
				{
					const auto& cp = abcFile->constant_pool;
					auto& method = abcFile->methods[bitstream.ReadEncodedU32()];
					idSWFScriptFunction_Script* func = idSWFScriptFunction_Script::Alloc();
					func->SetAbcFile( abcFile );
					func->methodInfo = &method;
					func->SetScope( scope );
					func->SetConstants( constants );
					func->SetDefaultSprite( defaultSprite );
					stack.Alloc() = idSWFScriptVar( func );
					stack.A().GetFunction()->Release();
					continue;
				}
				InlineWordCode( call )
				InlineWordCode( construct )
				{
					uint32 args = bitstream.ReadEncodedU32();
					if ( stack.B().IsObject() && stack.B().GetObject()->GetPrototype() == &vectorScriptObjectPrototype ) {
						( static_cast< idSWFVectorBase* >( stack.B().GetObject() ) )->SetNum(stack.A().ToInteger());
						stack.Pop( 1 );
					}
					else
					{
						//common->Printf( "popping %i constructor args \n", args );
						stack.Pop( args );
					}

					continue;
				}
				InlineWordCode( callmethod )
				InlineWordCode( callstatic )
				InlineWordCode( callsuper )
				{
					common->FatalError( "Not implemented" );
					continue;
				}
				InlineWordCode( callproperty )
				{
					//fold this with callpropvoid.
					const auto& cp = abcFile->constant_pool;
					const auto& mn = abcFile->constant_pool.multinameInfos[bitstream.ReadEncodedU32( )];
					const idStrPtr funcName = ( idStrPtr ) &cp.utf8Strings[mn.nameIndex];
					uint32 arg_count = bitstream.ReadEncodedU32( );
					idSWFParmList parms( arg_count );

					for( int i = 0; i < parms.Num( ); i++ )
					{
						parms[parms.Num() - 1 - i] = stack.A( );
						stack.Pop( 1 );
					}
					idSWFScriptVar& item = stack.A( );

					if( item.IsFunction( ) )
					{
						stack.Pop( 1 );
						stack.Alloc() = item.GetFunction( )->Call( registers[0].GetObject( ), parms );
					}
					else if( item.IsObject( ) )
					{
						auto func = item.GetObject()->Get( funcName->c_str() );
						if( !func.IsFunction() ) // search up scope
						{
							for( int i = scope.Num() - 1; i >= 0; i-- )
							{
								auto* s = scope[i];
								while( s )
								{
									if( s->HasProperty( funcName->c_str() ) )
									{
										func = s->Get( funcName->c_str() );
										s = NULL;
										i = -1;
										break;
									}
									else if( s->GetPrototype() && s->GetPrototype()->GetPrototype() )
									{
										s = s->GetPrototype()->GetPrototype();
									}
									else
									{
										s = NULL;
									}
								}
							}
						}
						if( func.IsFunction( ) )
						{
							if ( !( ( idSWFScriptFunction_Script * ) func.GetFunction( ) )->GetScope( )->Num( ) ) {
								( ( idSWFScriptFunction_Script * ) func.GetFunction( ) )->SetScope( *GetScope( ) );
							}
							stack.Pop( 1 );
							stack.Alloc() = func.GetFunction( )->Call( item.GetObject( ), parms );
						}
					}
					continue;
				}
				InlineWordCode( returnvalue )
				{
					returnValue = stack.A();
					[[fallthrough]];
				}
				InlineWordCode( returnvoid )
				{
					if( scope.Num() )
					{
						scope[scope.Num() - 1]->Release();
						scope.SetNum( scope.Num() - 1 );
					}
					continue;
				}
				InlineWordCode( constructsuper )
				{
					uint32 arg_count = bitstream.ReadEncodedU32( );
					idSWFParmList parms( arg_count );

					for ( int i = 0; i < parms.Num( ); i++ ) {
						parms[parms.Num( ) - 1 - i] = stack.A( );
						stack.Pop( 1 );
					}

					static int sSuperConstructorDepth = 0;
					int superIdxOffset = (sSuperConstructorDepth ? (2*sSuperConstructorDepth+1) : 2);
					if (scope[scope.Num() - superIdxOffset]->HasValidProperty("super"))
					{
						auto *super = scope[scope.Num()- superIdxOffset]->Get( "super" ).GetObject( )->GetPrototype( );
						if ( super ) {
							//if ( !super->HasProperty( "__constructor__" ) && stack.A().GetObject()->HasProperty( "__constructor__" ))
							//	super = stack.A().GetObject();
							if ( super->HasProperty( "__constructor__" ) ) {

								sSuperConstructorDepth++;
								auto &item = super->Get( "__constructor__" );
								if ( item.IsFunction( ) ) {
									auto func = ( ( idSWFScriptFunction_Script * ) item.GetFunction( ) );
									GetScope( )->Alloc( ) = super->GetPrototype( )->GetPrototype( )->GetPrototype( );
									if ( !func->GetScope( )->Num( ) ) {
										func->SetScope( *GetScope( ) );
									}
									item.GetFunction( )->Call( registers[0].GetObject( ), parms );
									GetScope( )->Remove ( scope[scope.Num()-1] );
								}
								sSuperConstructorDepth--;
							}
						}
					}
					continue;
				}
				InlineWordCode( constructprop )
				{
					static int sConstructorDepth = 0;
					//no need to call constructors for props that
					const auto& cp = abcFile->constant_pool;
					//const auto& mn = abcFile->constant_pool.multinameInfos[bitstream.ReadEncodedU32( )];
					//const idStrPtr propName = ( idStrPtr ) &cp.utf8Strings[mn.nameIndex];


					swfMultiname mn, tn;
					idStr fullName = ResolveMultiname( abcFile, stack, bitstream, mn, tn );
					//const auto& mn = cp.multinameInfos[bitstream.ReadEncodedU32()];
					const idStrPtr propName = ( idStrPtr ) &cp.utf8Strings[mn.nameIndex];


					uint32 arg_count = bitstream.ReadEncodedU32( );
					if( *propName == "Array" )
					{
						auto *arrayObj = stack[stack.Num( ) - ( arg_count + 1 )].GetObject( );
						if (arg_count == 1)
						{
							auto *lengthVar = arrayObj->GetVariable( "length", true );
							lengthVar->value.SetInteger( stack.A().ToInteger() );
						}
						else
						{
							for ( int i = 0; i < arg_count; i++ ) {
								arrayObj->Set( i, stack[stack.Num( ) - i - 1] );
							}
						}
						
					}else if (stack[stack.Num() - ( arg_count + 1 )].GetObject()->HasProperty("__constructor__" ))
					{
						idSWFParmList parms( arg_count );

						for ( int i = 0; i < parms.Num( ); i++ ) {
							parms[parms.Num( ) - 1 - i] = stack.A( );
							stack.Pop( 1 );
						}

						idSWFScriptVar &item = stack.A( ).GetObject()->Get("__constructor__" );

						sConstructorDepth++;
						if ( item.IsFunction( ) ) {

							idSWFScriptFunction_Script *funcPtr = dynamic_cast<idSWFScriptFunction_Script *>(item.GetFunction( ));
							assert(funcPtr);
							if (funcPtr)
							{
								//idSWFScriptFunction_Script func = *funcPtr;
								//auto func = *( ( idSWFScriptFunction_Script * ) );
								if ( !funcPtr->GetScope( )->Num( ) ) {
									funcPtr->SetScope( scope );
								}
								if (stack.A( ).GetObject()->HasProperty("["+fullName+"]" ))
									stack.A( ).GetObject( )->DeepCopy(stack.A( ).GetObject()->Get("["+fullName+"]" ).GetObject() );
								item.GetFunction( )->Call( stack.A( ).GetObject( ), parms );
								//stack.Alloc( ) = func.Call( stack.A( ).GetObject( ), parms );
							}

						}
						sConstructorDepth--;

						continue;
					}
					stack.Pop( arg_count );
					continue;
				}
				//ExecWordCode( callsuperid );
				//ExecWordCode( callproplex );
				//ExecWordCode( callinterface );
				//ExecWordCode( callsupervoid );
				ExecWordCode( callpropvoid );
				//ExecWordCode( sxi1 );
				//ExecWordCode( sxi8 );
				//ExecWordCode( sxi16 );
				InlineWordCode( applytype )
				{
					//this is already implicitly done by getLex on Vector::<CLASS>, so cleanup old base and set new one on top
					uint32 typeParamCount = bitstream.ReadEncodedU32();
					assert (typeParamCount == 1);
					idSWFScriptObject * baseType = stack.A( ).GetObject();
					stack.Pop( 2 ); // todo : use deletefast
					stack.Alloc().SetObject(  baseType );
					continue;
				}
				//ExecWordCode( DISABLED_pushfloat4 );
				InlineWordCode( newarray )
				{
					auto* newArray = idSWFScriptObject::Alloc();

					newArray->MakeArray();

					uint32 args = bitstream.ReadEncodedU32();
					for( int i = 0; i < args; i++ )
					{
						newArray->Set( i, stack.A() );
						stack.Pop( 1 );
					}

					idSWFScriptVar baseObjConstructor = scope[0]->Get( "Object" );
					idSWFScriptFunction* baseObj = baseObjConstructor.GetFunction();
					newArray->SetPrototype( baseObj->GetPrototype() );
					stack.Alloc().SetObject( newArray );

					newArray->Release();
					continue;
				}
				InlineWordCode( newobject );
				InlineWordCode( newactivation )
				{
					idSWFScriptObject* object = idSWFScriptObject::Alloc();
					idSWFScriptVar baseObjConstructor = scope[0]->Get( "Object" );
					idSWFScriptFunction* baseObj = baseObjConstructor.GetFunction();
					object->SetPrototype( baseObj->GetPrototype() );
					stack.Alloc().SetObject( object );
					object->Release();
					continue;
				}
				ExecWordCode( newclass );
				//ExecWordCode ( getdescendants );
				//ExecWordCode ( newcatch );
				//ExecWordCode ( findpropglobalstrict );
				//ExecWordCode ( findpropglobal );
				ExecWordCode( findproperty );
				ExecWordCode( findpropstrict );

				//ExecWordCode ( finddef );
				ExecWordCode( getlex );
				InlineWordCode( setproperty )
				{
					auto &cp = abcFile->constant_pool;
					uint32 mnIndex = bitstream.ReadEncodedU32( );
					auto &mn = cp.multinameInfos[mnIndex];
					SWF_Multiname &m = cp.extMultinameInfos[mnIndex];
					auto &n = cp.utf8Strings[mn.nameIndex];

					idStr index = n;
					idSWFScriptObject *target = nullptr;
					auto& val = stack.A();
					stack.Pop(1);
					if (!m.isRuntime())
					{
						if ( stack.A( ).IsObject( ) )
						{
							auto *targetVar = stack.A( ).GetObject( )->GetVariable( n, false );
							if ( targetVar )
								targetVar->value = val;
							else
								stack.A( ).GetObject( )->Set(n,val);
						}
						else
							stack.A( ) = val;

						stack.Pop( 1 );
						continue;
					}else if (!m.isRtns() && stack.A().IsInt() && stack.B().IsObject())
					{
						if ( target && target->GetPrototype( ) == &vectorScriptObjectPrototype ) {
							int32 idx = stack.A( ).ToInteger( );
							target = stack.B( ).GetObject( );
							stack.Pop( 2 );
							idSWFVectorBase::setproperty(idx,val,target );
							stack.Pop(2);
							continue;
						}
						target = stack.B( ).GetObject( );
						target->Set( stack.A( ).ToString(), val );
						stack.Pop(2);
					}else if (m.isRtns() || (stack.B().IsObject() && !stack.B().GetObject()->IsArray()) )
					{
						target = stack.B( ).GetObject( );
						target->Set( stack.A( ).ToString(), val );
						stack.Pop( 2 );
					}else
					{
						target = stack.B( ).GetObject( );
						target->Set( stack.A( ).ToString( ), val );
						stack.Pop( 2 );
					}
					continue;
					//const auto& cp = abcFile->constant_pool;
					//swfMultiname mn,tn;
					//
					////idStr fullName = ResolveMultiname(abcFile,stack,bitstream,mn,tn);
					////const auto& mn = cp.multinameInfos[bitstream.ReadEncodedU32()];
					//const auto& n = cp.utf8Strings[mn.nameIndex];
					//idStr strIndex = n;
					//idSWFScriptObject* target = nullptr;
					////multiname is not resolved properly!


					//auto& val = stack.A();
					//if (!SWF_AbcFile::IsRuntime(mn))
					//{
					//	target = stack.C().GetObject();
					//	target->Set( strIndex, val );
					//	stack.Pop( 2 );
					//	continue;
					//}else if( !SWF_AbcFile::IsRTns(mn) && stack.B().IsInt() && (!stack.C().IsValid() && stack.C().IsObject() ))
					//{
					//	stack.C().GetObject()->Set(strIndex,stack.B());
					//	stack.Pop( 3 );
					//	continue;
					//}else if (SWF_AbcFile::IsRTns(mn) || !stack.B().GetObject()->IsArray())
					//{
					//	auto &val = stack.A( );
					//	stack.C( ).GetObject( )->Set( n, stack.A( ) );
					//	stack.Pop( 3 );
					//	continue;
					//}else
					//{
					//	if ( stack.B( ).GetObject( )->GetPrototype( ) == &vectorScriptObjectPrototype ) {
					//		idSWFVectorBase::setproperty( stack, target );
					//		continue;
					//	} else if ( target->IsArray( ) )
					//	{
					//		assert(stack.C().IsObject());
					//		target = stack.C( ).GetObject( );
					//		strIndex = stack.A( ).ToString( );
					//		auto &val = stack.A( );
					//		stack.Pop( 3 ); // swap + pop, no alloc needed!
					//		target->Set( strIndex, val );
					//		stack.Alloc( ) = target;
					//		continue;
					//	}
					//}
					//assert(0);
					//else if( !mn.nameIndex )
					//{
					//	//target = stack.C().IsObject() ? stack.C().GetObject() : stack.B( ).GetObject( );
					//	target = stack.B().GetObject();
					//	if ( stack.B( ).IsObject( ) ) {
					//		target = stack.B( ).GetObject( );
					//		if (stack.B( ).GetObject( )->GetPrototype( ) == &vectorScriptObjectPrototype ) {
					//			idSWFVectorBase::setproperty( stack, target );
					//			continue;
					//		}else if (target->IsArray())
					//		{
					//			assert(stack.C().IsObject());
					//			target = stack.C( ).GetObject( );
					//			strIndex = stack.A( ).ToString( );
					//			auto &val = stack.A( );
					//			stack.Pop( 3 ); // swap + pop, no alloc needed!
					//			target->Set( strIndex, val );
					//			stack.Alloc( ) = target;
					//			continue;
					//		}
					//	} else 
					//	{
					//		assert( stack.C( ).IsObject( ) );
					//		target = stack.C( ).GetObject( );
					//	}
					//	
					//	strIndex = stack.B().ToString();
					//	
					//	target->Set( strIndex, val );
					//	stack.Pop( 2 ); // swap + pop, no alloc needed!
					//	//stack.Alloc( ) = val;
					//}
					//else if( stack.B().IsObject() )
					//{
						//target = stack.B().GetObject();
					//}
					//auto& val = stack.A();
					//stack.C().GetObject()->Set(n, stack.A());
					//stack.Pop( 3 );
					////stack.Alloc( ) = target;
					//continue;
				}
				InlineWordCode( getlocal )
				{
					stack.Alloc() = registers[bitstream.ReadEncodedU32()];
					continue;
				}
				InlineWordCode( setlocal );
				{
					uint32 idx = bitstream.ReadEncodedU32();
					if (idx >= registers.Num())
					{
						registers.AssureSize( idx );
						registers.AddGrow( stack.A( ) );
					}
					else 
					{ 
						assert(idx < registers.Num());
						registers[idx] = stack.A();
					}
					stack.Pop( 1 );
					continue;
				}
				//ExecWordCode ( getglobalscope );
				ExecWordCode( getscopeobject );
				InlineWordCode( getproperty )
				{

					auto &cp = abcFile->constant_pool;
					uint32 mnIndex = bitstream.ReadEncodedU32( );
					auto &mn = cp.multinameInfos[mnIndex];
					SWF_Multiname &m = cp.extMultinameInfos[mnIndex];
					auto &n = cp.utf8Strings[mn.nameIndex];

					idStr index = n;
					idSWFScriptObject* target = nullptr;

					if (!m.isRuntime())
					{
						target = stack.A( ).GetObject( );
						stack.Pop( 1 );
						stack.Alloc( ) = target->Get( n );
					}else if (!m.isRtns() && stack.A().IsInt() && stack.B().IsObject())
					{
						int32 idx = stack.A( ).ToInteger( );
						target = stack.B( ).GetObject( );
						stack.Pop( 2 );
						if ( target && target->GetPrototype( ) == &vectorScriptObjectPrototype ) {							
							stack.Append( ( static_cast< idSWFVectorBase * >( target ) )->GetVal( idx ) );
							continue;
						}
						stack.Alloc( ) = target->Get( idx );
					}else if (m.isRtns() || (stack.B().IsObject() && !stack.B().GetObject()->IsArray()) )
					{
						if ( target && target->GetPrototype( ) == &vectorScriptObjectPrototype ) {
							stack.Pop( 2 );
							stack.Append( ( static_cast< idSWFVectorBase * >( target ) )->GetVal( stack.A( ).ToInteger( ) ) );
							continue;
						}

						index = stack.A().ToString();
						target = stack.B().GetObject();
						stack.Pop(2);
						stack.Alloc() = target->Get(index);
					}
					//if( mn.nameIndex && !stack.A().IsObject() )
					//{
					//	target = scope[0];
					//}
					//else if( !mn.nameIndex )
					//{
					//	target = stack.B().GetObject();
					//
					//	if ( target && target->GetPrototype( ) == &vectorScriptObjectPrototype ) {
					//		int32 idx = stack.A().ToInteger();
					//		stack.Pop( 2 );
					//		stack.Append(( static_cast< idSWFVectorBase* >( target ) )->GetVal(idx));
					//		continue;
					//	}
					//
					//	index = stack.A().ToString();
					//	stack.Pop( 1 );
					//}
					//else
					//{
					//	target = stack.A().GetObject();
					//}
					//
					//stack.Pop( 1 );
					//
					//if( target->HasProperty( index.c_str() ) )
					//{
					//	stack.Append( target->Get( index.c_str() ) );
					//}
					//else
					//{
					//	stack.Alloc().SetObject( idSWFScriptObject::Alloc() );
					//	stack.A().GetObject()->Release();
					//}

					continue;
				}
				//ExecWordCode ( getouterscope );
				InlineWordCode( initproperty )
				{
					const auto& cp = abcFile->constant_pool;
					uint32 mnIndex = bitstream.ReadEncodedU32( );
					const auto& mn = cp.multinameInfos[mnIndex];
					const auto m = cp.extMultinameInfos[mnIndex];
					const auto& n = cp.utf8Strings[mn.nameIndex];

					idSWFScriptVar value = stack.A( );
					stack.Pop( 1 );
					stack.A().GetObject()->Set( n, value );
					continue;
				}
				//ExecWordCode ( 0x69 );
				//ExecWordCode ( deleteproperty );
				//ExecWordCode ( 0x6B );
				InlineWordCode( getslot )
				{
					if( stack.A().IsObject() )
					{
						stack.Append( stack.A().GetObject()->Get( bitstream.ReadEncodedU32() ) );
					}
					continue;
				}

				InlineWordCode( setslot )
				{
					auto& var = stack.A();

					if( stack.B().IsUndefined() || stack.B().IsNULL() )
					{
						stack.B().SetObject( idSWFScriptObject::Alloc() );
						stack.B().GetObject()->MakeArray();
						stack.B().GetObject()->Release();
					}

					stack.B().GetObject()->Set( bitstream.ReadEncodedU32(), var );
					continue;
				}
				//ExecWordCode ( getglobalslot );
				//ExecWordCode ( setglobalslot );
				//ExecWordCode ( convert_s );
				//ExecWordCode ( esc_xelem );
				//ExecWordCode ( esc_xattr );
				InlineWordCode ( convert_u )
				InlineWordCode ( convert_i )
				{
					stack.A( ).SetInteger( stack.A( ).ToInteger( ) );
					continue;
				}
				InlineWordCode ( convert_d )
				{
					stack.A().SetFloat( stack.A().ToFloat() );
					continue;
				}
				InlineWordCode ( convert_b )
				{
					stack.A().SetBool( stack.A().ToBool() );
				}
				//ExecWordCode ( convert_o );
				//ExecWordCode ( checkfilter );
				//ExecWordCode ( DISABLED_convert );
				//ExecWordCode ( DISABLED_unplus );
				//ExecWordCode ( DISABLED_convert );
				InlineWordCode ( coerce );
				{
					swfMultiname typeNameX;
					const swfMultiname* typeName = nullptr;
					idSWFBitStream tmpBS ;
					tmpBS.Load(bitstream.ReadData(0),bitstream.Length() - bitstream.Tell(),true);
					//idStr mns = ResolveMultiname(abcFile,stack,tmpBS,typeNameX);
					const swfConstant_pool_info &cp = abcFile->constant_pool;
					auto *mn = &cp.multinameInfos[bitstream.ReadEncodedU32( )];
					
					while (mn->type != Qname)
					{
						if ( mn->type == TypeName )
							typeName = &cp.multinameInfos[mn->indexT];
						mn =  &cp.multinameInfos[mn->nameIndex];
					}
					const auto &n = cp.utf8Strings[mn->nameIndex];
					
					//idStr &n = mns;
					idSWFScriptVar& value = stack.A();

					idSWFScriptVar coercedValue;
					bool success = false;
					
					// Resolve the type specified by the multiname
					if ( mn->type ==  Qname ) {
						if (value.IsUndefined() || (value.GetType() == idSWFScriptVar::SWF_VAR_UNDEF ))
						{
							idStr fullClassName = n;
							idSWFScriptVar proto = scope[0]->Get( n );
							if ( !proto.IsValid() )
							{
								idStr &className = abcFile->constant_pool.utf8Strings[mn->nameIndex];
								fullClassName = *abcFile->constant_pool.namespaceNames[mn->index];
								if ( !fullClassName.IsEmpty( ) ) {
									fullClassName += ".";
								}
								fullClassName += className;

								proto = scope[0]->Get( fullClassName );
							}
							if ( proto.IsObject() && proto.GetObject()->GetPrototype() ) 
							{								
								idSWFScriptObject *val_obj = idSWFScriptObject::Alloc( );
								if ( typeName != nullptr && proto.IsObject( ) ) {
									////hmm this is only valid if this is a vector.
									val_obj = proto.GetObject( )->Get( n ).GetObject( )->Get( cp.utf8Strings[typeName->nameIndex] ).GetObject();
								} else {		
									auto *super = proto.GetObject( );
									//make sure to dcopy 
									auto dcopy = super->Get( "[" + fullClassName + "]" );
									if ( dcopy.IsObject( ) ) {
										val_obj->DeepCopy( dcopy.GetObject( ) );
									}
									val_obj->SetPrototype( super->GetPrototype() );
								}
								
								value.SetObject(val_obj);
								val_obj->Release();
								
								coercedValue = value;
								success = true;
							}

						}else
						{

							idSWFScriptVar cls = scope[0]->Get( n );

							if ( !cls.IsValid( ) ) {
								idStr &className = abcFile->constant_pool.utf8Strings[mn->nameIndex];
								idStr fullClassName = *abcFile->constant_pool.namespaceNames[mn->index];
								if ( !fullClassName.IsEmpty( ) ) {
									fullClassName += ".";
								}
								fullClassName += className;

								cls = scope[0]->Get( fullClassName );
							}

							if (cls.IsObject())							
							{
								if (cls.GetObject()->GetPrototype())
								{
									const idSWFScriptObject *type = cls.GetObject()->GetPrototype();
									if ( type ) {
										// Check if the value can be coerced to the specified type
										if ( value.IsObject( ) ) {
											if ( value.GetObject( )->IsInstanceOf( type ) ) {
												coercedValue = value;
												success = true;
											}
										}
									}
								}
							}
						}
					}

					if ( !success ) {
						// Throw a TypeError if the value cannot be coerced to the specified type
						common->Warning( "TypeError: Cannot coerce value to the specified type %s",n.c_str() );
						bitstream.SeekToEnd();
						continue;
					}

					// Push the coerced value onto the stack
					stack.Alloc() = coercedValue;
					continue;
				}
				InlineWordCode ( coerce_a )
				{
					continue;
				}
				InlineWordCode( coerce_s )
				{
					auto& var = stack.A();
					stack.Pop( 1 );
					if( !stack.A().IsValid() )
					{
						stack.A().SetNULL();
						stack.Append( var );
					}
					else
					{
						if( !var.IsUndefined() )
						{
							stack.Append( var.ToString() );
						}
						else
						{
							stack.Append( var );
						}

					}

					continue;
				}
				//ExecWordCode ( coerce_i );
				//ExecWordCode ( coerce_d );
				//ExecWordCode ( coerce_b );
				//ExecWordCode ( astype );
				InlineWordCode ( astypelate )
				{
					idSWFScriptVar &type = stack.A( );
					idSWFScriptVar &value = stack.B( );
					stack.Pop( 1 );
					if ( value.IsObject( ) && type.IsObject( ) ) {
						if (!value.GetObject( )->IsInstanceOf( type.GetObject( )->GetPrototype( ) ) )
						{
							stack.Pop( 1 );
							stack.Alloc( );
						}
					} else {
						stack.Pop(1);
						stack.Alloc();
					}

					continue;
				}
				//ExecWordCode ( coerce_u );
				//ExecWordCode ( coerce_o );
				InlineWordCode( negate_i )
				InlineWordCode( negate )
				{
					auto& val = stack.A( );
					idSWFScriptVar result;
					switch( val.GetType( ) )
					{
						case idSWFScriptVar::SWF_VAR_FLOAT:
							val.SetFloat( -val.ToFloat() );
							continue;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							val.SetInteger( -val.ToInteger() );
							continue;
						default:
							common->Warning( " Tried to negate incompatible type %s", val.TypeOf( ) );
					}
					continue;
				}
				InlineWordCode( increment_i )
				InlineWordCode( increment )
				{
					auto& val = stack.A( );
					stack.Pop(1);
					switch( val.GetType( ) )
					{
						case idSWFScriptVar::SWF_VAR_FLOAT:
							stack.Alloc() =  val.ToFloat() + 1.0f;
							continue;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							stack.Alloc() =  val.ToInteger() + 1.0f;
							continue;
						default:
							common->Warning( " Tried to increment incompatible type %s", val.TypeOf( ) );
					}
					continue;
				}
				InlineWordCode( inclocal_i )
				InlineWordCode( inclocal )
				{
					auto& val = registers[bitstream.ReadEncodedU32()];
					idSWFScriptVar result;
					switch( val.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_FLOAT:
							val.SetFloat( val.ToFloat() + 1.0f );
							continue;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							val.SetInteger( val.ToInteger() + 1 );
							continue;
						default:
							common->Warning( " Tried to increment incompatible register type %s", val.TypeOf() );
					}
					continue;
				}
				InlineWordCode( decrement_i )
				InlineWordCode( decrement )
				{
					auto& val = stack.A( );
					stack.Pop(1);
					switch( val.GetType( ) )
					{
					case idSWFScriptVar::SWF_VAR_FLOAT:
						stack.Alloc() =  val.ToFloat() - 1.0f;
						continue;
					case idSWFScriptVar::SWF_VAR_INTEGER:
						stack.Alloc() =  val.ToInteger() - 1.0f;
						continue;
					default:
						common->Warning( " Tried to decrement incompatible type %s", val.TypeOf( ) );
					}
					continue;
				}
				InlineWordCode( declocal_i );
				InlineWordCode( declocal );
				{
					auto& val = registers[bitstream.ReadEncodedU32()];
					switch( val.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_FLOAT:
							val.SetFloat( val.ToFloat() - 1.0f );
							continue;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							val.SetInteger( val.ToInteger() - 1 );
							continue;
						default:
							common->Warning( " Tried to decrement incompatible register type %s", val.TypeOf() );
					}
					continue;
				}
				//ExecWordCode ( typeof );
				InlineWordCode( not )
				{
					stack.A().SetBool( !stack.A().ToBool() );
					continue;
				}
				//ExecWordCode ( bitnot );
				InlineWordCode( add_i )
				InlineWordCode( add )
				{
					auto& lH = stack.B();
					auto& rH = stack.A();
					idSWFScriptVar result;
					switch( lH.GetType( ) )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							result.SetString( lH.ToString( ) + rH.ToString( ) );
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							result.SetFloat( lH.ToFloat( ) + rH.ToFloat( ) );
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							result.SetInteger( lH.ToInteger( ) + rH.ToInteger( ) );
							break;
						case idSWFScriptVar::SWF_VAR_FUNCTION:
							result.SetString( lH.ToString( ) + rH.ToString( ) );
							break;
						default:
							common->Warning( " Tried to add incompatible types %s + %s", lH.TypeOf( ), rH.TypeOf( ) );
					}

					stack.Pop( 2 );
					stack.Alloc() = result;
					continue;
				}
				InlineWordCode( subtract_i )
				InlineWordCode( subtract )
				{
					auto& lH = stack.A();
					auto& rH = stack.B();
					idSWFScriptVar result;
					switch( lH.GetType( ) )
					{
						case idSWFScriptVar::SWF_VAR_FLOAT:
							result.SetFloat( lH.ToFloat( ) - rH.ToFloat( ) );
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							result.SetInteger( lH.ToInteger( ) - rH.ToInteger( ) );
							break;
						default:
							common->Warning( " Tried to subtract incompatible types %s + %s", lH.TypeOf( ), rH.TypeOf( ) );
					}

					stack.Pop( 2 );
					stack.Alloc() = result;
					continue;
				}
				InlineWordCode( multiply_i )
				InlineWordCode( multiply )
				{
					auto& lH = stack.A();
					auto& rH = stack.B();
					idSWFScriptVar result;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_FLOAT:
							result.SetFloat( lH.ToFloat() * rH.ToFloat() );
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							result.SetInteger( lH.ToInteger() * rH.ToInteger() );
							break;
						default:
							common->Warning( " Tried to multiply incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}

					stack.Pop( 2 );
					stack.Alloc() = result;
					continue;
				}
				InlineWordCode( divide )
				{
					auto& lH = stack.A();
					auto& rH = stack.B();
					idSWFScriptVar result;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_FLOAT:
							result.SetFloat( lH.ToFloat() / rH.ToFloat() );
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							result.SetInteger( lH.ToInteger() / rH.ToInteger() );
							break;
						default:
							common->Warning( " Tried to divide incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}

					stack.Pop( 2 );
					stack.Alloc() = result;
					continue;
				}
				//ExecWordCode ( modulo );
				//ExecWordCode ( lshift );
				//ExecWordCode ( rshift );
				//ExecWordCode ( urshift );
				InlineWordCode ( bitand )
				{
					stack.B( ).SetInteger( stack.B( ).ToInteger( ) & stack.A( ).ToInteger( ) );
					stack.Pop( 1 );
				}
				InlineWordCode ( bitor )
				{
					stack.B( ).SetInteger( stack.B( ).ToInteger( ) | stack.A( ).ToInteger( ) );
					stack.Pop( 1 );
				}
				InlineWordCode ( bitxor )
				{
					stack.B( ).SetInteger( stack.B( ).ToInteger( ) ^ stack.A( ).ToInteger( ) );
					stack.Pop( 1 );
				}
				InlineWordCode( equals )
				{
					auto& lH = stack.A();
					auto& rH = stack.B();
					stack.Pop( 2 );
					stack.Alloc() = lH.AbstractEquals( rH );
					continue;
				}
				InlineWordCode( strictequals )
				{
					auto& lH = stack.A();
					auto& rH = stack.B();
					stack.Pop( 2 );
					stack.Alloc() = lH.StrictEquals( rH );
					continue;
				}
				//ExecWordCode ( lessthan );
				//ExecWordCode ( lessequals );
				InlineWordCode( greaterthan )
				{
					//int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					switch( lH.GetType() )
					{
					case idSWFScriptVar::SWF_VAR_STRING:
						condition = lH.ToString() > rH.ToString();
						break;
					case idSWFScriptVar::SWF_VAR_FLOAT:
						condition = lH.ToFloat() > rH.ToFloat();
						break;
					case idSWFScriptVar::SWF_VAR_INTEGER:
						condition = lH.ToInteger() > rH.ToInteger();
						break;
					default:
						common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}
					stack.Alloc().SetBool(condition);
					continue;
				}
				//ExecWordCode ( greaterequals );
				//ExecWordCode ( instanceof );
				//ExecWordCode ( istype );
				InlineWordCode ( istypelate )
				{
					idSWFScriptVar& type = stack.A();
					idSWFScriptVar& value = stack.B();
					stack.Pop(2);
					if ( value.IsObject( ) && type.IsObject( ) ) {
						stack.Alloc().SetBool(value.GetObject( )->IsInstanceOf( type.GetObject( )->GetPrototype() ));
					}else
					{
						stack.Alloc().SetBool(false);
					}
				
					continue;
				}
				//ExecWordCode ( in );
				InlineWordCode( getlocal0 )
				{
					stack.Alloc() = registers[0];
					continue;
				}
				InlineWordCode( getlocal1 )
				{
					stack.Alloc() = registers[1];
					continue;
				}
				InlineWordCode( getlocal2 )
				{
					stack.Alloc() = registers[2];
					continue;
				}
				InlineWordCode( getlocal3 )
				{
					stack.Alloc() = registers[3];
					continue;
				}
				InlineWordCode( setlocal0 )
				{
					registers[0] = stack.A();
					stack.Pop( 1 );
					continue;
				}
				InlineWordCode( setlocal1 )
				{
					registers[1] = stack.A();
					stack.Pop( 1 );
					continue;
				}
				InlineWordCode( setlocal2 )
				{
					registers[2] = stack.A();
					stack.Pop( 1 );
					continue;
				}
				InlineWordCode( setlocal3 )
				{
					registers[3] = stack.A();
					stack.Pop( 1 );
					continue;
				}
				InlineWordCode( debug )
				{
					uint8 type = bitstream.ReadU8();
					uint32 index = bitstream.ReadEncodedU32();
					uint8 reg = bitstream.ReadU8();
					uint32 extra = bitstream.ReadEncodedU32();
					continue;
				}
				InlineWordCode( debugline )
				{
					uint32 nr = bitstream.ReadEncodedU32( );
					if (swf_debug.GetInteger( ) > 0)
						common->DWarning( "^8 debugLine %i", (int)nr );
					continue;
				}
				InlineWordCode( debugfile )
				{
					uint32 nr = bitstream.ReadEncodedU32();					
					if (swf_debug.GetInteger( ) > 0)
						common->DPrintf( "^8 debugfile %s\n", this->abcFile->constant_pool.utf8Strings[nr].c_str());
					continue;
				}
			//ExecWordCode ( bkptline );
			//ExecWordCode ( timestamp );
			//ExecWordCode ( restargc );
			//ExecWordCode ( restarg );
			//ExecWordCode ( codes );
			default:
			{
				const AbcOpcodeInfo* info = &opcodeInfo[opCode];
				common->DWarning( "^5Unhandled Opcode %s\n", info ? info->name : "Empty" );
				//bitstream.SeekToEnd( );
				DebugBreak();
			}

		}
	}
	abcCallstackLevel--;
	return returnValue;
}