#include<fstream>
#include <iostream>
#include<string>
#include<vector>
#include<iomanip>
#include <limits>
#include <cstdlib>
#include "bank_backend.h"   // struct sClients now lives here (moved unchanged)
using namespace std;

void ShowTransactionMenu();
void ShowMainMenu();

vector<sClients> Vclient;

bool IsClientExist(vector<sClients>& Vclient,string AccountNumber)
{
	for (sClients Client : Vclient)
	{
		if (Client.AccountNumber == AccountNumber)
			return true;
	}

	return false;
}

    void ReadClient(vector<sClients>& Vclient) {
	  
	   sClients client;
	   cout << "Enter Account Number ? ";
	   getline(cin, client.AccountNumber);

	   while (IsClientExist(Vclient,client.AccountNumber))
	   {
		   cout << "Client with [" << client.AccountNumber << "] already exists.\n";
		   cout << "Enter another Account Number ? ";
		   getline(cin, client.AccountNumber);
	   }

	   cout << "enter pincode ? ";
	   getline(cin, client.PinCode);

	   cout << "enter client name ? ";
	   getline(cin, client.Name);

	   cout << "enter balance ? ";
	   cin >> client.Balance;
	   cin.ignore(numeric_limits<streamsize>::max(), '\n');

	   Vclient.push_back(client);
}

   string ConvertRecordToLine(vector<sClients>& Vclient, string separator)
   {
	   string line;

	   for (int i = 0; i < Vclient.size(); i++)
	   {
		   line += Vclient[i].AccountNumber + separator;
		   line += Vclient[i].PinCode + separator;
		   line += Vclient[i].Name + separator;
		   line += to_string(Vclient[i].Balance);
		   line += "\n";
	   }

	   return line;
   }

 void SaveToFile(string FileName, string Lines, bool Append = true)
	   {
		   fstream MyFile;

		   if (Append)
			   MyFile.open(FileName, ios::app);
		   else
			   MyFile.open(FileName, ios::out);

		   if (MyFile.is_open())
		   {
			   MyFile << Lines;
			   MyFile.close();
		   }
	   }


vector<sClients> LoadDataFromFile(string FileName)
{
	vector<sClients> Vclient;

	fstream MyFile;
	MyFile.open(FileName, ios::in);

	if (MyFile.is_open())
	{
		string Line;

		while (getline(MyFile, Line))
		{
			if (Line.empty())
				continue;

			sClients Client;

			Client.AccountNumber = Line.substr(0, Line.find("#//#"));
			Line.erase(0, Line.find("#//#") + 4);

			Client.PinCode = Line.substr(0, Line.find("#//#"));
			Line.erase(0, Line.find("#//#") + 4);

			Client.Name = Line.substr(0, Line.find("#//#"));
			Line.erase(0, Line.find("#//#") + 4);

			try
			{
				Client.Balance = stod(Line);
			}
			catch (const invalid_argument&)
			{
				cout << "Invalid balance data: [" << Line << "]\n";
				continue;
			}

			Client.MarkForDeletion = false;

			Vclient.push_back(Client);
		}

		MyFile.close();
	}

	return Vclient;
}

// ---------------------------------------------------------------------------
// Shared save/apply helpers.
// These contain lines that used to sit inline inside the console functions
// below. They were moved here, unchanged, so the console menu and the Qt GUI
// run exactly the same code. The console functions now call them.
// ---------------------------------------------------------------------------

// Appends the LAST client of the vector to Bank.txt (was inline in AddNewClient).
void SaveLastClientToFile(vector<sClients>& Vclient)
{
	vector<sClients> NewClient;
	NewClient.push_back(Vclient.back());

	string lines = ConvertRecordToLine(NewClient, "#//#");
	SaveToFile("Bank.txt", lines);
}

// Rewrites Bank.txt without the clients marked for deletion (was inline in DeleteClient).
void SaveClientsNotMarkedForDeletion(vector<sClients>& Vclient)
{
	string lines;
	for (int i = 0; i < Vclient.size(); i++) {
		if (Vclient[i].MarkForDeletion == false) {

			lines += Vclient[i].AccountNumber + "#//#";
			lines += Vclient[i].PinCode + "#//#";
			lines += Vclient[i].Name + "#//#";
			lines += to_string(Vclient[i].Balance);
			lines += "\n";
		}
	}
	SaveToFile("Bank.txt", lines, false);
}

