/*============================================================================
  KWSys - Kitware System Library
  Copyright 2000-2009 Kitware, Inc., Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#ifndef itksys_SystemTools_hxx
#define itksys_SystemTools_hxx

#include <itksys/Configure.hxx>

#include <iosfwd>
#include <string>
#include <vector>
#include <map>

#include <itksys/String.hxx>

#include <sys/types.h>
#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> // For access permissions for use with access()
#endif

// Required for va_list
#include <stdarg.h>
// Required for FILE*
#include <stdio.h>
#if !defined(va_list)
// Some compilers move va_list into the std namespace and there is no way to
// tell that this has been done. Playing with things being included before or
// after stdarg.h does not solve things because we do not have control over
// what the user does. This hack solves this problem by moving va_list to our
// own namespace that is local for kwsys.
namespace std {} // Required for platforms that do not have std namespace
namespace itksys_VA_LIST
{
  using namespace std;
  typedef va_list hack_va_list;
}
namespace itksys
{
  typedef itksys_VA_LIST::hack_va_list va_list;
}
#endif // va_list

namespace itksys
{

class SystemToolsTranslationMap;
class SystemToolsPathCaseMap;

/** \class SystemToolsManager
 * \brief Use to make sure SystemTools is initialized before it is used
 * and is the last static object destroyed
 */
class itksys_EXPORT SystemToolsManager
{
public:
  SystemToolsManager();
  ~SystemToolsManager();
};

// This instance will show up in any translation unit that uses
// SystemTools. It will make sure SystemTools is initialized
// before it is used and is the last static object destroyed.
static SystemToolsManager SystemToolsManagerInstance;

// Flags for use with TestFileAccess.  Use a typedef in case any operating
// system in the future needs a special type.  These are flags that may be
// combined using the | operator.
typedef int TestFilePermissions;
#if defined(_WIN32) && !defined(__CYGWIN__)
  // On Windows (VC and Borland), no system header defines these constants...
  static const TestFilePermissions TEST_FILE_OK = 0;
  static const TestFilePermissions TEST_FILE_READ = 4;
  static const TestFilePermissions TEST_FILE_WRITE = 2;
  static const TestFilePermissions TEST_FILE_EXECUTE = 1;
#else
  // Standard POSIX constants
  static const TestFilePermissions TEST_FILE_OK = F_OK;
  static const TestFilePermissions TEST_FILE_READ = R_OK;
  static const TestFilePermissions TEST_FILE_WRITE = W_OK;
  static const TestFilePermissions TEST_FILE_EXECUTE = X_OK;
#endif

/** \class SystemTools
 * \brief A collection of useful platform-independent system functions.
 */
class itksys_EXPORT SystemTools
{
public:

  /** -----------------------------------------------------------------
   *               String Manipulation Routines
   *  -----------------------------------------------------------------
   */

  /**
   * Replace symbols in str that are not valid in C identifiers as
   * defined by the 1999 standard, ie. anything except [A-Za-z0-9_].
   * They are replaced with `_' and if the first character is a digit
   * then an underscore is prepended.  Note that this can produce
   * identifiers that the standard reserves (_[A-Z].* and __.*).
   */
  static std::string MakeCidentifier(const std::string& s);

  static std::string MakeCindentifier(const std::string& s)
  {
    return MakeCidentifier(s);
  }

  /**
   * Replace replace all occurences of the string in the source string.
   */
  static void ReplaceString(std::string& source,
                            const char* replace,
                            const char* with);
  static void ReplaceString(std::string& source,
                            const std::string& replace,
                            const std::string& with);

  /**
   * Return a capitalized string (i.e the first letter is uppercased,
   * all other are lowercased).
   */
  static std::string Capitalized(const std::string&);

  /**
   * Return a 'capitalized words' string (i.e the first letter of each word
   * is uppercased all other are left untouched though).
   */
  static std::string CapitalizedWords(const std::string&);

  /**
   * Return a 'uncapitalized words' string (i.e the first letter of each word
   * is lowercased all other are left untouched though).
   */
  static std::string UnCapitalizedWords(const std::string&);

  /**
   * Return a lower case string
   */
  static std::string LowerCase(const std::string&);

  /**
   * Return a lower case string
   */
  static std::string UpperCase(const std::string&);

