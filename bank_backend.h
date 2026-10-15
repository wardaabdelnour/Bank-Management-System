#ifndef BANK_BACKEND_H
#define BANK_BACKEND_H

// Public interface of the original backend (bank_project.cpp).
// The struct below is the original sClients, moved here unchanged so that
// both bank_project.cpp and the Qt GUI can share it.

#include <string>
#include <vector>

struct sClients {
	std::string AccountNumber;
	std::string PinCode;
	std::string Name;
	double Balance;
	bool MarkForDeletion;
};

// Original global client list (defined in bank_project.cpp).
extern std::vector<sClients> Vclient;

// Original function (unchanged) - reads Bank.txt using the "#//#" separator.
std::vector<sClients> LoadDataFromFile(std::string FileName);

// ---------------------------------------------------------------------------
// Console-free entry points used by the GUI.
// Each one runs the same load / change / save code as the console menu.
// ---------------------------------------------------------------------------
enum enOpResult {
	OpOk = 0,
	OpNotFound,
	OpAlreadyExists,
	OpInsufficientFunds
};

enOpResult AddClientRecord(std::vector<sClients>& Vclient, const sClients& NewClient);
enOpResult FindClientRecord(std::vector<sClients>& Vclient, const std::string& AccountNumber, sClients& Found);
enOpResult DeleteClientRecord(std::vector<sClients>& Vclient, const std::string& AccountNumber);
enOpResult UpdateClientRecord(std::vector<sClients>& Vclient, const std::string& AccountNumber,
                              const std::string& PinCode, const std::string& Name, double Balance);
enOpResult DepositToClient(std::vector<sClients>& Vclient, const std::string& AccountNumber,
                           double Amount, double& NewBalance);
enOpResult WithdrawFromClient(std::vector<sClients>& Vclient, const std::string& AccountNumber,
                              double Amount, double& NewBalance);
double CalculateTotalBalances(const std::vector<sClients>& Vclient);

#endif // BANK_BACKEND_H
