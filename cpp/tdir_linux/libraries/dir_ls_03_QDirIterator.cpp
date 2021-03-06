/*
#include <QDir>					// Access to directory structures and their contents.
#include <QDirIterator>			// An iterator for directory entrylists.
#include <QTextStream>			// A convenient interface for reading and writing QString text.
#include <QFileInfo>				// System-independent file information.
#include <QDateTime>				// Date and time functions.
//OFF #include <iostream>           // std::cin.get(). //TO DO: Remove it, if unnecessary.
//#include <set>
#include <QMap>

QTextStream out(stdout);				// Interface for writing QString text.
const QString dirPath = "/home/alex/3.1/";

void listFilesInDirectory() {
	 QDir dir(dirPath); //QDir dir("/home/alex/3.1/"); //QDir dir("."); //QDir dir("g:\\"); //QDir dir; //QDir dir("/home/alex"); //ORIG QDir dir; //TO DO: Comment.
	 //QTextStream out(stdout);				// Interface for writing QString text.

	 dir.setSorting(QDir::Name				// Sort by name.
		  | QDir::DirsFirst					// Put the directories first, then the files.
		  | QDir::IgnoreCase					// Sort case-insensitively.
		  | QDir::LocaleAware );			// Sort items appropriately using the current locale settings.

	 dir.setFilter(QDir::Dirs				// List directories that match the filters. //TO DO: Check if directories are needed.
		  | QDir::Files						// List files.
		  | QDir::NoSymLinks					// Do not list symbolic links (ignored by operating systems that don't support symbolic links).
		  | QDir::NoDotAndDotDot			// Do not list the special entries "." and "..".
		  | QDir::System);					// List system files (on Unix, FIFOs, sockets and device files are included; on Windows, .lnk files are included).

	 QStringList filters;					//Set the name filters. Each name filter is a wildcard filter that understands * and ? wildcards.
	 //OFF filters << "test*"; //TO DO: Set it.
	 dir.setNameFilters(filters);

	 QFileInfoList list = dir.entryInfoList();
	 //OFF out << "path/file_name\t file_size\t file_modification_time\n";						// Print the header.
	 for (int i = 0; i < list.size(); ++i) {
		  QFileInfo fileInfo = list.at(i);
		  out << fileInfo.absoluteFilePath() << '\t';												// Print the file path and name.
		  out << fileInfo.size() << '\t';																// Print the size.
		  out << fileInfo.lastModified().toString(Qt::SystemLocaleShortDate) << endl;		//	Print the last modification date in the short format used by the operating system.
	 }
	 out << '\n';
	 //OFF std::cin.get();
}

//void listDirectories(const QDir& dir) {
void listDirectories() {
	QDir dir(dirPath);
	dir.setFilter(QDir::Dirs			// List directories that match the filters. //TO DO: Check if directories are needed.
		 | QDir::Files						// List files.
		 | QDir::NoSymLinks				// Do not list symbolic links (ignored by operating systems that don't support symbolic links).
		 | QDir::NoDotAndDotDot			// Do not list the special entries "." and "..".
		 | QDir::System);					// List system files (on Unix, FIFOs, sockets and device files are included; on Windows, .lnk files are included).

	QDirIterator it(dir, QDirIterator::Subdirectories); // Here's how to find and read all files filtered by name, recursively: @ http://doc.qt.io/qt-5/qdiriterator.html
	while (it.hasNext()) {
		 QFile f(it.next());
		 out << f.fileName() << '\n';
	}
}

void listRecursively() {
	QDir dir(dirPath);
	dir.setFilter(QDir::Dirs			// List directories that match the filters. //TO DO: Check if directories are needed.
		 | QDir::Files						// List files.
		 | QDir::NoSymLinks				// Do not list symbolic links (ignored by operating systems that don't support symbolic links).
		 | QDir::NoDotAndDotDot			// Do not list the special entries "." and "..".
		 | QDir::System);					// List system files (on Unix, FIFOs, sockets and device files are included; on Windows, .lnk files are included).

	QDirIterator it(dir, QDirIterator::Subdirectories); // Here's how to find and read all files filtered by name, recursively: @ http://doc.qt.io/qt-5/qdiriterator.html
//	while (it.hasNext()) {
//		 QFile f(it.next());
//		 out << f.fileName() << '\n';
//	}

	//QStringList entryList;
	QMap<QString, QString> entryMap;
	while (it.hasNext())	{				// Populate recursevly all the entries from all subdirectories.
		 //out << it.next() << '\n';
		//entryList.append(it.next());
		entryMap.insert(it.next(), it.next());
	}

	foreach (const auto& entry, entryMap) {
		out << entry << '\n';
	}
}

int main() {
//	listFilesInDirectory();
//	listDirectories();
//	listRecursively();
}
*/