  /**
   * Count char in string
   */
  static size_t CountChar(const char* str, char c);

  /**
   * Remove some characters from a string.
   * Return a pointer to the new resulting string (allocated with 'new')
   */
  static char* RemoveChars(const char* str, const char *toremove);

  /**
   * Remove remove all but 0->9, A->F characters from a string.
   * Return a pointer to the new resulting string (allocated with 'new')
   */
  static char* RemoveCharsButUpperHex(const char* str);

  /**
   * Replace some characters by another character in a string (in-place)
   * Return a pointer to string
   */
  static char* ReplaceChars(char* str, const char *toreplace,char replacement);

  /**
   * Returns true if str1 starts (respectively ends) with str2
   */
  static bool StringStartsWith(const char* str1, const char* str2);
  static bool StringStartsWith(const std::string& str1, const char* str2);
  static bool StringEndsWith(const char* str1, const char* str2);
  static bool StringEndsWith(const std::string& str1, const char* str2);

  /**
   * Returns a pointer to the last occurence of str2 in str1
   */
  static const char* FindLastString(const char* str1, const char* str2);

  /**
   * Make a duplicate of the string similar to the strdup C function
   * but use new to create the 'new' string, so one can use
   * 'delete' to remove it. Returns 0 if the input is empty.
   */
  static char* DuplicateString(const char* str);

  /**
   * Return the string cropped to a given length by removing chars in the
   * center of the string and replacing them with an ellipsis (...)
   */
  static std::string CropString(const std::string&,size_t max_len);

  /** split a path by separator into an array of strings, default is /.
      If isPath is true then the string is treated like a path and if
      s starts with a / then the first element of the returned array will
      be /, so /foo/bar will be [/, foo, bar]
  */
  static std::vector<String> SplitString(const std::string& s, char separator = '/',
                                               bool isPath = false);
  /**
   * Perform a case-independent string comparison
   */
  static int Strucmp(const char *s1, const char *s2);

  /**
   * Convert a string in __DATE__ or __TIMESTAMP__ format into a time_t.
   * Return false on error, true on success
   */
  static bool ConvertDateMacroString(const char *str, time_t *tmt);
  static bool ConvertTimeStampMacroString(const char *str, time_t *tmt);

  /**
   * Split a string on its newlines into multiple lines
   * Return false only if the last line stored had no newline
   */
  static bool Split(const std::string& s, std::vector<std::string>& l);
  static bool Split(const std::string& s, std::vector<std::string>& l, char separator);

  /**
   * Return string with space added between capitalized words
   * (i.e. EatMyShorts becomes Eat My Shorts )
   * (note that IEatShorts becomes IEat Shorts)
   */
  static std::string AddSpaceBetweenCapitalizedWords(
    const std::string&);

  /**
   * Append two or more strings and produce new one.
   * Programmer must 'delete []' the resulting string, which was allocated
   * with 'new'.
   * Return 0 if inputs are empty or there was an error
   */
  static char* AppendStrings(
    const char* str1, const char* str2);
  static char* AppendStrings(
    const char* str1, const char* str2, const char* str3);

  /**
   * Estimate the length of the string that will be produced
   * from printing the given format string and arguments.  The
   * returned length will always be at least as large as the string
   * that will result from printing.
   * WARNING: since va_arg is called to iterate of the argument list,
   * you will not be able to use this 'ap' anymore from the beginning.
   * It's up to you to call va_end though.
   */
  static int EstimateFormatLength(const char *format, va_list ap);

  /**
   * Escape specific characters in 'str'.
   */
  static std::string EscapeChars(
    const char *str, const char *chars_to_escape, char escape_char = '\\');

  /** -----------------------------------------------------------------
   *               Filename Manipulation Routines
   *  -----------------------------------------------------------------
   */

  /**
   * Replace Windows file system slashes with Unix-style slashes.
   */
  static void ConvertToUnixSlashes(std::string& path);

#ifdef _WIN32
  /**
   * Convert the path to an extended length path to avoid MAX_PATH length
   * limitations on Windows. If the input is a local path the result will be
   * prefixed with \\?\; if the input is instead a network path, the result
   * will be prefixed with \\?\UNC\. All output will also be converted to
   * absolute paths with Windows-style backslashes.
   **/
  static std::wstring ConvertToWindowsExtendedPath(const std::string&);
#endif

