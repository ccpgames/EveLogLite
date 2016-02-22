#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "logmodel.h"
#include "logfilter.h"
#include "overlaylayout.h"
#include "logstatistics.h"
#include "logmonitorfilemodel.h"
#include <QShortcut>
#include <QMenu>
#include <QClipboard>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QFile>
#include <QSettings>
#include <QKeyEvent>
#include <QDesktopServices>
#include <QMimeData>

#include <QPainter>
#ifdef _WIN32
#include <QtWinExtras/QWinTaskbarButton>
#endif

#include "settingsdialog.h"
#include "fixedheader.h"
#include "filterdialog.h"
#include "highlightdialog.h"

namespace
{
const char* columnSettings[] = {
    "columns/time",
    "columns/pid",
    "columns/executable",
    "columns/machine",
    "columns/module",
    "columns/channel",
    "columns/message",
};

void generateOverlayIcons(QIcon* icons)
{
    for (int warnings = 0; warnings < 10; ++warnings)
    {
        for (int errors = 0; errors < 10; ++errors)
        {
            QPixmap pm(16, 16);
            pm.fill(Qt::transparent);
            QPainter p(&pm);
            p.fillRect(QRect(0, 15 - warnings, 8, warnings), QColor(255, 255, 0));
            p.fillRect(QRect(8, 15 - errors, 8, errors), QColor(255, 0, 0));
            icons[errors + 10 * warnings] = QIcon(pm);
        }
    }
}

struct PathRec
{
    QString path;
    int start;
    int length;
};

void findPaths(const QString& string, QList<PathRec>& paths)
{
#ifdef _WIN32
    QRegExp rx("\\w:([/\\\\]+[\\w- \\.]+)+");
#else
    QRegExp rx("([/\\\\]+[\\w- \\.]+)+");
#endif
    QRegExp slash("[/\\\\]");
    int pos = 0;
    while ((pos = rx.indexIn(string, pos)) != -1)
    {
        auto path = QFileInfo(rx.cap(0).trimmed());
        if (path.exists())
        {
            PathRec r;
            r.path = path.canonicalFilePath();
            r.start = pos;
            r.length = rx.matchedLength();
            paths.append(r);
        }
        else
        {
            auto full = rx.cap(0);
            int p = full.lastIndexOf(slash);

            while (true)
            {
                full = full.left(p);
                int p2 = full.lastIndexOf(slash);
                if (p2 == -1)
                {
                    break;
                }
                auto path = QFileInfo(full);
                if (path.exists())
                {
                    PathRec r;
                    r.path = path.canonicalFilePath();
                    r.start = pos;
                    r.length = full.length();
                    paths.append(r);
                    break;
                }
                p = p2;
            }
        }
        pos += rx.matchedLength();
    }
}

}