// Rewrites Bank.txt with every client (was inline in Update / Deposit / withdraw).
void SaveAllClientsToFile(vector<sClients>& Vclient)
{
	string lines = ConvertRecordToLine(Vclient, "#//#");
	SaveToFile("Bank.txt", lines, false);
}

// Was inline in UpdateClient.
void ApplyClientUpdate(vector<sClients>& Vclient, int i, string PinCode, string Name, double Balance)
{
	Vclient[i].PinCode = PinCode;
	Vclient[i].Name = Name;
	Vclient[i].Balance = Balance;

	SaveAllClientsToFile(Vclient);
}

// Was inline in Deposit.
void ApplyDeposit(vector<sClients>& Vclient, int i, double depositAmount)
{
	Vclient[i].Balance += depositAmount;

	SaveAllClientsToFile(Vclient);
}

// Was inline in withdraw: the "amount <= balance" rule.
bool HasSufficientBalance(vector<sClients>& Vclient, int i, double withdrawAmount)
{
	return withdrawAmount <= Vclient[i].Balance;
}

// Was inline in withdraw.
void ApplyWithdraw(vector<sClients>& Vclient, int i, double withdrawAmount)
{
	Vclient[i].Balance -= withdrawAmount;

	SaveAllClientsToFile(Vclient);
}

// Was the "+=" inside the TotalBalances loop.
double CalculateTotalBalances(const vector<sClients>& Vclient)
{
	double totalBalances = 0.0;
	for (int i = 0; i < Vclient.size(); i++)
		totalBalances += Vclient[i].Balance;
	return totalBalances;
}

void PrintClients(vector<sClients>& Vclient) {


			   for (sClients Client : Vclient)
			   {
				   cout << "Account Number: " << Client.AccountNumber << endl;
				   cout << "Pin Code: " << Client.PinCode << endl;
				   cout << "Name: " << Client.Name << endl;
				   cout << "Balance: " << Client.Balance << endl;
				   cout << "----------------------" << endl;
			   }
  }

void PrintClientCard(vector <sClients> &Vclient, string accountnumber )
{
	for (int i = 0; i < Vclient.size(); i++) {

		if (accountnumber == Vclient[i].AccountNumber) {
			cout << "account number :- " << Vclient[i].AccountNumber << endl;
			cout << "pin code:- " << Vclient[i].PinCode << endl;
			cout << " Name:- " << Vclient[i].Name<< endl;
			cout << " balance :- " << Vclient[i].Balance << endl;
		}
	}
}
	   
		 
 void showClientList(vector<sClients>& Vclient)
		  {
			  Vclient = LoadDataFromFile("Bank.txt");

			  cout << "________________________________________________________________________________" << endl;
			  cout << "\t\t\tClient List (" << Vclient.size() << ") Client(s))" << endl;
			  cout << "________________________________________________________________________________" << endl;

			  cout << "| " << left << setw(15) << "Account Number"
				  << "| " << left << setw(12) << "Pin Code"
				  << "| " << left << setw(20) << "Name"
				  << "| " << left << setw(12) << "Balance"
				  << "|" << endl;

			  cout << "________________________________________________________________________________" << endl;

			  for (int i = 0; i < Vclient.size(); i++)
			  {
				  cout << "| " << left << setw(15) << Vclient[i].AccountNumber
					  << "| " << left << setw(12) << Vclient[i].PinCode
					  << "| " << left << setw(20) << Vclient[i].Name
					  << "| " << left << setw(12) << Vclient[i].Balance
					  << "|" << endl;
			  }

			  cout << "________________________________________________________________________________" << endl;

			  cout << "enter any key to go to main menu";
			  system ("pause");
		  }


void AddNewClient() {

	bool AddMore = true;

	while (AddMore == true) {

		ReadClient(Vclient);

		SaveLastClientToFile(Vclient);

		char answer;
		cout << "Do you want to add more client (y/n)? ";
		cin >> answer;
		cin.ignore(numeric_limits<streamsize>::max(), '\n');

		if (answer == 'y' || answer == 'Y') {
			AddMore = true;
		}
		else {
			AddMore = false;
		}
	}
}


