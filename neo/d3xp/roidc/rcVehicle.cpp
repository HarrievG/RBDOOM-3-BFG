
#include "precompiled.h"
#pragma hdrstop

#include "../Game_local.h"


CLASS_DECLARATION( idInteractable , rcPilotable )
EVENT( EV_Activate, rcPilotable::Event_Activate )
END_CLASS

void rcPilotable::Spawn()
{
	idAnimatedEntity::Spawn();
	idVec3		size;
	idBounds	bounds;
	idVec3		pos;
	idMat3		axis;
	/*fl.takedamage = true;
	health = spawnArgs.GetInt("health", "100");*/

	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new( TAG_PHYSICS_CLIP_ENTITY ) idClipModel( GetPhysics()->GetClipModel() ), 1.0f );

	if( spawnArgs.GetVector( "mins", NULL, bounds[0] ) )
	{
		spawnArgs.GetVector( "maxs", NULL, bounds[1] );
		physicsObj.SetClipBox( bounds, 1.0f );
		physicsObj.SetContents( 0 );
	}
	else if( spawnArgs.GetVector( "size", NULL, size ) )
	{
		bounds[0].Set( size.x * -0.5f, size.y * -0.5f, 0.0f );
		bounds[1].Set( size.x * 0.5f, size.y * 0.5f, size.z );
	}
	else
	{
		animator.GetJointTransform( ( jointHandle_t )animator.GetJointHandle( "body" ), FRAME2MS( 0 ), pos, axis );
		animator.GetBounds( FRAME2MS( 0 ), bounds );
	}

	physicsObj.SetClipBox( bounds, 1.0f );
	physicsObj.SetContents( CONTENTS_SOLID );


	GetJointTransformForAnim( animator.GetJointHandle( "body" ), animator.GetAnim( "af_pose" ), FRAME2MS( 0 ), pos, axis );

	physicsObj.SetOrigin( renderEntity.origin + pos );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );


	physicsObj.SetBouncyness( 0.01f );
	physicsObj.SetFriction( 0.6f, 0.6f, 0.2f );
	physicsObj.SetGravity( gameLocal.GetGravity() );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP );
	SetPhysics( &physicsObj );

	physicsObj.SetMass( 20.0f );

	PostEventSec( &EV_Activate, 0, this );
	UpdateVisuals();


}

void rcPilotable::Interact( idPlayer* player )
{

	if( currentInteractor )
	{
		if( currentInteractor == player )
		{
			player->Unbind();
			currentInteractor = NULL;
		}
	}
	else
	{
		currentInteractor = player;
		idVec3 origin = renderEntity.origin;
		origin[2] += GetPhysics()->GetClipModel()->GetAbsBounds().GetMaxExtent();

		currentInteractor->GetPhysics()->SetOrigin( origin );
		currentInteractor->BindToBody( this, 0, true );
	}
	common->Warning( " ^3 %s ^7 is interacted with \n", GetName() );
}

bool rcPilotable::CanBeInteractedWith()
{
	return true;
}

void rcPilotable::Think()
{

	RunPhysics();
	UpdateAnimation();
	if( thinkFlags & TH_UPDATEVISUALS )
	{
		Present();
	}

	BecomeActive( TH_PHYSICS );

	//UpdateAnimation();
	//if (thinkFlags & TH_UPDATEVISUALS)
	//{
	//	Present();
	//	//LinkCombat();
	//}
	//idAnimatedEntity::Think();
	//idEntity::Think();
}

void rcPilotable::Event_Activate( idEntity* activator )
{
	Show();
	physicsObj.Activate();
	physicsObj.EnableImpact();

}
