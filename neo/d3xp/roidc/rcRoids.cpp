

#include "precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

idCVar g_AsteroidDebug( "g_AsteroidDebug", "0", CVAR_GAME | CVAR_BOOL, "" );
idCVar g_AsteroidNoMineChance( "g_AsteroidNoMineChance", "1", CVAR_GAME | CVAR_BOOL, "" );

CLASS_DECLARATION( idEntity, rcAsteroid )
EVENT( EV_Activate, rcAsteroid::Event_Activate )
END_CLASS

asteroidMine_s rcAsteroid::invalidMine = {vec3_zero, vec3_zero, mat3_identity, idMath::FLOAT_EPSILON};

void rcAsteroid::Spawn()
{
	totalMines = spawnArgs.GetInt( "mines", "20" );
	minRadius = spawnArgs.GetFloat( "minRadius", 128.0f );
	maxRadius = spawnArgs.GetFloat( "maxRadius", 512.0f );
	mineChance = spawnArgs.GetFloat( "mineChance", 0.5f );
	if( g_AsteroidNoMineChance.GetBool() || ( ( gameLocal.random.CRandomFloat() + 1.0f ) / 2.0f ) >= mineChance )
	{
		CreateMines();
	}
}

void rcAsteroid::Think()
{
	idStaticEntity::Think();

	if( g_AsteroidDebug.GetBool() )
	{
		if( mines.Num() )
		{
			gameRenderWorld->DrawText( va( "%i", mines.Num() ), GetRenderEntity()->origin + idVec3( 0.0f, 0.0f, GetPhysics()->GetClipModel()->GetBounds().GetRadius() ), 5.0f, colorMagenta, gameLocal.GetLocalPlayer()->viewAngles.ToMat3() );

			gameRenderWorld->DrawText( va( "%d", mines.Num() ),
									   GetRenderEntity()->origin + idVec3( 0.0f, 0.0f, GetPhysics()->GetBounds().GetRadius() * 2 ), 1,
									   colorDkGrey, gameLocal.GetLocalPlayer()->viewAngles.ToMat3() );
		}
		for( auto& mine : mines )
		{
			gameRenderWorld->DebugSphere( colorRed, idSphere( mine.posOnSurface, mine.radius ), 0, true );
			idVec3 dir = mine.alignAxis[0];
			dir.Normalize();

			gameRenderWorld->DebugLine( colorRed, mine.alignPos, mine.alignPos + mine.alignAxis[0] * 20.0f, 2 );
			gameRenderWorld->DebugLine( colorGreen, mine.alignPos, mine.alignPos + mine.alignAxis[1] * 20.0f, 2 );
			gameRenderWorld->DebugLine( colorBlue, mine.alignPos, mine.alignPos + mine.alignAxis[2] * 20.0f, 2 );

			gameRenderWorld->DebugArrow( colorMagenta, mine.alignPos, mine.alignPos + dir * 200.0f, 2 );
		}
	}

	static int nextDecal = gameLocal.time;
	if( gameLocal.time >= nextDecal )
	{
		for( auto& mine : mines )
		{
			ProjectOverlay( mine.posOnSurface, mine.alignAxis[0], mine.radius, spawnArgs.GetString( "mtr_vain", "textures/decals/bloodspray" ) );
			//gameLocal.ProjectDecal(mine.posOnSurface, mine.alignAxis[0], 128.0f, true, mine.radius, spawnArgs.GetString("mtr_vain","textures/decals/bloodspray"), 1.0f);
		}
		nextDecal = gameLocal.time + SEC2MS( 1 );
	}
	BecomeActive( TH_THINK );
}

const asteroidMine_s& rcAsteroid::GetClosestMine( const idVec3& position )
{
	float		bestDistSquared;
	float		distSquared;
	const asteroidMine_s* closest = nullptr;
	bestDistSquared = idMath::INFINITUM;
	for( const asteroidMine_s& mine : mines )
	{
		distSquared = ( mine.alignPos - position ).LengthSqr();
		if( distSquared < bestDistSquared )
		{
			bestDistSquared = distSquared;
			closest = &mine;
		}
	}
	if( closest )
	{
		return *closest;
	}
	else
	{
		return invalidMine;
	}
}

void rcAsteroid ::CreateMines()
{
	mines.Clear();
	int tries = 0;
	bool overlap = false;
	for( int i = 0 ; i < totalMines ; i++ )
	{
		idVec3 rndAng;
		rndAng[0] = gameLocal.random.CRandomFloat();
		rndAng[1] = gameLocal.random.CRandomFloat();
		rndAng[2] = gameLocal.random.CRandomFloat();
		rndAng.Normalize();
		idVec3 end = GetRenderEntity()->origin;
		idVec3 start = end + rndAng * GetPhysics()->GetBounds().GetRadius();

		float radiusDelta = ( maxRadius - minRadius ) - minRadius;
		float newRadius = ( minRadius + ( ( ( gameLocal.random.CRandomFloat() + 1.0f ) / 2.0f ) * radiusDelta ) * 2.0f ) ;
		trace_t tr;
		if( gameLocal.clip.TracePoint( tr, start, end, MASK_SHOT_RENDERMODEL, NULL ) )
		{
			overlap = false;
			for( int j = 0; j < mines.Num(); j++ )
			{
				if( ( tr.c.point - mines[j].posOnSurface ).Length() < ( ( mines[j].radius / 2.0f + newRadius ) * 2.0f ) )
				{
					overlap = true;
					--i;
					break;
				}
			}
			//make sure no mines overlap on the asteroid's surface.
			//if overlap occurs, we just try again X amount of times before giving up creating more mines.
			if( overlap )
			{
				if( ++tries > 100 )
				{
					i = totalMines;
				}
				continue;
			}
			auto& newMine = mines.Alloc();
			newMine.posOnSurface = tr.c.point;
			newMine.alignPos = start;
			newMine.radius = newRadius;


			idVec3 vec = tr.c.point - start;
			vec.Normalize();
			idVec3 left, up;
			vec.OrthogonalBasis( left, up );
			idMat3 temp( vec.x, vec.y, vec.z, up.x, up.y, up.z, left.x, left.y, left.z );

			newMine.alignAxis = temp;

			//float angle = (float)RAD2DEG(vec.Normalize());
			//idRotation rotation = idRotation(start, vec, angle);
			//rotation.Normalize180();
			//newMine.alignAxis = rotation.ToMat3();


			tries = 0;
		}
		if( overlap )
		{
			tries = 0;
		}
	}




	if( totalMines > mines.Num( ) )
	{
		common->Warning( "%s could not be populated with the requested amount of mines! %i/%i", name.c_str(), mines.Num(), totalMines );
	}
}

void rcAsteroid::Event_Activate( idEntity* activator )
{
	UpdateVisuals();
	BecomeActive( TH_THINK );
}