/*============================================================================
  KWSys - Kitware System Library
  Copyright 2000-2009 Kitware, Inc., Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#ifndef itksys_String_hxx
#define itksys_String_hxx

#include <string>

namespace itksys
{

/** \class String
 * \brief Short-name version of the STL basic_string class template.
 *
 * The standard library "string" type is actually a typedef for
 * "basic_string<..long argument list..>".  This string class is
 * simply a subclass of this type with the same interface so that the
 * name is shorter in debugging symbols and error messages.
 */
class String: public std::string
{
  /** The original string type.  */
  typedef std::string stl_string;

public:

  /** String member types.  */
  typedef stl_string::value_type             value_type;
  typedef stl_string::pointer                pointer;
  typedef stl_string::reference              reference;
  typedef stl_string::const_reference        const_reference;
  typedef stl_string::size_type              size_type;
  typedef stl_string::difference_type        difference_type;
  typedef stl_string::iterator               iterator;
  typedef stl_string::const_iterator         const_iterator;
  typedef stl_string::reverse_iterator       reverse_iterator;
  typedef stl_string::const_reverse_iterator const_reverse_iterator;

  /** String constructors.  */
  String(): stl_string() {}
  String(const value_type* s): stl_string(s) {}
  String(const value_type* s, size_type n): stl_string(s, n) {}
  String(const stl_string& s, size_type pos=0, size_type n=npos):
    stl_string(s, pos, n) {}
}; // End Class: String

#if defined(__WATCOMC__)
inline bool operator<(String const& l, String const& r)
  {
  return (static_cast<std::string const&>(l) <
          static_cast<std::string const&>(r));
  }
#endif

} // namespace itksys

#endif