MainWindow::MainWindow(const QString &fileName, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_taskbarButton(nullptr),
    m_monospaceFont(false)
{
    ui->setupUi(this);

    QSettings settings;

    m_monospaceFont = settings.value("monospaceFont", 0).toBool();

    AbstractLogModel *model;
    if (fileName.isEmpty())
    {
        auto logModel = new LogModel(this);
        model = logModel;
        if (settings.contains("autoSaveDirectory"))
        {
            logModel->setAutoSaveDirectory(settings.value("autoSaveDirectory").toString());
        }
        else
        {
            logModel->setAutoSaveDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        }
        logModel->setMaxMessages(settings.value("maxMessages", 10000).toInt());
        logModel->setServerMode(settings.value("serverMode", false).toBool());
        setWindowTitle(windowTitle().arg(model->isListening() ? "Listening" : "Not listening"));

        connect(ui->actionDisconnectAll, &QAction::triggered, logModel, &LogModel::disconnectAll);
    }
    else
    {
        model = new LogMonitorFileModel(fileName, this);
        setWindowTitle(windowTitle().arg(QFileInfo(fileName).fileName()));
    }
    model->setBreakLines(settings.value("breakLines", 0).toBool());
    model->setColorBackground(LogColorBackground(settings.value("colorBackground", 0).toInt()));
    model->setTimestampPrecision(TimestampPrecision(settings.value("timestampPrecision", 0).toInt()));

    auto filtered = new LogFilter(this);
    filtered->setFilterCaseSensitivity(Qt::CaseInsensitive);
    filtered->setSourceModel(model);
    ui->tableView->setModel(filtered);

    connect(ui->errorFilter, &QToolButton::toggled, filtered, &LogFilter::showErrors);
    connect(ui->warningFilter, &QToolButton::toggled, filtered, &LogFilter::showWarnings);
    connect(ui->noticeFilter, &QToolButton::toggled, filtered, &LogFilter::showNotices);
    connect(ui->infoFilter, &QToolButton::toggled, filtered, &LogFilter::showInfos);

    ui->logMap->setModel(filtered);
    ui->logMap->setBuddyView(ui->tableView);

    auto src = ui->tableParent->layout();
    auto dest = new OverlayLayout(ui->tableParent);
    while (src->count())
    {
        dest->addItem(src->takeAt(0));
    }
    delete src;
    ui->tableParent->setLayout(dest);
    ui->searchBox->setVisible(false);

    ui->tableView->setVerticalHeader(new FixedHeader(Qt::Vertical, ui->tableView));
    ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableView->verticalHeader()->setDefaultSectionSize(23);
    ui->tableView->verticalHeader()->setHighlightSections(true);
    ui->tableView->verticalHeader()->setStretchLastSection(false);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableView->horizontalHeader()->setStretchLastSection(false);
    ui->tableView->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionsMovable(true);
    auto order = settings.value("columnOrder").toByteArray();
    for (int i = 0; i < order.size(); ++i)
    {
        ui->tableView->horizontalHeader()->moveSection(ui->tableView->horizontalHeader()->visualIndex(i), order[i]);
    }
    connect(ui->tableView->horizontalHeader(), &QHeaderView::sectionMoved, this, &MainWindow::columnMoved);

    connect(ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::itemSelected);
    connect(ui->messageText, &QTextBrowser::anchorClicked, this, &MainWindow::anchorClicked);
    connect(ui->tableView, &QTableView::customContextMenuRequested, this, &MainWindow::itemMenu);

    ui->errorFilter->setChecked(settings.value("quickFilter/error", true).toBool());
    ui->warningFilter->setChecked(settings.value("quickFilter/warning", true).toBool());
    ui->noticeFilter->setChecked(settings.value("quickFilter/notice", true).toBool());
    ui->infoFilter->setChecked(settings.value("quickFilter/info", true).toBool());

    if (settings.contains("customFilter"))
    {
        auto name = settings.value("customFilter").toString();
        auto& filters = Filter::getFilters();
        for (auto it = filters.begin(); it != filters.end(); ++it)
        {
            if (it->m_name == name)
            {
                static_cast<LogFilter*>(ui->tableView->model())->setCustomFilter(&(*it));
                ui->actionFilterNone->setChecked(false);
                ui->filterButton->setIcon(QIcon(":/default/filter"));
                ui->filterButton->setToolTip(name);
                break;
            }
        }
    }

    if (settings.contains("customHighlight"))
    {
        auto name = settings.value("customHighlight").toString();
        auto& highlights = HighlightSet::getSets();
        for (auto it = highlights.begin(); it != highlights.end(); ++it)
        {
            if (it->m_name == name)
            {
                static_cast<LogFilter*>(ui->tableView->model())->setHighlight(&(*it));
                ui->actionHighlightNone->setChecked(false);
                ui->highlightsButton->setIcon(QIcon(":/default/highlights"));
                ui->highlightsButton->setToolTip(name);
                break;
            }
        }
    }

    m_quickFilterEdited.setSingleShot(true);
    m_quickFilterEdited.setInterval(1000);
    connect(ui->textFilter, &QLineEdit::textChanged, this, &MainWindow::textFilterChanged);
    connect(ui->textFilter, &QLineEdit::editingFinished, this, &MainWindow::textFilterTimeout);
    connect(&m_quickFilterEdited, &QTimer::timeout, this, &MainWindow::textFilterTimeout);

    connect(ui->searchClose, &QToolButton::clicked, ui->searchBox, &QFrame::hide);
    connect(ui->searchDown, &QToolButton::clicked, this, &MainWindow::findNext);
    connect(ui->searchUp, &QToolButton::clicked, this, &MainWindow::findPrevious);
    ui->searchText->installEventFilter(this);

    ui->actionOpen->setShortcut(QKeySequence(QKeySequence::Open));
    ui->actionSaveAs->setShortcut(QKeySequence(QKeySequence::Save));
    ui->actionExit->setShortcut(QKeySequence(QKeySequence::Quit));
    ui->actionSelectAll->setShortcut(QKeySequence(QKeySequence::SelectAll));
    ui->actionFind->setShortcut(QKeySequence(QKeySequence::Find));
    ui->actionCopy->setShortcut(QKeySequence(QKeySequence::Copy));

    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::saveFile);
    connect(ui->actionExport, &QAction::triggered, this, &MainWindow::exportAsText);

    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionFind, &QAction::triggered, this, &MainWindow::openFind);
    connect(ui->actionCloseFind, &QAction::triggered, ui->searchBox, &QFrame::hide);
    connect(ui->actionSelectAll, &QAction::triggered, this, &MainWindow::selectAll);
    connect(ui->actionCopy, &QAction::triggered, this, &MainWindow::copySelection);
    connect(ui->actionCopyMessagesOnly, &QAction::triggered, this, &MainWindow::copyMessages);

    connect(ui->actionNextError, &QAction::triggered, ui->tableView, &LogView::nextError);
    connect(ui->actionPreviousError, &QAction::triggered, ui->tableView, &LogView::previousError);
    connect(ui->actionNextWarning, &QAction::triggered, ui->tableView, &LogView::nextWarning);
    connect(ui->actionPreviousWarning, &QAction::triggered, ui->tableView, &LogView::previousWarning);
    connect(ui->actionNextNotice, &QAction::triggered, ui->tableView, &LogView::nextNotice);
    connect(ui->actionPreviousNotice, &QAction::triggered, ui->tableView, &LogView::previousNotice);

    connect(ui->actionClear, &QAction::triggered, model, &AbstractLogModel::clear);

    ui->actionServerMode->setChecked(settings.value("serverMode").toBool());
    connect(ui->actionServerMode, &QAction::triggered, this, &MainWindow::setServerMode);
    connect(ui->actionSplitByPIDs, &QAction::triggered, this, &MainWindow::splitByPids);
    ui->actionSplitByPIDs->setChecked(settings.value("splitByPids").toBool());
    splitByPids();
    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::showSettings);
    connect(ui->actionAboutLogLite, &QAction::triggered, this, &MainWindow::showAboutDialog);

    connect(ui->actionScrollToBeginning, &QAction::triggered, ui->tableView, &LogView::scrollToBeginning);
    connect(ui->actionScrollToEnd, &QAction::triggered, ui->tableView, &LogView::scrollToEnd);


    QAction* columnActions[] = {
        ui->actionTime,
        ui->actionPID,
        ui->actionExecutable,
        ui->actionMachine,
        ui->actionModule,
        ui->actionChannel,
        ui->actionMessage,
    };
    for (int i = 0; i < int(sizeof(columnActions)/sizeof(columnActions[0])); ++i)
    {
        if (i + 1 < int(sizeof(columnActions)/sizeof(columnActions[0])))
        {
            auto key = QString(columnSettings[i]) + "Width";
            if (settings.contains(key))
            {
                ui->tableView->setColumnWidth(i, std::max(settings.value(key).toInt(), 32));
            }
        }
        columnActions[i]->setProperty("columnIndex", i);
        columnActions[i]->setProperty("setting", columnSettings[i]);
        connect(columnActions[i], &QAction::toggled, this, &MainWindow::columnMenuChanged);
        columnActions[i]->setChecked(settings.value(columnSettings[i], true).toBool());
        if (!columnActions[i]->isChecked())
        {
            ui->tableView->hideColumn(i);
        }
    }

    ui->tableView->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableView->horizontalHeader(), &QHeaderView::customContextMenuRequested, this, &MainWindow::columnMenu);

    m_stats = new LogStatistics(this);
    m_stats->setModel(model);
    ui->statusBar->addPermanentWidget(m_stats);

    ui->splitter->setCollapsible(0, false);

    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    ui->splitter->restoreState(settings.value("splitterSizes").toByteArray());

    if (fileName.isEmpty())
    {
        ui->tableView->selectionModel()->setCurrentIndex(ui->tableView->model()->index(0, 0), QItemSelectionModel::SelectCurrent|QItemSelectionModel::Rows);
    }

