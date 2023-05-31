
#include "precompiled.h"
#pragma hdrstop

#include "../../../Game_local.h"
#include "d3xp/gamesys/Class.h"

idCVar g_DroneMinerDebug( "g_DroneMinerDebug", "1", CVAR_GAME | CVAR_BOOL, "" );

#define DRONE_FIRE_RATE		0.1
#define DRONE_MIN_ATTACK_TIME	1.5
#define DRONE_SHUTDOWN_TIME	4

CLASS_DECLARATION( idAI, rcDrone_Miner )
END_CLASS


void rcDrone_Miner::Spawn()
{

}

float rcDrone_Miner::GetMaxRange()
{
	// "The linear velocity of a physics object is a vector that defines the
	// translation of the center of mass in units per second."
	//this should become an calculation which takes "thruster burn time" into account
	return 5000.0f;
}

void rcDrone_Miner::Init()
{
	idBounds	bounds;
	idVec3		pos;
	idMat3		axis;
	animator.GetJointTransform( ( jointHandle_t )animator.GetJointHandle( "body" ), FRAME2MS( 0 ), pos, axis );
	animator.GetBounds( FRAME2MS( 0 ), bounds );
	idTraceModel tm;
	tm.SetupDodecahedron( bounds.GetMaxExtent() * 1.1f );
	clipModel = new( TAG_PHYSICS_CLIP_ENTITY ) idClipModel( tm );
	GetPhysics()->SetClipModel( clipModel, 1.0f );
	targetMine = nullptr;
	float team = 0.0f;

	// don't take damage
	Event_IgnoreDamage();

	// can't move, so only fire on sight
	ambush = true;

	if( GetIntKey( "light" ) )
	{
		spawn_light();
	}

}

stateResult_t rcDrone_Miner::state_WakeUp( stateParms_t* parms )
{
	//idAI::state_WakeUp(parms);
	GetPhysics()->SetGravity( vec3_zero );
	disableGravity = true;
	if( cinematic )
	{
		Event_SetState( "state_Cine" );
		Event_SetMoveType( MOVETYPE_ANIM );
	}
	else
	{
		Event_SetMoveType( MOVETYPE_FLY );

		Event_SetState( "state_WaitForCommands" );
	}

	return SRESULT_DONE_FRAME;
}

stateResult_t rcDrone_Miner::state_WaitForCommands( stateParms_t* parms )
{
	Event_SetState( "state_Mining" );
	return SRESULT_DONE_FRAME;
	//return SRESULT_WAIT;
}


stateResult_t rcDrone_Miner::state_Mining( stateParms_t* parms )
{
	idVec3 origin = GetPhysics()->GetOrigin();
	idMat3 axis = GetPhysics()->GetAxis();
	idVec3 dir = ( renderEntity.axis *  axis )[0];
	dir.Normalize();

	if( g_DroneMinerDebug.GetBool() )
	{
		//idClass::Get
		gameRenderWorld->DrawText( va( "%s", idTypeInfo::GetEnumTypeInfo( "state_Mining_t", parms->stage ) ), origin + idVec3( 0.0f, 0.0f, clipModel->GetBounds().GetMaxExtent() / 4.0f ), 0.5f, colorYellow, gameLocal.GetLocalPlayer()->viewAngles.ToMat3() );
		if( targetAsteroid.IsValid() )
		{
			//gameRenderWorld->DebugArrow(colorMdGrey, origin, GetPhysics()->GetAxis()[0], 10, 2);
			gameRenderWorld->DrawText( va( "%d", ( int )( targetAsteroid->GetPhysics()->GetOrigin() - origin ).Length() ), origin, 1, colorDkGrey, gameLocal.GetLocalPlayer()->viewAngles.ToMat3() );
		}

		gameRenderWorld->DebugArrow( colorDkGrey, origin, origin + dir * 200, 10, 2 );

	}
	switch( parms->stage )
	{
		case STAGE_FIND_ASTEROID:
		{
			idTraceModel tm;
			tm.SetupBox( GetMaxRange() );
			idClipModel* clipModels[MAX_GENTITIES];
			const asteroidMine_s* tmpMine = nullptr;
			const rcAsteroid* tmpRoid = nullptr;
			int numClipModels = gameLocal.clip.ClipModelsTouchingBounds( tm.bounds, CONTENTS_SOLID, clipModels, MAX_GENTITIES );
			float closestMineSqrd = idMath::INFINITUM;
			float newDist = 0.0f;
			for( int i = 0 ; i < numClipModels; i++ )
			{
				idEntity* clippedEntity = clipModels[i]->GetEntity();
				if( clippedEntity->IsType( rcAsteroid::Type ) )
				{
					//is this sorted by distance?
					targetAsteroid = static_cast<const rcAsteroid*>( clippedEntity );
					targetMine = &targetAsteroid->GetClosestMine( origin );
					newDist = ( targetMine->alignPos - origin ).LengthSqr();
					if( newDist < closestMineSqrd )
					{
						closestMineSqrd = newDist;
						tmpMine = targetMine;
						tmpRoid = targetAsteroid;
					}
				}
			}
			targetAsteroid = tmpRoid;
			targetMine = tmpMine;

			parms->stage = STAGE_MOVE_TO_ASTEROID;
			MoveToPositionOriented( targetMine->alignPos, targetMine->alignAxis );
			AI_RUN = true;

			return SRESULT_WAIT;
		}
		break;
		case STAGE_MOVE_TO_ASTEROID:
		{
			float distToAlign = ( targetMine->alignPos - origin ).Length();
			if( distToAlign < 200 )
			{
				fly_speed = 100;
				MoveToPositionOriented( targetMine->alignPos + targetMine->alignAxis[0] * 100, targetMine->alignAxis );
				parms->stage = STAGE_ALIGN_TO_MINE_ON_ASTEROID;
			}

			return SRESULT_WAIT;
		}
		break;
		case STAGE_ALIGN_TO_MINE_ON_ASTEROID:
		{
			if( move.moveStatus == MOVE_STATUS_DONE && TurnToward( targetMine->alignAxis.ToAngles() ) )
			{
				parms->stage = STAGE_MINE;
			}
			return SRESULT_WAIT;
		}
		break;
		case STAGE_MINE:

			if( ( targetMine->alignPos - origin ).Length() > 250.0f )
			{
				MoveToPositionOriented( targetMine->alignPos + targetMine->alignAxis[0] * 100, targetMine->alignAxis );
				parms->stage = STAGE_MOVE_TO_ASTEROID;
			}
			else if( g_DroneMinerDebug.GetBool() )
			{
				gameRenderWorld->DebugArrow( colorPink, origin, origin + targetMine->alignAxis[0] * 100, 10, 2 );
				gameRenderWorld->DebugArrow( colorOrange, origin, origin + GetPhysics()->GetLinearVelocity(), 10, 2 );
			}



			return SRESULT_WAIT;
			break;
		case STAGE_DEPOSIT:
			break;
		case STAGE_RETURN_TO_ASTEROID:
			break;
		default:
			break;
	}

	return SRESULT_ERROR;
}

