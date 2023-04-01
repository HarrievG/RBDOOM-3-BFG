
#ifndef __GAME_PILOTABLE_H__
#define __GAME_PILOTABLE_H__

class rcPilotable : public idInteractable
{
	CLASS_PROTOTYPE( rcPilotable );
public:
	void	Spawn();

	virtual void Interact( idPlayer* player ) ;
	virtual bool CanBeInteractedWith() ;
	virtual void Think() ;

protected:
	void					Event_Activate( idEntity* activator );

	idPhysics_RigidBody		physicsObj;				// physics object
};

#endif