#ifdef _WIN32
    auto timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateTaskbarIcon);
    timer->start(1000);

    generateOverlayIcons(m_icons);
#endif

    connect(ui->actionEditFilters, &QAction::triggered, this, &MainWindow::editFilters);
    connect(ui->actionEditHighlights, &QAction::triggered, this, &MainWindow::editHighlights);

    connect(ui->menuFilters, &QMenu::aboutToShow, this, &MainWindow::buildFiltersMenu);
    connect(ui->menuFilters, &QMenu::triggered, this, &MainWindow::customFilterSet);

    connect(ui->menuHighlights, &QMenu::aboutToShow, this, &MainWindow::buildHighlightsMenu);
    connect(ui->menuHighlights, &QMenu::triggered, this, &MainWindow::highlightSet);

    connect(ui->filterButton, &QToolButton::clicked, this, &MainWindow::showFilterMenu);
    connect(ui->highlightsButton, &QToolButton::clicked, this, &MainWindow::showHighlightsMenu);

    connect(ui->menu_Disconnect_Client, &QMenu::aboutToShow, this, &MainWindow::buildClientsMenu);

    connect(ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);

    setAcceptDrops(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *)
{
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("splitterSizes", ui->splitter->saveState());

    for (int i = 0; i + 1 < int(sizeof(columnSettings)/sizeof(columnSettings[0])); ++i)
    {
        settings.setValue(QString(columnSettings[i]) + "Width", ui->tableView->columnWidth(i));
    }

    settings.setValue("quickFilter/error", ui->errorFilter->isChecked());
    settings.setValue("quickFilter/warning", ui->warningFilter->isChecked());
    settings.setValue("quickFilter/notice", ui->noticeFilter->isChecked());
    settings.setValue("quickFilter/info", ui->infoFilter->isChecked());
}

