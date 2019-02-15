#include "stdafx.h"
#include "DatabaseService.h"
#ifdef NDEBUG
#include "filters.h"//crypt++
#include "default.h"//crypt++
#include "hex.h"//crypt++
#include "base64.h"
#include "conio.h"

DatabaseService::DatabaseService()
{
}

DatabaseService::~DatabaseService()
{
	disconnectDB();
}

void DatabaseService::initDBConn()
{
	std::cout << std::endl;
	std::cout << "Trying to make a connection with the DB" << std::endl;

	try {
		sql::Driver *driver;

		// Create a connection
		driver = get_driver_instance();
		con = driver->connect("tcp://127.0.0.1:3306", "root", "root");
		sql::Statement * stmt = con->createStatement();

		//Create Schema and connect to it
		stmt->execute("CREATE SCHEMA IF NOT EXISTS GameAnalytics");
		con->setSchema("GameAnalytics");
		std::cout << "Connection made with database schema GameAnalytics" << std::endl;

		//Create the table with user information
		//stmt->execute("DROP TABLE IF EXISTS userinfo");
		stmt->execute("CREATE TABLE IF NOT EXISTS userinfo(playerId int NOT NULL AUTO_INCREMENT, username CHAR(50), age int, password CHAR(64), PRIMARY KEY (playerId));");

		//Create the table for the Game Statistics
		//stmt->execute("DROP TABLE IF EXISTS gameinfo");
		stmt->execute("CREATE TABLE IF NOT EXISTS gameinfo(gameId int NOT NULL AUTO_INCREMENT, playerId int, undos int, hints int, suiterrors int, rankerrors int, toomanycardserr int, score int, endofgame CHAR(3), gameresult CHAR(4), starttime DATETIME, totaltimeinsec int, nrmoves int, avgThinkDuration int, avgMoveDuration int, gameType CHAR(50), gameDifficulty CHAR(50), gameSeed CHAR(20), PRIMARY KEY (gameId), FOREIGN KEY (playerId) REFERENCES userinfo(playerId) ON DELETE CASCADE);");

		//Create the table with all Move metrics
		//stmt->execute("DROP TABLE IF EXISTS moveinfo");
		stmt->execute("CREATE TABLE IF NOT EXISTS moveinfo(moveId int NOT NULL AUTO_INCREMENT, gameId int, moveSucces BOOLEAN, timeOfMove DATETIME, thinkDuration int, moveDuration int, rankError BOOLEAN, suitError BOOLEAN, tooManyCardMovement BOOLEAN, unmovableCardError BOOLEAN, sourceLocation int, destLocation int, movedCard CHAR(2), destCard CHAR(2), numberOfCardsMoved int, PRIMARY KEY (moveId), FOREIGN KEY (gameId) REFERENCES gameinfo(gameId) ON DELETE CASCADE);");

		//Create the table with pressed card locations
		//stmt->execute("DROP TABLE IF EXISTS pressedlocations");
		stmt->execute("CREATE TABLE IF NOT EXISTS pressedlocations(pressedLocId int NOT NULL AUTO_INCREMENT, gameId int, pile int, talon int, build1 int, build2 int, build3 int, build4 int, build5 int, build6 int, build7 int, build8 int, storage1 int, storage2 int, storage3 int, storage4 int, suit1 int, suit2 int, suit3 int, suit4 int, PRIMARY KEY (pressedLocId), FOREIGN KEY (gameId) REFERENCES gameinfo(gameId) ON DELETE CASCADE);");

		//Create the table with all the click coordinates
		//stmt->execute("DROP TABLE IF EXISTS clickcoord");
		stmt->execute("CREATE TABLE IF NOT EXISTS clickcoord(clickId int NOT NULL AUTO_INCREMENT, gameId int, xcoord int, ycoord int, PRIMARY KEY (clickId), FOREIGN KEY (gameId) REFERENCES gameinfo(gameId) ON DELETE CASCADE);");

		stmt->close();
	}
	catch (sql::SQLException &e) {
		printErrors(e);
		if (con != nullptr)	con->close();
	}
	std::cout << std::endl;

}

