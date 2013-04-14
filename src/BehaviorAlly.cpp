#include "BehaviorAlly.h"
#include "Enemy.h"

const unsigned short MINIMUM_FOLLOW_DISTANCE_LOWER = 100;
const unsigned short MINIMUM_FOLLOW_DISTANCE = 250;
const unsigned short MAXIMUM_FOLLOW_DISTANCE = 2000;

const unsigned short BLOCK_TICKS = 10;

BehaviorAlly::BehaviorAlly(Enemy *_e, EnemyManager *_em) : BehaviorStandard(_e, _em)
{
}

BehaviorAlly::~BehaviorAlly()
{
}

void BehaviorAlly::findTarget()
{
    // stunned minions can't act
    if (e->stats.effects.stun) return;

    // check distance and line of sight between minion and hero
    if (e->stats.hero_alive)
        hero_dist = e->getDistance(e->stats.hero_pos);
    else
        hero_dist = 0;

    //if the minion gets too far, transport it to the player pos
    if(hero_dist > MAXIMUM_FOLLOW_DISTANCE && !e->stats.in_combat)
    {
        e->stats.pos.x = e->stats.hero_pos.x;
        e->stats.pos.y = e->stats.hero_pos.y;
        hero_dist = 0;
    }

    bool enemies_in_combat = false;
    //enter combat because enemy is targeting the player or a summon
    for (unsigned int i=0; i < enemies->enemies.size(); i++) {
        if(enemies->enemies[i]->stats.in_combat && !enemies->enemies[i]->stats.hero_ally)
        {
            Enemy* enemy = enemies->enemies[i];

            //now work out the distance to the enemy and compare it to the distance to the current targer (we want to target the closest enemy)
            if(enemies_in_combat){
                int enemy_dist = e->getDistance(enemy->stats.pos);
                if(enemy_dist < target_dist){
                    pursue_pos.x = enemy->stats.pos.x;
                    pursue_pos.y = enemy->stats.pos.y;
                    target_dist = enemy_dist;
                }
            }
            else
            {//minion is not already chasig another enemy so chase this one
                pursue_pos.x = enemy->stats.pos.x;
                pursue_pos.y = enemy->stats.pos.y;
                target_dist = e->getDistance(enemy->stats.pos);
            }

            e->stats.in_combat = true;
            enemies_in_combat = true;
        }
    }


    //break combat if the player gets too far or all enemies die
    if(!enemies_in_combat)
        e->stats.in_combat = false;

    //the default target is the player
    if(!e->stats.in_combat)
    {
        pursue_pos.x = e->stats.hero_pos.x;
        pursue_pos.y = e->stats.hero_pos.y;
        target_dist = hero_dist;
    }

    // check line-of-sight
	if (target_dist < e->stats.threat_range && e->stats.hero_alive)
		los = e->map->collider.line_of_sight(e->stats.pos.x, e->stats.pos.y, pursue_pos.x, pursue_pos.y);
	else
		los = false;

}


/*
checks whether the entity in pos 1 is facing the point at pos 2
based on a 45 degree field of vision
*/
bool BehaviorAlly::is_facing(int x, int y, char direction, int x2, int y2){

    //45 degree fov
    switch (direction) {
		case 2:
		    return x2 < x && y2 < y;
		case 3:
		    return (y2 < y && ((x2-x) < ((-1 * y2)-(-1 * y)) && (((-1 * x2)-(-1 * x)) < ((-1 * y2)-(-1 * y)))));
		case 4:
		    return x2 > x && y2 < y;
		case 5:
		    return (x2 > x && ((x2-x) > (y2-y)) && ((x2-x) > ((-1 * y2)-(-1 * y))));
		case 6:
		    return x2 > x && y2 > y;
		case 7:
            return (y2 > y && ((x2-x) < (y2-y)) && (((-1 * x2)-(-1 * x)) < (y2-y)));
		case 0:
		    return x2 < x && y2 > y;
		case 1:
            return (x2 < x && (((-1 * x2)-(-1 * x)) > (y2-y)) && (((-1 * x2)-(-1 * x)) > ((-1 * y2)-(-1 * y))));
	}
	return false;
}

void BehaviorAlly::checkMoveStateStance()
{

    if(e->stats.in_combat && target_dist > e->stats.melee_range)
        e->newState(ENEMY_MOVE);


    //if the player is blocked, all summons which the player is facing to move away for the specified frames
    //if hero is facing the summon
    if(!enemies->player_blocked && hero_dist < MINIMUM_FOLLOW_DISTANCE_LOWER
       && is_facing(e->stats.hero_pos.x,e->stats.hero_pos.y,e->stats.hero_direction,e->stats.pos.x,e->stats.pos.y)){
            enemies->player_blocked = true;
            enemies->player_blocked_ticks = BLOCK_TICKS;
    }

    //if the player is blocked, all summons in front of him moves away
    if(enemies->player_blocked && !e->stats.in_combat
       && is_facing(e->stats.hero_pos.x,e->stats.hero_pos.y,e->stats.hero_direction,e->stats.pos.x,e->stats.pos.y)){

        e->stats.direction = calcDirection(e->stats.hero_pos, e->stats.pos);

        if (e->move()) {
            e->newState(ENEMY_MOVE);
        }
        else {
            int prev_direction = e->stats.direction;

            // hit an obstacle, try the next best angle
            e->stats.direction = e->faceNextBest(pursue_pos.x, pursue_pos.y);
            if (e->move()) {
                e->newState(ENEMY_MOVE);
            }
            else e->stats.direction = prev_direction;
        }
    }

    else

    if(!e->stats.in_combat && hero_dist > MINIMUM_FOLLOW_DISTANCE)
    {
        if (e->move()) {
            e->newState(ENEMY_MOVE);
        }
        else {
            int prev_direction = e->stats.direction;

            // hit an obstacle, try the next best angle
            e->stats.direction = e->faceNextBest(pursue_pos.x, pursue_pos.y);
            if (e->move()) {
                e->newState(ENEMY_MOVE);
            }
            else e->stats.direction = prev_direction;
        }
    }
}

void BehaviorAlly::checkMoveStateMove()
{

    if(enemies->player_blocked && !e->stats.in_combat
       && is_facing(e->stats.hero_pos.x,e->stats.hero_pos.y,e->stats.hero_direction,e->stats.pos.x,e->stats.pos.y)){

        e->stats.direction = calcDirection(e->stats.hero_pos, e->stats.pos);

        if (!e->move()) {
            int prev_direction = e->stats.direction;
            // hit an obstacle.  Try the next best angle
            e->stats.direction = e->faceNextBest(pursue_pos.x, pursue_pos.y);
            if (!e->move()) {
                //minion->newState(MINION_STANCE);
                e->stats.direction = prev_direction;
            }
        }

    }
    else

    //if close enough to hero, stop miving
    if(hero_dist < MINIMUM_FOLLOW_DISTANCE && !e->stats.in_combat)
    {
        e->newState(ENEMY_STANCE);
    }

    // try to continue moving
    else if (!e->move()) {
        int prev_direction = e->stats.direction;
        // hit an obstacle.  Try the next best angle
        e->stats.direction = e->faceNextBest(pursue_pos.x, pursue_pos.y);
        if (!e->move()) {
            //minion->newState(MINION_STANCE);
            e->stats.direction = prev_direction;
        }
    }
}