  /**
   * For windows this calls ConvertToWindowsOutputPath and for unix
   * it calls ConvertToUnixOutputPath
   */
  static std::string ConvertToOutputPath(const std::string&);

  /**
   * Convert the path to a string that can be used in a unix makefile.
   * double slashes are removed, and spaces are escaped.
   */
  static std::string ConvertToUnixOutputPath(const std::string&);

  /**
   * Convert the path to string that can be used in a windows project or
   * makefile.   Double slashes are removed if they are not at the start of
   * the string, the slashes are converted to windows style backslashes, and
   * if there are spaces in the string it is double quoted.
   */
  static std::string ConvertToWindowsOutputPath(const std::string&);

  /**
   * Return true if a file exists in the current directory.
   * If isFile = true, then make sure the file is a file and
   * not a directory.  If isFile = false, then return true
   * if it is a file or a directory.  Note that the file will
   * also be checked for read access.  (Currently, this check
   * for read access is only done on POSIX systems.)
   */
  static bool FileExists(const char* filename, bool isFile);
  static bool FileExists(const std::string& filename, bool isFile);
  static bool FileExists(const char* filename);
  static bool FileExists(const std::string& filename);

  /**
   * Test if a file exists and can be accessed with the requested
   * permissions.  Symbolic links are followed.  Returns true if
   * the access test was successful.
   *
   * On POSIX systems (including Cygwin), this maps to the access
   * function.  On Windows systems, all existing files are
   * considered readable, and writable files are considered to
   * have the read-only file attribute cleared.
   */
  static bool TestFileAccess(const char* filename,
                             TestFilePermissions permissions);
  static bool TestFileAccess(const std::string& filename,
                             TestFilePermissions permissions);

  /**
   * Converts Cygwin path to Win32 path. Uses dictionary container for
   * caching and calls to cygwin_conv_to_win32_path from Cygwin dll
   * for actual translation.  Returns true on success, else false.
   */
#ifdef __CYGWIN__
  static bool PathCygwinToWin32(const char *path, char *win32_path);
#endif

  /**
   * Return file length
   */
  static unsigned long FileLength(const std::string& filename);

  /**
     Change the modification time or create a file
  */
  static bool Touch(const std::string& filename, bool create);

  /**
   *  Compare file modification times.
   *  Return true for successful comparison and false for error.
   *  When true is returned, result has -1, 0, +1 for
   *  f1 older, same, or newer than f2.
   */
  static bool FileTimeCompare(const std::string& f1,
                              const std::string& f2,
                              int* result);

  /**
   *  Get the file extension (including ".") needed for an executable
   *  on the current platform ("" for unix, ".exe" for Windows).
   */
  static const char* GetExecutableExtension();

  /**
   *  Given a path that exists on a windows machine, return the
   *  actuall case of the path as it was created.  If the file
   *  does not exist path is returned unchanged.  This does nothing
   *  on unix but return path.
   */
  static std::string GetActualCaseForPath(const std::string& path);

  /**
   * Given the path to a program executable, get the directory part of
   * the path with the file stripped off.  If there is no directory
   * part, the empty string is returned.
   */
  static std::string GetProgramPath(const std::string&);
  static bool SplitProgramPath(const std::string& in_name,
                               std::string& dir,
                               std::string& file,
                               bool errorReport = true);

  /**
   *  Given argv[0] for a unix program find the full path to a running
   *  executable.  argv0 can be null for windows WinMain programs
   *  in this case GetModuleFileName will be used to find the path
   *  to the running executable.  If argv0 is not a full path,
   *  then this will try to find the full path.  If the path is not
   *  found false is returned, if found true is returned.  An error
   *  message of the attempted paths is stored in errorMsg.
   *  exeName is the name of the executable.
   *  buildDir is a possibly null path to the build directory.
   *  installPrefix is a possibly null pointer to the install directory.
   */
  static bool FindProgramPath(const char* argv0,
                              std::string& pathOut,
                              std::string& errorMsg,
                              const char* exeName = 0,
                              const char* buildDir = 0,
                              const char* installPrefix = 0);

