#include "include/mainwindow.h"

#include "include/custommodel.h"
#include "include/leftpanel.h"
#include "include/rightpanel.h"
#include "include/toolbar.h"

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QIcon>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QToolBar>
#include <QTreeView>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	setWindowTitle("FAT Manager");
	resize(1000, 800);
	QSplitter *splitter = new QSplitter(this);
	LeftPanel *leftPanel = new LeftPanel(this);
	RightPanel *rightPanel = new RightPanel(this);
	QStringList args = QApplication::arguments();
	if (args.size() > 2)
	{
		QMessageBox::warning(this, "Error", "Invalid number of arguments specified. Expected one path to a FAT image file.");
	}
	else if (args.size() == 2)
	{
		QString filePath = args[1];
		QFileInfo fileInfo(filePath);
		if (!fileInfo.exists() || !fileInfo.isFile())
		{
			QMessageBox::warning(this, "Error", QString("File '%1' does not exist or is not a file.").arg(filePath));
		}
		else
		{
			rightPanel->openFatImage(filePath);
		}
	}
	splitter->addWidget(leftPanel);
	splitter->addWidget(rightPanel);
	setCentralWidget(splitter);

	ToolBar *toolBar = new ToolBar(this, leftPanel, rightPanel);
	addToolBar(Qt::BottomToolBarArea, toolBar);
}
