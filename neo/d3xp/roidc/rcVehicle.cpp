
#include "precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
idCVar g_PilotableDebug(				"g_PilotableDebug",		"0",			CVAR_GAME | CVAR_BOOL, "" );

CLASS_DECLARATION( idInteractable , rcPilotable )
EVENT( EV_Activate, rcPilotable::Event_Activate )
END_CLASS

void rcPilotable::Spawn()
{
	idVec3		size;
	idBounds	bounds;
	
	idVec3		pos;
	idMat3		axis;

	clipModel = nullptr;

	/*fl.takedamage = true;
	health = spawnArgs.GetInt("health", "100");*/

	spawnArgs.GetFloat("maxSpeed", 100.0f, maxSpeed);
	spawnArgs.GetFloat("steerSpeed", "5", steerSpeed);

	physicsObj.SetSelf( this );

	if( spawnArgs.GetVector( "mins", NULL, bounds[0] ) )
	{
		spawnArgs.GetVector( "maxs", NULL, bounds[1] );
		physicsObj.SetClipBox( bounds, 1.0f );
		physicsObj.SetContents( 0 );
		clipModel = new idClipModel( bounds );
	}
	else if( spawnArgs.GetVector( "size", NULL, size ) )
	{
		bounds[0].Set( size.x * -0.5f, size.y * -0.5f, 0.0f );
		bounds[1].Set( size.x * 0.5f, size.y * 0.5f, size.z );
		clipModel = new(TAG_PHYSICS_CLIP_ENTITY)  idClipModel(bounds);
	}
	else
	{
		animator.GetJointTransform( ( jointHandle_t )animator.GetJointHandle( "body" ), FRAME2MS( 0 ), pos, axis );
		animator.GetBounds( FRAME2MS( 0 ), bounds );
		idTraceModel tm;
		tm.SetupDodecahedron(bounds.GetMaxExtent() * 1.1f);
		clipModel = new(TAG_PHYSICS_CLIP_ENTITY) idClipModel(tm);
		
	}
	
	physicsObj.SetClipModel(clipModel, 1 );
	physicsObj.SetContents( CONTENTS_SOLID );

	GetJointTransformForAnim( animator.GetJointHandle( "body" ), animator.GetAnim( "af_pose" ), FRAME2MS( 0 ), pos, axis );

	physicsObj.SetOrigin( renderEntity.origin + pos );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );


	physicsObj.SetBouncyness( 0.5 );
	physicsObj.SetFriction( 0.6f, 0.6f, 0.2f );
	physicsObj.SetGravity( gameLocal.GetGravity() );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP );
	SetPhysics( &physicsObj );

	physicsObj.SetMass( 1000.0f );

	BecomeActive(TH_THINK);
	BecomeActive(TH_PHYSICS);
	fl.forcePhysicsUpdate = true;

	PostEventSec( &EV_Activate, 0, this );

}

void rcPilotable::Interact( idPlayer* player )
{

	if( currentInteractor )
	{
		if( currentInteractor == player )
		{
			player->Unbind();
			player->GetPhysics()->GetClipModel()->Enable();
			currentInteractor = NULL;
			physicsObj.SetGravity( gameLocal.GetGravity() );
			
		}
	}
	else
	{
		currentInteractor = player;
		idVec3 origin = renderEntity.origin;

		idVec3		pos;
		idMat3		axis;

		GetJointTransformForAnim(animator.GetJointHandle("eyes"), animator.GetAnim("af_pose"), FRAME2MS(0), pos, axis);
		origin = origin + pos * renderEntity.axis; 
		idVec3 dir = renderEntity.axis[0];
		idAngles ang(0, dir.ToYaw(), 0);
		currentInteractor->SetViewAngles(ang);
		currentInteractor->SetAngles(ang);
		currentInteractor->GetPhysics()->SetOrigin( origin + (-dir * 100));
		currentInteractor->BindToJoint(this, animator.GetJointHandle("body"), true );
		currentInteractor->GetPhysics()->GetClipModel()->Disable();
		physicsObj.SetGravity(vec3_zero);
	}
	common->Warning( " ^3 %s ^7 is interacted with \n", GetName() );
}

bool rcPilotable::CanBeInteractedWith()
{
	return true;
}

void rcPilotable::Think()
{
	if (currentInteractor)
	{
		currentInteractor->Unbind();
		const idVec3& origin = physicsObj.GetOrigin();
		const idMat3& axis = physicsObj.GetAxis();
		idVec3 dir = axis[0];
		dir.Normalize();

		float force = 0.0f;

		idVec3 pilotDir = currentInteractor->firstPersonViewAxis[0];

		force = idMath::Fabs(currentInteractor->usercmd.forwardmove * 1000) * (1.0f / 128.0f);

		physicsObj.SetAngularVelocity(currentInteractor->GetDeltaViewAngles().ToAngularVelocity());
		physicsObj.SetAxis(currentInteractor->firstPersonViewAxis);

		if (force != 0.0f)
		{
			if (currentInteractor->usercmd.forwardmove < 0)
			{
				force = -force;
			}
			physicsObj.SetLinearVelocity((pilotDir)*force);
		}

		idVec3 jpos;
		idMat3 jaxis;

		GetJointWorldTransform(animator.GetJointHandle("eyes"), FRAME2MS(0), jpos, jaxis);
		currentInteractor->GetPhysics()->SetOrigin(jpos + (-dir * 1000));
		currentInteractor->BindToJoint(this, animator.GetJointHandle("eyes"), false);
	}

	RunPhysics();
	UpdateAnimation();

	if ( thinkFlags & TH_UPDATEVISUALS )
	{
		deltaTime = gameLocal.time;
		Present();
	}

	if (g_PilotableDebug.GetBool())
	{
		const idVec3& origin = physicsObj.GetOrigin();
		const idMat3& axis = physicsObj.GetAxis();

		idVec3 dir = axis[0];
		dir.Normalize();
		dir *= 100;
		gameRenderWorld->DebugLine(colorYellow, origin ,origin + dir, 5000);

		if ( currentInteractor )
		{
			idVec3 pilotOrigin = vec3_zero;
			idMat3 pilotAxis = mat3_identity;
			currentInteractor->GetViewPos(pilotOrigin, pilotAxis);
			idVec3 pilotDir = pilotAxis[0];
			pilotDir.Normalize();
			gameRenderWorld->DebugArrow(colorBlue, origin, origin + pilotDir * 200.0f, 2);
		}
	}

	BecomeActive(TH_PHYSICS);
}

void rcPilotable::Event_Activate( idEntity* activator )
{
	physicsObj.Activate();
	physicsObj.EnableImpact();

	BecomeActive(TH_THINK);
}
