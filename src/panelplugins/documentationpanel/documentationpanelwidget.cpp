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
    Copyright (C) 2020 Shubham <aryan100jangid@gmail.com>
 */

#include "cantor_macros.h"
#include "documentationpanelplugin.h"

#include <KLocalizedString>
#include <KMessageBox>

#include <QAction>
#include <QCompleter>
#include <QComboBox>
#include <QDebug>
#include <QFrame>
#include <QHBoxLayout>
#include <QHelpContentWidget>
#include <QHelpEngine>
#include <QHelpIndexWidget>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QModelIndex>
#include <QPushButton>
#include <QShortcut>
#include <QStandardPaths>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWebEngineDownloadItem>
#include <QWebEngineProfile>
#include <QWebEngineUrlScheme>
#include <QWebEngineView>
#include <QDebug>

DocumentationPanelWidget::DocumentationPanelWidget(QWidget* parent) : QWidget(parent)
{
    m_previousQch = QString();

    // Maintain a map of backend -> doc files
    m_helpFiles.insert(QLatin1String("Maxima"), {QLatin1String("Maxima_v5.42"), QLatin1String("Maxima_v5.44")});
    m_helpFiles.insert(QLatin1String("Python"), {QLatin1String("Python_v3.8.4"), QLatin1String("NumPy_v1.19")});
    m_helpFiles.insert(QLatin1String("Octave"), {QLatin1String("Octave_v5.2.0")});
    m_helpFiles.insert(QLatin1String("Julia"), {QLatin1String("Julia_v1.5")});

    m_textBrowser = new QWebEngineView(this);
    m_textBrowser->page()->action(QWebEnginePage::ViewSource)->setVisible(false);
    m_textBrowser->page()->action(QWebEnginePage::OpenLinkInNewTab)->setVisible(false);
    m_textBrowser->page()->action(QWebEnginePage::OpenLinkInNewWindow)->setVisible(false);
    m_textBrowser->page()->action(QWebEnginePage::DownloadLinkToDisk)->setVisible(false);
    m_textBrowser->page()->action(QWebEnginePage::Reload)->setVisible(false);

    /////////////////////////
    // Top toolbar layout //
    ///////////////////////
    QPushButton* home = new QPushButton(this);
    home->setIcon(QIcon::fromTheme(QLatin1String("go-home")));
    home->setToolTip(i18nc("@button go to contents page", "Go to the contents"));
    home->setEnabled(false);

    m_documentationSelector = new QComboBox(this);

    // real time searcher
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(i18nc("@info:placeholder", "Search through keywords..."));
    m_search->setClearButtonEnabled(true);

    // Add a seperator
    QFrame *seperator = new QFrame(this);
    seperator->setFrameShape(QFrame::VLine);
    seperator->setFrameShadow(QFrame::Sunken);

    QPushButton* findPage = new QPushButton(this);
    findPage->setEnabled(false);
    findPage->setIcon(QIcon::fromTheme(QLatin1String("edit-find")));
    findPage->setToolTip(i18nc("@info:tooltip", "Find in text of current documentation page"));
    findPage->setShortcut(QKeySequence(/*Qt::CTRL + */Qt::Key_F3));

    QPushButton* resetZoom = new QPushButton(this);
    resetZoom->setEnabled(false);
    resetZoom->setIcon(QIcon::fromTheme(QLatin1String("zoom-fit-best")));
    resetZoom->setToolTip(i18nc("@info:tooltip", "Reset zoom level to 100%"));

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(home);
    layout->addWidget(m_documentationSelector);
    layout->addWidget(m_search);
    layout->addWidget(seperator);
    layout->addWidget(findPage);
    layout->addWidget(resetZoom);

    QWidget* toolBarContainer = new QWidget(this);
    toolBarContainer->setLayout(layout);

    // Add zoom in, zoom out behaviour on SHIFT++ and SHIFT--
    auto zoomIn = new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Plus), this);
    zoomIn->setContext(Qt::WidgetWithChildrenShortcut);

    connect(zoomIn, &QShortcut::activated, this, [=]{
        m_textBrowser->setZoomFactor(m_textBrowser->zoomFactor() + 0.1);
        emit zoomFactorChanged();
    });

    auto zoomOut = new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Minus), this);
    zoomOut->setContext(Qt::WidgetWithChildrenShortcut);

    connect(zoomOut, &QShortcut::activated, this, [=]{
        m_textBrowser->setZoomFactor(m_textBrowser->zoomFactor() - 0.1);
        emit zoomFactorChanged();
    });

    connect(this, &DocumentationPanelWidget::zoomFactorChanged, [=]{
        if(m_textBrowser->zoomFactor() != 1.0)
            resetZoom->setEnabled(true);
        else
            resetZoom->setEnabled(false);
    });

    // Later on, add Contents, Browser and Index on this stacked widget whenever setBackend() is called
    m_displayArea = new QStackedWidget(this);

    /////////////////////////////////
    // Find in Page widget layout //
    ///////////////////////////////

    // Add the Find in Page widget at the bottom, add all the widgets into a layout so that we can hide it
    QToolButton* hideButton = new QToolButton(this);
    hideButton->setIcon(QIcon::fromTheme(QLatin1String("dialog-close")));
    hideButton->setToolTip(i18nc("@info:tooltip", "Close"));

    QLabel* label = new QLabel(this);
    label->setText(i18n("Find:"));

    m_findText = new QLineEdit(this);
    m_findText->setClearButtonEnabled(true);

    QToolButton* next = new QToolButton(this);
    next->setIcon(QIcon::fromTheme(QLatin1String("go-down-search")));
    next->setToolTip(i18nc("@info:tooltip", "Jump to next match"));

    QToolButton* previous = new QToolButton(this);
    previous->setIcon(QIcon::fromTheme(QLatin1String("go-up-search")));
    previous->setToolTip(i18nc("@info:tooltip", "Jump to previous match"));

    m_matchCase = new QToolButton(this);
    m_matchCase->setIcon(QIcon::fromTheme(QLatin1String("format-text-superscript")));
    m_matchCase->setToolTip(i18nc("@info:tooltip", "Match case sensitively"));
    m_matchCase->setCheckable(true);

    // Create a layout for find in text widgets
    QHBoxLayout* lout = new QHBoxLayout(this);
    lout->addWidget(hideButton);
    lout->addWidget(label);
    lout->addWidget(m_findText);
    lout->addWidget(next);
    lout->addWidget(previous);
    lout->addWidget(m_matchCase);

    QWidget* findPageWidgetContainer = new QWidget(this);
    findPageWidgetContainer->setLayout(lout);
    findPageWidgetContainer->hide();

    // Add topmost toolbar, display area and find in page widget in a Vertical layout
    QVBoxLayout* vlayout = new QVBoxLayout(this);
    vlayout->addWidget(toolBarContainer);
    vlayout->addWidget(m_displayArea);
    vlayout->addWidget(findPageWidgetContainer);

    connect(this, &DocumentationPanelWidget::activateBrowser, [=]{
        m_textBrowser->hide();
        m_displayArea->setCurrentIndex(1);
        m_textBrowser->show();
    });

    connect(m_documentationSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DocumentationPanelWidget::updateDocumentation);

    connect(m_displayArea, &QStackedWidget::currentChanged, [=]{
        //disable Home and Search in Page buttons when stackwidget shows contents widget, enable when shows web browser
        if(m_displayArea->currentIndex() != 1) //0->contents 1->browser
        {
            findPage->setEnabled(false);
            home->setEnabled(false);
        }
        else
        {
            findPage->setEnabled(true);
            home->setEnabled(true);
        }
    });

    connect(home, &QPushButton::clicked, [=]{
        m_displayArea->setCurrentIndex(0);
        findPageWidgetContainer->hide();
    });

    connect(resetZoom, &QPushButton::clicked, [=]{
        m_textBrowser->setZoomFactor(1.0);
        resetZoom->setEnabled(false);
    });

    connect(m_search, &QLineEdit::returnPressed, this, &DocumentationPanelWidget::returnPressed);

    // connect statements for Find in Page text widget
    connect(findPage, &QPushButton::clicked, [=]{
        findPageWidgetContainer->show();
        m_findText->clear();
        m_findText->setFocus();
    });

    connect(hideButton, &QToolButton::clicked, this, [=]{
        findPageWidgetContainer->hide();
        m_textBrowser->findText(QString()); // this clears up the selected text
    });

    connect(m_findText, &QLineEdit::returnPressed, this, &DocumentationPanelWidget::searchForward);
    connect(m_findText, &QLineEdit::textEdited, this, &DocumentationPanelWidget::searchForward); // for highlighting found string in real time
    connect(next, &QToolButton::clicked, this, &DocumentationPanelWidget::searchForward);
    connect(previous, &QToolButton::clicked, this, &DocumentationPanelWidget::searchBackward);
    connect(m_matchCase, &QAbstractButton::toggled, this, &DocumentationPanelWidget::searchForward);
    connect(m_matchCase, &QAbstractButton::toggled, this, [=]{
        m_textBrowser->findText(QString());
        searchForward();
    });

    // for webenginebrowser for downloading of images or html pages
    connect(m_textBrowser->page()->profile(), &QWebEngineProfile::downloadRequested, this, &DocumentationPanelWidget::downloadResource);
}

