#pragma once
#include "Creature.h"
class Monster :
    public Creature
{ 
public:
    enum Type
    {
        DRAGON,
        ORC,
        SLIME,
        MAX_TYPES
    };
    Monster(Type _monsterType);
private:
    struct MonsterData
    {
        std::string name;
        char symbol;
        int hp;
        int dmg;
        int gold;
    };
public:
    static MonsterData monsterData[MAX_TYPES];
    static int getRandomNumber(int min, int max);
    static Monster getRandomMonster();
};



