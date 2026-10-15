#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStringList>

#include "mainwindow.h"

// The backend opens "Bank.txt" by its relative name (exactly as the original
// console program does), so it reads and writes the current working directory.
// Qt Creator starts the program in the *build* folder, which is rarely where
// Bank.txt lives, so before the window opens we switch the working directory
// to the first folder that already contains a Bank.txt:
//   1. the current working directory
//   2. the folder of the .exe, then up to 4 parent folders - and, at every one
//      of those levels, its direct sub-folders (the project folder normally
//      sits next to the build folder)
// If none has one, the working directory is kept and the backend creates
// Bank.txt there the first time a client is added.
static bool useIfContainsBankFile(const QDir &dir)
{
	if (!dir.exists(QStringLiteral("Bank.txt")))
		return false;
	QDir::setCurrent(dir.absolutePath());
	return true;
}

static void useFolderContainingBankFile()
{
	if (QFileInfo::exists(QStringLiteral("Bank.txt")))
		return;

	QDir dir(QCoreApplication::applicationDirPath());
	for (int level = 0; level < 5; ++level) {
		if (useIfContainsBankFile(dir))
			return;

		const QStringList subFolders = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
		for (const QString &name : subFolders) {
			if (useIfContainsBankFile(QDir(dir.absoluteFilePath(name))))
				return;
		}

		if (!dir.cdUp())
			break;
	}
}

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	app.setApplicationName(QStringLiteral("Bank Management System"));
	QApplication::setStyle(QStringLiteral("Fusion"));   // consistent look on every Windows version

	useFolderContainingBankFile();

	MainWindow window;
	window.show();
	return app.exec();
}