  /**
   * Given a path to a file or directory, convert it to a full path.
   * This collapses away relative paths relative to the cwd argument
   * (which defaults to the current working directory).  The full path
   * is returned.
   */
  static std::string CollapseFullPath(const std::string& in_relative);
  static std::string CollapseFullPath(const std::string& in_relative,
                                            const char* in_base);
  static std::string CollapseFullPath(const std::string& in_relative,
                                            const std::string& in_base);

  /**
   * Get the real path for a given path, removing all symlinks.  In
   * the event of an error (non-existent path, permissions issue,
   * etc.) the original path is returned if errorMessage pointer is
   * NULL.  Otherwise empty string is returned and errorMessage
   * contains error description.
   */
  static std::string GetRealPath(const std::string& path,
                                       std::string* errorMessage = 0);

  /**
   * Split a path name into its root component and the rest of the
   * path.  The root component is one of the following:
   *    "/"   = UNIX full path
   *    "c:/" = Windows full path (can be any drive letter)
   *    "c:"  = Windows drive-letter relative path (can be any drive letter)
   *    "//"  = Network path
   *    "~/"  = Home path for current user
   *    "~u/" = Home path for user 'u'
   *    ""    = Relative path
   *
   * A pointer to the rest of the path after the root component is
   * returned.  The root component is stored in the "root" string if
   * given.
   */
  static const char* SplitPathRootComponent(const std::string& p,
                                            std::string* root=0);

  /**
   * Split a path name into its basic components.  The first component
   * always exists and is the root returned by SplitPathRootComponent.
   * The remaining components form the path.  If there is a trailing
   * slash then the last component is the empty string.  The
   * components can be recombined as "c[0]c[1]/c[2]/.../c[n]" to
   * produce the original path.  Home directory references are
   * automatically expanded if expand_home_dir is true and this
   * platform supports them.
   */
  static void SplitPath(const std::string& p,
                        std::vector<std::string>& components,
                        bool expand_home_dir = true);

  /**
   * Join components of a path name into a single string.  See
   * SplitPath for the format of the components.
   */
  static std::string JoinPath(
    const std::vector<std::string>& components);
  static std::string JoinPath(
    std::vector<std::string>::const_iterator first,
    std::vector<std::string>::const_iterator last);

  /**
   * Compare a path or components of a path.
   */
  static bool ComparePath(const std::string& c1, const std::string& c2);


  /**
   * Return path of a full filename (no trailing slashes)
   */
  static std::string GetFilenamePath(const std::string&);

  /**
   * Return file name of a full filename (i.e. file name without path)
   */
  static std::string GetFilenameName(const std::string&);

  /**
   * Split a program from its arguments and handle spaces in the paths
   */
  static void SplitProgramFromArgs(
    const std::string& path,
    std::string& program, std::string& args);

  /**
   * Return longest file extension of a full filename (dot included)
   */
  static std::string GetFilenameExtension(const std::string&);

  /**
   * Return shortest file extension of a full filename (dot included)
   */
  static std::string GetFilenameLastExtension(
    const std::string& filename);

  /**
   * Return file name without extension of a full filename
   */
  static std::string GetFilenameWithoutExtension(
    const std::string&);

  /**
   * Return file name without its last (shortest) extension
   */
  static std::string GetFilenameWithoutLastExtension(
    const std::string&);

  /**
   * Return whether the path represents a full path (not relative)
   */
  static bool FileIsFullPath(const std::string&);
  static bool FileIsFullPath(const char*);

  /**
   * For windows return the short path for the given path,
   * Unix just a pass through
   */
  static bool GetShortPath(const std::string& path, std::string& result);

  /**
   * Read line from file. Make sure to get everything. Due to a buggy stream
   * library on the HP and another on Mac OS X, we need this very carefully
   * written version of getline. Returns true if any data were read before the
   * end-of-file was reached. If the has_newline argument is specified, it will
   * be true when the line read had a newline character.
   */
  static bool GetLineFromStream(std::istream& istr,
                                std::string& line,
                                bool* has_newline=0,
                                long sizeLimit=-1);

  /**
   * Get the parent directory of the directory or file
   */
  static std::string GetParentDirectory(const std::string& fileOrDir);

  /**
   * Check if the given file or directory is in subdirectory of dir
   */
  static bool IsSubDirectory(const std::string& fileOrDir, const std::string& dir);

