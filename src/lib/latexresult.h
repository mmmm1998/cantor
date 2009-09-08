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
    Copyright (C) 2009 Alexander Rieder <alexanderrieder@gmail.com>
 */

#ifndef _LATEXRESULT_H
#define _LATEXRESULT_H

#include "epsresult.h"

namespace MathematiK{
class LatexResultPrivate;

/**Class used for LaTeX results, it is basically an Eps result,
   but it exports a different type, and additionally stores the
   LaTeX code, used to generate the Eps, so it can be retrieved
   later
**/
class LatexResult : public EpsResult
{
  public:
    enum {Type=7};
    LatexResult( const QString& code, const KUrl& url);
    ~LatexResult();
    
    int type();

    QString code();

  private:
    LatexResultPrivate* d;
};

}

#endif /* _LATEXRESULT_H */
