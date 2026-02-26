#include "include/toolbar.h"

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QIcon>
#include <QInputDialog>
#include <QLocale>
#include <QMessageBox>
#include <QSet>

ToolBar::ToolBar(QWidget *parent, LeftPanel *leftPanel, RightPanel *rightPanel) : QToolBar("Tools", parent)
{
	setIconSize(QSize(24, 24));
	setMovable(true);
	setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

	QAction *about = new QAction(QIcon(":/icons/about.png"), "About\nF1", this);
	about->setShortcut(QKeySequence(Qt::Key_F1));
	connect(
		about,
		&QAction::triggered,
		this,
		[this]()
		{
			QMessageBox::information(
				this,
				"About",
				"This is a dual panel Fat Manager.\nThe author of the project is Alexey Samoylovskikh.\n"
				"Also the toolbar buttons are available by pressing the FN keys.");
		});

	QAction *copy = new QAction(QIcon(":/icons/copy.png"), "Copy\nF5", this);
	copy->setShortcut(QKeySequence(Qt::Key_F5));
	connect(copy, &QAction::triggered, this, [=]() { ToolBar::copyFiles(rightPanel, leftPanel); });

	QAction *exit = new QAction(QIcon(":/icons/exit.png"), "Exit\nF10", this);
	exit->setShortcut(QKeySequence(Qt::Key_F10));
	connect(
		exit,
		&QAction::triggered,
		this,
		[=]()
		{
			QMessageBox::StandardButton ans =
				QMessageBox::question(this, "Confirmation", "Are you sure you want to close the program?", QMessageBox::Yes | QMessageBox::No);
			if (ans == QMessageBox::Yes)
			{
				QApplication::quit();
			}
		});

	QAction *size = new QAction(QIcon(":/icons/size.png"), "Size\nF3", this);
	size->setShortcut(QKeySequence(Qt::Key_F3));
	connect(
		size,
		&QAction::triggered,
		this,
		[leftPanel, rightPanel]()
		{
			QWidget *focus = QApplication::focusWidget();
			if (leftPanel->isAncestorOf(focus))
			{
				leftPanel->calculateSize();
			}
			else
			{
				rightPanel->calculateSize();
			}
		});

	QAction *mount = new QAction(QIcon(":/icons/mount.png"), "Mount\nF2", this);
	mount->setShortcut(QKeySequence(Qt::Key_F2));
	connect(
		mount,
		&QAction::triggered,
		this,
		[leftPanel, rightPanel]()
		{
			if (!leftPanel->hasFocus())
			{
				return;
			}
			QString selectedPath = leftPanel->selectedPath();
			QFileInfo fileInfo(selectedPath);
			if (!fileInfo.isFile())
			{
				return;
			}
			rightPanel->openFatImage(selectedPath);
		});

	QAction *mountAction = new QAction(QIcon(":/icons/mountDialog.png"), "Mount…\nF4", this);
	mountAction->setShortcut(QKeySequence(Qt::Key_F4));
	connect(
		mountAction,
		&QAction::triggered,
		this,
		[this, rightPanel]()
		{
			bool buttonOk = false;
			QString filePath =
				QInputDialog::getText(this, "Mount FAT Image", "Enter the path to the FAT image file:", QLineEdit::Normal, QString(), &buttonOk);
			if (!buttonOk)
			{
				return;
			}
			if (filePath.isEmpty())
			{
				QMessageBox::warning(this, "Ошибка", "The path cannot be empty.");
				return;
			}
			QFileInfo fileInfo(filePath);
			if (!fileInfo.isFile() || !fileInfo.exists())
			{
				QMessageBox::warning(this, "Error", "The specified path is invalid.");
				return;
			}
			rightPanel->openFatImage(filePath);
		});

	QAction *root = new QAction(QIcon(":/icons/home.png"), "Root\nF9", this);
	root->setShortcut(QKeySequence(Qt::Key_F9));
	connect(root, &QAction::triggered, this, [leftPanel]() { leftPanel->moveHome(); });

	addAction(root);
	addAction(about);
	addAction(copy);
	addAction(size);
	addAction(mount);
	addAction(mountAction);
	addAction(exit);
}

