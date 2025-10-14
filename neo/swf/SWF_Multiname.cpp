
#include "precompiled.h"
#pragma hdrstop

#include "SWF_Abc.h"
#include "SWF_Multiname.h"

// Default constructor
SWF_Multiname::SWF_Multiname( )
	: name( "" ), ns( nullptr ), flags( 0 ), next_index( 0 ) { }

// Constructor with namespace set
SWF_Multiname::SWF_Multiname( idList<idStrPtr> nsset )
	: name( "" ), nsset( nsset ), flags( NSSET ), next_index( 0 ) { }

// Copy constructor
SWF_Multiname::SWF_Multiname( const SWF_Multiname &other )
	: name( other.name ), flags( other.flags ), next_index( other.next_index ) {
	if ( flags & NSSET ) {
		nsset = other.nsset;
	} else {
		ns = other.ns;
	}
}

// Constructor with namespace and name
SWF_Multiname::SWF_Multiname( idStrPtr ns, idStr name )
	: name( name ), ns( ns ), flags( 0 ), next_index( 0 ) { }

// Constructor with namespace, name, and qualified flag
SWF_Multiname::SWF_Multiname( idStrPtr ns, idStr name, bool qualified )
	: name( name ), ns( ns ), flags( qualified ? QNAME : 0 ), next_index( 0 ) { }

// Destructor
SWF_Multiname::~SWF_Multiname( ) { }

// Get name
idStr SWF_Multiname::getName( ) const {
	return name;
}

// Set name
void SWF_Multiname::setName( idStr _name ) {
	name = _name;
}

// Set name from another SWF_Multiname
void SWF_Multiname::setName( const SWF_Multiname *other ) {
	name = other->name;
}

// Get namespace count
int32 SWF_Multiname::namespaceCount( ) const {
	return ( flags & NSSET ) ? nsset.Num( ) : 1;
}

// Get namespace by index
idStrPtr SWF_Multiname::getNamespace( int32 i ) const {
	return ( flags & NSSET ) ? nsset[i] : ns;
}

// Get namespace
idStrPtr SWF_Multiname::getNamespace( ) const {
	return ns;
}

// Set namespace
void SWF_Multiname::setNamespace( idStrPtr _ns ) {
	ns = _ns;
}

// Set namespace from another SWF_Multiname
void SWF_Multiname::setNamespace( const SWF_Multiname *other ) {
	ns = other->ns;
}

// Get namespace set
idList<idStrPtr> SWF_Multiname::getNsset( ) const {
	return nsset;
}

// Set namespace set
void SWF_Multiname::setNsset( idList<idStrPtr> _nsset ) {
	nsset = _nsset;
	flags |= NSSET;
}

// Get type parameter
uint32 SWF_Multiname::getTypeParameter( ) const {
	return next_index;
}

// Set type parameter
void SWF_Multiname::setTypeParameter( uint32 index ) {
	next_index = index;
	flags |= TYPEPARAM;
}

// Check if contains namespace
bool SWF_Multiname::contains( idStrPtr ns ) const {
	if ( flags & NSSET ) {
		return nsset.FindIndex( ns ) != -1;
	}
	return this->ns == ns;
}

// Check if contains any public namespace
bool SWF_Multiname::containsAnyPublicNamespace( ) const {
	if ( flags & NSSET ) {
		for ( int i = 0; i < nsset.Num( ); i++ ) {
			//if ( nsset[i]->IsPublic( ) ) {
			return true;
			//}
		}
	} else {
		//return ns->IsPublic( );
	}
	return false;
}

// Check if valid dynamic name
bool SWF_Multiname::isValidDynamicName( ) const {
	return !( flags & ( RTNAME | RTNS ) );
}

// Get compile-time flags
int32 SWF_Multiname::ctFlags( ) const {
	return flags;
}

// Check if binding
int32 SWF_Multiname::isBinding( ) const {
	return flags & ( QNAME | RTNAME | RTNS );
}

// Check if runtime
int32 SWF_Multiname::isRuntime( ) const {
	return flags & ( RTNAME | RTNS );
}

// Check if runtime namespace
int32 SWF_Multiname::isRtns( ) const {
	return flags & RTNS;
}

// Check if runtime name
int32 SWF_Multiname::isRtname( ) const {
	return flags & RTNAME;
}

// Check if qualified name
int32 SWF_Multiname::isQName( ) const {
	return flags & QNAME;
}

// Check if attribute
bool SWF_Multiname::isAttr( ) const {
	return flags & ATTR;
}

// Check if any name
bool SWF_Multiname::isAnyName( ) const {
	return name.IsEmpty( );
}

// Check if any namespace
bool SWF_Multiname::isAnyNamespace( ) const {
	return ns == nullptr;
}

// Check if namespace set
int32 SWF_Multiname::isNsset( ) const {
	return flags & NSSET;
}

// Check if parameterized type
int32 SWF_Multiname::isParameterizedType( ) const {
	return flags & TYPEPARAM;
}

// Set attribute flag
void SWF_Multiname::setAttr( bool b ) {
	if ( b ) {
		flags |= ATTR;
	} else {
		flags &= ~ATTR;
	}
}

// Set qualified name flag
void SWF_Multiname::setQName( ) {
	flags |= QNAME;
}

// Set runtime namespace flag
void SWF_Multiname::setRtns( ) {
	flags |= RTNS;
}

// Set runtime name flag
void SWF_Multiname::setRtname( ) {
	flags |= RTNAME;
}

// Set any name flag
void SWF_Multiname::setAnyName( ) {
	name.Clear( );
}

// Set any namespace flag
void SWF_Multiname::setAnyNamespace( ) {
	ns = nullptr;
}

// Check if matches another SWF_Multiname
bool SWF_Multiname::matches( const SWF_Multiname *mn ) const {
	if ( name != mn->name ) {
		return false;
	}
	if ( flags & NSSET ) {
		for ( int i = 0; i < nsset.Num( ); i++ ) {
			if ( mn->contains( nsset[i] ) ) {
				return true;
			}
		}
		return false;
	}
	return mn->contains( ns );
}

// Increment reference count
void SWF_Multiname::IncrementRef( ) {
	// Implementation for reference counting (if needed)
}

// Decrement reference count
void SWF_Multiname::DecrementRef( ) {
	// Implementation for reference counting (if needed)
}
