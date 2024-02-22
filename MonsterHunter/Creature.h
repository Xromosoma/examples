#pragma once
#include <string>
class Creature
{
protected:
	std::string m_name;
	char m_symbol;
	int m_hp;
	int m_dmg;
	int m_gold;

public:
	Creature(const std::string _name, char _symbol, int _hp, int _dmg, int m_gold);
	const std::string& getName() const;
	char getSymbol() const;
	int getHp() const;
	int getDmg() const;
	int getGold() const;
	void reduceHp(int _value);
	bool isDead() const;
	void addGold(int _value);
};