DocumentationPanelWidget::~DocumentationPanelWidget()
{
    delete m_index;
    delete m_content;
    delete m_engine;
    delete m_textBrowser;
    delete m_displayArea;
    delete m_search;
    delete m_findText;
    delete m_matchCase;
    delete m_documentationSelector;
}

void DocumentationPanelWidget::updateBackend(const QString& newBackend, const QString& icon)
{
    Q_UNUSED(icon)

    //nothing to do if the same backend was provided
    if(m_backend == newBackend)
        return;

    m_backend = newBackend;

    // show all available documentation files for the new backend
    m_initializing = true;
    m_documentationSelector->clear();
    m_documentationSelector->addItems(m_helpFiles[m_backend]);
    m_initializing = false;

    //select the first available documentation file which will trigger the re-initialization of QHelpEngine
    //TODO: restore from the saved state the previously selected documentation in m_documentationSelector for the current backend
    m_documentationSelector->setCurrentIndex(0);
    updateDocumentation();
}

/*!
 * called if another documentation file was selected in the ComboBox for all available documentations
 * for the current backend. This slot triggers the re-initialization of QHelpEngine with the proper
 * documentation file and also updates the content of the widgets showing the documentation.
 */
void DocumentationPanelWidget::updateDocumentation()
{
    if (m_initializing)
        return;

    //remove the currently shown widgets
    if(m_displayArea->count())
    {
        for(int i = m_displayArea->count(); i >= 0; i--)
            m_displayArea->removeWidget(m_displayArea->widget(i));

        m_search->clear();
    }

    //unregister the previous help engine qch files
    if(m_previousQch != QString())
    {
        const QString& fileNamespace = QHelpEngineCore::namespaceName(m_previousQch);
        if(m_engine->registeredDocumentations().contains(fileNamespace))
            m_engine->unregisterDocumentation(m_previousQch);
    }

    //initialize the Qt Help engine and provide the proper help collection file for the current backend
    //and for the currently selected documentation for this backend
    const QString& docSelected = m_documentationSelector->currentText();
    const QString& fileName = QStandardPaths::locate(QStandardPaths::AppDataLocation,
                                                     QLatin1String("documentation/") + m_backend + QLatin1String("/") +
                                                     docSelected + QLatin1String("/help.qhc"));

    m_engine = new QHelpEngine(fileName, this);
    /*if(!m_engine->setupData())
         qWarning() << "Couldn't setup QtHelp Engine: " << m_engine->error();*/

    if(m_backend != QLatin1String("Octave"))
      m_engine->setProperty("_q_readonly", QVariant::fromValue<bool>(true));

    //index widget
    m_index = m_engine->indexWidget();
    connect(m_index, &QHelpIndexWidget::linkActivated, this, &DocumentationPanelWidget::displayHelp);
    connect(m_index, &QHelpIndexWidget::linkActivated, this, &DocumentationPanelWidget::displayHelp);

    //content widget
    m_content = m_engine->contentWidget();
    connect(m_content, &QHelpContentWidget::linkActivated, this, &DocumentationPanelWidget::displayHelp);
    connect(m_content, &QHelpContentWidget::linkActivated, this, &DocumentationPanelWidget::displayHelp);

    //TODO QHelpIndexWidget::linkActivated is obsolete, use QHelpIndexWidget::documentActivated instead
    // display the documentation browser whenever contents are clicked
    connect(m_content, &QHelpContentWidget::linkActivated, [=]{
        m_displayArea->setCurrentIndex(1);
    });

    //search widget
    auto* completer = new QCompleter(m_index->model(), m_search);
    m_search->setCompleter(completer);
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    connect(completer, QOverload<const QModelIndex&>::of(&QCompleter::activated), this, &DocumentationPanelWidget::returnPressed);

    //add the widgets to the display area
    m_displayArea->addWidget(m_content);
    m_displayArea->addWidget(m_textBrowser);
    /* Adding the index widget to implement the logic for context sensitive help
     * This widget would be NEVER shown*/
    m_displayArea->addWidget(m_index);

    // handle the URL scheme handler
    //m_textBrowser->page()->profile()->removeUrlScheme("qthelp");
    m_textBrowser->page()->profile()->removeAllUrlSchemeHandlers(); // remove previously installed scheme handler and then install new one
    m_textBrowser->page()->profile()->installUrlSchemeHandler("qthelp", new QtHelpSchemeHandler(m_engine));

    // register the compressed help file (qch)
    const QString& qchFileName = QStandardPaths::locate(QStandardPaths::AppDataLocation,
                                                        QLatin1String("documentation/") + m_backend + QLatin1String("/") + docSelected +
                                                        QLatin1String("/help.qch"));
    m_previousQch = qchFileName;

    const QString& nameSpace = QHelpEngineCore::namespaceName(qchFileName);
    if(!m_engine->registeredDocumentations().contains(nameSpace))
    {
        if(m_engine->registerDocumentation(qchFileName))
            qDebug()<<"The documentation file " << qchFileName << " successfully registered.";
        else
            qWarning() << m_engine->error();
    }
}

