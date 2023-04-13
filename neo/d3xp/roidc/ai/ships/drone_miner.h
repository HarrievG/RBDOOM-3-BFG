
#pragma once

enum state_Mining_t
{
	STAGE_FIND_ASTEROID = 0,
	STAGE_MOVE_TO_ASTEROID,
	STAGE_MINE_SILICON,
	STAGE_DEPOSIT_SILICON,
	STAGE_RETURN_TO_ASTEROID,
};

class rcAsteroid;
class rcDrone_Miner : public idAI
{
	CLASS_PROTOTYPE( rcDrone_Miner );
public:

	virtual void				Init() override;
	virtual void				AI_Begin() override;

	void						Spawn();

	float						GetMaxRange();
protected:
	void						destory() { };

	// actions
	bool						canHit();
	void						spawn_light() { };
	void						light_off() { };
	void						light_on() { };

	// anim states
	stateResult_t				Torso_Death( stateParms_t* parms );
	stateResult_t				Torso_Idle( stateParms_t* parms );
	stateResult_t				Torso_Attack( stateParms_t* parms );
	stateResult_t				Torso_CustomCycle( stateParms_t* parms );
	stateResult_t				state_Killed( stateParms_t* parms );

	virtual bool				checkForEnemy( float use_fov ) override;
private:
	//
	// States
	//
	stateResult_t				state_WakeUp( stateParms_t* parms );
	stateResult_t				state_WaitForCommands( stateParms_t* parms );
	stateResult_t				state_Mining( stateParms_t* parms );

	stateResult_t				state_Begin( stateParms_t* parms );
	stateResult_t				state_Idle( stateParms_t* parms );
	stateResult_t				state_Combat( stateParms_t* parms );
	stateResult_t				combat_attack( stateParms_t* parms );
	stateResult_t				state_Disabled( stateParms_t* parms );

	idEntity	light;
	boolean		light_is_on;

	idEntityPtr<rcAsteroid> targetAsteroid;
	idList< jointHandle_t> flashJointWorldHandles;
	//////////////////////////////////////////////////////////////////////////
	idClipModel* clipModel;
};