
#ifndef __GAME_RCROIDS_H__
#define __GAME_RCROIDS_H__

struct asteroidMine_s
{
	idVec3 posOnSurface;
	idVec3 alignPos;
	idMat3 alignAxis;
	float	radius;
} ;

class rcAsteroid : public idStaticEntity
{
public:
	CLASS_PROTOTYPE( rcAsteroid );
	static asteroidMine_s invalidMine;

	void			Spawn();
	void			Event_Activate( idEntity* activator );

	virtual void	Think();

	const asteroidMine_s& rcAsteroid::GetClosestMine( const idVec3& position );
protected:
	static idBoxOctree asteroidList;
private:
	void CreateMines();
	idList< asteroidMine_s > mines;
	int						totalMines;
	float					minRadius;
	float					maxRadius;
	float					mineChance;
};

#endif