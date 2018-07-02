#include <QDir>			// Access to directory structures and their contents.
#include <QDirIterator>	// An iterator for directory entrylists.
#include <QTextStream>	// A convenient interface for reading and writing QString text.
#include <QDateTime>		// Date and time functions.
#include <iostream>		// std::cerr, std::cin.get(). //TO DO: Substitute with Qt.
#include <exception>

void distinguishPathMask(QString&, QString&);
void checkPathAndList(const QString&, const QString&, const bool);
void listRecursively(QDir, const QString&, const bool);

const char* const cmdTdir{"tdir"};	// The tdir command.
const QString maskAll("*");			// All directories/files mask ('*' wildcard).

QTextStream out(stdout);				// Interface for writing QString text.

//*********************************************************************************************************************************************************
// main
// TO DO: Comment.
//*********************************************************************************************************************************************************
int main() {
	try {
		enum struct argumentCount : int {tdir_exit = 1, tdir_path, tdir_path_rkey};	// Acceptable number of arguments: 1) tdir, 2) tdir path/mask, 3) tdir path/mask -r.
		const char* const recurKey{"-r"};															// The recursion key.
		const char* const cmdExit{"exit"};															// The exit command.
		const QString prompt = QDir::toNativeSeparators(QDir::currentPath()) + " >> ";// A prompt with the current directory.

		constexpr unsigned cmdLength{256};							// The command's maximal lengh.
		char line[cmdLength]{};											// The line entered: 'tdir [path/mask] [-r]' or 'exit'.
		char command[cmdLength]{};										// Either of the three arguments might be of the maximal lenght. Reserve a maximal space for them.
		char pathRKey1[cmdLength]{}, pathRKey2[cmdLength]{};	// Either a path/mask or a recursion key (-r) (arguments 2 and 3) can be entered under these variables.
		char redundantArgumnt[cmdLength]{};							// A redundant agrument as an indication of a wrong number of arguments entered.
		bool recursive1{}, recursive2{};								// Respective recursion key flags (for arguments 2 and 3).
		QString path;														// Path to a directory listed.
		QString mask;														// File mask.

		out  << "The " << cmdTdir << " File Listing Utility Program" << endl						//Welcome. Abstract.
			 << "List files:\t'tdir [path/mask] [-r]', 'tdir [-r] [path/mask]'" << endl		//TO DO: Print the maximal length restriction.
			 << "Quit program:\t'" << cmdExit << "'" << endl << endl;

		while (1) {																		// In an endless loop, keep entering 'tdir [path/mask] [-r]' to list files and directories or 'Ctrl+C' to quit.
			out << prompt;																// Print the prompt with the current directory in each cycle.
			out.flush();

			if (fgets(line, sizeof(line), stdin) == nullptr) {				// Enter command arguments.
				perror("Error reading characters entered");
				exit(EXIT_SUCCESS);													// Exit on an error.
			}

			int argumentsRead = sscanf(line, "%s %s %s %s", command, pathRKey1, pathRKey2, redundantArgumnt);  // Get the number of successfully read arguments. //TO DO: Use sscan_s instead.
			if (argumentsRead == EOF || argumentsRead == 0) {						// Input failure.
				fprintf(stderr, "No valid input!\n\n");
				continue;
			} else if (argumentsRead > (int)argumentCount::tdir_path_rkey) {	// Redundant agruments entered.
				fprintf(stderr, "Command not found\n\n");					// Command not found.
				continue;
			}

			if (strcmp(command, cmdExit) == 0)									// The first agrument must be tdir or exit. Check exit first.
				exit(EXIT_SUCCESS);													// Exit.
			else if (strcmp(command, cmdTdir) != 0) {							// Check a tdir command otherwise.
				fprintf(stderr, "%s: command not found\n\n", command);	// Command not found.
				continue;
			}

			switch (argumentsRead) {												// Parse the command line based on the number of arguments successfully read.
			case (int)argumentCount::tdir_exit :								// One argument entered. That must be a tdir command or an exit one.
//				if (strcmp(command, cmdExit) == 0)								// Is it an exit command?
//					exit(EXIT_SUCCESS);
//				else if (strcmp(command, cmdTdir) != 0) {						// Check a tdir command otherwise.
//					fprintf(stderr, "%s: command not found\n\n", command);// Command not found.
//					continue;
//				} else
//					listRecursively(QDir::current(), maskAll, false);		// List without recursion.
				listRecursively(QDir::current(), maskAll, false);			// List without recursion.
				break;

			case (int)argumentCount::tdir_path:									// Two arguments entered. The second argument can be either a path or a recursion key.
				if (strcmp(pathRKey1, recurKey) == 0)							// Check if the second argument is a recursion key or not.
					listRecursively(QDir::current(), maskAll, true);		// The second argument is a recursion key. List recursively.
				else {
					path = QString(pathRKey1);										// The second argument is a path.
					distinguishPathMask(path, mask);								// If included, retrieve a mask from the end of the path.
					checkPathAndList(path, mask, false);						// Check the path and list without recursion.
				}
				break;

			case (int)argumentCount::tdir_path_rkey: {						// All the three arguments entered.
				if (strcmp(pathRKey1, pathRKey2) == 0) {						// The second and the third arguments must not be the same.
					fprintf(stderr, "The second and the third arguments must not be the same!\n\n"); // Bad command.
					continue;
				}

				if (strcmp(pathRKey1, recurKey) == 0)							// Check if the second argument is a recursion key or not.
					recursive1 = true;												// The second argument is a recursion key. List recursively.
				else
					path = QString(pathRKey1);										// The second argument is a path.

				if (strcmp(pathRKey2, recurKey) == 0)							// Check if the third argument is a recursion key or not.
					recursive2 = true;												// The third argument a recursion key. List recursively.
				else
					path = QString(pathRKey2);										// The third argument is a path.

				if (!(recursive1 || recursive2)) {								// Neither the second nor the third argument is a recursion key.
					fprintf(stderr, "Neither the second argument nor the third one is a recursion key!\n\n"); // Bad command. //TO DO: Improve this warning.
					continue;
				}

				distinguishPathMask(path, mask);									// If included, retrieve a mask from the end of the path.
				checkPathAndList(path, mask, true);								// Check the path and list recursively.
				break;
			}
			}
		}
	}
	catch(const std::exception& exc) {											// Catch and dislplay an exception.
		std::cerr << "An exception occured: " << exc.what() << std::endl;
	}
	return 0;
}