void DeleteClient(vector<sClients>& Vclient) {
	string accountnumber;
	Vclient = LoadDataFromFile("Bank.txt");
	cout << "enter account number?";
	
	getline(cin, accountnumber);
	for (int i = 0; i < Vclient.size(); i++) {
		if (accountnumber == Vclient[i].AccountNumber) {
			PrintClientCard(Vclient, accountnumber);

			Vclient[i].MarkForDeletion = true;
			
		}
	}
	char answer;
	cout << "are you sure you want to delete this client(y/n)?";
	cin >> answer;
	if (answer == 'y' || answer == 'Y') {
		SaveClientsNotMarkedForDeletion(Vclient);
	}
	
}

void UpdateClient(vector<sClients>& Vclient) {
	
		
		string accountnumber;
		sClients client;

		Vclient = LoadDataFromFile("Bank.txt");

		cout << "Enter account number? ";
		getline(cin, accountnumber);

		for (int i = 0; i < Vclient.size(); i++) {

			if (accountnumber == Vclient[i].AccountNumber) {

				PrintClientCard(Vclient, accountnumber);

				char answer;
				cout << "Are you sure you want to update this client (y/n)? ";
				cin >> answer;
				cin.ignore();

				if (answer == 'y' || answer == 'Y') {

					cout << "Enter PinCode? ";
					getline(cin, client.PinCode);

					cout << "Enter client name? ";
					getline(cin, client.Name);

					cout << "Enter balance? ";
					cin >> client.Balance;

					// Update the old client
					ApplyClientUpdate(Vclient, i, client.PinCode, client.Name, client.Balance);


					cout << "\nClient updated successfully!\n";
				}

				return;
			}
		}

		cout << "Client not found!\n";
	}

void FindClient(vector<sClients>& Vclient) {
	string accountNumber;
	cout << "enter account number ?";
	getline(cin, accountNumber);
	for (int i = 0; i < Vclient.size(); i++) {

		if (accountNumber == Vclient[i].AccountNumber) {

			PrintClientCard(Vclient, accountNumber);
		}
	}
}

void GoBackToMainMenu() {
  
	cout << "press any key to go back to main menu...";
	system("pause>0");
	system("cls");
	ShowMainMenu();
}

short ReadMainMenuOption() {
	short option=0;
	cout << " Enter number of service {1 to 7}? \n";
	cin >> option;
	cin.ignore(numeric_limits<streamsize>::max(), '\n');
	return option;
}

enum enMainMenuOption {
	elistClient = 1, eaddclient = 2,
	eDeleteClient =3 ,eUpdateClient=4,
	eFindClient =5 , eTransaction=6 , eExit=7
};

void ShowEndScreen()
{
	cout << "\n-----------------------------------\n";
	cout << "\tProgram Ends :-)";
	cout << "\n-----------------------------------\n";
}

void PerformMainMenuOption(enMainMenuOption mainMenuOption) {
	switch (mainMenuOption) {
	case enMainMenuOption::elistClient:
		system("cls");
			showClientList(Vclient);
			GoBackToMainMenu();
			break;
	case enMainMenuOption::eaddclient:
		system("cls");
		AddNewClient();
		GoBackToMainMenu();
		break;
	case enMainMenuOption::eDeleteClient:
		system("cls");
		DeleteClient(Vclient);
		GoBackToMainMenu();
		break;
	case enMainMenuOption::eUpdateClient:
		system("cls");
		UpdateClient(Vclient);
		GoBackToMainMenu();
		break;
	case enMainMenuOption::eFindClient:
		system("cls");
		FindClient(Vclient);
		GoBackToMainMenu();
		break;
	case enMainMenuOption::eTransaction:
		system("cls");
		ShowTransactionMenu();
		break;
	case enMainMenuOption::eExit:
		system("cls");
		ShowEndScreen();
		break;
	}

}

void ShowMainMenu() {
	

		cout << "===================================================================\n";
		cout << "                       service list                       \n";
		cout << "====================================================================\n";
		cout << "\n";
		cout << " {1} show clients list\n";
		cout << " {2} Add new client(s)\n";
		cout << " {3} delete client(s) \n";
		cout << " {4} update client(s) \n";
		cout << " {5}find client(s) \n";
		cout << " {6} transaction \n";
		cout << " {7} Exit \n";
		cout << " --------------------------------------------------------------------\n";
		PerformMainMenuOption((enMainMenuOption)ReadMainMenuOption());
		

	}