  /** -----------------------------------------------------------------
   *               File Manipulation Routines
   *  -----------------------------------------------------------------
   */

  /**
   * Open a file considering unicode.
   */
  static FILE* Fopen(const std::string& file, const char* mode);

  /**
   * Make a new directory if it is not there.  This function
   * can make a full path even if none of the directories existed
   * prior to calling this function.
   */
  static bool MakeDirectory(const char* path);
  static bool MakeDirectory(const std::string& path);

  /**
   * Copy the source file to the destination file only
   * if the two files differ.
   */
  static bool CopyFileIfDifferent(const std::string& source,
                                  const std::string& destination);

  /**
   * Compare the contents of two files.  Return true if different
   */
  static bool FilesDiffer(const std::string& source, const std::string& destination);

  /**
   * Return true if the two files are the same file
   */
  static bool SameFile(const std::string& file1, const std::string& file2);

  /**
   * Copy a file.
   */
  static bool CopyFileAlways(const std::string& source, const std::string& destination);

  /**
   * Copy a file.  If the "always" argument is true the file is always
   * copied.  If it is false, the file is copied only if it is new or
   * has changed.
   */
  static bool CopyAFile(const std::string& source, const std::string& destination,
                        bool always = true);

  /**
   * Copy content directory to another directory with all files and
   * subdirectories.  If the "always" argument is true all files are
   * always copied.  If it is false, only files that have changed or
   * are new are copied.
   */
  static bool CopyADirectory(const std::string& source, const std::string& destination,
                             bool always = true);

  /**
   * Remove a file
   */
  static bool RemoveFile(const std::string& source);

  /**
   * Remove a directory
   */
  static bool RemoveADirectory(const std::string& source);

  /**
   * Get the maximum full file path length
   */
  static size_t GetMaximumFilePathLength();

  /**
   * Find a file in the system PATH, with optional extra paths
   */
  static std::string FindFile(
    const std::string& name,
    const std::vector<std::string>& path =
    std::vector<std::string>(),
    bool no_system_path = false);

  /**
   * Find a directory in the system PATH, with optional extra paths
   */
  static std::string FindDirectory(
    const std::string& name,
    const std::vector<std::string>& path =
    std::vector<std::string>(),
    bool no_system_path = false);

  /**
   * Find an executable in the system PATH, with optional extra paths
   */
  static std::string FindProgram(
    const char* name,
    const std::vector<std::string>& path =
    std::vector<std::string>(),
    bool no_system_path = false);
  static std::string FindProgram(
    const std::string& name,
    const std::vector<std::string>& path =
    std::vector<std::string>(),
    bool no_system_path = false);
  static std::string FindProgram(
    const std::vector<std::string>& names,
    const std::vector<std::string>& path =
    std::vector<std::string>(),
    bool no_system_path = false);

  /**
   * Find a library in the system PATH, with optional extra paths
   */
  static std::string FindLibrary(
    const std::string& name,
    const std::vector<std::string>& path);

  /**
   * Return true if the file is a directory
   */
  static bool FileIsDirectory(const std::string& name);

  /**
   * Return true if the file is a symlink
   */
  static bool FileIsSymlink(const std::string& name);

  /**
   * Return true if the file has a given signature (first set of bytes)
   */
  static bool FileHasSignature(
    const char* filename, const char *signature, long offset = 0);

  /**
   * Attempt to detect and return the type of a file.
   * Up to 'length' bytes are read from the file, if more than 'percent_bin' %
   * of the bytes are non-textual elements, the file is considered binary,
   * otherwise textual. Textual elements are bytes in the ASCII [0x20, 0x7E]
   * range, but also \\n, \\r, \\t.
   * The algorithm is simplistic, and should probably check for usual file
   * extensions, 'magic' signature, unicode, etc.
   */
  enum FileTypeEnum
  {
    FileTypeUnknown,
    FileTypeBinary,
    FileTypeText
  };
  static SystemTools::FileTypeEnum DetectFileType(
    const char* filename,
    unsigned long length = 256,
    double percent_bin = 0.05);

  /**
   * Create a symbolic link if the platform supports it.  Returns whether
   * creation succeeded.
   */
  static bool CreateSymlink(const std::string& origName, const std::string& newName);

