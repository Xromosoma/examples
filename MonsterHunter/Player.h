#pragma once
#include "Creature.h"
class Player :
    public Creature
{
private:
    int m_level = 1;
public:
    Player(const std::string _name);
    void levelUp();
    bool hasWon();
};