enum enTransactionMenue {

	edposit = 1, eWithdraw = 2, eTotalBalance = 3, eMainMenu = 4
};
void GoBackToTransactionMenu() {

	cout << "press any key to go back to transaction menu...";
	system("pause>0");
	system("cls");
	ShowTransactionMenu();
}
void Deposit(vector<sClients>& Vclient) {
	char Areyousure;
	string Accountnumber;
	double depositAmount;
	Vclient = LoadDataFromFile("Bank.txt");
	cout << "enter account number ? ";
	getline(cin, Accountnumber);
	for (int i = 0; i < Vclient.size(); i++) {

		if (Accountnumber == Vclient[i].AccountNumber) {
			PrintClientCard(Vclient, Accountnumber);

			cout << "enter deposit amount ? ";
			cin >> depositAmount;
			cout << " are you sure you want to deposit to this account ? (y/n)";
			cin >> Areyousure;
			if (Areyousure == 'y' || Areyousure == 'Y') {

				ApplyDeposit(Vclient, i, depositAmount);

				cout << "Deposit done successfully!\n";
				cout << "New balance = " << Vclient[i].Balance << endl;
				return;
			}

		}
		cout << " account number not found ";

	}
}

 void withdraw(vector<sClients>&Vclient) {
	 char Areyousure;
	 string Accountnumber;
	 double withdrawAmount;
	 Vclient = LoadDataFromFile("Bank.txt");
	 cout << "enter account number ? ";
	 getline(cin, Accountnumber);
	 for (int i = 0; i < Vclient.size(); i++) {

		 if (Accountnumber == Vclient[i].AccountNumber) {
			 PrintClientCard(Vclient, Accountnumber);



			 cout << "enter withdraw amount ? ";
			 cin >> withdrawAmount;
			 if (HasSufficientBalance(Vclient, i, withdrawAmount)) {
				 cout << " are you sure you want to deposit to this account ? (y/n)";
				 cin >> Areyousure;
				 if (Areyousure == 'y' || Areyousure == 'Y') {

					 ApplyWithdraw(Vclient, i, withdrawAmount);

					 cout << " WithDraw done successfully!\n";
					 cout << "New balance = " << Vclient[i].Balance << endl;
				 }
			 }

			 cout << " the withdraw amount exceed client balance \n ";
		 }

		 
	 }
	 cout << "user not found \n";
 }

 void TotalBalances(vector<sClients>& Vclient) {

	 Vclient = LoadDataFromFile("Bank.txt");
	 double totalBalances = CalculateTotalBalances(Vclient);
	 cout << "________________________________________________________________________________" << endl;
	 cout << "\t\t\tClient List (" << Vclient.size() << ") Client(s))" << endl;
	 cout << "________________________________________________________________________________" << endl;

	 cout << "| " << left << setw(15) << "Account Number"
		 << "| " << left << setw(20) << "Name"
		 << "| " << left << setw(12) << "Balance"
		 << "|" << endl;

	 cout << "________________________________________________________________________________" << endl;

	 for (int i = 0; i < Vclient.size(); i++)
	 {
		 cout << "| " << left << setw(15) << Vclient[i].AccountNumber
			 
			 << "| " << left << setw(20) << Vclient[i].Name
			 << "| " << left << setw(12) << Vclient[i].Balance
			 << "|" << endl;
	 }
	 cout << "   total balances = " << totalBalances << endl;
	 cout << "________________________________________________________________________________" << endl;

	 cout << "enter any key to go to main menu";
	 system("pause");

 }

 short readTransactionOption() {
	 short choice = 0;
	 cout << " Enter number of service {1 to 4}? \n";
	 cin >> choice;
	 cin.ignore(numeric_limits<streamsize>::max(), '\n');
	 return choice;

 }

 void performTransactionMenu(enTransactionMenue transactions) {
	 switch (transactions) {

	 case enTransactionMenue::edposit:
		 system("cls");
		 Deposit(Vclient);
		 GoBackToTransactionMenu();
		 break;

	 case enTransactionMenue::eWithdraw:
		 system("cls");
		withdraw(Vclient);
		 GoBackToTransactionMenu();
		 break;
	 case enTransactionMenue::eTotalBalance:
		 system("cls");
		TotalBalances(Vclient);
		 GoBackToTransactionMenu();
		 break;
	 case enTransactionMenue::eMainMenu:
		 system("cls");
		 GoBackToMainMenu();
		 break;
	 }

 }

 void ShowTransactionMenu() {

	 cout << "====================================================================\n";
	 cout << "                        transactios                                  \n";
	 cout << "=====================================================================\n";
	 cout << "{1} Deposit \n";
	 cout << "{2} withdraw \n";
	 cout << "{3} total balances \n";
	 cout << "{4} main menu \n";
	 cout << "======================================================================\n";

	 performTransactionMenu((enTransactionMenue)readTransactionOption());

 }

