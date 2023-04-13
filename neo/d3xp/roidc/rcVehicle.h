
#ifndef __GAME_RCVEHICLE_H__
#define __GAME_RCVEHICLE_H__

class rcPilotable : public idInteractable
{
public:
	CLASS_PROTOTYPE( rcPilotable );
	void			Spawn();
	virtual void	Interact( idPlayer* player );
	virtual bool	CanBeInteractedWith();

	virtual void	Think();

	virtual void	OnEnter() {}
	virtual void	OnExit() {}
protected:
	void			Event_Activate( idEntity* activator );

	idPhysics_RigidBody		physicsObj;				// physics object
	idEntityPtr<idActor>	pilot;
	idMat3					initialAxis;

	idClipModel*			clipModel;
	float					steerAngle;
	float					steerSpeed;

	float					maxSpeed;
	float					deltaTime;
};

class rcShipPodLarge : public rcPilotable
{
public:
	CLASS_PROTOTYPE( rcShipPodLarge );
	rcShipPodLarge();
	void			Spawn();
	idFuncEmitter*	AddThrusterFx( idStr fx, idStr bone );
	idEntity*		GetEmitter( const char* name );

	virtual void	Think();

	void OnEnter() override;


	void OnExit() override;

private:
	idHashTable<funcEmitter_t> funcEmitters;
	idFuncEmitter* prtBackCenter;
	idFuncEmitter* prtBackTop;
	idFuncEmitter* prtBackBottom;
};

#endif