  /**
   * Read the contents of a symbolic link.  Returns whether reading
   * succeeded.
   */
  static bool ReadSymlink(const std::string& newName, std::string& origName);

  /**
   * Try to locate the file 'filename' in the directory 'dir'.
   * If 'filename' is a fully qualified filename, the basename of the file is
   * used to check for its existence in 'dir'.
   * If 'dir' is not a directory, GetFilenamePath() is called on 'dir' to
   * get its directory first (thus, you can pass a filename as 'dir', as
   * a convenience).
   * 'filename_found' is assigned the fully qualified name/path of the file
   * if it is found (not touched otherwise).
   * If 'try_filename_dirs' is true, try to find the file using the
   * components of its path, i.e. if we are looking for c:/foo/bar/bill.txt,
   * first look for bill.txt in 'dir', then in 'dir'/bar, then in 'dir'/foo/bar
   * etc.
   * Return true if the file was found, false otherwise.
   */
  static bool LocateFileInDir(const char *filename,
                              const char *dir,
                              std::string& filename_found,
                              int try_filename_dirs = 0);

  /** compute the relative path from local to remote.  local must
      be a directory.  remote can be a file or a directory.
      Both remote and local must be full paths.  Basically, if
      you are in directory local and you want to access the file in remote
      what is the relative path to do that.  For example:
      /a/b/c/d to /a/b/c1/d1 -> ../../c1/d1
      from /usr/src to /usr/src/test/blah/foo.cpp -> test/blah/foo.cpp
  */
  static std::string RelativePath(const std::string& local, const std::string& remote);

  /**
   * Return file's modified time
   */
  static long int ModifiedTime(const std::string& filename);

  /**
   * Return file's creation time (Win32: works only for NTFS, not FAT)
   */
  static long int CreationTime(const std::string& filename);

  /**
   * Visual C++ does not define mode_t (note that Borland does, however).
   */
  #if defined( _MSC_VER )
  typedef unsigned short mode_t;
  #endif

  /**
   * Get and set permissions of the file.  If honor_umask is set, the umask
   * is queried and applied to the given permissions.  Returns false if
   * failure.
   *
   * WARNING:  A non-thread-safe method is currently used to get the umask
   * if a honor_umask parameter is set to true.
   */
  static bool GetPermissions(const char* file, mode_t& mode);
  static bool GetPermissions(const std::string& file, mode_t& mode);
  static bool SetPermissions(const char* file, mode_t mode, bool honor_umask = false);
  static bool SetPermissions(const std::string& file, mode_t mode, bool honor_umask = false);

  /** -----------------------------------------------------------------
   *               Time Manipulation Routines
   *  -----------------------------------------------------------------
   */

  /** Get current time in seconds since Posix Epoch (Jan 1, 1970).  */
  static double GetTime();

  /**
   * Get current date/time
   */
  static std::string GetCurrentDateTime(const char* format);

  /** -----------------------------------------------------------------
   *               Registry Manipulation Routines
   *  -----------------------------------------------------------------
   */

  /**
   * Specify access to the 32-bit or 64-bit application view of
   * registry values.  The default is to match the currently running
   * binary type.
   */
  enum KeyWOW64 { KeyWOW64_Default, KeyWOW64_32, KeyWOW64_64 };

  /**
   * Get a list of subkeys.
   */
  static bool GetRegistrySubKeys(const std::string& key,
                                 std::vector<std::string>& subkeys,
                                 KeyWOW64 view = KeyWOW64_Default);

  /**
   * Read a registry value
   */
  static bool ReadRegistryValue(const std::string& key, std::string &value,
                                KeyWOW64 view = KeyWOW64_Default);

  /**
   * Write a registry value
   */
  static bool WriteRegistryValue(const std::string& key, const std::string& value,
                                 KeyWOW64 view = KeyWOW64_Default);

  /**
   * Delete a registry value
   */
  static bool DeleteRegistryValue(const std::string& key,
                                  KeyWOW64 view = KeyWOW64_Default);

  /** -----------------------------------------------------------------
   *               Environment Manipulation Routines
   *  -----------------------------------------------------------------
   */

  /**
   *  Add the paths from the environment variable PATH to the
   *  string vector passed in.  If env is set then the value
   *  of env will be used instead of PATH.
   */
  static void GetPath(std::vector<std::string>& path,
                      const char* env=0);