void DatabaseService::printErrors(sql::SQLException & e)
{
	std::cout << "# ERR: SQLException in " << __FILE__;
	std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
	std::cout << "# ERR: " << e.what();
	std::cout << " (MySQL error code: " << e.getErrorCode();
	std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
}

int DatabaseService::initLogin()
{
	std::cout << "Welcome to the Game Analytics tool of Solitaire" << std::endl;
	std::cout << "---------------------------------------------------------------------------------------------------------" << std::endl;

	try {
		int playerID;
		int input, age;
		std::string username, password, passwordcheck;
		bool loggedin = false;

		while (loggedin != true)
		{
			printLoginOptions();
			std::cin >> input;
			std::cin.clear();
			std::cin.ignore(10000, '\n');
			switch (input)
			{
			case 27:
				exit(EXIT_SUCCESS);
				break;
			case 1:
				playerID = -1; //negative value to show in DB it's an unregistered player
				std::cout << std::endl;
				std::cout << "You chose to play without logging in, so no data will be linked to your personal account. Enjoy the game!" << std::endl;
				std::cout << "---------------------------------------------------------------------------------------------------------" << std::endl;
				std::cout << std::endl;
				loggedin = true;
				break;
			case 2:
			{
				std::cout << "Username: ";
				std::cin >> username;
				std::cout << "Password: ";
				password = getpass();

				stmt = con->createStatement();
				res = stmt->executeQuery("SELECT password, playerId FROM userinfo WHERE username = '" + username + "'");
				stmt->close();
				if (res->next())
				{
					std::string hashedPassInFromDB = res->getString(1);
					bool outOfAttempts = false;
					for (int i = 3; i > 0; i--)
					{
						std::string hashedPass = hashPassword(password);
						if (hashedPass.compare(hashedPassInFromDB) != 0)
						{

							std::cout << "Wrong password. " << i - 1 << " attempts left. " << std::endl;
							std::cout << "Password (hidden): ";
							password = getpass();
						}
						else break;
					}
					if (outOfAttempts) 
					{
						std::cout << std::endl;
						std::cout << "Out of attempts. Try logging in again or register another account" << std::endl;
						input = 0;
						break;
					}
					playerID = res->getInt(2);
					loggedin = true;
					std::cout << std::endl;
					std::cout << "Login succesful! Enjoy playing the game" << std::endl;
					std::cout << "-----------------------------------------------" << std::endl;
					std::cout << std::endl;
					res->close();

				}
				else
				{
					if (res != nullptr) res->close();
					std::cout << "Username not found! Try to login with another username or register first." << std::endl;
					std::cout << std::endl;
					input = 0;
					break;
				}
				break;
			}
			case 3:
			{
				std::cout << "Choose username: ";
				std::cin >> username;
				stmt = con->createStatement();
				res = stmt->executeQuery("SELECT playerId FROM userinfo WHERE username = '" + username + "'");
				while (res->next())
				{
					std::cout << "username is not unique, try another" << std::endl;
					std::cout << "Choose username: ";
					std::cin >> username;
					res = stmt->executeQuery("SELECT playerId FROM userinfo WHERE username = '" + username + "'");
				}
				res->close();
				std::cout << "Choose password: ";
				//std::cin >> password;
				password = getpass();
				std::cout << "Retype chosen password: ";
				//std::cin >> passwordcheck;
				passwordcheck = getpass();
				while (password != passwordcheck) {

					std::cout << "Passwords don't match. Please give in password again" << std::endl;
					std::cout << "Choose password: ";
					password = getpass();
					std::cout << "Retype chosen password: ";
					//std::cin >> passwordcheck;
					passwordcheck = getpass();

				}
				std::cout << "What is your age: ";
				std::cin >> age;
				std::string hashedPass = hashPassword(password);
			
				prep_stmt = con->prepareStatement("INSERT INTO UserInfo(username, password, age) VALUES (?, ?, ?)");
				prep_stmt->setString(1, username);
				prep_stmt->setString(2, hashedPass);
				prep_stmt->setInt(3, age);
				prep_stmt->execute();
				prep_stmt->close();

				stmt = con->createStatement();
				res = stmt->executeQuery("SELECT playerId FROM userinfo WHERE username = '" + username + "'");
				stmt->close();
				if (res->next())
				{
					playerID = res->getInt(1);
					std::cout << "new id returned from database" << std::endl;
				}
				else playerID = 1;
				res->close();

				std::cout << "You are now registered. Please login to continue" << std::endl;
				std::cout << std::endl;
				input = 2;
				break;
			}
			default:
				std::cout << "Not a valid input. Please press a number between 1 and 3" << std::endl;
				std::cout << std::endl;
				break;
			}
		}
		return playerID;
	}
	catch (sql::SQLException &e) {
		printErrors(e);
	}
}

