#include <Game/Game.hpp>
#include <Config/Globals.hpp>
#include <Misc/Utils.hpp>
#include <Interface/LayoutGame.hpp>

#include <stdlib.h>

// Options of the Pause Menu
enum NamesToEasilyIdentifyTheMenuItemsInsteadOfRawNumbers
{
	RESUME, QUIT_MENU, QUIT_GAME
};

Game::Game():
	layout(nullptr),
	gameOver(false),
	score(nullptr),
	highScore(nullptr),
	isPaused(false),
	showPauseMenu(false),
	showHelp(false),
	pauseMenu(nullptr),
	player(nullptr),
	board(nullptr),
	fruits(nullptr)
{ }
Game::~Game()
{
	SAFE_DELETE(this->layout);
	SAFE_DELETE(this->score);
	SAFE_DELETE(this->pauseMenu);
	SAFE_DELETE(this->player);
	SAFE_DELETE(this->board);
	SAFE_DELETE(this->fruits);
}
void Game::start()
{
	// Cleaning things from the previous game (if any)
	SAFE_DELETE(this->layout);
	SAFE_DELETE(this->score);
	SAFE_DELETE(this->pauseMenu);
	SAFE_DELETE(this->player);
	SAFE_DELETE(this->board);
	SAFE_DELETE(this->fruits);

	this->userAskedToQuit     = false;
	this->userAskedToGoToMenu = false;
	this->gameOver            = false;

	// The interface
	this->layout = new LayoutGame(this, 80, 24);

	// Initializing the player and it's attributes
	this->score = new Score();
	this->score->level = Globals::Game::starting_level;

	// Creating the menu and adding each item
	this->pauseMenu = new Menu(1,
	                           1,
	                           this->layout->pause->getW() - 2,
	                           this->layout->pause->getH() - 2);

	MenuItem* item;

	item = new MenuItem("Resume", RESUME);
	this->pauseMenu->add(item);

	this->pauseMenu->addBlank();

	item = new MenuItem("Quit to Main Menu", QUIT_MENU);
	this->pauseMenu->add(item);

	item = new MenuItem("Quit Game", QUIT_GAME);
	this->pauseMenu->add(item);

	this->player = new Player(5, 5);

	this->board = new Board(78, 21, ((Globals::Game::teleport) ?
	                                 Board::TELEPORT :
	                                 Board::SOLID));
	if (Globals::Game::random_walls)
		this->board->randomlyFillExceptBy(this->player->getX(),
		                                  this->player->getY());

	this->fruits = new FruitManager(Globals::Game::fruits_at_once);
	this->fruits->update(this->player, this->board);

	// Starting timers
	this->timerSnake.start();
	this->timer.start();
}
void Game::handleInput(int c)
{
	if (c == ERR)
		return;

	// The only two absolute inputs are to quit and pause.
	// Others depend if the game is paused or not.

	if (c == Globals::Input::quit)
	{
		this->userAskedToQuit = true;
	}
	else if (c == Globals::Input::pause)
	{
		(this->isPaused) ?
			this->pause(false) :
			this->pause(true);

		return;
	}
	else if ((c == '\n') || (c == KEY_ENTER))
	{
		if (! this->isPaused)
		{
			this->pause(true);
			return;
			// This needs to be here otherwise
			// ENTER goes to the menu and immediately
			// unpauses the game.
		}
	}
	else if (c == 'h' || c == 'H')
	{
		// Toggling Pause and Help window
		if (this->isPaused)
		{
			this->isPaused = false;
			this->showHelp = false;
			this->timer.unpause();
			this->timerSnake.unpause();
		}
		else
		{
			this->isPaused = true;
			this->showHelp = true;
			this->timer.pause();
			this->timerSnake.pause();
		}
	}

	// Other keys are not used when paused.
	if (this->isPaused)
	{
		this->pauseMenu->handleInput(c);
		return;
	}

	if (c == Globals::Input::left)
	{
		this->player->move(Player::LEFT);
	}
	else if (c == Globals::Input::right)
	{
		this->player->move(Player::RIGHT);
	}
	else if (c == Globals::Input::up)
	{
		this->player->move(Player::UP);
	}
	else if (c == Globals::Input::down)
	{
		this->player->move(Player::DOWN);
	}
}
void Game::update()
{
	// If we're paused, only handle the menu.
	if (this->isPaused)
	{
		if (this->pauseMenu->willQuit())
		{
			int option = this->pauseMenu->currentID();

			switch(option)
			{
			case RESUME:
				this->pause(false);
				break;

			case QUIT_MENU:
				this->userAskedToGoToMenu = true;
				break;

			case QUIT_GAME:
				this->userAskedToQuit = true;
				break;
			}
			this->pauseMenu->reset();
		}
		return;
	}

	// Forcing Snake to move if enough time has passed
	// (time based on current level)
	this->timerSnake.pause();
	int delta = this->getDelay(this->score->level);

	if (this->timerSnake.delta_ms() >= delta)
	{
		// Checking if on the previous frame
		// the Snake died.
		if (! this->player->isAlive())
		{
			this->gameOver = true;

			if (this->score->points > Globals::Game::highScore.points)
			{
				Globals::Game::highScore.points = this->score->points;
				Globals::Game::highScore.level = this->score->level;
			}
		}
		else
		{
			// Actually move the player
			this->player->update(this->board);

			while (this->fruits->eatenFruit(this->player))
			{
				this->player->increase();

				// Score formula is kinda random and
				// scattered all over this file.
				// TODO: Center it all on the Score class.
				this->score->points += this->score->level * 2;
			}

			this->fruits->update(this->player, this->board);
		}
		this->timerSnake.start();
	}
	else
		this->timerSnake.unpause();
}
void Game::draw()
{
	this->layout->draw(this->pauseMenu);
}
bool Game::isOver()
{
	return (this->gameOver);
}
bool Game::willQuit()
{
	return this->userAskedToQuit;
}
bool Game::willReturnToMenu()
{
	return this->userAskedToGoToMenu;
}
int Game::getDelay(int level)
{
	// returning delay in milliseconds
	if (level < 2)  return 1000;
	if (level < 3)  return 900;
	if (level < 4)  return 850;
	if (level < 5)  return 800;
	if (level < 6)  return 750;
	if (level < 7)  return 700;
	if (level < 8)  return 650;
	if (level < 9)  return 600;
	if (level < 10) return 550;
	if (level < 11) return 500;
	if (level < 12) return 450;
	if (level < 13) return 400;
	if (level < 14) return 350;
	if (level < 15) return 300;
	if (level < 16) return 250;
	if (level < 17) return 200;
	if (level < 18) return 150;
	if (level < 19) return 100;
	if (level < 20) return 80;
	if (level < 21) return 50;
	if (level < 22) return 25;
	if (level < 23) return 0;

	return 0;
}
void Game::pause(bool option)
{
	if (option)
	{
		if (this->isPaused)
			return;

		this->isPaused = true;
		this->showPauseMenu = true;
		this->timer.pause();
		this->timerSnake.pause();
	}
	else
	{
		if (! this->isPaused)
			return;

		this->isPaused = false;
		this->showPauseMenu = false;
		this->timer.unpause();
		this->timerSnake.unpause();
	}
}