void MainWindow::itemSelected()
{
    auto selection = ui->tableView->selectionModel();
    auto message = ui->tableView->message(selection->currentIndex().row());
    if (message)
    {
        QList<PathRec> paths;
        auto text = message->originalMessage.toHtmlEscaped();
        findPaths(text, paths);
        for (int i = paths.size() - 1; i >= 0; --i)
        {
            auto& rec = paths[i];
            text.insert(rec.start + rec.length, "</a>");
            text.insert(rec.start, QString("<a href=\"%1\">").arg(rec.path.toHtmlEscaped()));
        }
        if (m_monospaceFont)
        {
            ui->messageText->setHtml("<pre>" + text + "</pre>");
        }
        else
        {
            ui->messageText->setHtml(text.replace("\n", "<br/>"));
        }
    }
    else
    {
        ui->messageText->setPlainText("");
    }
}

void MainWindow::anchorClicked(const QUrl& url)
{
    QDesktopServices::openUrl(url);
}

void MainWindow::itemMenu(QPoint pos)
{
    QMenu* menu = new QMenu(this);

    auto selection = ui->tableView->selectionModel();
    auto message = ui->tableView->message(selection->currentIndex().row());
    if (message)
    {
        QList<PathRec> paths;
        auto text = message->originalMessage.toHtmlEscaped();
        findPaths(text, paths);
        for (int i = 0; i < paths.size(); ++i)
        {
            menu->addAction(paths[i].path, this, "uurlClicked()");
        }
        if (paths.size())
        {
            menu->addSeparator();
        }
        if (auto logModel = dynamic_cast<LogModel*>(ui->tableView->sourceModel()))
        {
            LogModel::Client client;
            if (logModel->clientFromMessage(*message, client))
            {
                QFontMetrics metrics(menu->font());
                auto path = metrics.elidedText(client.path(), Qt::ElideMiddle, 200);
                auto name = QString("Disconnect [%1] %2").arg(client.pid()).arg(path);
                auto action = new QAction(name, this);
                action->setProperty("socket", quint64(client.socket()));
                connect(action, &QAction::triggered, this, &MainWindow::disconnectClient);
                menu->addAction(action);
                menu->addSeparator();
            }
        }
    }
    menu->addAction(ui->actionClear);
    menu->exec(ui->tableView->mapToGlobal(pos));
    delete menu;
}

