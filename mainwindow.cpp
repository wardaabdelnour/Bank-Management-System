#include "mainwindow.h"
#include "bank_backend.h"

#include <QButtonGroup>
#include <QDir>
#include <QFontMetrics>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <cmath>
#include <exception>
#include <string>

namespace {

const char *kBankFile = "Bank.txt";

enum Page { PageClients = 0, PageAdd, PageDelete, PageUpdate, PageFind, PageTransactions };

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------
QString toQ(const std::string &s) { return QString::fromStdString(s); }
std::string toStd(const QString &s) { return s.toStdString(); }

QString money(double v)
{
	return QLocale(QLocale::English).toString(v, 'f', 2);
}

// Text that ends up in Bank.txt must not break the "#//#" / one-line-per-client format.
bool validText(const QString &value, const QString &what, QString &err)
{
	if (value.isEmpty()) {
		err = what + " is required.";
		return false;
	}
	if (value.contains(QStringLiteral("#//#")) || value.contains(QLatin1Char('\n')) || value.contains(QLatin1Char('\r'))) {
		err = what + " cannot contain \"#//#\" or line breaks.";
		return false;
	}
	return true;
}

bool parseMoney(const QString &raw, const QString &what, bool allowZero, double &out, QString &err)
{
	const QString text = raw.trimmed();
	if (text.isEmpty()) {
		err = what + " is required.";
		return false;
	}

	QLocale c = QLocale::c();
	c.setNumberOptions(QLocale::RejectGroupSeparator);
	bool ok = false;
	const double v = c.toDouble(text, &ok);
	if (!ok || !std::isfinite(v)) {
		err = what + " must be a valid number, for example 250 or 250.75.";
		return false;
	}
	if (v < 0 || (!allowZero && v == 0)) {
		err = allowZero ? what + " cannot be negative." : what + " must be greater than zero.";
		return false;
	}
	if (v > 1e12) {
		err = what + " is too large.";
		return false;
	}
	if (std::fabs(v * 100.0 - std::round(v * 100.0)) > 1e-6) {
		err = what + " can have at most two decimal places.";
		return false;
	}
	out = v;
	return true;
}

QPushButton *makeButton(const QString &text, const char *role)
{
	auto *b = new QPushButton(text);
	b->setObjectName(QString::fromLatin1(role));
	b->setCursor(Qt::PointingHandCursor);
	return b;
}

QLabel *makeLabel(const QString &text, const char *objectName)
{
	auto *l = new QLabel(text);
	l->setObjectName(QString::fromLatin1(objectName));
	return l;
}

QLineEdit *addField(QFormLayout *form, const QString &label, const QString &placeholder)
{
	auto *edit = new QLineEdit;
	edit->setPlaceholderText(placeholder);
	edit->setMaximumWidth(420);
	form->addRow(makeLabel(label, "fieldLabel"), edit);
	return edit;
}

QVBoxLayout *startPage(QWidget *page, const QString &title, const QString &hint)
{
	auto *lay = new QVBoxLayout(page);
	lay->setContentsMargins(32, 26, 32, 26);
	lay->setSpacing(14);
	lay->addWidget(makeLabel(title, "pageTitle"));
	auto *h = makeLabel(hint, "pageHint");
	h->setWordWrap(true);
	lay->addWidget(h);
	lay->addSpacing(6);
	return lay;
}

QFormLayout *makeForm()
{
	auto *form = new QFormLayout;
	form->setHorizontalSpacing(28);
	form->setVerticalSpacing(14);
	form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	return form;
}

QTableWidget *makeTable(const QStringList &headers)
{
	auto *t = new QTableWidget(0, headers.size());
	t->setHorizontalHeaderLabels(headers);
	t->setEditTriggers(QAbstractItemView::NoEditTriggers);
	t->setSelectionBehavior(QAbstractItemView::SelectRows);
	t->setSelectionMode(QAbstractItemView::SingleSelection);
	t->setAlternatingRowColors(true);
	t->setShowGrid(false);
	t->setFocusPolicy(Qt::NoFocus);
	t->verticalHeader()->setVisible(false);
	t->verticalHeader()->setDefaultSectionSize(38);
	t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	t->horizontalHeader()->setHighlightSections(false);
	t->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	return t;
}

QTableWidgetItem *cell(const QString &text, bool rightAligned = false)
{
	auto *item = new QTableWidgetItem(text);
	item->setTextAlignment((rightAligned ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter);
	return item;
}

const char *kStyleSheet = R"QSS(
* { font-family: "Segoe UI", "Helvetica Neue", Arial, sans-serif; font-size: 10pt; color: #222B36; }
QMainWindow, #root { background: #F5F3EE; }

#header { background: #FFFFFF; border-bottom: 1px solid #E3E1DB; }
#appTitle { font-size: 17pt; font-weight: bold; color: #14284B; }
#appSubtitle { color: #6B7280; }

#sidebar { background: #FFFFFF; border: 1px solid #E3E1DB; border-radius: 10px; }
#sideCaption { color: #8A8F98; font-size: 8pt; }
#sidePath { color: #6B7280; font-size: 8pt; }
QPushButton#navButton { text-align: left; padding: 10px 14px; border: none; border-radius: 8px; background: transparent; color: #33415C; }
QPushButton#navButton:hover { background: #EEF2F7; }
QPushButton#navButton:checked { background: #14284B; color: #FFFFFF; font-weight: bold; }
QPushButton#exitButton { text-align: left; padding: 10px 14px; border: none; border-radius: 8px; background: transparent; color: #9F3A32; }
QPushButton#exitButton:hover { background: #F8ECEA; }

#card { background: #FFFFFF; border: 1px solid #E3E1DB; border-radius: 10px; }
#pageTitle { font-size: 15pt; font-weight: bold; color: #14284B; }
#pageHint { color: #6B7280; }
#fieldLabel { font-weight: bold; color: #33415C; }
#sectionLabel { font-weight: bold; color: #33415C; margin-top: 6px; }
#accountHint { color: #2F7F86; }
#totalLabel { font-size: 13pt; font-weight: bold; color: #14284B; }
#countLabel { color: #6B7280; }

QLineEdit { background: #FFFFFF; border: 1px solid #D5D2CA; border-radius: 6px; padding: 8px 10px; selection-background-color: #2F7F86; selection-color: #FFFFFF; }
QLineEdit:focus { border: 1px solid #2F7F86; }
QLineEdit:disabled { background: #F2F1EC; color: #9AA0A8; }

QPushButton#primary { background: #14284B; color: #FFFFFF; border: none; border-radius: 6px; padding: 10px 24px; font-weight: bold; }
QPushButton#primary:hover { background: #1D3A6B; }
QPushButton#primary:disabled { background: #C9CED8; color: #F5F6F8; }
QPushButton#secondary { background: #FFFFFF; color: #14284B; border: 1px solid #C5CAD6; border-radius: 6px; padding: 9px 18px; }
QPushButton#secondary:hover { background: #F1F4F8; }
QPushButton#danger { background: #9F3A32; color: #FFFFFF; border: none; border-radius: 6px; padding: 10px 24px; font-weight: bold; }
QPushButton#danger:hover { background: #B3453C; }
QPushButton#danger:disabled { background: #E3CFCD; color: #FBF6F5; }

#summary { background: #FAF9F6; border: 1px solid #E3E1DB; border-radius: 8px; }
#summaryKey { color: #6B7280; }
#summaryValue { font-weight: bold; color: #14284B; }

QTableWidget { background: #FFFFFF; alternate-background-color: #F8F7F3; border: 1px solid #E3E1DB; border-radius: 8px; selection-background-color: #DCEBEC; selection-color: #14284B; }
QTableWidget::item { padding: 0 10px; border: none; }
QHeaderView::section { background: #F1EFE9; color: #33415C; border: none; border-bottom: 1px solid #E3E1DB; padding: 9px 10px; font-weight: bold; }

QTabWidget::pane { border: none; border-top: 1px solid #E3E1DB; }
QTabBar::tab { padding: 10px 22px; background: transparent; border: none; border-bottom: 2px solid transparent; color: #6B7280; }
QTabBar::tab:hover { color: #14284B; }
QTabBar::tab:selected { color: #14284B; font-weight: bold; border-bottom: 2px solid #14284B; }

QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
QScrollBar::handle:vertical { background: #D5D2CA; border-radius: 4px; min-height: 28px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
)QSS";

} // namespace

// ---------------------------------------------------------------------------
// Read-only "client card" used by the Delete and Find pages.
// ---------------------------------------------------------------------------
class ClientSummary : public QFrame
{
public:
	explicit ClientSummary(QWidget *parent = nullptr) : QFrame(parent)
	{
		setObjectName(QStringLiteral("summary"));
		auto *grid = new QGridLayout(this);
		grid->setContentsMargins(20, 16, 20, 16);
		grid->setHorizontalSpacing(32);
		grid->setVerticalSpacing(10);
		grid->setColumnStretch(1, 1);

		const char *keys[4] = { "Account Number", "PIN Code", "Name", "Balance" };
		for (int i = 0; i < 4; ++i) {
			grid->addWidget(makeLabel(QString::fromLatin1(keys[i]), "summaryKey"), i, 0);
			m_values[i] = makeLabel(QString(), "summaryValue");
			m_values[i]->setTextInteractionFlags(Qt::TextSelectableByMouse);
			grid->addWidget(m_values[i], i, 1);
		}
		clear();
		setMaximumWidth(520);
	}

	void set(const sClients &c)
	{
		m_values[0]->setText(toQ(c.AccountNumber));
		m_values[1]->setText(toQ(c.PinCode));
		m_values[2]->setText(toQ(c.Name));
		m_values[3]->setText(money(c.Balance));
	}

	void clear()
	{
		for (auto *v : m_values)
			v->setText(QStringLiteral("\u2014"));
	}

private:
	QLabel *m_values[4];
};

// ---------------------------------------------------------------------------
// MainWindow
// ---------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	setWindowTitle(QStringLiteral("Bank Management System"));
	setStyleSheet(QString::fromLatin1(kStyleSheet));
	resize(1120, 740);
	setMinimumSize(980, 660);

	auto *root = new QWidget;
	root->setObjectName(QStringLiteral("root"));
	auto *rootLayout = new QVBoxLayout(root);
	rootLayout->setContentsMargins(0, 0, 0, 0);
	rootLayout->setSpacing(0);

	rootLayout->addWidget(buildHeader());

	auto *body = new QHBoxLayout;
	body->setContentsMargins(24, 22, 24, 12);
	body->setSpacing(20);

	body->addWidget(buildSidebar());

	// Content card with the stacked pages
	auto *card = new QFrame;
	card->setObjectName(QStringLiteral("card"));
	auto *cardLayout = new QVBoxLayout(card);
	cardLayout->setContentsMargins(0, 0, 0, 0);

	m_stack = new QStackedWidget;
	m_stack->addWidget(buildClientsPage());       // PageClients
	m_stack->addWidget(buildAddPage());           // PageAdd
	m_stack->addWidget(buildDeletePage());        // PageDelete
	m_stack->addWidget(buildUpdatePage());        // PageUpdate
	m_stack->addWidget(buildFindPage());          // PageFind
	m_stack->addWidget(buildTransactionsPage());  // PageTransactions
	cardLayout->addWidget(m_stack);
	body->addWidget(card, 1);

	rootLayout->addLayout(body, 1);

	// Status / feedback area
	m_status = new QLabel;
	m_status->setMinimumHeight(42);
	auto *statusRow = new QHBoxLayout;
	statusRow->setContentsMargins(24, 0, 24, 18);
	statusRow->addWidget(m_status);
	rootLayout->addLayout(statusRow);

	setCentralWidget(root);
	showPage(PageClients);
}

QWidget *MainWindow::buildHeader()
{
	auto *header = new QFrame;
	header->setObjectName(QStringLiteral("header"));
	auto *lay = new QVBoxLayout(header);
	lay->setContentsMargins(32, 16, 32, 14);
	lay->setSpacing(2);
	lay->addWidget(makeLabel(QStringLiteral("Bank Management System"), "appTitle"));
	lay->addWidget(makeLabel(QStringLiteral("Client & Transaction Management"), "appSubtitle"));
	return header;
}

QWidget *MainWindow::buildSidebar()
{
	auto *side = new QFrame;
	side->setObjectName(QStringLiteral("sidebar"));
	side->setFixedWidth(230);
	auto *lay = new QVBoxLayout(side);
	lay->setContentsMargins(12, 14, 12, 14);
	lay->setSpacing(4);

	const QStringList names = {
		QStringLiteral("Clients List"), QStringLiteral("Add Client"), QStringLiteral("Delete Client"),
		QStringLiteral("Update Client"), QStringLiteral("Find Client"), QStringLiteral("Transactions")
	};

	m_navGroup = new QButtonGroup(this);
	m_navGroup->setExclusive(true);
	for (int i = 0; i < names.size(); ++i) {
		auto *b = new QPushButton(names.at(i));
		b->setObjectName(QStringLiteral("navButton"));
		b->setCheckable(true);
		b->setCursor(Qt::PointingHandCursor);
		m_navGroup->addButton(b, i);
		connect(b, &QPushButton::clicked, this, [this, i]() { showPage(i); });
		lay->addWidget(b);
	}

	lay->addStretch(1);

	// Where the backend is reading/writing
	const QString path = QDir::current().absoluteFilePath(QString::fromLatin1(kBankFile));
	lay->addWidget(makeLabel(QStringLiteral("DATA FILE"), "sideCaption"));
	auto *pathLabel = makeLabel(QString(), "sidePath");
	pathLabel->setText(QFontMetrics(pathLabel->font()).elidedText(path, Qt::ElideMiddle, 200));
	pathLabel->setToolTip(path);
	lay->addWidget(pathLabel);
	lay->addSpacing(8);

	auto *exitBtn = new QPushButton(QStringLiteral("Exit"));
	exitBtn->setObjectName(QStringLiteral("exitButton"));
	exitBtn->setCursor(Qt::PointingHandCursor);
	connect(exitBtn, &QPushButton::clicked, this, &QWidget::close);
	lay->addWidget(exitBtn);

	return side;
}

// ---------------------------------------------------------------------------
// Navigation / status
// ---------------------------------------------------------------------------
void MainWindow::showPage(int index)
{
	m_stack->setCurrentIndex(index);
	if (auto *b = m_navGroup->button(index))
		b->setChecked(true);

	setStatus(QString());

	switch (index) {
	case PageClients:
		refreshClientsList();
		break;
	case PageAdd:
		m_addAccount->setFocus();
		break;
	case PageDelete:
		resetDeletePage();
		m_delAccount->setFocus();
		break;
	case PageUpdate:
		resetUpdatePage();
		m_updAccount->setFocus();
		break;
	case PageFind:
		resetFindPage();
		m_findAccount->setFocus();
		break;
	case PageTransactions:
		refreshTotalBalances();
		break;
	}
}

void MainWindow::setStatus(const QString &text, StatusKind kind)
{
	QString color = QStringLiteral("#6B7280");
	QString bg = QStringLiteral("transparent");
	QString border = QStringLiteral("transparent");
	QString shown = text;

	switch (kind) {
	case StatusSuccess:
		color = QStringLiteral("#2E6F49"); bg = QStringLiteral("#EAF4EE"); border = QStringLiteral("#2E7D4F");
		break;
	case StatusError:
		color = QStringLiteral("#8F2F28"); bg = QStringLiteral("#FBEFED"); border = QStringLiteral("#B3382F");
		break;
	case StatusInfo:
		if (shown.isEmpty())
			shown = QStringLiteral("Ready");
		break;
	}

	m_status->setText(shown);
	m_status->setStyleSheet(QStringLiteral(
		"QLabel { color: %1; background: %2; border-left: 4px solid %3; border-radius: 6px; padding: 10px 14px; }")
		.arg(color, bg, border));
}

bool MainWindow::runBackend(const std::function<void()> &fn)
{
	try {
		fn();
		return true;
	} catch (const std::exception &e) {
		setStatus(QStringLiteral("Could not read or write %1 (%2). Please check the file's contents.")
			.arg(QString::fromLatin1(kBankFile), QString::fromLocal8Bit(e.what())), StatusError);
	} catch (...) {
		setStatus(QStringLiteral("Unexpected error while accessing %1.").arg(QString::fromLatin1(kBankFile)), StatusError);
	}
	return false;
}

// ---------------------------------------------------------------------------
// Clients list
// ---------------------------------------------------------------------------
QWidget *MainWindow::buildClientsPage()
{
	auto *page = new QWidget;
	auto *lay = startPage(page, QStringLiteral("Clients List"),
		QStringLiteral("All clients currently stored in Bank.txt."));

	auto *bar = new QHBoxLayout;
	m_clientsCount = makeLabel(QString(), "countLabel");
	bar->addWidget(m_clientsCount);
	bar->addStretch(1);
	auto *refresh = makeButton(QStringLiteral("Refresh"), "secondary");
	connect(refresh, &QPushButton::clicked, this, [this]() {
		setStatus(QStringLiteral("Client list refreshed."), StatusSuccess);   // an error from the reload replaces this
		refreshClientsList();
	});
	bar->addWidget(refresh);
	lay->addLayout(bar);

	m_clientsTable = makeTable({ QStringLiteral("Account Number"), QStringLiteral("PIN Code"),
		QStringLiteral("Name"), QStringLiteral("Balance") });
	lay->addWidget(m_clientsTable, 1);
	return page;
}

void MainWindow::refreshClientsList()
{
	if (!runBackend([]() { Vclient = LoadDataFromFile(kBankFile); }))
		return;

	m_clientsTable->setRowCount(static_cast<int>(Vclient.size()));
	for (int i = 0; i < static_cast<int>(Vclient.size()); ++i) {
		const sClients &c = Vclient[i];
		m_clientsTable->setItem(i, 0, cell(toQ(c.AccountNumber)));
		m_clientsTable->setItem(i, 1, cell(toQ(c.PinCode)));
		m_clientsTable->setItem(i, 2, cell(toQ(c.Name)));
		m_clientsTable->setItem(i, 3, cell(money(c.Balance), true));
	}
	m_clientsCount->setText(QStringLiteral("%1 client(s)").arg(static_cast<int>(Vclient.size())));
}

// ---------------------------------------------------------------------------
// Add client
// ---------------------------------------------------------------------------
QWidget *MainWindow::buildAddPage()
{
	auto *page = new QWidget;
	auto *lay = startPage(page, QStringLiteral("Add Client"),
		QStringLiteral("Enter the new client's details. The account number must not already exist."));

	auto *form = makeForm();
	m_addAccount = addField(form, QStringLiteral("Account Number"), QStringLiteral("e.g. A1001"));
	m_addPin = addField(form, QStringLiteral("PIN Code"), QStringLiteral("e.g. 4821"));
	m_addName = addField(form, QStringLiteral("Client Name"), QStringLiteral("Full name"));
	m_addBalance = addField(form, QStringLiteral("Balance"), QStringLiteral("0.00"));
	lay->addLayout(form);

	auto *btn = makeButton(QStringLiteral("Add Client"), "primary");
	lay->addSpacing(8);
	lay->addWidget(btn, 0, Qt::AlignLeft);
	lay->addStretch(1);

	connect(btn, &QPushButton::clicked, this, &MainWindow::onAddClient);
	connect(m_addAccount, &QLineEdit::returnPressed, m_addPin, qOverload<>(&QWidget::setFocus));
	connect(m_addPin, &QLineEdit::returnPressed, m_addName, qOverload<>(&QWidget::setFocus));
	connect(m_addName, &QLineEdit::returnPressed, m_addBalance, qOverload<>(&QWidget::setFocus));
	connect(m_addBalance, &QLineEdit::returnPressed, btn, &QPushButton::click);
	return page;
}

void MainWindow::onAddClient()
{
	const QString account = m_addAccount->text().trimmed();
	const QString pin = m_addPin->text().trimmed();
	const QString name = m_addName->text().trimmed();
	QString err;
	double balance = 0;

	if (!validText(account, QStringLiteral("Account number"), err)
		|| !validText(pin, QStringLiteral("PIN code"), err)
		|| !validText(name, QStringLiteral("Client name"), err)
		|| !parseMoney(m_addBalance->text(), QStringLiteral("Balance"), true, balance, err)) {
		setStatus(err, StatusError);
		return;
	}

	sClients client;
	client.AccountNumber = toStd(account);
	client.PinCode = toStd(pin);
	client.Name = toStd(name);
	client.Balance = balance;
	client.MarkForDeletion = false;

	enOpResult result = OpNotFound;
	if (!runBackend([&]() { result = AddClientRecord(Vclient, client); }))
		return;

	if (result == OpAlreadyExists) {
		setStatus(QStringLiteral("Client with [%1] already exists. Please enter another account number.").arg(account), StatusError);
		m_addAccount->setFocus();
		m_addAccount->selectAll();
		return;
	}

	setStatus(QStringLiteral("Client \"%1\" was added successfully.").arg(name), StatusSuccess);
	m_addAccount->clear();
	m_addPin->clear();
	m_addName->clear();
	m_addBalance->clear();
	m_addAccount->setFocus();
}

// ---------------------------------------------------------------------------
// Delete client
// ---------------------------------------------------------------------------
QWidget *MainWindow::buildDeletePage()
{
	auto *page = new QWidget;
	auto *lay = startPage(page, QStringLiteral("Delete Client"),
		QStringLiteral("Load a client by account number, review the details, then delete."));

	auto *row = new QHBoxLayout;
	row->setSpacing(12);
	row->addWidget(makeLabel(QStringLiteral("Account Number"), "fieldLabel"));
	m_delAccount = new QLineEdit;
	m_delAccount->setPlaceholderText(QStringLiteral("e.g. A1001"));
	m_delAccount->setMaximumWidth(300);
	row->addWidget(m_delAccount, 1);
	auto *load = makeButton(QStringLiteral("Load Client"), "secondary");
	row->addWidget(load);
	row->addStretch(2);
	lay->addLayout(row);

	lay->addWidget(makeLabel(QStringLiteral("Selected client"), "sectionLabel"));
	m_delSummary = new ClientSummary;
	lay->addWidget(m_delSummary);

	m_delButton = makeButton(QStringLiteral("Delete Client"), "danger");
	m_delButton->setEnabled(false);
	lay->addSpacing(8);
	lay->addWidget(m_delButton, 0, Qt::AlignLeft);
	lay->addStretch(1);

	connect(load, &QPushButton::clicked, this, &MainWindow::onLoadForDelete);
	connect(m_delAccount, &QLineEdit::returnPressed, load, &QPushButton::click);
	connect(m_delButton, &QPushButton::clicked, this, &MainWindow::onDeleteClient);
	// Editing the account number invalidates the loaded client, so a stale card can never be deleted.
	connect(m_delAccount, &QLineEdit::textChanged, this, [this](const QString &text) {
		if (text.trimmed() != m_delLoadedAccount) {
			m_delLoadedAccount.clear();
			m_delSummary->clear();
			m_delButton->setEnabled(false);
		}
	});
	return page;
}

void MainWindow::resetDeletePage()
{
	m_delAccount->clear();
	m_delLoadedAccount.clear();
	m_delSummary->clear();
	m_delButton->setEnabled(false);
}

void MainWindow::onLoadForDelete()
{
	const QString account = m_delAccount->text().trimmed();
	QString err;
	if (account.isEmpty()) {
		setStatus(QStringLiteral("Account number is required."), StatusError);
		return;
	}

	sClients found;
	enOpResult result = OpNotFound;
	if (!runBackend([&]() { result = FindClientRecord(Vclient, toStd(account), found); }))
		return;

	if (result != OpOk) {
		m_delLoadedAccount.clear();
		m_delSummary->clear();
		m_delButton->setEnabled(false);
		setStatus(QStringLiteral("Client not found: no client has account number [%1].").arg(account), StatusError);
		return;
	}

	m_delLoadedAccount = account;
	m_delSummary->set(found);
	m_delButton->setEnabled(true);
	setStatus(QStringLiteral("Client loaded. Review the details, then click Delete Client."));
}

void MainWindow::onDeleteClient()
{
	if (m_delLoadedAccount.isEmpty())
		return;

	const QString account = m_delLoadedAccount;
	const auto answer = QMessageBox::question(this, QStringLiteral("Delete client"),
		QStringLiteral("Are you sure you want to delete the client with account number [%1]?").arg(account),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
	if (answer != QMessageBox::Yes)
		return;

	enOpResult result = OpNotFound;
	if (!runBackend([&]() { result = DeleteClientRecord(Vclient, toStd(account)); }))
		return;

	if (result != OpOk) {
		setStatus(QStringLiteral("Client [%1] was not found; nothing was deleted.").arg(account), StatusError);
		return;
	}

	resetDeletePage();
	setStatus(QStringLiteral("Client [%1] was deleted successfully.").arg(account), StatusSuccess);
	m_delAccount->setFocus();
}

// ---------------------------------------------------------------------------
// Update client
// ---------------------------------------------------------------------------
QWidget *MainWindow::buildUpdatePage()
{
	auto *page = new QWidget;
	auto *lay = startPage(page, QStringLiteral("Update Client"),
		QStringLiteral("Load a client by account number, edit the details below, then save the changes."));

	auto *row = new QHBoxLayout;
	row->setSpacing(12);
	row->addWidget(makeLabel(QStringLiteral("Account Number"), "fieldLabel"));
	m_updAccount = new QLineEdit;
	m_updAccount->setPlaceholderText(QStringLiteral("e.g. A1001"));
	m_updAccount->setMaximumWidth(300);
	row->addWidget(m_updAccount, 1);
	auto *load = makeButton(QStringLiteral("Load Client"), "secondary");
	row->addWidget(load);
	row->addStretch(2);
	lay->addLayout(row);

	lay->addWidget(makeLabel(QStringLiteral("Client details"), "sectionLabel"));
	auto *form = makeForm();
	m_updPin = addField(form, QStringLiteral("PIN Code"), QString());
	m_updName = addField(form, QStringLiteral("Client Name"), QString());
	m_updBalance = addField(form, QStringLiteral("Balance"), QString());
	lay->addLayout(form);

	m_updButton = makeButton(QStringLiteral("Update Client"), "primary");
	lay->addSpacing(8);
	lay->addWidget(m_updButton, 0, Qt::AlignLeft);
	lay->addStretch(1);

	connect(load, &QPushButton::clicked, this, &MainWindow::onLoadForUpdate);
	connect(m_updAccount, &QLineEdit::returnPressed, load, &QPushButton::click);
	connect(m_updButton, &QPushButton::clicked, this, &MainWindow::onUpdateClient);
	connect(m_updBalance, &QLineEdit::returnPressed, m_updButton, &QPushButton::click);
	connect(m_updPin, &QLineEdit::returnPressed, m_updName, qOverload<>(&QWidget::setFocus));
	connect(m_updName, &QLineEdit::returnPressed, m_updBalance, qOverload<>(&QWidget::setFocus));
	// Editing the account number drops the loaded client so the wrong record can't be overwritten.
	connect(m_updAccount, &QLineEdit::textChanged, this, [this](const QString &text) {
		if (text.trimmed() != m_updLoadedAccount) {
			m_updLoadedAccount.clear();
			m_updPin->clear();
			m_updName->clear();
			m_updBalance->clear();
			m_updPin->setEnabled(false);
			m_updName->setEnabled(false);
			m_updBalance->setEnabled(false);
			m_updButton->setEnabled(false);
		}
	});
	return page;
}

void MainWindow::resetUpdatePage()
{
	m_updAccount->clear();
	m_updLoadedAccount.clear();
	m_updPin->clear();
	m_updName->clear();
	m_updBalance->clear();
	m_updPin->setEnabled(false);
	m_updName->setEnabled(false);
	m_updBalance->setEnabled(false);
	m_updButton->setEnabled(false);
}

void MainWindow::onLoadForUpdate()
{
	const QString account = m_updAccount->text().trimmed();
	if (account.isEmpty()) {
		setStatus(QStringLiteral("Account number is required."), StatusError);
		return;
	}

	sClients found;
	enOpResult result = OpNotFound;
	if (!runBackend([&]() { result = FindClientRecord(Vclient, toStd(account), found); }))
		return;

	if (result != OpOk) {
		m_updLoadedAccount.clear();
		m_updPin->clear();
		m_updName->clear();
		m_updBalance->clear();
		m_updPin->setEnabled(false);
		m_updName->setEnabled(false);
		m_updBalance->setEnabled(false);
		m_updButton->setEnabled(false);
		setStatus(QStringLiteral("Client not found: no client has account number [%1].").arg(account), StatusError);
		return;
	}

	m_updLoadedAccount = account;
	m_updPin->setText(toQ(found.PinCode));
	m_updName->setText(toQ(found.Name));
	m_updBalance->setText(QString::number(found.Balance, 'f', 2));
	m_updPin->setEnabled(true);
	m_updName->setEnabled(true);
	m_updBalance->setEnabled(true);
	m_updButton->setEnabled(true);
	setStatus(QStringLiteral("Client loaded. Edit the details, then click Update Client."));
	m_updPin->setFocus();
}

void MainWindow::onUpdateClient()
{
	if (m_updLoadedAccount.isEmpty())
		return;

	const QString pin = m_updPin->text().trimmed();
	const QString name = m_updName->text().trimmed();
	QString err;
	double balance = 0;
	if (!validText(pin, QStringLiteral("PIN code"), err)
		|| !validText(name, QStringLiteral("Client name"), err)
		|| !parseMoney(m_updBalance->text(), QStringLiteral("Balance"), true, balance, err)) {
		setStatus(err, StatusError);
		return;
	}

	enOpResult result = OpNotFound;
	if (!runBackend([&]() {
			result = UpdateClientRecord(Vclient, toStd(m_updLoadedAccount), toStd(pin), toStd(name), balance);
		}))
		return;

	if (result != OpOk) {
		setStatus(QStringLiteral("Client [%1] was not found; nothing was updated.").arg(m_updLoadedAccount), StatusError);
		return;
	}

	m_updBalance->setText(QString::number(balance, 'f', 2));
	setStatus(QStringLiteral("Client [%1] updated successfully!").arg(m_updLoadedAccount), StatusSuccess);
}

// ---------------------------------------------------------------------------
// Find client
// ---------------------------------------------------------------------------
QWidget *MainWindow::buildFindPage()
{
	auto *page = new QWidget;
	auto *lay = startPage(page, QStringLiteral("Find Client"),
		QStringLiteral("Search for a client by account number."));

	auto *row = new QHBoxLayout;
	row->setSpacing(12);
	row->addWidget(makeLabel(QStringLiteral("Account Number"), "fieldLabel"));
	m_findAccount = new QLineEdit;
	m_findAccount->setPlaceholderText(QStringLiteral("e.g. A1001"));
	m_findAccount->setMaximumWidth(300);
	row->addWidget(m_findAccount, 1);
	auto *search = makeButton(QStringLiteral("Search"), "primary");
	row->addWidget(search);
	row->addStretch(2);
	lay->addLayout(row);

	lay->addWidget(makeLabel(QStringLiteral("Result"), "sectionLabel"));
	m_findSummary = new ClientSummary;
	lay->addWidget(m_findSummary);
	lay->addStretch(1);

	connect(search, &QPushButton::clicked, this, &MainWindow::onFindClient);
	connect(m_findAccount, &QLineEdit::returnPressed, search, &QPushButton::click);
	return page;
}

void MainWindow::resetFindPage()
{
	m_findAccount->clear();
	m_findSummary->clear();
}

void MainWindow::onFindClient()
{
	const QString account = m_findAccount->text().trimmed();
	if (account.isEmpty()) {
		setStatus(QStringLiteral("Account number is required."), StatusError);
		return;
	}

	sClients found;
	enOpResult result = OpNotFound;
	if (!runBackend([&]() { result = FindClientRecord(Vclient, toStd(account), found); }))
		return;

	if (result != OpOk) {
		m_findSummary->clear();
		setStatus(QStringLiteral("Client not found: no client has account number [%1].").arg(account), StatusError);
		return;
	}

	m_findSummary->set(found);
	setStatus(QStringLiteral("Client found."), StatusSuccess);
}

// ---------------------------------------------------------------------------
// Transactions
// ---------------------------------------------------------------------------
QWidget *MainWindow::buildTransactionsPage()
{
	auto *page = new QWidget;
	auto *lay = new QVBoxLayout(page);
	lay->setContentsMargins(32, 26, 32, 26);
	lay->setSpacing(14);
	lay->addWidget(makeLabel(QStringLiteral("Transactions"), "pageTitle"));

	m_txTabs = new QTabWidget;
	m_txTabs->setDocumentMode(true);
	lay->addWidget(m_txTabs, 1);

	// --- Deposit ---
	{
		auto *tab = new QWidget;
		auto *t = new QVBoxLayout(tab);
		t->setContentsMargins(0, 20, 0, 0);
		t->setSpacing(14);
		auto *hint = makeLabel(QStringLiteral("Add money to a client's account."), "pageHint");
		t->addWidget(hint);
		auto *form = makeForm();
		m_depAccount = addField(form, QStringLiteral("Account Number"), QStringLiteral("e.g. A1001"));
		m_depAmount = addField(form, QStringLiteral("Deposit Amount"), QStringLiteral("0.00"));
		t->addLayout(form);
		m_depHint = makeLabel(QString(), "accountHint");
		t->addWidget(m_depHint);
		auto *btn = makeButton(QStringLiteral("Deposit"), "primary");
		t->addWidget(btn, 0, Qt::AlignLeft);
		t->addStretch(1);
		m_txTabs->addTab(tab, QStringLiteral("Deposit"));

		connect(btn, &QPushButton::clicked, this, &MainWindow::onDeposit);
		connect(m_depAccount, &QLineEdit::editingFinished, this, [this]() { showAccountHint(m_depAccount, m_depHint); });
		connect(m_depAccount, &QLineEdit::returnPressed, m_depAmount, qOverload<>(&QWidget::setFocus));
		connect(m_depAmount, &QLineEdit::returnPressed, btn, &QPushButton::click);
	}

	// --- Withdraw ---
	{
		auto *tab = new QWidget;
		auto *t = new QVBoxLayout(tab);
		t->setContentsMargins(0, 20, 0, 0);
		t->setSpacing(14);
		auto *hint = makeLabel(QStringLiteral("Take money out of a client's account. The amount cannot exceed the balance."), "pageHint");
		t->addWidget(hint);
		auto *form = makeForm();
		m_wdAccount = addField(form, QStringLiteral("Account Number"), QStringLiteral("e.g. A1001"));
		m_wdAmount = addField(form, QStringLiteral("Withdrawal Amount"), QStringLiteral("0.00"));
		t->addLayout(form);
		m_wdHint = makeLabel(QString(), "accountHint");
		t->addWidget(m_wdHint);
		auto *btn = makeButton(QStringLiteral("Withdraw"), "primary");
		t->addWidget(btn, 0, Qt::AlignLeft);
		t->addStretch(1);
		m_txTabs->addTab(tab, QStringLiteral("Withdraw"));

		connect(btn, &QPushButton::clicked, this, &MainWindow::onWithdraw);
		connect(m_wdAccount, &QLineEdit::editingFinished, this, [this]() { showAccountHint(m_wdAccount, m_wdHint); });
		connect(m_wdAccount, &QLineEdit::returnPressed, m_wdAmount, qOverload<>(&QWidget::setFocus));
		connect(m_wdAmount, &QLineEdit::returnPressed, btn, &QPushButton::click);
	}

	// --- Total balances ---
	{
		auto *tab = new QWidget;
		auto *t = new QVBoxLayout(tab);
		t->setContentsMargins(0, 20, 0, 0);
		t->setSpacing(12);

		auto *bar = new QHBoxLayout;
		bar->addWidget(makeLabel(QStringLiteral("Balance of every client and the combined total."), "pageHint"));
		bar->addStretch(1);
		auto *refresh = makeButton(QStringLiteral("Refresh"), "secondary");
		bar->addWidget(refresh);
		t->addLayout(bar);

		m_totalTable = makeTable({ QStringLiteral("Account Number"), QStringLiteral("Name"), QStringLiteral("Balance") });
		t->addWidget(m_totalTable, 1);

		auto *totalRow = new QHBoxLayout;
		totalRow->addStretch(1);
		m_totalLabel = makeLabel(QStringLiteral("Total Balances: 0.00"), "totalLabel");
		totalRow->addWidget(m_totalLabel);
		t->addLayout(totalRow);
		m_txTabs->addTab(tab, QStringLiteral("Total Balances"));

		connect(refresh, &QPushButton::clicked, this, [this]() {
			setStatus(QStringLiteral("Balances refreshed."), StatusSuccess);   // an error from the reload replaces this
			refreshTotalBalances();
		});
	}

	connect(m_txTabs, &QTabWidget::currentChanged, this, [this](int) {
		setStatus(QString());
		refreshTotalBalances();
	});
	return page;
}

void MainWindow::showAccountHint(QLineEdit *accountEdit, QLabel *hint)
{
	const QString account = accountEdit->text().trimmed();
	if (account.isEmpty()) {
		hint->clear();
		return;
	}

	sClients found;
	enOpResult result = OpNotFound;
	if (!runBackend([&]() { result = FindClientRecord(Vclient, toStd(account), found); }))
		return;

	if (result == OpOk)
		hint->setText(QStringLiteral("%1  \u00B7  Current balance: %2").arg(toQ(found.Name), money(found.Balance)));
	else
		hint->setText(QStringLiteral("No client has account number [%1].").arg(account));
}

void MainWindow::onDeposit()
{
	const QString account = m_depAccount->text().trimmed();
	QString err;
	double amount = 0;
	if (!validText(account, QStringLiteral("Account number"), err)
		|| !parseMoney(m_depAmount->text(), QStringLiteral("Deposit amount"), false, amount, err)) {
		setStatus(err, StatusError);
		return;
	}

	enOpResult result = OpNotFound;
	double newBalance = 0;
	if (!runBackend([&]() { result = DepositToClient(Vclient, toStd(account), amount, newBalance); }))
		return;

	if (result != OpOk) {
		setStatus(QStringLiteral("Account number [%1] was not found.").arg(account), StatusError);
		return;
	}

	setStatus(QStringLiteral("Deposit done successfully! New balance = %1").arg(money(newBalance)), StatusSuccess);
	m_depAmount->clear();
	showAccountHint(m_depAccount, m_depHint);
}

void MainWindow::onWithdraw()
{
	const QString account = m_wdAccount->text().trimmed();
	QString err;
	double amount = 0;
	if (!validText(account, QStringLiteral("Account number"), err)
		|| !parseMoney(m_wdAmount->text(), QStringLiteral("Withdrawal amount"), false, amount, err)) {
		setStatus(err, StatusError);
		return;
	}

	enOpResult result = OpNotFound;
	double newBalance = 0;
	if (!runBackend([&]() { result = WithdrawFromClient(Vclient, toStd(account), amount, newBalance); }))
		return;

	if (result == OpNotFound) {
		setStatus(QStringLiteral("Account number [%1] was not found.").arg(account), StatusError);
		return;
	}
	if (result == OpInsufficientFunds) {
		setStatus(QStringLiteral("The withdraw amount exceeds the client balance."), StatusError);
		return;
	}

	setStatus(QStringLiteral("Withdraw done successfully! New balance = %1").arg(money(newBalance)), StatusSuccess);
	m_wdAmount->clear();
	showAccountHint(m_wdAccount, m_wdHint);
}

void MainWindow::refreshTotalBalances()
{
	if (!runBackend([]() { Vclient = LoadDataFromFile(kBankFile); }))
		return;

	m_totalTable->setRowCount(static_cast<int>(Vclient.size()));
	for (int i = 0; i < static_cast<int>(Vclient.size()); ++i) {
		const sClients &c = Vclient[i];
		m_totalTable->setItem(i, 0, cell(toQ(c.AccountNumber)));
		m_totalTable->setItem(i, 1, cell(toQ(c.Name)));
		m_totalTable->setItem(i, 2, cell(money(c.Balance), true));
	}

	const double total = CalculateTotalBalances(Vclient);   // computed by the backend
	m_totalLabel->setText(QStringLiteral("Total Balances: %1").arg(money(total)));
}
