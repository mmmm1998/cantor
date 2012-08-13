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

#include "testmaxima.h"

#include "session.h"
#include "backend.h"
#include "expression.h"
#include "result.h"
#include "imageresult.h"
#include "epsresult.h"

#include <kdebug.h>

QString TestMaxima::backendName()
{
    return "maxima";
}


void TestMaxima::testSimpleCommand()
{
    Cantor::Expression* e=evalExp( "2+2" );

    QVERIFY( e!=0 );
    QVERIFY( e->result()!=0 );

    QCOMPARE( cleanOutput( e->result()->toHtml() ), QString("4") );
}

void TestMaxima::testMultilineCommand()
{
    Cantor::Expression* e=evalExp( "2+2;3+3" );

    QVERIFY( e!=0 );
    QVERIFY( e->result()!=0 );

    QString result=e->result()->toHtml();

    QCOMPARE( cleanOutput(result ), QString("4\n6") );
}

//WARNING: for this test to work, Integration of Plots must be anabled
//and CantorLib must be compiled with EPS-support
void TestMaxima::testPlot()
{
    Cantor::Expression* e=evalExp( "plot2d(sin(x), [x, -10,10])" );

    QVERIFY( e!=0 );
    QVERIFY( e->result()!=0 );

    QEXPECT_FAIL("",  "In the current implementation the image result might arrive after the expression is done running",  Continue);
    QCOMPARE( e->result()->type(), (int)Cantor::EpsResult::Type );
    QVERIFY( !e->result()->data().isNull() );
    QVERIFY( e->errorMessage().isNull() );
}

void TestMaxima::testInvalidSyntax()
{
    Cantor::Expression* e=evalExp( "2+2*(" );

    QVERIFY( e!=0 );
    QVERIFY( e->status()==Cantor::Expression::Error );
}

void TestMaxima::testExprNumbering()
{
    Cantor::Expression* e=evalExp( "kill(labels)" ); //first reset the labels

    e=evalExp( "2+2" );
    QVERIFY( e!=0 );
    int id=e->id();
    QCOMPARE( id, 1 );

    e=evalExp( QString("%O%1+1" ).arg( id ) );
    QVERIFY( e != 0 );
    QVERIFY( e->result()!=0 );
    QCOMPARE( cleanOutput( e->result()->toHtml() ), QString( "5" ) );
}

QTEST_MAIN( TestMaxima )

#include "testmaxima.moc"
