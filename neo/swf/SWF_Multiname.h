
#pragma once

#include "SWF_Enums.h"

class SWF_Multiname {
	const static int32 ATTR = 0x01; // attribute name
	const static int32 QNAME = 0x02; // qualified name (size==1, explicit in code)
	const static int32 RTNS = 0x04; // runtime namespace
	const static int32 RTNAME = 0x08; // runtime name
	const static int32 NSSET = 0x10;
	const static int32 TYPEPARAM = 0x20;

	idStr name;
	idStrPtr ns;
	idList<idStrPtr> nsset;

	int32 flags;
	uint32 next_index;

public:

	// Move assignment operator
	SWF_Multiname &operator=( SWF_Multiname &&other ) noexcept {
		if ( this != &other ) {
			// Move the name
			name = std::move( other.name );

			// Move the union members
			if ( flags & NSSET ) {
				nsset = std::move( other.nsset );
			} else {
				ns = std::move( other.ns );
			}

			// Move the flags and next_index
			flags = other.flags;
			next_index = other.next_index;

			// Reset the other object
			other.flags = 0;
			other.next_index = 0;
		}
		return *this;
	}

	// Deleted copy assignment operator to prevent copying
	SWF_Multiname &operator=( const SWF_Multiname & ) = delete;

	SWF_Multiname( );
	SWF_Multiname( idList<idStrPtr> nsset );
	SWF_Multiname( const SWF_Multiname &other );
	SWF_Multiname( idStrPtr ns, idStr name );
	SWF_Multiname( idStrPtr ns, idStr name, bool qualified );
	~SWF_Multiname( );

	idStr getName( ) const;
	void setName( idStr _name );
	void setName( const SWF_Multiname *other );
	int32 namespaceCount( ) const;
	idStrPtr getNamespace( int32 i ) const;
	idStrPtr getNamespace( ) const;
	void setNamespace( idStrPtr _ns );
	void setNamespace( const SWF_Multiname *other );
	idList<idStrPtr> getNsset( ) const;
	void setNsset( idList<idStrPtr> _nsset );
	uint32 getTypeParameter( ) const;
	void setTypeParameter( uint32 index );

	bool contains( idStrPtr ns ) const;
	bool containsAnyPublicNamespace( ) const;
	bool isValidDynamicName( ) const;

	int32 ctFlags( ) const;
	int32 isBinding( ) const;

	int32 isRuntime( ) const;
	int32 isRtns( ) const;
	int32 isRtname( ) const;
	int32 isQName( ) const;
	bool isAttr( ) const;
	bool isAnyName( ) const;
	bool isAnyNamespace( ) const;
	int32 isNsset( ) const;
	int32 isParameterizedType( ) const;

	void setAttr( bool b = true );
	void setQName( );
	void setRtns( );
	void setRtname( );
	void setAnyName( );
	void setAnyNamespace( );
	bool matches( const SWF_Multiname *mn ) const;

	void IncrementRef( );
	void DecrementRef( );
};