  /**
   * Read an environment variable
   */
  static const char* GetEnv(const char* key);
  static const char* GetEnv(const std::string& key);
  static bool GetEnv(const char* key, std::string& result);
  static bool GetEnv(const std::string& key, std::string& result);

  /** Put a string into the environment
      of the form var=value */
  static bool PutEnv(const std::string& env);

  /** Remove a string from the environment.
      Input is of the form "var" or "var=value" (value is ignored). */
  static bool UnPutEnv(const std::string& env);

  /**
   * Get current working directory CWD
   */
  static std::string GetCurrentWorkingDirectory(bool collapse =true);

  /**
   * Change directory to the directory specified
   */
  static int ChangeDirectory(const std::string& dir);

  /**
   * Get the result of strerror(errno)
   */
  static std::string GetLastSystemError();

  /**
   * When building DEBUG with MSVC, this enables a hook that prevents
   * error dialogs from popping up if the program is being run from
   * DART.
   */
  static void EnableMSVCDebugHook();

  /**
   * Get the width of the terminal window. The code may or may not work, so
   * make sure you have some resonable defaults prepared if the code returns
   * some bogus size.
   */
  static int GetTerminalWidth();

  /**
   * Add an entry in the path translation table.
   */
  static void AddTranslationPath(const std::string& dir, const std::string& refdir);

  /**
   * If dir is different after CollapseFullPath is called,
   * Then insert it into the path translation table
   */
  static void AddKeepPath(const std::string& dir);

  /**
   * Update path by going through the Path Translation table;
   */
  static void CheckTranslationPath(std::string & path);

  /**
   * Delay the execution for a specified amount of time specified
   * in miliseconds
   */
  static void Delay(unsigned int msec);

  /**
   * Get the operating system name and version
   * This is implemented for Win32 only for the moment
   */
  static std::string GetOperatingSystemNameAndVersion();

  /** -----------------------------------------------------------------
   *               URL Manipulation Routines
   *  -----------------------------------------------------------------
   */

  /**
   * Parse a character string :
   *       protocol://dataglom
   * and fill protocol as appropriate.
   * Return false if the URL does not have the required form, true otherwise.
   */
   static bool ParseURLProtocol( const std::string& URL,
                                 std::string& protocol,
                                 std::string& dataglom );

  /**
   * Parse a string (a URL without protocol prefix) with the form:
   *  protocol://[[username[':'password]'@']hostname[':'dataport]]'/'[datapath]
   * and fill protocol, username, password, hostname, dataport, and datapath
   * when values are found.
   * Return true if the string matches the format; false otherwise.
   */
  static bool ParseURL( const std::string& URL,
                        std::string& protocol,
                        std::string& username,
                        std::string& password,
                        std::string& hostname,
                        std::string& dataport,
                        std::string& datapath );

private:
  /**
   * Allocate the stl map that serve as the Path Translation table.
   */
  static void ClassInitialize();

  /**
   * Deallocate the stl map that serve as the Path Translation table.
   */
  static void ClassFinalize();

  /**
   * This method prevents warning on SGI
   */
  SystemToolsManager* GetSystemToolsManager()
    {
    return &SystemToolsManagerInstance;
    }

  /**
   * Actual implementation of ReplaceString.
   */
  static void ReplaceString(std::string& source,
                            const char* replace,
                            size_t replaceSize,
                            const std::string& with);

  /**
   * Actual implementation of FileIsFullPath.
   */
  static bool FileIsFullPath(const char*, size_t);

  /**
   * Find a filename (file or directory) in the system PATH, with
   * optional extra paths.
   */
  static std::string FindName(
    const std::string& name,
    const std::vector<std::string>& path =
    std::vector<std::string>(),
    bool no_system_path = false);


  /**
   * Path translation table from dir to refdir
   * Each time 'dir' will be found it will be replace by 'refdir'
   */
  static SystemToolsTranslationMap *TranslationMap;
#ifdef _WIN32
  static SystemToolsPathCaseMap *PathCaseMap;
#endif
#ifdef __CYGWIN__
  static SystemToolsTranslationMap *Cyg2Win32Map;
#endif
  friend class SystemToolsManager;
};

} // namespace itksys

#endif
