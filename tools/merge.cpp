#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMap>
#include <QSet>
#include <QString>
#include <QTextStream>

#include "../globals.h"

// Definitions for this source file
#include "merge.h"

// Holds defining properties for a package source directory
struct sourceProperties {
  // Ordered index of the source
  signed int sourceIndex;
  // String to prepend to file names and variables
  QString mergePrefix;
  // Filesystem path to the source directory
  QString sourcePath;
  // Set of all VScript globals found within the source
  QSet<QString> scriptGlobals;
  // List of all VScript files found within the source
  QStringList scriptFiles;

  sourceProperties (int index, const QString &path) {
    // Construct properties from parameters
    this->sourceIndex = index;
    this->mergePrefix = "sppmerge" + QString::number(index) + "_";
    this->sourcePath = path;
    // The initial list contains names which are known to often conflict
    this->scriptGlobals = {
      "ppmod_portals_p_anchor",
      "ppmod_portals_r_anchor",
      "ppmod_eyes",
      "pplayer_eyes",
      "pplayer_ent_"
    };
    // Empty container for list of encountered script files
    this->scriptFiles = QStringList();
  }
};

// Restricted keywords, omitted from searches of global script variables
const QSet<QString> squirrelKeywords = {
  "break", "case", "catch", "class", "clone", "continue", "const", "default", "delegate", "delete", "else",
  "enum", "extends", "for", "foreach", "function", "if", "in", "local", "null", "resume", "return",
  "switch", "this", "throw", "try", "typeof", "while", "parent", "yield", "constructor", "vargc", "vargv",
  "instanceof", "true", "false", "static", "_set", "_get", "_newslot", "_delslot", "_add", "_sub", "_mul",
  "_div", "_modulo", "_unm", "_typeof", "_cmp", "_call", "_cloned", "_nexti", "_tostring", "_inherited", "_newmember"
};

// Returns true if the input character is whitespace in Squirrel
bool charIsWhitespace (QChar c) { return c == ' ' || c == '\t'; }
// Returns true if the input character is a valid Squirrel variable name
bool charIsNamePart (QChar c) { return c.isLetterOrNumber() || c == '_'; }

// Extracts a Squirel variable name near a given position in a string
QString extractName (const QString &str, int pos, int length) {

  // If the input position is invalid, return an empty string
  if (pos == -1 || pos + length >= str.length()) return QString();
  pos += length;
  // Use `length` parameter to determine seeking direction
  int dir = length > 0 ? 1 : -1;

  // Keep track of other end of variable name
  int end;

  if (dir == -1) {
    // Seek left past all whitespace, then through variable name
    while (pos > 0 && charIsWhitespace(str[pos - 1])) pos--;
    end = pos;
    while (end > 0 && charIsNamePart(str[end - 1])) end--;
  } else {
    // Seek right past all whitespace, then through variable name
    while (pos < str.length() && charIsWhitespace(str[pos])) pos++;
    end = pos;
    while (end < str.length() && charIsNamePart(str[end])) end++;
  }

  // Return the substring between the seeked parts
  return pos > end ? str.mid(end, pos - end) : str.mid(pos, end - pos);

}

// Helper function: reads a file as a string and returns it
QString readFileString (const QString &path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    LOGFILE << "[W] Merge error, unable to read file: " + path.toStdString();
    return QString();
  }
  QTextStream in(&file);
  return in.readAll();
}

// Helper function: writes the given string to a file
void writeFileString (const QString &path, const QString &content) {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    LOGFILE << "[W] Merge error, unable to write file: " + path.toStdString();
    return;
  }
  QTextStream out(&file);
  out << content;
}

