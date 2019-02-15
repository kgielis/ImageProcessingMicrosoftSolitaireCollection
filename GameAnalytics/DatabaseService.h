#pragma once
#pragma comment(lib, "User32.lib")

#ifdef NDEBUG
#include "mysql_connection.h"
#include "cppconn/driver.h"
#include "cppconn/exception.h"
#include "cppconn/resultset.h"
#include "cppconn/statement.h"
#include <cppconn/prepared_statement.h> 
#endif
#include "globals.h"
#include <ctime>
class DatabaseService
{
public:
	#ifdef NDEBUG
	DatabaseService();
	~DatabaseService();
	void initDBConn();	// initialize the database connection	
	int initLogin();
	void insertMetricsDB(int playerID, GameMetrics metrics, BOOL endOfGameBool);
	void disconnectDB();
	#endif

private:
	#ifdef NDEBUG
	void printErrors(sql::SQLException & e);
	void printLoginOptions();
	int addGameStatsToDatabase(int playerID, GameMetrics metrics, BOOL endOfGameBool);
	void addClickCoordinatesToDatabase(int gameID, GameMetrics metrics);
	void addPressedLocationsToDatabase(int gameID, GameMetrics metrics);
	void addGameMoveStatsToDatabase(int gameID, GameMetrics metrics);
	std::string getpass();
	std::string hashPassword(std::string &password);

	sql::Connection *con;
	sql::Statement *stmt;
	sql::ResultSet *res;
	sql::PreparedStatement  *prep_stmt;
	std::time_t startOfGameDB;
	#endif
};