void MainWindow::urlClicked()
{
    anchorClicked(QUrl(static_cast<QAction*>(sender())->text()));
}

void MainWindow::textFilterChanged()
{
    m_quickFilterEdited.start(1000);
}

void MainWindow::textFilterTimeout()
{
    m_quickFilterEdited.stop();
    static_cast<LogFilter*>(ui->tableView->model())->setFilterFixedString(ui->textFilter->text());
}

void MainWindow::openFind()
{
    ui->searchBox->setVisible(true);
    ui->searchText->setFocus();
}

void MainWindow::columnMenu(const QPoint &position)
{
    ui->menuColumns->exec(ui->tableView->horizontalHeader()->mapToGlobal(position));
}

void MainWindow::columnMenuChanged()
{
    QAction* action = static_cast<QAction*>(sender());

    if (action->isChecked())
    {
        QSettings().setValue(action->property("setting").toString(), true);
        ui->tableView->showColumn(action->property("columnIndex").toInt());
    }
    else
    {
        QSettings().setValue(action->property("setting").toString(), false);
        ui->tableView->hideColumn(action->property("columnIndex").toInt());
    }
}

void MainWindow::columnMoved()
{
    QByteArray indexes;
    for (int i = 0; i < LOGFIELD_MESSAGE; ++i)
    {
        indexes.append(ui->tableView->horizontalHeader()->visualIndex(i));
    }
    QSettings().setValue("columnOrder", indexes);
}

void MainWindow::copySelection()
{
    if (focusWidget() == ui->messageText)
    {
        ui->messageText->copy();
        return;
    }
    QApplication::clipboard()->setText(ui->tableView->selectionAsText());
}

void MainWindow::copyMessages()
{
    QApplication::clipboard()->setText(ui->tableView->selectionMessagesAsText());
}

void MainWindow::selectAll()
{
    if (focusWidget() == ui->messageText)
    {
        ui->messageText->selectAll();
        return;
    }
    ui->tableView->selectAll();
}

void MainWindow::openFile()
{
    auto fileName = QFileDialog::getOpenFileName(this, "Open File", QSettings().value("lastDir").toString(), "CCP LogMonitor files (*.lsw);;All files (*.*)");
    if (fileName.isEmpty())
    {
        return;
    }
    QSettings().setValue("lastDir", QFileInfo(fileName).path());
    openFilePath(fileName);
}

void MainWindow::openFilePath(QString fileName)
{
    auto model = dynamic_cast<LogModel*>(ui->tableView->sourceModel());
    if (model && !model->isListening())
    {
        auto newModel = new LogMonitorFileModel(fileName, this);
        static_cast<LogFilter*>(ui->tableView->model())->setSourceModel(newModel);
        m_stats->setModel(newModel);
        setWindowTitle(QString("LogLite - %1").arg(QFileInfo(fileName).fileName()));
    }
    else
    {
        auto window = new MainWindow(fileName);
        window->show();
    }
}

void MainWindow::saveFile()
{
    auto fileName = QFileDialog::getSaveFileName(this, "Save File", QSettings().value("lastDir").toString(), "CCP LogMonitor files (*.lsw);;All files (*.*)");
    if (fileName.isEmpty())
    {
        return;
    }
    QSettings().setValue("lastDir", QFileInfo(fileName).path());
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();
    auto result = LogMonitorFileModel::saveModel(ui->tableView->sourceModel(), fileName);
    QApplication::restoreOverrideCursor();
    if (!result)
    {
        QMessageBox msg(this);
        msg.setIcon(QMessageBox::Warning);
        msg.setText(QString("Could not save file").arg(fileName));
        msg.setInformativeText(QString("Could not open file %1 for writing").arg(fileName));
        msg.exec();
        return;
    }
}

