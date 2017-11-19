#include "mainwindow.h"

void MainWindow::bindSignals()
{
    connect(&AppContext::get(), &AppContext::fileOpened, this, &MainWindow::onFileOpened);
    connect(&AppContext::get(), &AppContext::fileSaved, this, &MainWindow::onFileSaved);

    connect(&mOutlinePanel, &OutlinePanel::selectionChanged, [&](QList<nap::rtti::RTTIObject*>& objects) {
        mInspectorPanel.setObject(objects.isEmpty() ? nullptr : objects.first());
    });
}

void MainWindow::showEvent(QShowEvent* event)
{
    BaseWindow::showEvent(event);
    AppContext::get().restoreUI();
}


void MainWindow::addDocks()
{
    addDock("Outline", &mOutlinePanel);
    addDock("Available Types", &mHierarchyPanel);
    addDock("Inspector", &mInspectorPanel);
    addDock("History", &mHistoryPanel);
    addDock("Log", &mLogPanel);
    addDock("AppRunner", &mAppRunnerPanel);
}

void MainWindow::addMenu()
{
    auto filemenu = new QMenu("File", menuBar());
    {
        auto newFileAction = new NewFileAction();
        addAction(newFileAction);
        filemenu->addAction(newFileAction);

        auto openFileAction = new OpenFileAction();
        addAction(openFileAction);
        filemenu->addAction(openFileAction);

        auto saveFileAction = new SaveFileAction();
        addAction(saveFileAction);
        filemenu->addAction(saveFileAction);

        auto saveFileAsAction = new SaveFileAsAction();
        addAction(saveFileAction);
        filemenu->addAction(saveFileAsAction);
    }
    menuBar()->insertMenu(windowMenu()->menuAction(), filemenu);

    auto optionsMenu = new QMenu("Options", menuBar());
    {
        optionsMenu->addMenu(&mThemeMenu);
    }
    menuBar()->insertMenu(windowMenu()->menuAction(), optionsMenu);
}

void MainWindow::onNewFile()
{
    updateWindowTitle();
}


void MainWindow::onFileOpened(const QString& filename)
{
    updateWindowTitle();
}

void MainWindow::onFileSaved(const QString& filename)
{
    updateWindowTitle();
}



void MainWindow::updateWindowTitle()
{
    setWindowTitle(QString("%1 - %2").arg(QApplication::applicationName(), AppContext::get().currentFilename()));
}



