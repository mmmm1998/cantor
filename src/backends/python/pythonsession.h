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
    Copyright (C) 2012 Filipe Saraiva <filipe@kde.org>
    Copyright (C) 2015 Minh Ngo <minh@fedoraproject.org>
 */

#ifndef _PYTHONSESSION_H
#define _PYTHONSESSION_H

#include "session.h"
#include <QStringList>
#include <QUrl>
#include <QProcess>

class PythonSession : public Cantor::Session
{
  Q_OBJECT
  public:
    PythonSession(Cantor::Backend* backend);
    ~PythonSession() override;

    void login() override;
    void logout() override;

    void interrupt() override;

    Cantor::Expression* evaluateExpression(const QString& command, Cantor::Expression::FinishingBehavior behave = Cantor::Expression::FinishingBehavior::DoNotDelete, bool internal = false) override;
    Cantor::CompletionObject* completionFor(const QString& command, int index=-1) override;
    QSyntaxHighlighter* syntaxHighlighter(QObject* parent) override;
    void setWorksheetPath(const QString& path) override;

    QString plotFilePrefixPath();
    int& plotFileCounter();

  private:
    QProcess* m_process;
    QString m_worksheetPath;
    QString m_output;
    QString m_plotFilePrefixPath;
    int m_plotFileCounter;

  private Q_SLOT:
    void readOutput();
    void reportServerProcessError(QProcess::ProcessError serverError);

  private:
    void runFirstExpression() override;
    void updateGraphicPackagesFromSettings();
    QString graphicPackageErrorMessage(QString packageId) const override;

    void sendCommand(const QString& command, const QStringList arguments = QStringList()) const;
};

#endif /* _PYTHONSESSION_H */
