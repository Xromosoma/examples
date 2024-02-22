#include "Player.h"

Player::Player(const std::string _name):
	Creature(_name,'@',10,1,0)
{
}

void Player::levelUp()
{
	++m_level;
	++m_dmg;
}

bool Player::hasWon()
{
	return m_level >= 20;
}