// Extracts candidates for global variables from the given script string
QSet<QString> extractGlobals (const QString &script) {

  QSet<QString> globals;
  QStringList lines = script.split('\n');

  // Iterate through the code line-by-line
  // This works if we assume that the code was written somewhat sensibly
  for (QString &line : lines) {

    // Skip reading comments at the end of lines
    int comment = line.indexOf("//");
    if (comment != -1) line = line.left(comment);

    // Look for variable names to the left of `<-` operators
    QString varName = extractName(line, line.indexOf("<-"), 0);
    // Look for variable names to the right of `class` definitions
    if (varName.isEmpty()) varName = extractName(line, line.indexOf("class "), 6);
    // If nothing was found, continue to the next line
    if (varName.isEmpty()) continue;

    // Insert name into the globals list if it's not a Squirrel keyword
    if (!squirrelKeywords.contains(varName)) {
      globals.insert(varName);
    }
  }

  // Remove variables prefixed with a dot
  // This is likely to exclude regular table slots that aren't globals
  QSet<QString>::iterator curr = globals.begin();
  while (curr != globals.end()) {
    QString dotVar = "." + *curr;
    // Match only full words by checking trailing character
    int index = 0;
    while ((index = script.indexOf(dotVar, index)) != -1) {
      if (!charIsNamePart(script[index + dotVar.length()])) break;
      index += dotVar.length();
    }
    // If a match was found, remove the global
    if (index != -1) curr = globals.erase(curr);
    else curr ++;
  }

  return globals;

}

// Handles moving a script file into the merge destination directory
void processScriptFile (const QString &source, const QString &destination, sourceProperties &properties) {

  // Extract globals from the current file and add to the source's global list
  QSet<QString> fileGlobals = extractGlobals(readFileString(source));
  properties.scriptGlobals.unite(fileGlobals);

  // Move the script file, but change its name to avoid conflicts
  QString parentPath = QFileInfo(destination).path();
  QString newFileName = properties.mergePrefix + QFileInfo(source).fileName();
  QString renamedPath = parentPath + "/" + newFileName;
  QFile::rename(source, renamedPath);

  // Push this file to the list of all scripts for handling later
  properties.scriptFiles.append(destination);

  // The original destination contains IncludeScript lines for all conflicting sources
  QString includeScript = "try { IncludeScript(\"" + newFileName + "\") } catch (e) { printl(e) }\n";

  // If the include file is just being made, add some hacky patches first
  // These make it slightly less likely for any one script to fail completely
  if (!QFileInfo::exists(destination)) {
    includeScript = ""
      "if (typeof compilestring == \"native function\") {\n"
      "  local _compilestring = compilestring;\n"
      "  ::compilestring <- function (str):(_compilestring) {\n"
      "    return function (...):(str, _compilestring) {\n"
      "      try { return _compilestring(str)() } catch (e) { printl(e) }\n"
      "    };\n"
      "  };\n"
      "}\n\n" + includeScript;
  }

  // Append current script's include line to the central include script
  QFile includeFile(destination);
  if (!includeFile.open(QIODevice::Append | QIODevice::Text)) {
    LOGFILE << "[W] Merge error, unable to append to script include file: " + destination.toStdString();
    return;
  }
  QTextStream(&includeFile) << includeScript;

}

// Renames global variables in all script files to avoid conflicts
void replaceScriptGlobals (const sourceProperties &properties) {

  // Iterate through every script file handled earlier
  for (const QString &file : properties.scriptFiles) {

    // Read the now-renamed file's code into a string
    QString renamedPath = QFileInfo(file).path() + "/" + properties.mergePrefix + QFileInfo(file).fileName();
    QString script = readFileString(renamedPath);

    // Iterate through global variables and replace them with renamed variants
    for (const QString &global : properties.scriptGlobals) {
      // Prefix the variables with the source's respective merge prefix
      QString renamed = properties.mergePrefix + global;
      // Search the script for all occurances of the relevant global
      int pos = 0;
      while ((pos = script.indexOf(global, pos)) != -1) {
        // Ensure that we're replacing whole words, not parts of words
        if ((pos > 0 && charIsNamePart(script[pos - 1])) || charIsNamePart(script[pos + global.length()])) {
          pos += global.length();
          continue;
        }
        // If current result is on the same line as an IncludeScript, don't replace
        if (script.lastIndexOf("\n", pos) < script.lastIndexOf("IncludeScript(\"", pos)) {
          pos += global.length();
          continue;
        }
        // Perform substring replacement
        script.replace(pos, global.length(), renamed);
        pos += renamed.length();
      }
    }

    // Iterate again through all script files
    // This time, we're looking for includes of the original file names
    for (const QString &includeFile : properties.scriptFiles) {

      // Get just the script file's base name (without .nut extension)
      QString scriptName = QFileInfo(includeFile).baseName();

      // Replace calls to (Do)IncludeScript with calls to the renamed files
      QString searchString = "IncludeScript(\"" + scriptName + "\"";
      QString replaceString = "IncludeScript(\"" + properties.mergePrefix + scriptName + "\"";

      // First, replace all calls to the script without .nut extension
      int pos = 0;
      while ((pos = script.indexOf(searchString, pos)) != -1) {
        script.replace(pos, searchString.length(), replaceString);
        pos += replaceString.length();
      }
      // Then, iterate again, but this time with the .nut extension
      pos = 0;
      searchString = "IncludeScript(\"" + scriptName + ".nut\"";
      while ((pos = script.indexOf(searchString, pos)) != -1) {
        script.replace(pos, searchString.length(), replaceString);
        pos += replaceString.length();
      }

    }

    // Write changes back to the renamed file
    writeFileString(renamedPath, script);

  }

}

