#include "Monster.h"

#include <cstdlib> // для rand() и srand()
#include <ctime> // для time()

Monster::Monster(Type _monsterType) :
	Creature(monsterData[_monsterType].name, monsterData[_monsterType].symbol, monsterData[_monsterType].hp, monsterData[_monsterType].dmg, monsterData[_monsterType].gold)
{

}

Monster::MonsterData Monster::monsterData[Monster::MAX_TYPES]
{
	{ "dragon", 'D', 20, 4, 100 },
	{ "orc", 'o', 4, 2, 25 },
	{ "slime", 's', 1, 1, 10 }
	
};

int Monster::getRandomNumber(int min, int max)
{
	static const double fraction = 1.0 / (static_cast<double>(RAND_MAX) + 1.0);
	// Равномерно распределяем генерацию значения из диапазона
	return static_cast<int>(rand() * fraction * (max - min + 1) + min);
}

Monster Monster::getRandomMonster()
{
	int randomMonstaType = Monster::getRandomNumber(0, Monster::MAX_TYPES - 1);
	return Monster(static_cast<Monster::Type>(randomMonstaType));
}




