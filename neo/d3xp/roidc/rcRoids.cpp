

#include "precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

CLASS_DECLARATION( idEntity, rcAsteroid )
EVENT( EV_Activate, rcAsteroid::Event_Activate )
END_CLASS


rcAsteroid::rcAsteroid()
{

}

rcAsteroid::~rcAsteroid()
{

}

void rcAsteroid::Spawn()
{
}

void rcAsteroid::Think()
{
	idStaticEntity::Think();
}

void rcAsteroid::Event_Activate( idEntity* activator )
{
	//idStaticEntity::Event_Activate();
	UpdateVisuals();
}