//*********************************************************************************************************************************************************
// Distinguish path and mask. Unless the path argument is completely a file mask, retrieve it from the end of the path. A mask can be appended at the end
// of a path after the last right/left slash (Linux: /, Windows: \). A mask must contain wildcards ('*', '?', [, ]). If there is any one of them that is a
// file mask.
// TO DO: More comments on the function agruments and return values.
//*********************************************************************************************************************************************************
void distinguishPathMask(QString& path, QString& mask) {
	const char wildcards[] = {'*', '?', '['};	// The wildcards to detect a file mask.

	int wildcardPos{};
	for (auto& wildcard : wildcards) {			// Find a wildcard to detect a file mask, if any.
		wildcardPos = path.indexOf(wildcard);	// The first wildcard's position.
		if (wildcardPos > -1)
			break;										// The first wildcard found.
	}
	if (wildcardPos == -1) {
		mask = maskAll;								// No wildcard. No mask entered, just set '*'.
		return;
	}

	int slashPos = path.lastIndexOf(QDir::separator(), wildcardPos);	// Find the position of the a slash before the first wildcard.
	if (slashPos == -1) {															// No slash there.
		mask = path;																	// The whole path is a mask.
		path = QDir::currentPath();												// The path is supposed to be the current directory.
	} else {
		mask = path.mid(slashPos + 1);											// Retrieve a mask that is the section of the path after the last slash.
		path = path.left(slashPos);												// Cut the mask from the path.
	}
}

