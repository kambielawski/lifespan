/*============================================================================
  KWSys - Kitware System Library
  Copyright 2000-2009 Kitware, Inc., Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#ifndef itksys_Glob_hxx
#define itksys_Glob_hxx

#include <itksys/Configure.h>
#include <itksys/Configure.hxx>

#include <string>
#include <vector>

namespace itksys
{

class GlobInternals;

/** \class Glob
 * \brief Portable globbing searches.
 *
 * Globbing expressions are much simpler than regular
 * expressions. This class will search for files using
 * globbing expressions.
 *
 * Finds all files that match a given globbing expression.
 */
class itksys_EXPORT Glob
{
public:
  enum MessageType
  {
    error,
    cyclicRecursion
  };

  struct Message
  {
    MessageType type;
    std::string content;

    Message(MessageType t, const std::string& c) :
      type(t),
      content(c)
    {}
    Message(const Message& msg) :
      type(msg.type),
      content(msg.content)
    {}
    Message& operator=(Message const& msg)
      {
      this->type = msg.type;
      this->content = msg.content;
      return *this;
      }
  };

  typedef std::vector<Message> GlobMessages;
  typedef std::vector<Message>::iterator GlobMessagesIterator;
public:
  Glob();
  ~Glob();

  //! Find all files that match the pattern.
  bool FindFiles(const std::string& inexpr,
    GlobMessages* messages = 0);

  //! Return the list of files that matched.
  std::vector<std::string>& GetFiles();

  //! Set recurse to true to match subdirectories.
  void RecurseOn() { this->SetRecurse(true); }
  void RecurseOff() { this->SetRecurse(false); }
  void SetRecurse(bool i) { this->Recurse = i; }
  bool GetRecurse() { return this->Recurse; }

  //! Set recurse through symlinks to true if recursion should traverse the
  // linked-to directories
  void RecurseThroughSymlinksOn() { this->SetRecurseThroughSymlinks(true); }
  void RecurseThroughSymlinksOff() { this->SetRecurseThroughSymlinks(false); }
  void SetRecurseThroughSymlinks(bool i) { this->RecurseThroughSymlinks = i; }
  bool GetRecurseThroughSymlinks() { return this->RecurseThroughSymlinks; }

  //! Get the number of symlinks followed through recursion
  unsigned int GetFollowedSymlinkCount() { return this->FollowedSymlinkCount; }

  //! Set relative to true to only show relative path to files.
  void SetRelative(const char* dir);
  const char* GetRelative();

  /** Convert the given globbing pattern to a regular expression.
      There is no way to quote meta-characters.  The
      require_whole_string argument specifies whether the regex is
      automatically surrounded by "^" and "$" to match the whole
      string.  This is on by default because patterns always match
      whole strings, but may be disabled to support concatenating
      expressions more easily (regex1|regex2|etc).  */
  static std::string PatternToRegex(const std::string& pattern,
                                          bool require_whole_string = true,
                                          bool preserve_case = false);

  /** Getters and setters for enabling and disabling directory
      listing in recursive and non recursive globbing mode.
      If listing is enabled in recursive mode it also lists
      directory symbolic links even if follow symlinks is enabled. */
  void SetListDirs(bool list) { this->ListDirs=list; }
  bool GetListDirs() const { return this->ListDirs; }
  void SetRecurseListDirs(bool list) { this->RecurseListDirs=list; }
  bool GetRecurseListDirs() const { return this->RecurseListDirs; }

protected:
  //! Process directory
  void ProcessDirectory(std::string::size_type start,
    const std::string& dir,
    GlobMessages* messages);

  //! Process last directory, but only when recurse flags is on. That is
  // effectively like saying: /path/to/file/**/file
  bool RecurseDirectory(std::string::size_type start,
    const std::string& dir,
    GlobMessages* messages);

  //! Add regular expression
  void AddExpression(const std::string& expr);

  //! Add a file to the list
  void AddFile(std::vector<std::string>& files, const std::string& file);

  GlobInternals* Internals;
  bool Recurse;
  std::string Relative;
  bool RecurseThroughSymlinks;
  unsigned int FollowedSymlinkCount;
  std::vector<std::string> VisitedSymlinks;
  bool ListDirs;
  bool RecurseListDirs;

private:
  Glob(const Glob&);  // Not implemented.
  void operator=(const Glob&);  // Not implemented.
};

} // namespace itksys

#endif
