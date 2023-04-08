
#ifndef __GAME_RC_VEHICLE_H__
#define __GAME_RC_VEHICLE_H__

class rcPilotable : public idInteractable
{
	CLASS_PROTOTYPE( rcPilotable );
public:
	void			Spawn();
	virtual void	Interact( idPlayer* player ) ;
	virtual bool	CanBeInteractedWith() ;
	
	virtual void	Think();

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

#endif