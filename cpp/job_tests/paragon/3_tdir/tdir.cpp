/*
#include <QDir> // Access to directory structures and their contents.
#include <QDirIterator> // An iterator for directory entrylists.
#include <QTextStream> // A convenient interface for reading and writing QString text.
#include <QDateTime> // Date and time functions.
#include <exception>

void distinguishPathMask(QString&, QString&);
void checkPathAndList(const QString&, const QString&, const bool);
void listRecursively(QDir, const QString&, const bool);

const char* const cmdTdir{"tdir"}; // The tdir command.
const QString maskAll("*"); // All directories/files mask ('*' wildcard).

QTextStream out(stdout); // Interface for writing QString text.

//*********************************************************************************************************************************************************
// The main function.
// In the command line, enter 'tdir [path/mask] [-r]' or 'tdir [-r] [path/mask]' to list files.
// Before lising files, the tdir command and its arguments ([path/mask] [-r]) are validated.
// Files are listed from a single directory or recursively from its subdirectories given the '-r' agrument.
//*********************************************************************************************************************************************************
int main(int argc, char** argv) {
	try {
		constexpr unsigned tdirOnly{1}; // The count of agruments when only the tdir command entered w/o path/mask or -r.
		const char* const recurKey{"-r"}; // The recursion key.
		const QString prompt = QDir::toNativeSeparators(QDir::currentPath()) + " >> "; // A prompt with the current directory.
		bool recursive{}; // The -r flag.

		out << "argc: " << argc << endl;
		for (int i = 0; i < argc; i++)
			out << "argv[" << i << "]: " << argv[i] << endl;
		out << endl;

		if (argc == tdirOnly) { // Only the tdir command entered w/o path/mask or -r.
			listRecursively(QDir::current(), maskAll, false); // List the current directory without recursion.
			return 0;
		}

		for (int i = 0; i < argc; i++) { // Check all combinations of arguments in a loop.
			if (strcmp(argv[i], recurKey) == 0) { // Is that -r?
				if (!recursive) { // Has not -r been entered before?
					recursive = true;
					continue;
				} else { // Prevent multiple -r.
					fprintf(stderr, "Only one -r can be entered.\n\n");
					return 0;
				}
			}
		}
		listRecursively(QDir::current(), maskAll, recursive); // List the current directory with or without recursion.
	}
	catch(const std::exception& exc) { // Catch and dislplay an exception.
		fprintf(stderr, "An exception occured: %s\n\n", exc.what());
	}
	return 0;
}

//*********************************************************************************************************************************************************
// The main function.
// In the command line, enter 'tdir [path/mask] [-r]' or 'tdir [-r] [path/mask]' to list files.
// Before lising files, the tdir command and its arguments ([path/mask] [-r]) are validated.
// Files are listed from a single directory or recursively from its subdirectories given the '-r' agrument.
//*********************************************************************************************************************************************************
int main1(int argc, char** argv) {
	try {
		enum struct wordCount : int {tdir_exit = 1, tdir_path, tdir_path_rkey}; // Acceptable number of input words: 1) tdir/exit, 2) tdir path/mask, 3) tdir path/mask -r.
		const char* const recurKey{"-r"}; // The recursion key.
		const QString prompt = QDir::toNativeSeparators(QDir::currentPath()) + " >> "; // A prompt with the current directory.

		const char* const pathRKey1{argv[1]}; // Command line arguments 1 and 2 can be either a path/mask or a recursion key (-r).
		const char* const pathRKey2{argv[1]};
		bool recursive1{}, recursive2{}; // Respective recursion key flags (for words 2 and 3).
		QString path; // Path to a directory listed.
		QString mask; // File mask.

		out << "argc: " << argc << endl;
		for (int i = 0; i < argc; i++)
			out << "argv[" << i << "]: " << argv[i] << endl;
		out << endl;

		switch (argc) { // Parse the command line based on the number of words successfully read.
		case (int)wordCount::tdir_exit:// One word entered. That must be a tdir or exit command.
			listRecursively(QDir::current(), maskAll, false); // List without recursion.
			break;

		case (int)wordCount::tdir_path: // Two words entered. The second word can be either a path or a recursion key.
			if (strcmp(pathRKey1, recurKey) == 0) // Check if the second word is a recursion key or not.
				listRecursively(QDir::current(), maskAll, true); // The second word is a recursion key. List recursively.
			else {
				path = QString(pathRKey1); // The second word is a path.
				distinguishPathMask(path, mask); // If included, retrieve a mask from the end of the path.
				checkPathAndList(path, mask, false); // Check the path and list without recursion.
			}
			break;

		case (int)wordCount::tdir_path_rkey: { // Three words entered.
			if (strcmp(pathRKey1, pathRKey2) == 0) { // The second and the third words must not be the same.
				fprintf(stderr, "Wrong agruments are entered or agruments 1 and 2 are the same.\n\n");
				return 0;
			}

			if (strcmp(pathRKey1, recurKey) == 0) // Check if the second word is a recursion key or not.
				recursive1 = true; // The second word is a recursion key. List recursively.
			else
				path = QString(pathRKey1); // The second word is a path.

			if (strcmp(pathRKey2, recurKey) == 0) // Check if the third word is a recursion key or not.
				recursive2 = true; // The third word a recursion key. List recursively.
			else
				path = QString(pathRKey2); // The third word is a path.

			if (!(recursive1 || recursive2)) { // Neither of the two agruments is a recursion key.
				fprintf(stderr, "Wrong agruments are entered or neither of the two agruments is -r.\n\n");
				return 0;
			}

			distinguishPathMask(path, mask); // If included, retrieve a mask from the end of the path.
			checkPathAndList(path, mask, true); // Check the path and list recursively.
			break;
		default: // Too many agruments entered or redundant spaces in between.
				fprintf(stderr, "Too many agruments entered or redundant spaces in between.\n\n");
				return 0;
			}
		}
	}
	catch(const std::exception& exc) { // Catch and dislplay an exception.
		fprintf(stderr, "An exception occured: %s\n\n", exc.what());
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
	const char wildcards[] = {'*', '?', '['}; // The wildcards to detect a file mask.

	int wildcardPos{};
	for (auto& wildcard : wildcards) { // Find a wildcard to detect a file mask, if any.
		wildcardPos = path.indexOf(wildcard); // The first wildcard's position.
		if (wildcardPos > -1)
			break; // The first wildcard found.
	}
	if (wildcardPos == -1) {
		mask = maskAll; // No wildcard. No mask entered, just set '*'.
		return;
	}

	int slashPos = path.lastIndexOf(QDir::separator(), wildcardPos); // Find the position of the a slash before the first wildcard.
	if (slashPos == -1) { // No slash there.
		mask = path; // The whole path is a mask.
		path = QDir::currentPath(); // The path is supposed to be the current directory.
	} else {
		mask = path.mid(slashPos + 1); // Retrieve a mask that is the section of the path after the last slash.
		path = path.left(slashPos); // Cut the mask from the path.
	}
}

//*********************************************************************************************************************************************************
// Check a path and list files and directories, if a path exists.
// TO DO: More comments on the function agruments.
//*********************************************************************************************************************************************************
void checkPathAndList(const QString& path, const QString& mask, const bool recursive) {
	QDir dir(path);
	if	(dir.exists()) // Validate a path if it exists.
		listRecursively(dir, mask, recursive); // List.
	else // No directory.
		fprintf(stderr, "%s: cannot access '%s': No such file or directory\n\n", cmdTdir, path.toStdString().c_str());
}

//*********************************************************************************************************************************************************
// List files recursively in the selected directory and all its subdirectories, if necessary.
// TO DO: More comments on the function agruments.
//*********************************************************************************************************************************************************
void listRecursively(QDir dir, const QString& mask, const bool recursive) { //TO DO: Arrange a break-out with a thread.
	enum struct fieldWidth : int {size = 15, modificationDate = 20}; // Field widths for printing.
	const QString dirMark{"<DIR>"}; // A directory mark.

	dir.setSorting(QDir::Name // Sort by name.
						| QDir::DirsFirst	// Put the directories first, then the files.
						| QDir::IgnoreCase // Sort case-insensitively.
						| QDir::LocaleAware); // Sort items appropriately using the current locale settings.

	dir.setFilter(QDir::Dirs // List directories that match the filters.
					  | QDir::Files // List files.
					  | QDir::NoSymLinks // Do not list symbolic links (ignored by operating systems that don't support symbolic links).
					  | QDir::NoDotAndDotDot // Do not list the special entries "." and "..".
					  | QDir::System); // List system files (on Unix, FIFOs, sockets and device files are included; on Windows, .lnk files are included).

	QStringList filters(mask); //Set the name filters. Each name filter is a wildcard filter that understands wildcards.
	dir.setNameFilters(filters);

	out << QDir::toNativeSeparators(dir.absolutePath()) << ":" << endl; // Print the directory path atop the files contained.
	QFileInfoList list = dir.entryInfoList(); // Print each file inside that directory.
	for (const auto& fileInfo : dir.entryInfoList()) {
		out.setFieldAlignment(QTextStream::AlignRight); // Right-align file sizes and modification dates.
		out.setFieldWidth((int)fieldWidth::size);
		out << fileInfo.size();

		out.setFieldWidth((int)fieldWidth::modificationDate);
		out << fileInfo.lastModified().toString(Qt::SystemLocaleShortDate); //	The last modification date in the short format used by the operating system.

		out.setFieldAlignment(QTextStream::AlignLeft); // Left-align file names.
		out.setFieldWidth(0);
		out << "   ";
		if (fileInfo.isDir()) // <DIR> Prefix before a directory name for readability.
			out << dirMark << " ";
		out << fileInfo.fileName() << endl; // File/directory name.
	}
	out << endl;

	if (recursive) { // If listing is recursive keep iterating recursively.
		dir.setSorting(QDir::Name // Sort by name.
							| QDir::IgnoreCase // Sort case-insensitively.
							| QDir::LocaleAware ); // Sort items appropriately using the current locale settings.

		dir.setFilter(QDir::AllDirs // List all directories; i.e. don't apply the filters to directory names.
						  | QDir::NoDotAndDotDot // Do not list the special entries "." and "..".
						  | QDir::NoSymLinks // Do not list symbolic links (ignored by operating systems that don't support symbolic links).
						  | QDir::System); // List system files (on Unix, FIFOs, sockets and device files are included; on Windows, .lnk files are included).

		for (const auto& entry : dir.entryList()) {
			QString subDir = QString("%1/%2").arg(dir.absolutePath()).arg(entry);
			listRecursively(QDir(subDir), mask, recursive);
		}
	}
}
*/