stateResult_t rcDrone_Miner::state_Cine( stateParms_t* parms )
{
	return SRESULT_DONE_FRAME;
}

stateResult_t rcDrone_Miner::state_Begin( stateParms_t* parms )
{
	Event_SetMoveType( MOVETYPE_FLY );
	GetPhysics()->SetGravity( vec3_zero );

	if( GetIntKey( "trigger" ) )
	{
		Event_SetState( "state_Disabled" );
		return SRESULT_DONE;
	}
	else
	{
		Event_SetState( "state_Disabled" );
		return SRESULT_DONE;
	}
}

void rcDrone_Miner::AI_Begin()
{
	Event_SetState( "state_Begin" );
}

stateResult_t	rcDrone_Miner::combat_attack( stateParms_t* parms )
{
	//should never hit, satisfying compiler.
	assert( 0 );
	return SRESULT_DONE;
}

bool rcDrone_Miner::canHit()
{
	return false;// CanHitEnemyFromJoint(currentBarrelStr);
}

stateResult_t rcDrone_Miner::Torso_Death( stateParms_t* parms )
{
	Event_FinishAction( "dead" );
	return SRESULT_WAIT;
}

stateResult_t rcDrone_Miner::Torso_Idle( stateParms_t* parms )
{
	Event_PlayCycle( ANIMCHANNEL_TORSO, "idle" );
	return SRESULT_WAIT;
}

stateResult_t rcDrone_Miner::Torso_Attack( stateParms_t* parms )
{
	Event_FinishAction( "attack" );
	Event_AnimState( ANIMCHANNEL_TORSO, "Torso_Idle", 1 );
	return SRESULT_DONE;
}

stateResult_t rcDrone_Miner::Torso_CustomCycle( stateParms_t* parms )
{
	Event_AnimState( ANIMCHANNEL_TORSO, "Torso_Idle", 1 );
	return SRESULT_DONE;
}

bool rcDrone_Miner::checkForEnemy( float use_fov )
{
	return false;
}

stateResult_t rcDrone_Miner::state_Combat( stateParms_t* parms )
{
	return SRESULT_DONE;
}

stateResult_t rcDrone_Miner::state_Disabled( stateParms_t* parms )
{
	return SRESULT_DONE;
}

stateResult_t rcDrone_Miner::state_Idle( stateParms_t* parms )
{
	return SRESULT_DONE;
}

stateResult_t rcDrone_Miner::state_Killed( stateParms_t* parms )
{
	Event_StopMove();

	light_off();

	Event_AnimState( ANIMCHANNEL_TORSO, "Torso_Death", 1 );

	Event_WaitAction( "dead" );

	Event_StopThinking();

	return SRESULT_DONE;
}