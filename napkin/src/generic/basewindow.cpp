#include "basewindow.h"
#include "napkinglobals.h"
#include <QSettings>



napkin::BaseWindow::BaseWindow()
{
	setWindowTitle(QApplication::applicationName());
	setDockNestingEnabled(true);
	mWindowMenu = new QMenu("Window");
	menuBar()->addMenu(mWindowMenu);
}

QDockWidget* napkin::BaseWindow::addDock(const QString& name, QWidget* widget, Qt::DockWidgetArea area)
{
	QDockWidget* dock = new QDockWidget(this);
	dock->setObjectName(name);
	dock->setWidget(widget);
	dock->setWindowTitle(name);

	if (widget->objectName().isEmpty())
		widget->setObjectName(QString("%1_Widget").arg(name));

	QAction* action = mWindowMenu->addAction(name);
	action->setCheckable(true);
	action->setChecked(true);
	connect(action, &QAction::triggered, [dock, action]() { dock->setVisible(action->isChecked()); });
	addDockWidget(area, dock);
	return dock;
}

void napkin::BaseWindow::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	QSettings s;
	restoreGeometry(s.value(settingsKey::WIN_GEO).toByteArray());
	restoreState(s.value(settingsKey::WIN_STATE).toByteArray());
}

void napkin::BaseWindow::closeEvent(QCloseEvent* event)
{
	QSettings s;
	s.setValue(settingsKey::WIN_STATE, saveState());
	s.setValue(settingsKey::WIN_GEO, saveGeometry());
	QWidget::closeEvent(event);
}