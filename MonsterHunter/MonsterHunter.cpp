#include <iostream>
#include "creature.h"
#include "player.h"
#include "monster.h"

bool getPlayerChoice()
{
	std::cout << "(R)un or (F)ight:";
	
	std::string answer;
	while (true)
	{
		std::getline(std::cin, answer);
		if (answer == "r" || answer == "R")
		{
			std::cout << "You decided to run."<<std::endl;
			return false;
		}
		else if (answer == "f" || answer == "F")
		{
			std::cout << "You decided to fight."<< std::endl;
			return true;
		}
		else 
			std::cout << "No another way, only run or fight, as a nature calls."<<std::endl;
	}
}

void meetMonster()
{
}

void attackPlayer(Monster& _monsta, Player& _player, bool canRun = true)
{
	std::cout << _monsta.getName() << " tries to attack you.\n";
	if (canRun)
	{
		int dodged = Monster::getRandomNumber(0, 1);
		if (dodged)
		{
			_monsta = Monster::getRandomMonster();
			std::cout << "You are pretty tricky, so the attack was dodged and you run away. But this place is dangerous, and after 10 minutes you have encountered a new monster (" << _monsta.getName() << ", has " << _monsta.getHp() << " hp, reward: " << _monsta.getGold() << " gold)\n";;
			return;
		}
		else
			std::cout << "Your attempt to run was failed.\n";
	}
	_player.reduceHp(_monsta.getDmg());
	std::cout << "You take " << _monsta.getDmg() << " dmg and now have "<< _player.getHp()<<" hp.\n";
}

void attackMonster(Player& _player, Monster& _monsta)
{
	std::cout << "You attack " << _monsta.getName() << " and damaged him on " << _player.getDmg()<<".\n";
	_monsta.reduceHp(_player.getDmg());
	if (_monsta.isDead())
	{
		_player.addGold(_monsta.getGold());
		_player.levelUp();
		std::cout << "Excellent, you killed him! Level up and take " << _monsta.getGold() << " gold.\n";
		_monsta = Monster::getRandomMonster();
		std::cout << "You have encountered a new monster (" << _monsta.getName() << ", has " << _monsta.getHp() << " hp, reward: " << _monsta.getGold() << " gold)\n";
	}
	else
	{
		std::cout << "Arrrr-g-hhh, " << _monsta.getName() << " is still alive and has "<< _monsta.getHp()<<" hp.\n";
		attackPlayer(_monsta, _player,false);
	}
}



int main()
{
	srand(static_cast<unsigned int>(time(0))); 
	rand();

	std::cout << "Brave warrior, say your name:" << std::endl;
	std::string playerName = "";
	std::getline(std::cin, playerName);
	Player player(playerName);
	std::cout << "Welcome, "<< playerName <<". ";
	std::cout << "You have " << player.getHp() << " hp and are carrying " << player.getGold() << " gold." << std::endl;
	std::cout << "Lets start our hunt!\n";
	
	Monster monster = Monster::getRandomMonster();
	std::cout << "You have encountered a new monster (" << monster.getName() << ", has " << monster.getHp() << " hp, reward: " << monster.getGold() << " gold)\n";

	while (!player.isDead())
	{
		bool gonnaFight = getPlayerChoice();
		if (gonnaFight)
		{
			attackMonster(player, monster);
			if (player.hasWon())
			{
				std::cout << "Well done, you reached max lvl 20 and won. Your final gold: " << player.getGold() << ".\n";
				return 0;
			}
		}
		else
			attackPlayer(monster, player);
	}
	std::cout << "Today your heartbeat was stopped...You found death with " << player.getGold() << " gold in pouch.\n";
	return 0;
}