void MainWindow::exportAsText()
{
    auto fileName = QFileDialog::getSaveFileName(this, "Export Text File", QSettings().value("lastDir").toString(), "Text files (*.txt);;All files (*.*)");
    if (fileName.isEmpty())
    {
        return;
    }
    QFile f(fileName);
    if (!f.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox msg(this);
        msg.setIcon(QMessageBox::Warning);
        msg.setText(QString("Could not save file").arg(fileName));
        msg.setInformativeText(QString("Could not open file %1 for writing").arg(fileName));
        msg.exec();
        return;
    }
    ui->tableView->sourceModel()->saveAsText(&f);
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    if (object == ui->searchText && event->type() == QEvent::KeyPress)
    {
        auto keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
        {
            findNext();
            return true;
        }
    }
    return QMainWindow::eventFilter(object, event);
}

void MainWindow::findNext()
{
    ui->tableView->selectNextMatching(ui->searchText->text());
}

void MainWindow::findPrevious()
{
    ui->tableView->selectPreviousMatching(ui->searchText->text());
}

void MainWindow::setServerMode()
{
    auto model = dynamic_cast<LogModel*>(ui->tableView->sourceModel());
    if (model)
    {
        model->setServerMode(ui->actionServerMode->isChecked());
    }
    QSettings().setValue("serverMode", ui->actionServerMode->isChecked());
}

void MainWindow::splitByPids()
{
    bool split = ui->actionSplitByPIDs->isChecked();
    ui->tableView->sourceModel()->setSplitByPid(split);
    QSettings().setValue("splitByPids", split);
    ui->tableView->horizontalHeader()->setSectionResizeMode(6, split ? QHeaderView::Interactive : QHeaderView::Stretch);
}

void MainWindow::showSettings()
{
    SettingsDialog dlg(this);

    if (dlg.exec() == QDialog::Accepted)
    {
        auto allWindows = QApplication::topLevelWidgets();
        for (auto it = allWindows.begin(); it != allWindows.end(); ++it)
        {
            if (auto wnd = dynamic_cast<MainWindow*>(*it))
            {
                auto model = wnd->ui->tableView->sourceModel();
                QSettings settings;

                model->setBreakLines(settings.value("breakLines", 0).toBool());
                model->setColorBackground(LogColorBackground(settings.value("colorBackground", 0).toInt()));
                if (auto logModel = dynamic_cast<LogModel*>(model))
                {
                    logModel->setAutoSaveDirectory(settings.value("autoSaveDirectory").toString());
                    logModel->setMaxMessages(settings.value("maxMessages").toInt());
                }
                model->setTimestampPrecision(TimestampPrecision(settings.value("timestampPrecision", 0).toInt()));
                wnd->m_monospaceFont = settings.value("monospaceFont", 0).toBool();
                wnd->itemSelected();
            }
        }
    }
}

void MainWindow::showAboutDialog()
{
    QMessageBox::about(this, "EVE LogLite",
        "EVE LogLite server version " APP_VERSION ", build <a href=\"https://github.com/ccpgames/EveLogLite/commit/" GIT_VERSION "\">" GIT_VERSION "</a><br/>"
        "<br/>"
        "Copyright © 2015, 2016 CCP hf.<br/>"
        "<br/>"
        "Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:<br/>"
        "<br/>"
        "The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.<br/>"
        "<br/>"
        "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.<br/>"
        "<br/>"
        "Source code is available at <a href=\"https://github.com/ccpgames/EveLogLite\">https://github.com/ccpgames/EveLogLite</a>"
    );
}