void DatabaseService::printLoginOptions()
{
	
	std::cout << "If you want to continue without logging in, press 1" << std::endl;
	std::cout << "If you want to log in, press 2" << std::endl;
	std::cout << "If you want to register, press 3" << std::endl;
	std::cout << "If you want to quit, press esc" << std::endl;
	std::cout << "Press enter after you made your choice." << std::endl;
	std::cout << std::endl;
	std::cout << "Your choice: ";
}

void DatabaseService::insertMetricsDB(int playerID, GameMetrics metrics, BOOL endOfGameBool) {

	try {
		int gameID = addGameStatsToDatabase(playerID, metrics, endOfGameBool);
		addPressedLocationsToDatabase(gameID, metrics);
		addClickCoordinatesToDatabase(gameID, metrics);
		addGameMoveStatsToDatabase(gameID, metrics);

		std::cout << "The data has been saved in the database." << std::endl;
	}
	catch (sql::SQLException &e) {
		printErrors(e);
	}

}

void DatabaseService::addGameMoveStatsToDatabase(int gameID, GameMetrics metrics) { //STILL NEED TO ADD SUCCES MOVE
	try {
		prep_stmt = con->prepareStatement("INSERT INTO moveinfo(gameId, thinkDuration, moveSucces, moveDuration, rankError, suitError, tooManyCardMovement, unmovableCardError, timeOfMove, sourceLocation, destLocation, movedCard, destCard, numberOfCardsMoved) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
		for each (Move move in metrics.listOfMoves)
		{
			prep_stmt->setInt(1, gameID);
			prep_stmt->setInt(2, move.thinkDuration);
			prep_stmt->setInt(3, move.succesfull);
			prep_stmt->setInt(4, move.moveDuration);
			prep_stmt->setInt(5, move.rankErr);
			prep_stmt->setInt(6, move.suitErr);
			prep_stmt->setInt(7, move.tooManyCardsMovementErr);
			prep_stmt->setInt(8, move.unmovableErr);
			
			std::tm tm;
			localtime_s(&tm, &move.timeOfMove);
			std::ostringstream oss;
			oss << std::put_time(&tm, "%F %T");
			prep_stmt->setDateTime(9, oss.str());
			
			std::ostringstream ossMovedCard;
			ossMovedCard << static_cast<char>(move.movedCard.getRank()) << static_cast<char>(move.movedCard.getSuit());
			std::ostringstream ossDestCard;
			ossDestCard << static_cast<char>(move.destCard.getRank()) << static_cast<char>(move.destCard.getSuit());

			prep_stmt->setInt(10, move.sourceLocation);
			prep_stmt->setInt(11, move.destLocation);
			prep_stmt->setString(12, ossMovedCard.str());
			prep_stmt->setString(13, ossDestCard.str());
			prep_stmt->setInt(14, move.numberOfCardsMoved);

			prep_stmt->execute();
		}
		prep_stmt->close();
	}
	catch (sql::SQLException &e) {
		printErrors(e);
	}
}

int DatabaseService::addGameStatsToDatabase(int playerID, GameMetrics metrics, BOOL endOfGameBool)
{
	try {
		prep_stmt = con->prepareStatement("INSERT INTO gameinfo(playerId, undos, hints, suiterrors, rankerrors, toomanycardserr, score, endofgame, gameresult, starttime, totaltimeinsec, nrmoves, avgThinkDuration, avgMoveDuration, gameType, gameDifficulty, gameSeed) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);");
		prep_stmt->setInt(1, playerID);
		prep_stmt->setInt(2, metrics.numberOfUndos);
		prep_stmt->setInt(3, metrics.numberOfHints);
		prep_stmt->setInt(4, metrics.numberOfSuitErrors);
		prep_stmt->setInt(5, metrics.numberOfRankErrors);
		prep_stmt->setInt(6, metrics.numberOfTooManyCardsMovedErrors);
		prep_stmt->setInt(7, metrics.score);

		if (endOfGameBool == TRUE)
			prep_stmt->setString(8, "YES");
		else
			prep_stmt->setString(8, "NO");

		if (metrics.gameWon == TRUE)
			prep_stmt->setString(9, "WON");
		else
			prep_stmt->setString(9, "LOST");

		std::tm tm;
		localtime_s(&tm, &metrics.startingTime);


		std::ostringstream oss;
		oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
		prep_stmt->setDateTime(10, oss.str());

		//insert duration
		prep_stmt->setInt(11, metrics.duration);
		prep_stmt->setInt(12, metrics.listOfMoves.size());
		prep_stmt->setInt(13, metrics.avgThinkDuration);	
		prep_stmt->setInt(14, metrics.avgMoveDuration);
		prep_stmt->setString(15, metrics.gameType);
		prep_stmt->setString(16, metrics.gameDifficulty);
		prep_stmt->setString(17, metrics.gameSeed);
	
		prep_stmt->execute();
		prep_stmt->close();

		stmt = con->createStatement();
		res = stmt->executeQuery("SELECT LAST_INSERT_ID() AS id");
		int gameId = -1;
		if (res->next()) 
		{
			gameId = res->getInt(1);
		}
		res->close();
		stmt->close();
		return gameId;
	}
	catch (sql::SQLException &e) {
		printErrors(e);
	} 
	return -1;
}

void DatabaseService::addClickCoordinatesToDatabase(int gameID, GameMetrics metrics)
{
	try
	{
		prep_stmt = con->prepareStatement("INSERT INTO ClickCoord(gameId, xcoord, ycoord) VALUES (?,?,?)");
		for (int i = 0; i < metrics.locationOfPresses.size(); i++) {
			prep_stmt->setInt(1, gameID);
			prep_stmt->setInt(2, metrics.locationOfPresses.at(i).x);
			prep_stmt->setInt(3, metrics.locationOfPresses.at(i).y);
			prep_stmt->execute();
		}
		prep_stmt->close();
	}
	catch (sql::SQLException &e) {
		printErrors(e);
	}
}

void DatabaseService::addPressedLocationsToDatabase(int gameID, GameMetrics metrics)
{
	try
	{
		prep_stmt = con->prepareStatement("INSERT INTO PressedLocations(gameId, pile, talon, build1, build2, build3, build4, build5, build6, build7, build8, storage1, storage2, storage3, storage4, suit1, suit2, suit3, suit4) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
		if (metrics.gameType.compare("Klondike") == 0)
		{
			prep_stmt->setInt(1, gameID);
			prep_stmt->setInt(2, metrics.numberOfPilePresses);
			prep_stmt->setInt(3, metrics.numberOfPresses.at(7));
			prep_stmt->setInt(4, metrics.numberOfPresses.at(0));
			prep_stmt->setInt(5, metrics.numberOfPresses.at(1));
			prep_stmt->setInt(6, metrics.numberOfPresses.at(2));
			prep_stmt->setInt(7, metrics.numberOfPresses.at(3));
			prep_stmt->setInt(8, metrics.numberOfPresses.at(4));
			prep_stmt->setInt(9, metrics.numberOfPresses.at(5));
			prep_stmt->setInt(10, metrics.numberOfPresses.at(6));
			prep_stmt->setInt(11, 0);
			prep_stmt->setInt(12, 0);
			prep_stmt->setInt(13, 0);
			prep_stmt->setInt(14, 0);
			prep_stmt->setInt(15, 0);
			prep_stmt->setInt(16, metrics.numberOfPresses.at(8));
			prep_stmt->setInt(17, metrics.numberOfPresses.at(9));
			prep_stmt->setInt(18, metrics.numberOfPresses.at(10));
			prep_stmt->setInt(19, metrics.numberOfPresses.at(11));
			prep_stmt->execute();
			prep_stmt->close();
		}
		else if (metrics.gameType.compare("Freecell") == 0) 
		{
			prep_stmt->setInt(1, gameID);
			prep_stmt->setInt(2, 0);
			prep_stmt->setInt(3, 0);
			prep_stmt->setInt(4, metrics.numberOfPresses.at(0));
			prep_stmt->setInt(5, metrics.numberOfPresses.at(1));
			prep_stmt->setInt(6, metrics.numberOfPresses.at(2));
			prep_stmt->setInt(7, metrics.numberOfPresses.at(3));
			prep_stmt->setInt(8, metrics.numberOfPresses.at(4));
			prep_stmt->setInt(9, metrics.numberOfPresses.at(5));
			prep_stmt->setInt(10, metrics.numberOfPresses.at(6));
			prep_stmt->setInt(11, metrics.numberOfPresses.at(7));
			prep_stmt->setInt(12, metrics.numberOfPresses.at(8));
			prep_stmt->setInt(13, metrics.numberOfPresses.at(9));
			prep_stmt->setInt(14, metrics.numberOfPresses.at(10));
			prep_stmt->setInt(15, metrics.numberOfPresses.at(11));
			prep_stmt->setInt(16, metrics.numberOfPresses.at(12));
			prep_stmt->setInt(17, metrics.numberOfPresses.at(13));
			prep_stmt->setInt(18, metrics.numberOfPresses.at(14));
			prep_stmt->setInt(19, metrics.numberOfPresses.at(15));
			prep_stmt->execute();
			prep_stmt->close();
		}
	}
	catch (sql::SQLException &e) {
		printErrors(e);
	}
}

void DatabaseService::disconnectDB() 
{
	try {
		if (con != nullptr) con->close();
	}catch (sql::SQLException &e) {
		printErrors(e);
	}
	
	delete res;
	delete prep_stmt;
	delete stmt;
	delete con;
}

std::string DatabaseService::getpass()
{
	const char BACKSPACE = 8;
	const char RETURN = 13;

	std::string password;
	unsigned char ch = 0;

	DWORD con_mode;
	DWORD dwRead;

	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

	GetConsoleMode(hIn, &con_mode);
	SetConsoleMode(hIn, con_mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));

	while (ReadConsoleA(hIn, &ch, 1, &dwRead, NULL) && ch != RETURN)
	{
		if (ch == BACKSPACE)
		{
			if (password.length() != 0)
			{
				std::cout << "\b \b";
				password.resize(password.length() - 1);
			}
		}
		else
		{
			password += ch;
			std::cout << '*';
		}
	}
	SetConsoleMode(hIn, con_mode);
	std::cout << std::endl;
	return password;
}

std::string DatabaseService::hashPassword(std::string &password)
{
	CryptoPP::SHA256 hash;
	std::string hashedPassword = "";
	CryptoPP::StringSource ss(password, true,
		new CryptoPP::HashFilter(hash,
			new CryptoPP::HexEncoder(
				new CryptoPP::StringSink(hashedPassword))));

	return hashedPassword;
}

#endif