// ===========================================================================
// Console-free entry points used by the Qt GUI (declared in bank_backend.h).
// They take their values as parameters instead of cin, and report the outcome
// through a return code instead of cout. Everything else - loading, matching
// the account number, the "amount <= balance" rule, and saving - is the same
// code the console menu above uses.
// ===========================================================================

// Account-number match used by the functions below (same comparison as the console loops).
int FindClientIndex(const vector<sClients>& Vclient, const string& AccountNumber)
{
	for (int i = 0; i < Vclient.size(); i++) {
		if (AccountNumber == Vclient[i].AccountNumber)
			return i;
	}
	return -1;
}

enOpResult AddClientRecord(vector<sClients>& Vclient, const sClients& NewClient)
{
	Vclient = LoadDataFromFile("Bank.txt");

	if (IsClientExist(Vclient, NewClient.AccountNumber))   // original uniqueness check
		return OpAlreadyExists;

	Vclient.push_back(NewClient);
	Vclient.back().MarkForDeletion = false;

	SaveLastClientToFile(Vclient);
	return OpOk;
}

enOpResult FindClientRecord(vector<sClients>& Vclient, const string& AccountNumber, sClients& Found)
{
	Vclient = LoadDataFromFile("Bank.txt");

	int i = FindClientIndex(Vclient, AccountNumber);
	if (i < 0)
		return OpNotFound;

	Found = Vclient[i];
	return OpOk;
}

enOpResult DeleteClientRecord(vector<sClients>& Vclient, const string& AccountNumber)
{
	Vclient = LoadDataFromFile("Bank.txt");

	bool found = false;
	for (int i = 0; i < Vclient.size(); i++) {
		if (AccountNumber == Vclient[i].AccountNumber) {
			Vclient[i].MarkForDeletion = true;
			found = true;
		}
	}
	if (!found)
		return OpNotFound;

	SaveClientsNotMarkedForDeletion(Vclient);
	return OpOk;
}

enOpResult UpdateClientRecord(vector<sClients>& Vclient, const string& AccountNumber,
	const string& PinCode, const string& Name, double Balance)
{
	Vclient = LoadDataFromFile("Bank.txt");

	int i = FindClientIndex(Vclient, AccountNumber);
	if (i < 0)
		return OpNotFound;

	ApplyClientUpdate(Vclient, i, PinCode, Name, Balance);
	return OpOk;
}

enOpResult DepositToClient(vector<sClients>& Vclient, const string& AccountNumber,
	double Amount, double& NewBalance)
{
	Vclient = LoadDataFromFile("Bank.txt");

	int i = FindClientIndex(Vclient, AccountNumber);
	if (i < 0)
		return OpNotFound;

	ApplyDeposit(Vclient, i, Amount);
	NewBalance = Vclient[i].Balance;
	return OpOk;
}

enOpResult WithdrawFromClient(vector<sClients>& Vclient, const string& AccountNumber,
	double Amount, double& NewBalance)
{
	Vclient = LoadDataFromFile("Bank.txt");

	int i = FindClientIndex(Vclient, AccountNumber);
	if (i < 0)
		return OpNotFound;

	if (!HasSufficientBalance(Vclient, i, Amount))
		return OpInsufficientFunds;

	ApplyWithdraw(Vclient, i, Amount);
	NewBalance = Vclient[i].Balance;
	return OpOk;
}

// The Qt project defines BANK_GUI, so its own main() (in main.cpp) is used.
// Build this file without BANK_GUI and you get the original console program.
#ifndef BANK_GUI
int main()
{

	Vclient = LoadDataFromFile("Bank.txt");
	ShowMainMenu();
	system("pause>0");
	return 0;
}
#endif