void MainWindow::updateTaskbarIcon()
{
#ifdef _WIN32
    if (auto model = dynamic_cast<LogModel*>(ui->tableView->sourceModel()))
    {
        int warnings = std::min(model->getRunningCount(SEVERITY_WARN) * 2, 9);
        int errors = std::min(model->getRunningCount(SEVERITY_ERR) * 2, 9);

        if (!m_taskbarButton)
        {
            m_taskbarButton = new QWinTaskbarButton(this);
            m_taskbarButton->setWindow(windowHandle());
        }
        m_taskbarButton->setOverlayIcon(m_icons[errors + 10 * warnings]);
    }
#endif
}

void MainWindow::editFilters()
{
    FilterDialog dlg(this);
    auto currentFilter = static_cast<LogFilter*>(ui->tableView->model())->customFilter();
    if (currentFilter)
    {
        dlg.selectFilter(currentFilter->m_name);
    }
    if (dlg.exec() == QDialog::Accepted)
    {
        Filter::getFilters() = dlg.getFilters();
        Filter::saveFilters();
        if (currentFilter)
        {
            auto name = currentFilter->m_name;
            auto& filters = Filter::getFilters();
            for (auto it = filters.begin(); it != filters.end(); ++it)
            {
                if (name == it->m_name)
                {
                    static_cast<LogFilter*>(ui->tableView->model())->setCustomFilter(&(*it));
                    return;
                }
            }
            customFilterSet(ui->actionFilterNone);
        }
    }
}

void MainWindow::editHighlights()
{
    HighlightDialog dlg(this);
    auto currentHighlight = static_cast<LogFilter*>(ui->tableView->model())->highlight();
    if (currentHighlight)
    {
        dlg.selectHighlight(currentHighlight->m_name);
    }
    if (dlg.exec() == QDialog::Accepted)
    {
        HighlightSet::getSets() = dlg.getHighlightSets();
        HighlightSet::saveSets();
        if (currentHighlight)
        {
            auto name = currentHighlight->m_name;
            auto& highlights = HighlightSet::getSets();
            for (auto it = highlights.begin(); it != highlights.end(); ++it)
            {
                if (name == it->m_name)
                {
                    static_cast<LogFilter*>(ui->tableView->model())->setHighlight(&(*it));
                    return;
                }
            }
            highlightSet(ui->actionHighlightNone);
        }
    }
}

void MainWindow::buildFiltersMenu()
{
    auto actions = ui->menuFilters->actions();
    for (auto it = actions.begin(); it != actions.end(); ++it)
    {
        if (*it == ui->actionFilterNone)
        {
            break;
        }
        ui->menuFilters->removeAction(*it);
        delete *it;
    }
    auto currentFilter = static_cast<LogFilter*>(ui->tableView->model())->customFilter();
    auto& filters = Filter::getFilters();
    for (auto it = filters.begin(); it != filters.end(); ++it)
    {
        auto action = new QAction(it->m_name, this);
        action->setCheckable(true);
        action->setChecked(currentFilter && currentFilter->m_name == it->m_name);
        ui->menuFilters->insertAction(ui->actionFilterNone, action);
    }
}

void MainWindow::customFilterSet(QAction *action)
{
    if (!action->isCheckable())
    {
        return;
    }
    if (action == ui->actionFilterNone)
    {
        QSettings().remove("customFilter");
        action->setChecked(true);
        static_cast<LogFilter*>(ui->tableView->model())->setCustomFilter(nullptr);
        ui->filterButton->setIcon(QIcon(":/default/nofilter"));
        ui->filterButton->setToolTip(action->text());
        return;
    }
    auto name = action->text();
    auto& filters = Filter::getFilters();
    for (auto it = filters.begin(); it != filters.end(); ++it)
    {
        if (name == it->m_name)
        {
            QSettings().setValue("customFilter", it->m_name);
            static_cast<LogFilter*>(ui->tableView->model())->setCustomFilter(&(*it));
            ui->actionFilterNone->setChecked(false);
            ui->filterButton->setIcon(QIcon(":/default/filter"));
            ui->filterButton->setToolTip(action->text());
            break;
        }
    }
}