//*********************************************************************************************************************************************************
// Check a path and list files and directories, if a path exists.
// TO DO: More comments on the function agruments.
//*********************************************************************************************************************************************************
void checkPathAndList(const QString& path, const QString& mask, const bool recursive) {
	QDir dir(path);
	if	(dir.exists())													// Validate a path if it exists.
		listRecursively(dir, mask, recursive);					// List.
	else																	// No directory.
		fprintf(stderr, "%s: cannot access '%s': No such file or directory\n\n", cmdTdir, path.toStdString().c_str());
}

//*********************************************************************************************************************************************************
// List files recursively in the selected directory and all its subdirectories, if necessary.
// TO DO: More comments on the function agruments.
//*********************************************************************************************************************************************************
void listRecursively(QDir dir, const QString& mask, const bool recursive) { //TO DO: Arrange a break-out with a thread.
	enum struct fieldWidth : int {size = 15, modificationDate = 20};	// Field widths for printing.
	const QString dirMark{"<DIR>"};											// A directory mark.

	dir.setSorting(QDir::Name					// Sort by name.
						| QDir::DirsFirst			// Put the directories first, then the files.
						| QDir::IgnoreCase		// Sort case-insensitively.
						| QDir::LocaleAware );	// Sort items appropriately using the current locale settings.

	dir.setFilter(QDir::Dirs					// List directories that match the filters.
					  | QDir::Files				// List files.
					  | QDir::NoSymLinks			// Do not list symbolic links (ignored by operating systems that don't support symbolic links).
					  | QDir::NoDotAndDotDot	// Do not list the special entries "." and "..".
					  | QDir::System);			// List system files (on Unix, FIFOs, sockets and device files are included; on Windows, .lnk files are included).

	QStringList filters(mask);					//Set the name filters. Each name filter is a wildcard filter that understands wildcards.
	dir.setNameFilters(filters);

	out << QDir::toNativeSeparators(dir.absolutePath()) << ":" << endl;		// Print the directory path atop the files contained.
	QFileInfoList list = dir.entryInfoList();											// Print each file inside that directory.
	for (const auto& fileInfo : dir.entryInfoList()) {
		out.setFieldAlignment(QTextStream::AlignRight);								// Right-align file sizes and modification dates.
		out.setFieldWidth((int)fieldWidth::size);
		if (fileInfo.isFile())																// Print the sizes of files only but skip directories'.
			out << fileInfo.size();
		else
			out << dirMark;
		out.setFieldWidth((int)fieldWidth::modificationDate);
		out << fileInfo.lastModified().toString(Qt::SystemLocaleShortDate);	//	The last modification date in the short format used by the operating system.
		out.setFieldAlignment(QTextStream::AlignLeft);								// Left-align file names.
		out.setFieldWidth(0);
		out << "   " << fileInfo.fileName() << endl;									// File name.
	}
	out << endl;

	if (recursive) {									// If listing is recursive keep iterating recursively.
		dir.setSorting(QDir::Name					// Sort by name.
							| QDir::IgnoreCase		// Sort case-insensitively.
							| QDir::LocaleAware );	// Sort items appropriately using the current locale settings.

		dir.setFilter(QDir::AllDirs				// List all directories; i.e. don't apply the filters to directory names.
						  | QDir::NoDotAndDotDot	// Do not list the special entries "." and "..".
						  | QDir::NoSymLinks			// Do not list symbolic links (ignored by operating systems that don't support symbolic links).
						  | QDir::System);			// List system files (on Unix, FIFOs, sockets and device files are included; on Windows, .lnk files are included).

		for (const auto& entry : dir.entryList()) {
			QString subDir = QString("%1/%2").arg(dir.absolutePath()).arg(entry);
			listRecursively(QDir(subDir), mask, recursive);
		}
	}
}