
#ifndef __GAME_RCROIDS_H__
#define __GAME_RCROIDS_H__

class rcAsteroid : public idStaticEntity
{
public:
	CLASS_PROTOTYPE( rcAsteroid );
	rcAsteroid();
	~rcAsteroid();

	void			Spawn();
	void			Event_Activate( idEntity* activator );

	virtual void	Think();

protected:
	static idBoxOctree asteroidList;
private:

	float	minableSilicon;

};

#endif