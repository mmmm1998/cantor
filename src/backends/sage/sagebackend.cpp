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

#include "sagebackend.h"

#include "sagesession.h"
#include "settings.h"
#include "ui_settings.h"
#include "sageextensions.h"
#include "sagehighlighter.h"

#include "kdebug.h"
#include <QWidget>

#include "cantor_macros.h"


SageBackend::SageBackend( QObject* parent,const QList<QVariant> args ) : Cantor::Backend( parent,args )
{
    setObjectName("sagebackend");
    kDebug()<<"Creating SageBackend";
    //initialize the supported extensions
    new SageHistoryExtension(this);
    new SageScriptExtension(this);
    new SageCASExtension(this);
    new SageCalculusExtension(this);
    new SageLinearAlgebraExtension(this);
    new SagePlotExtension(this);
}

SageBackend::~SageBackend()
{
    kDebug()<<"Destroying SageBackend";
}

Cantor::Session* SageBackend::createSession()
{
    kDebug()<<"Spawning a new Sage session";

    return new SageSession(this);
}

Cantor::Backend::Capabilities SageBackend::capabilities()
{
    kDebug()<<"Requesting capabilities of SageSession";
    return Cantor::Backend::LaTexOutput|Cantor::Backend::SyntaxHighlighting|Cantor::Backend::TabCompletion;
}

bool SageBackend::requirementsFullfilled()
{
    QFileInfo info(SageSettings::self()->path().toLocalFile());
    return info.isExecutable();
}

QWidget* SageBackend::settingsWidget(QWidget* parent)
{
    QWidget* widget=new QWidget(parent);
    Ui::SageSettingsBase s;
    s.setupUi(widget);
    return widget;
}

KConfigSkeleton* SageBackend::config()
{
    return SageSettings::self();
}

QString SageBackend::description()
{
    return i18n("Sage is a free open-source mathematics software system licensed under the GPL. <br/>" \
                "It combines the power of many existing open-source packages into a common Python-based interface.");
}

K_EXPORT_CANTOR_PLUGIN(sagebackend, SageBackend)

#include "sagebackend.moc"
