#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <functional>

class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class QButtonGroup;
class QTabWidget;
class ClientSummary;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);

private slots:
	void showPage(int index);

	// Clients list
	void refreshClientsList();

	// Add
	void onAddClient();

	// Delete
	void onLoadForDelete();
	void onDeleteClient();

	// Update
	void onLoadForUpdate();
	void onUpdateClient();

	// Find
	void onFindClient();

	// Transactions
	void onDeposit();
	void onWithdraw();
	void refreshTotalBalances();

private:
	enum StatusKind { StatusInfo, StatusSuccess, StatusError };

	// Page builders
	QWidget *buildClientsPage();
	QWidget *buildAddPage();
	QWidget *buildDeletePage();
	QWidget *buildUpdatePage();
	QWidget *buildFindPage();
	QWidget *buildTransactionsPage();

	QWidget *buildSidebar();
	QWidget *buildHeader();

	void setStatus(const QString &text, StatusKind kind = StatusInfo);
	bool runBackend(const std::function<void()> &fn);   // runs a backend call, reports exceptions in the status bar
	void resetDeletePage();
	void resetUpdatePage();
	void resetFindPage();
	void showAccountHint(QLineEdit *accountEdit, QLabel *hint);

	// Navigation
	QStackedWidget *m_stack = nullptr;
	QButtonGroup *m_navGroup = nullptr;

	// Status
	QLabel *m_status = nullptr;

	// Clients list
	QTableWidget *m_clientsTable = nullptr;
	QLabel *m_clientsCount = nullptr;

	// Add
	QLineEdit *m_addAccount = nullptr;
	QLineEdit *m_addPin = nullptr;
	QLineEdit *m_addName = nullptr;
	QLineEdit *m_addBalance = nullptr;

	// Delete
	QLineEdit *m_delAccount = nullptr;
	ClientSummary *m_delSummary = nullptr;
	QPushButton *m_delButton = nullptr;
	QString m_delLoadedAccount;

	// Update
	QLineEdit *m_updAccount = nullptr;
	QLineEdit *m_updPin = nullptr;
	QLineEdit *m_updName = nullptr;
	QLineEdit *m_updBalance = nullptr;
	QPushButton *m_updButton = nullptr;
	QString m_updLoadedAccount;

	// Find
	QLineEdit *m_findAccount = nullptr;
	ClientSummary *m_findSummary = nullptr;

	// Transactions
	QTabWidget *m_txTabs = nullptr;
	QLineEdit *m_depAccount = nullptr;
	QLineEdit *m_depAmount = nullptr;
	QLabel *m_depHint = nullptr;
	QLineEdit *m_wdAccount = nullptr;
	QLineEdit *m_wdAmount = nullptr;
	QLabel *m_wdHint = nullptr;
	QTableWidget *m_totalTable = nullptr;
	QLabel *m_totalLabel = nullptr;
};

#endif // MAINWINDOW_H