void DocumentationPanelWidget::displayHelp(const QUrl& url)
{
    qDebug() << url;
    m_textBrowser->load(url);
    m_textBrowser->show();
}

void DocumentationPanelWidget::returnPressed()
{
    const QString& input = m_search->text();

    if (input.isEmpty())
        return;

    contextSensitiveHelp(input);
}

void DocumentationPanelWidget::contextSensitiveHelp(const QString& keyword)
{
    qDebug() << keyword;

    // First make sure we have display browser as the current widget on the QStackedWidget
    emit activateBrowser();

    m_index->filterIndices(keyword); // filter exactly, no wildcards
    m_index->activateCurrentItem(); // this internally emitts the QHelpIndexWidget::linkActivated signal

    // called in order to refresh and restore the index widget
    // otherwise filterIndices() filters the indices list, and then the index widget only contains the matched keywords
    m_index->filterIndices(QString());
}

void DocumentationPanelWidget::searchForward()
{
    m_matchCase->isChecked() ? m_textBrowser->findText(m_findText->text(), QWebEnginePage::FindCaseSensitively) :
                               m_textBrowser->findText(m_findText->text());
}

void DocumentationPanelWidget::searchBackward()
{
    m_matchCase->isChecked() ? m_textBrowser->findText(m_findText->text(), QWebEnginePage::FindCaseSensitively | QWebEnginePage::FindBackward) :
                               m_textBrowser->findText(m_findText->text(), QWebEnginePage::FindBackward);
}

void DocumentationPanelWidget::downloadResource(QWebEngineDownloadItem* resource)
{
    // default download directory is ~/Downloads on Linux
    m_textBrowser->page()->download(resource->url());
    resource->accept();

    KMessageBox::information(this, i18n("The file has been downloaded successfully at Downloads."), i18n("Download Successfull"));

    disconnect(m_textBrowser->page()->profile(), &QWebEngineProfile::downloadRequested, this, &DocumentationPanelWidget::downloadResource);
}