void MainWindow::buildHighlightsMenu()
{
    auto actions = ui->menuHighlights->actions();
    for (auto it = actions.begin(); it != actions.end(); ++it)
    {
        if (*it == ui->actionHighlightNone)
        {
            break;
        }
        ui->menuHighlights->removeAction(*it);
        delete *it;
    }
    auto currentHighlight = static_cast<LogFilter*>(ui->tableView->model())->highlight();
    auto& highlights = HighlightSet::getSets();
    for (auto it = highlights.begin(); it != highlights.end(); ++it)
    {
        auto action = new QAction(it->m_name, this);
        action->setCheckable(true);
        action->setChecked(currentHighlight && currentHighlight->m_name == it->m_name);
        ui->menuHighlights->insertAction(ui->actionHighlightNone, action);
    }
}

void MainWindow::highlightSet(QAction *action)
{
    if (!action->isCheckable())
    {
        return;
    }
    if (action == ui->actionHighlightNone)
    {
        QSettings().remove("customHighlight");
        action->setChecked(true);
        static_cast<LogFilter*>(ui->tableView->model())->setHighlight(nullptr);
        ui->highlightsButton->setIcon(QIcon(":/default/nohighlights"));
        ui->highlightsButton->setToolTip(action->text());
        return;
    }
    auto name = action->text();
    auto& highlights = HighlightSet::getSets();
    for (auto it = highlights.begin(); it != highlights.end(); ++it)
    {
        if (name == it->m_name)
        {
            QSettings().setValue("customHighlight", it->m_name);
            static_cast<LogFilter*>(ui->tableView->model())->setHighlight(&(*it));
            ui->actionHighlightNone->setChecked(false);
            ui->highlightsButton->setIcon(QIcon(":/default/highlights"));
            ui->highlightsButton->setToolTip(action->text());
            break;
        }
    }
}

void MainWindow::showFilterMenu()
{
    ui->menuFilters->exec(QCursor::pos());
}

void MainWindow::showHighlightsMenu()
{
    ui->menuHighlights->exec(QCursor::pos());
}

void MainWindow::buildClientsMenu()
{
    if (auto model = dynamic_cast<LogModel*>(ui->tableView->sourceModel()))
    {
        auto actions = ui->menu_Disconnect_Client->actions();
        for (auto it = actions.begin(); it != actions.end(); ++it)
        {
            if (*it == ui->actionDisconnectAll)
            {
                break;
            }
            ui->menuHighlights->removeAction(*it);
            delete *it;
        }
        auto& clients = model->clients();
        QFontMetrics metrics(ui->menu_Disconnect_Client->font());
        for (auto it = clients.begin(); it != clients.end(); ++it)
        {
            auto path = metrics.elidedText(it->path(), Qt::ElideMiddle, 200);
            auto name = QString("[%1] %2").arg(it->pid()).arg(path);
            auto action = new QAction(name, this);
            action->setProperty("socket", quint64(it->socket()));
            connect(action, &QAction::triggered, this, &MainWindow::disconnectClient);
            ui->menu_Disconnect_Client->insertAction(ui->actionDisconnectAll, action);
        }
    }
}

void MainWindow::disconnectClient()
{
    if (auto model = dynamic_cast<LogModel*>(ui->tableView->sourceModel()))
    {
        auto action = static_cast<QAction*>(sender());
        auto socket = reinterpret_cast<QTcpSocket*>(action->property("socket").toULongLong());
        model->disconnect(LogModel::Client(socket));
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasUrls())
    {
        bool accept = true;

        QList<QUrl> urlList = mimeData->urls();
        for (int i = 0; i < urlList.size(); ++i)
        {
            if (!urlList.at(i).isLocalFile())
            {
                accept = false;
                break;
            }
            auto path = urlList.at(i).toLocalFile();
            if (!path.toLower().endsWith(".lsw"))
            {
                accept = false;
                break;
            }
        }
        if (accept)
        {
            event->acceptProposedAction();
        }
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasUrls())
    {
        QList<QUrl> urlList = mimeData->urls();
        for (int i = 0; i < urlList.size(); ++i)
        {
            if (urlList.at(i).isLocalFile())
            {
                auto path = urlList.at(i).toLocalFile();
                if (path.toLower().endsWith(".lsw"))
                {
                    openFilePath(path);
                }
            }
        }
    }
}
