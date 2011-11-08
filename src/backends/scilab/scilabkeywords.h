/*
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA  02110-1301, USA.

    ---
    Copyright (C) 2011 Filipe Saraiva <filip.saraiva@gmail.com>
 */

#ifndef _SCILABKEYWORDS_H
#define _SCILABKEYWORDS_H

#include <QStringList>

/*
  Class storint a listsof names, known to maxima
  used for syntax highlighting and tab completion
 */
class ScilabKeywords
{
  private:
    ScilabKeywords();
    ~ScilabKeywords();
  public:
    static ScilabKeywords* instance();

    const QStringList& functions() const;
    const QStringList& keywords() const;
    const QStringList& variables() const;

  private:
    void loadFromFile();

  private:
    QStringList m_functions;
    QStringList m_keywords;
    QStringList m_variables;
};
#endif /* _SCILABKEYWORDS_H */