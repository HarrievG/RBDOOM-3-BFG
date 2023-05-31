
#include "precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

idCVar g_PilotableDebug(	"g_PilotableDebug",		"0",			CVAR_GAME | CVAR_BOOL, "" );
idCVar g_ShipPodLargeDebug(	"g_ShipPodLargeDebug",	"0",			CVAR_GAME | CVAR_BOOL, "" );

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

	spawnArgs.GetFloat( "maxSpeed", 100.0f, maxSpeed );
	spawnArgs.GetFloat( "steerSpeed", "5", steerSpeed );

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
		clipModel = new( TAG_PHYSICS_CLIP_ENTITY )  idClipModel( bounds );
	}
	else
	{
		animator.GetJointTransform( ( jointHandle_t )animator.GetJointHandle( "body" ), FRAME2MS( 0 ), pos, axis );
		animator.GetBounds( FRAME2MS( 0 ), bounds );
		idTraceModel tm;
		tm.SetupDodecahedron( bounds.GetMaxExtent() * 1.1f );
		clipModel = new( TAG_PHYSICS_CLIP_ENTITY ) idClipModel( tm );

	}

	physicsObj.SetClipModel( clipModel, 1 );
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

	BecomeActive( TH_THINK );
	BecomeActive( TH_PHYSICS );
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
			OnExit();

		}
	}
	else
	{
		currentInteractor = player;
		idVec3 origin = renderEntity.origin;

		idVec3		pos;
		idMat3		axis;

		GetJointTransformForAnim( animator.GetJointHandle( "eyes" ), animator.GetAnim( "af_pose" ), FRAME2MS( 0 ), pos, axis );
		origin = origin + pos * renderEntity.axis;
		idVec3 dir = renderEntity.axis[0];
		idAngles ang( 0, dir.ToYaw(), 0 );
		currentInteractor->SetViewAngles( ang );
		currentInteractor->SetAngles( ang );
		currentInteractor->GetPhysics()->SetOrigin( origin + ( -dir * 100 ) );
		currentInteractor->BindToJoint( this, animator.GetJointHandle( "body" ), true );
		currentInteractor->GetPhysics()->GetClipModel()->Disable();
		physicsObj.SetGravity( vec3_zero );

		OnEnter();
	}
	common->Warning( " ^3 %s ^7 is interacted with \n", GetName() );
}

bool rcPilotable::CanBeInteractedWith()
{
	return true;
}

void rcPilotable::Think()
{
	if( currentInteractor )
	{
		currentInteractor->Unbind();
		const idVec3& origin = physicsObj.GetOrigin();
		const idMat3& axis = physicsObj.GetAxis();
		idVec3 dir = axis[0];
		dir.Normalize();

		float force = 0.0f;

		idVec3 pilotDir = currentInteractor->firstPersonViewAxis[0];

		force = idMath::Fabs( currentInteractor->usercmd.forwardmove * 50000 ) * ( 1.0f / 128.0f );


		physicsObj.SetAxis( currentInteractor->firstPersonViewAxis );

		if( currentInteractor->usercmd.forwardmove > 0 ) //force != 0.0f)
		{
			if( currentInteractor->usercmd.forwardmove < 0 )
			{
				force = -force;
			}

			trace_t	trace;
			if( !gameLocal.clip.TraceBounds( trace, origin, origin , clipModel->GetAbsBounds(), MASK_SOLID, this ) )
			{
				physicsObj.ApplyImpulse( 0, origin, ( pilotDir * force ) );
			}
			else
			{
				physicsObj.ApplyImpulse( 0, origin, ( trace.c.normal * force ) );
			}
		}

		idVec3 jpos;
		idMat3 jaxis;

		GetJointWorldTransform( animator.GetJointHandle( "eyes" ), FRAME2MS( 0 ), jpos, jaxis );
		currentInteractor->GetPhysics()->SetOrigin( jpos + ( -dir * 1000 ) );
		currentInteractor->BindToJoint( this, animator.GetJointHandle( "eyes" ), false );
	}

	RunPhysics();
	UpdateAnimation();

	if( thinkFlags & TH_UPDATEVISUALS )
	{
		deltaTime = gameLocal.time;
		Present();
	}

	if( g_PilotableDebug.GetBool() )
	{
		const idVec3& origin = physicsObj.GetOrigin();
		const idMat3& axis = physicsObj.GetAxis();

		idVec3 dir = axis[0];
		dir.Normalize();
		dir *= 100;
		gameRenderWorld->DebugLine( colorYellow, origin , origin + dir, 5000 );

		if( currentInteractor )
		{
			idVec3 pilotOrigin = vec3_zero;
			idMat3 pilotAxis = mat3_identity;
			currentInteractor->GetViewPos( pilotOrigin, pilotAxis );
			idVec3 pilotDir = pilotAxis[0];
			pilotDir.Normalize();
			gameRenderWorld->DebugArrow( colorBlue, origin, origin + pilotDir * 200.0f, 2 );
		}
	}

	BecomeActive( TH_PHYSICS );
}

