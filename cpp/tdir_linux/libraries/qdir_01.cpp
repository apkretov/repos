#include <QCoreApplication> // A program that lists all the files in the current directory (excluding symbolic links), sorted by size, smallest first: @ http://doc.qt.io/qt-5/qdir.html#examples
#include <QDir>
#include <iostream>

#include <QTextStream> // You forgot to include <QTextStream> or <QFile>. @ https://stackoverflow.com/questions/17115609/c-error-in-qt-has-initializer-but-incomplete-type
#include <QFile>
#include <QDataStream>

using std::cout;
using std::endl;

int main_qdir_01(int argc, char *argv[]) {
	QCoreApplication app(argc, argv);
	//ORIG QDir dir;
	QDir dir("/home/alex");
	dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
	//TEST dir.setFilter(QDir::Dirs);
	dir.setSorting(QDir::Size | QDir::Reversed);
	//TEST dir.setSorting(QDir::Name);

	QFileInfoList list = dir.entryInfoList();
	cout << "qdir_01\n\n";
	cout << "A program that lists all the files in the current directory (excluding symbolic links), sorted by size, smallest first" << endl;
	cout << "     Bytes Filename" << endl;
	for (int i = 0; i < list.size(); ++i) {
		QFileInfo fileInfo = list.at(i);
		cout << qPrintable(QString("%1 %2").arg(fileInfo.size(), 10).arg(fileInfo.fileName()));
		cout << endl;
	}
	return 0;
}
