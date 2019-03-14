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
    Copyright (C) 2011 Alexander Rieder <alexanderrieder@gmail.com>
 */

#include "latexrenderer.h"
using namespace Cantor;

#include <KProcess>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QEventLoop>
#include <QTemporaryFile>
#include <KColorScheme>

#include <config-cantorlib.h>
#include "settings.h"

class Cantor::LatexRendererPrivate
{
  public:
    QString latexCode;
    QString header;
    LatexRenderer::Method method;
    bool isEquationOnly;
    LatexRenderer::EquationType equationType;
    QString errorMessage;
    bool success;
    QString latexFilename;
    QTemporaryFile* texFile;
};

static const QLatin1String tex("\\documentclass[12pt,fleqn]{article}"\
                         "\\usepackage{latexsym,amsfonts,amssymb,ulem}"\
                         "\\usepackage{amsmath}"\
                         "\\usepackage[dvips]{graphicx}"\
                         "\\usepackage[utf8]{inputenc}"\
                         "\\usepackage{xcolor}"\
                         "\\setlength\\textwidth{5in}"\
                         "\\setlength{\\parindent}{0pt}"\
                         "%1"\
                         "\\pagecolor[rgb]{%2,%3,%4}"\
                         "\\pagestyle{empty}"\
                         "\\begin{document}"\
                         "\\color[rgb]{%5,%6,%7}"\
                         "%8"\
                         "\\end{document}");

static const QLatin1String eqnHeader("\\begin{eqnarray*}%1\\end{eqnarray*}");
static const QLatin1String inlineEqnHeader("$%1$");

LatexRenderer::LatexRenderer(QObject* parent) : QObject(parent),
                                                d(new LatexRendererPrivate)
{
    d->method=LatexMethod;
    d->isEquationOnly=false;
    d->equationType=InlineEquation;
    d->success=false;
    d->texFile=nullptr;
}

LatexRenderer::~LatexRenderer()
{
    delete d;
}

QString LatexRenderer::latexCode() const
{
    return d->latexCode;
}

void LatexRenderer::setLatexCode(const QString& src)
{
    d->latexCode=src;
}

QString LatexRenderer::header() const
{
    return d->header;
}

void LatexRenderer::addHeader(const QString& header)
{
    d->header.append(header);
}

void LatexRenderer::setHeader(const QString& header)
{
    d->header=header;
}

LatexRenderer::Method LatexRenderer::method() const
{
    return d->method;
}

void LatexRenderer::setMethod(LatexRenderer::Method method)
{
    d->method=method;
}

void LatexRenderer::setEquationType(LatexRenderer::EquationType type)
{
    d->equationType=type;
}

LatexRenderer::EquationType LatexRenderer::equationType() const
{
    return d->equationType;
}


void LatexRenderer::setErrorMessage(const QString& msg)
{
    d->errorMessage=msg;
}

QString LatexRenderer::errorMessage() const
{
    return d->errorMessage;
}

bool LatexRenderer::renderingSuccessful() const
{
    return d->success;
}

void LatexRenderer::setEquationOnly(bool isEquationOnly)
{
    d->isEquationOnly=isEquationOnly;
}

bool LatexRenderer::isEquationOnly() const
{
    return d->isEquationOnly;
}


QString LatexRenderer::imagePath() const
{
    return d->latexFilename;
}

void LatexRenderer::render()
{
    switch(d->method)
    {
        case LatexRenderer::LatexMethod:  renderWithLatex(); break;
        case LatexRenderer::MmlMethod:    renderWithMml(); break;
    };
}

void LatexRenderer::renderBlocking()
{
    QEventLoop event;
    connect(this, &LatexRenderer::done, &event, &QEventLoop::quit);
    connect(this, &LatexRenderer::error, &event, &QEventLoop::quit);

    render();
    event.exec();
}

void LatexRenderer::renderWithLatex()
{
    qDebug()<<"rendering using latex method";
    QString dir=QStandardPaths::writableLocation(QStandardPaths::TempLocation);

    if (d->texFile)
        delete d->texFile;

    d->texFile=new QTemporaryFile(dir + QDir::separator() + QLatin1String("cantor_tex-XXXXXX.tex"));
    d->texFile->open();

    KColorScheme scheme(QPalette::Active);
    const QColor &backgroundColor=scheme.background().color();
    const QColor &foregroundColor=scheme.foreground().color();
    QString expressionTex=tex;
    expressionTex=expressionTex.arg(d->header)
                               .arg(backgroundColor.redF()).arg(backgroundColor.greenF()).arg(backgroundColor.blueF())
                               .arg(foregroundColor.redF()).arg(foregroundColor.greenF()).arg(foregroundColor.blueF());
    if(isEquationOnly())
    {
        switch(equationType())
        {
            case FullEquation: expressionTex=expressionTex.arg(eqnHeader); break;
            case InlineEquation: expressionTex=expressionTex.arg(inlineEqnHeader); break;
        }
    }
    expressionTex=expressionTex.arg(d->latexCode);

//     qDebug()<<"full tex:\n"<<expressionTex;

    d->texFile->write(expressionTex.toUtf8());
    d->texFile->flush();

    QString fileName = d->texFile->fileName();
    qDebug()<<"fileName: "<<fileName;
    d->latexFilename=fileName;
    d->latexFilename.replace(QLatin1String(".tex"), QLatin1String(".eps"));
    KProcess *p=new KProcess( this );
    p->setWorkingDirectory(dir);

    (*p)<<Settings::self()->latexCommand()<<QStringLiteral("-interaction=batchmode")<<QStringLiteral("-halt-on-error")<<fileName;

    connect(p, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(convertToPs()) );
    p->start();
}

void LatexRenderer::convertToPs()
{
    QString dviFile=d->latexFilename;
    dviFile.replace(QLatin1String(".eps"), QLatin1String(".dvi"));
    KProcess *p=new KProcess( this );
    qDebug()<<"converting to eps: "<<Settings::self()->dvipsCommand()<<"-E"<<"-o"<<d->latexFilename<<dviFile;
    (*p)<<Settings::self()->dvipsCommand()<<QStringLiteral("-E")<<QStringLiteral("-o")<<d->latexFilename<<dviFile;

    connect(p, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(convertingDone()) );
    p->start();
}

void LatexRenderer::convertingDone()
{
    QFileInfo info(d->latexFilename);
    qDebug() <<"remove temporary files for " << d->latexFilename;

    delete d->texFile;
    d->texFile = nullptr;

    QString pathWithoutExtention = info.path() + QDir::separator() + info.completeBaseName();
    QFile::remove(pathWithoutExtention + QLatin1String(".log"));
    QFile::remove(pathWithoutExtention + QLatin1String(".aux"));
    QFile::remove(pathWithoutExtention + QLatin1String(".tex"));
    QFile::remove(pathWithoutExtention + QLatin1String(".dvi"));

    if(info.exists())
    {
        d->success=true;
        emit done();
    }else
    {
        d->success=false;
        setErrorMessage(QStringLiteral("failed to create the latex preview image"));
        emit error();
    }
}

void LatexRenderer::renderWithMml()
{
    qDebug()<<"WARNING: MML rendering not implemented yet!";
    emit done();
}