// Copies only the unique commands from each config to the destination
void processConfigFile (const QString &source, const QString &destination) {

  // This function assumes that a conflicting file already exists at the destination
  QString config = readFileString(destination);
  QString sourceConfig = readFileString(source);

  // Get all commands already present in the config
  QStringList configCommands = config.split('\n');
  for (int i = 0; i < configCommands.length(); i ++) {
    // Ignore comments
    if (configCommands[i].startsWith("//")) continue;
    // Keep just the command - the part before the first space
    configCommands[i] = configCommands[i].split(" ")[0];
  }

  // Split up the source config by lines
  QStringList sourceLines = sourceConfig.split('\n');

  // Open the config file for appending new lines
  QFile outFile(destination);
  if (!outFile.open(QIODevice::Append | QIODevice::Text)) {
    LOGFILE << "[W] Merge error, unable to append to config file: " + destination.toStdString();
    return;
  }
  QTextStream outStream(&outFile);

  // Append only lines with unique commands
  for (const QString &line : sourceLines) {
    // Ignore comments
    if (line.startsWith("//")) continue;
    // Check if this command has already appeared in the file
    const QString command = line.split(" ")[0];
    if (configCommands.contains(command)) continue;
    // If not, add the full line
    outStream << "\n" << line;
  }

}

// Recursively merges a package source directory into the destination
void mergeSource (const QString &src, const QString &dest, sourceProperties &properties) {

  // Check if the source directory exists
  QDir srcDir(src);
  if (!srcDir.exists()) {
    LOGFILE << "[W] Merge error, missing source directory: " + src.toStdString();
    return;
  }

  // Ensure a destination directory
  if (!QDir(dest).exists()) QDir().mkpath(dest);

  // Iterate through all entries in the current directory
  QFileInfoList entries = srcDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
  for (const QFileInfo &entry : entries) {

    QString srcPath = entry.filePath();
    QString destPath = dest + "/" + entry.fileName();

    // If this is a directory, recurse into it
    if (entry.isDir()) {
      mergeSource(srcPath, destPath, properties);
      continue;
    }

    // If not a file, no clue what this, skip it
    if (!entry.isFile()) continue;

    // Handle files based on file type
    QString suffix = entry.suffix();
    if ((suffix == "cfg" || suffix == "rc") && QFileInfo::exists(destPath)) {
      processConfigFile(srcPath, destPath);
    } else if (suffix == "nut") {
      processScriptFile(srcPath, destPath, properties);
    } else if (suffix == "sav") {
      continue; // Ignore save files, they prevent merging
    } else {
      QFile::rename(srcPath, destPath);
    }

  }

}

// Merges a list of package source directories into the destination directory
void ToolsMerge::mergeSourcesList (QStringList &sources, QString &destination) {
  int index = 1;
  for (const QString &sourceDir : sources) {
    sourceProperties properties = sourceProperties(index, sourceDir);
    index++;

    mergeSource(sourceDir, destination, properties);
    QDir(sourceDir).removeRecursively();
    replaceScriptGlobals(properties);
  }
}
