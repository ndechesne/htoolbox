/*
    Copyright (C) 2011  Hervé Fache

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _CRITICALITY_H
#define _CRITICALITY_H

namespace htoolbox {
  //! Levels
  enum Level {
    // These go to error output
    alert,        /*!< Your're dead */
    error,        /*!< Big issue, but might recover */
    warning,      /*!< Non-blocking issue */
    // These go to standard output
    info,         /*!< Basic information */
    verbose,      /*!< Extra information */
    debug,        /*!< Developper information */
    regression    /*!< For regression testing purposes */
  };

  class Criticality {
    Level _level;
  public:
    Criticality(Level level = info) : _level(level) {}
    void set(Level level) { _level = level; };
    operator Level () const { return _level; }
    int toInt() const { return _level; }
    const char* toString() const;
    int setFromInt(int level);
    int setFromString(const char* str);
    bool operator<(const Criticality& c) const { return _level < c._level; }
    bool operator>(const Criticality& c) const { return _level > c._level; }
    bool operator<=(const Criticality& c) const { return _level <= c._level; }
    bool operator>=(const Criticality& c) const { return _level >= c._level; }
    bool operator==(const Criticality& c) const { return _level == c._level; }
    bool operator!=(const Criticality& c) const { return _level != c._level; }
  };
}

#endif // _CRITICALITY_H