void rcPilotable::Event_Activate( idEntity* activator )
{
	physicsObj.Activate();
	physicsObj.EnableImpact();

	BecomeActive( TH_THINK );
}

CLASS_DECLARATION( rcPilotable, rcShipPodLarge )
END_CLASS


rcShipPodLarge::rcShipPodLarge()
{

}

void rcShipPodLarge::Spawn()
{
	prtBackCenter = AddThrusterFx( "thruster_red_big.prt", "Thruster_Back_Center" );
	prtBackTop = AddThrusterFx( "thrusterBlue.prt", "Thruster_Back_Top" );
	prtBackBottom = AddThrusterFx( "thrusterBlue.prt", "Thruster_Back_Bottom" );
}

idFuncEmitter* rcShipPodLarge::AddThrusterFx( idStr fx, idStr bone )
{
	idEntity* existing = GetEmitter( bone + fx );
	if( existing )
	{
		return ( idFuncEmitter* )existing;
	}

	jointHandle_t thrusterJoint = animator.GetJointHandle( bone );
	if( thrusterJoint != INVALID_JOINT )
	{
		idVec3 thrusterOrigin;
		idMat3 thrusterAxis;

		animator.GetJointTransform( thrusterJoint, FRAME2MS( 0 ), thrusterOrigin, thrusterAxis );
		thrusterOrigin = renderEntity.origin + thrusterOrigin * renderEntity.axis;

		idFuncEmitter* thrusterEnt;
		idDict thrusterArgs;

		thrusterArgs.Set( "model", fx );
		thrusterArgs.Set( "origin", thrusterOrigin.ToString() );
		thrusterArgs.SetBool( "start_off", true );

		thrusterEnt = static_cast<idFuncEmitter*>( gameLocal.SpawnEntityType( idFuncEmitter::Type, &thrusterArgs ) );

		thrusterEnt->SetOrigin( thrusterOrigin );

		//particles are authored aiming up. + 7 degrees when thirdperson
		thrusterAxis *= idAngles( 0, 0, -97.0f ).ToMat3();
		thrusterEnt->SetAxis( thrusterAxis );

		funcEmitter_t newEmitter;
		strcpy( newEmitter.name, bone + fx );
		newEmitter.particle = ( idFuncEmitter* )thrusterEnt;
		newEmitter.joint = thrusterJoint;
		funcEmitters.Set( newEmitter.name, newEmitter );

		newEmitter.particle->BindToJoint( this, thrusterJoint, true );
		newEmitter.particle->BecomeActive( TH_THINK );
		newEmitter.particle->Hide();
		newEmitter.particle->PostEventMS( &EV_Activate, 0, this );
		newEmitter.particle->SetAxis( thrusterAxis );

		return thrusterEnt;
	}

}

idEntity* rcShipPodLarge::GetEmitter( const char* name )
{
	funcEmitter_t* emitter;
	funcEmitters.Get( name, &emitter );
	if( emitter )
	{
		return emitter->particle;
	}
	return NULL;
}

void rcShipPodLarge::Think()
{
	rcPilotable::Think();

	//use forward vector to fire back up and down
	//input for big center
	//input ? player yaw for left top and right top
	//stationary ? player yaw for left right thrusters

	if( currentInteractor && currentInteractor->usercmd.forwardmove > 0 )
	{
		prtBackCenter->Show();
		prtBackCenter->GetRenderEntity()->shaderParms[SHADERPARM_PARTICLE_STOPTIME] = 0;
	}
	else if( prtBackCenter->GetRenderEntity()->shaderParms[SHADERPARM_PARTICLE_STOPTIME] == 0 )
	{
		prtBackCenter->GetRenderEntity()->shaderParms[SHADERPARM_PARTICLE_STOPTIME] = MS2SEC( gameLocal.time + 1 );
	}

	if( g_ShipPodLargeDebug.GetBool() )
	{
		for( int i = 0; i < funcEmitters.Num(); i++ )
		{
			funcEmitter_t* emitter = funcEmitters.GetIndex( i );
			idVec3 origin = emitter->particle->GetPhysics()->GetOrigin();
			gameRenderWorld->DebugArrow( colorGreen, origin, origin + emitter->particle->GetPhysics()->GetAxis().ToAngles().ToForward() * 200.0f, 2 );
		}
	}
}

void rcShipPodLarge::OnEnter()
{
}

void rcShipPodLarge::OnExit()
{
}