void ToolBar::copyFiles(RightPanel *rightPanel, LeftPanel *leftPanel)
{
	QWidget *focus = QApplication::focusWidget();
	if (!rightPanel->isAncestorOf(focus))
	{
		QMessageBox::information(this, "Copy", "Please select files in the right panel.");
		return;
	}

	QString dirPath = leftPanel->selectedPath();
	QFileInfo info(dirPath);
	if (!info.isDir())
	{
		QModelIndex current = leftPanel->currentIndex();
		if (current.isValid())
		{
			QModelIndex sourceIndex = leftPanel->getCustomModel()->mapToSource(current);
			dirPath = leftPanel->getLeftModel()->filePath(sourceIndex);
			info = QFileInfo(dirPath);
			if (!info.isDir())
			{
				dirPath = info.absolutePath();
				info = QFileInfo(dirPath);
			}
		}
		else
		{
			dirPath = QDir::homePath();
			info = QFileInfo(dirPath);
		}
	}

	if (!info.isWritable())
	{
		QMessageBox::warning(this, "Copy Error", QString("Destination directory %1 is not writable.").arg(dirPath));
		return;
	}

	QModelIndexList selectedRows = rightPanel->selectionModel()->selectedRows();
	if (selectedRows.isEmpty())
	{
		QModelIndex current = rightPanel->currentIndex();
		if (current.isValid())
		{
			selectedRows.append(current);
		}
	}

	if (selectedRows.isEmpty())
	{
		QMessageBox::information(this, "Copy", "No files selected for copying.");
		return;
	}

	if (!rightPanel->getParser() || !rightPanel->getParser()->isValid())
	{
		QMessageBox::warning(this, "Copy Error", "No valid FAT image is mounted.");
		return;
	}

	for (const QModelIndex &index : selectedRows)
	{
		if (!index.isValid())
			continue;
		QModelIndex sourceIndex = rightPanel->getCustomModel()->mapToSource(index);
		if (!sourceIndex.isValid())
			continue;
		const FatModel::FatItem *item = rightPanel->getFatModel()->itemFromIndex(sourceIndex);
		if (!item || item->entry.isDirectory || item->entry.isDeleted)
		{
			continue;
		}
		QString fileName = item->entry.name;
		uint32_t fileSize = item->entry.size;
		uint32_t startCluster = item->entry.startCluster;
		QString filePath = QDir(dirPath).filePath(fileName);
		QFileInfo fileInfo(filePath);

		if (fileInfo.exists())
		{
			QMessageBox::StandardButton ans = QMessageBox::warning(
				this,
				"Copy Error",
				QString("File %1 already exists in %2. Abort copying?").arg(fileName).arg(dirPath),
				QMessageBox::Yes | QMessageBox::No);
			if (ans == QMessageBox::Yes)
			{
				return;
			}
			continue;
		}

		if (!info.isWritable())
		{
			QMessageBox::StandardButton ans = QMessageBox::warning(
				this,
				"Copy Error",
				QString("Cannot write to %1. Abort copying?").arg(filePath),
				QMessageBox::Yes | QMessageBox::No);
			if (ans == QMessageBox::Yes)
			{
				return;
			}
			continue;
		}

		QByteArray fileData = rightPanel->getParser()->readFile(startCluster, fileSize);
		if (fileData.isEmpty())
		{
			QMessageBox::StandardButton ans = QMessageBox::warning(
				this,
				"Copy Error",
				QString("Failed to read file %1 from FAT image. Abort copying?").arg(fileName),
				QMessageBox::Yes | QMessageBox::No);
			if (ans == QMessageBox::Yes)
			{
				return;
			}
			continue;
		}
		QFile mainFile(filePath);
		if (!mainFile.open(QIODevice::WriteOnly))
		{
			QMessageBox::StandardButton ans = QMessageBox::warning(
				this,
				"Copy Error",
				QString("Cannot open %1 for writing: %2. Abort copying?").arg(filePath).arg(mainFile.errorString()),
				QMessageBox::Yes | QMessageBox::No);
			if (ans == QMessageBox::Yes)
			{
				return;
			}
			continue;
		}
		if (mainFile.write(fileData) != fileData.size())
		{
			mainFile.close();
			mainFile.remove();
			QMessageBox::StandardButton ans = QMessageBox::warning(
				this,
				"Copy Error",
				QString("Failed to write file %1: %2. Abort copying?").arg(fileName).arg(mainFile.errorString()),
				QMessageBox::Yes | QMessageBox::No);
			if (ans == QMessageBox::Yes)
			{
				return;
			}
			continue;
		}
		mainFile.close();
	}
	QMessageBox::information(this, "Copy", "Copy is successfully.");
}
