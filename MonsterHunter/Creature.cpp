#include "Creature.h"

Creature::Creature(const std::string _name, char _symbol, int _hp, int _dmg, int m_gold)
	: m_name{ _name }, m_symbol{ _symbol }, m_hp{ _hp }, m_dmg{ _dmg }, m_gold{ m_gold }
{
}

const std::string& Creature::getName() const
{
	return m_name;
}

char Creature::getSymbol() const
{
	return m_symbol;
}

int Creature::getHp() const
{
	return m_hp;
}

int Creature::getDmg() const
{
	return m_dmg;
}

int Creature::getGold() const
{
	return m_gold;
}

void Creature::reduceHp(int _value)
{
	m_hp -= _value;
}

bool Creature::isDead() const
{
		return m_hp <= 0;
}

void Creature::addGold(int _value)
{
	m_gold += _value;
}
