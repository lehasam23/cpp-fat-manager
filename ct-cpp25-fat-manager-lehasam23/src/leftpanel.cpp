#include "include/leftpanel.h"

#include <QDir>
#include <QMessageBox>
#include <QMouseEvent>
#include <QStandardPaths>

LeftPanel::LeftPanel(QWidget *parent) :
	QTreeView(parent), leftModel(new QFileSystemModel(this)), customModel(new CustomModel(this))
{
	leftModel->setRootPath("");
	QString path = QDir::currentPath();
	leftModel->setFilter(QDir::AllEntries | QDir::NoDot);
	customModel->setSourceModel(leftModel);

	setModel(customModel);
	setRootIndex(customModel->mapFromSource(leftModel->index(path)));
	setSelectionMode(QAbstractItemView::ExtendedSelection);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSortingEnabled(true);
	setItemsExpandable(false);
	setExpandsOnDoubleClick(false);

	for (int i = 0; i < 3; ++i)
	{
		sortByColumn(i, Qt::AscendingOrder);
	}
}

void LeftPanel::mouseDoubleClickEvent(QMouseEvent *event)
{
	QModelIndex index = indexAt(event->pos());
	if (!index.isValid())
	{
		QTreeView::mouseDoubleClickEvent(event);
		return;
	}

	QModelIndex sourceIndex = customModel->mapToSource(index);
	if (!sourceIndex.isValid())
	{
		QTreeView::mouseDoubleClickEvent(event);
		return;
	}
	QFileInfo info = leftModel->fileInfo(sourceIndex);
	if (info.isDir() || info.fileName() == "..")
	{
		QString newPath;
		if (info.fileName() == "..")
		{
			QModelIndex currentRootSource = customModel->mapToSource(rootIndex());
			QString currentPath = leftModel->filePath(currentRootSource);

			if (currentPath.isEmpty())
			{
				QTreeView::mouseDoubleClickEvent(event);
				return;
			}

#ifdef Q_OS_WIN
			if (currentPath.endsWith(":/") || currentPath.endsWith(":\\"))
			{
				QTreeView::mouseDoubleClickEvent(event);
				return;
			}
#endif

			QDir dir(currentPath);
			if (!dir.cdUp())
			{
				QTreeView::mouseDoubleClickEvent(event);
				return;
			}
			newPath = dir.absolutePath();
		}
		else
		{
			newPath = info.absoluteFilePath();
		}
		QModelIndex newSourceIndex = leftModel->index(newPath);
		if (!newSourceIndex.isValid())
		{
			QTreeView::mouseDoubleClickEvent(event);
			return;
		}
		setRootIndex(customModel->mapFromSource(newSourceIndex));
		return;
	}
	QTreeView::mouseDoubleClickEvent(event);
}

QString LeftPanel::selectedPath() const
{
	QModelIndex index = currentIndex();
	if (index.isValid())
	{
		QModelIndex sourceIndex = customModel->mapToSource(index);
		return leftModel->filePath(sourceIndex);
	}
	return QString();
}

void LeftPanel::calculateSize()
{
	QModelIndexList selectedRows = selectionModel()->selectedRows();
	if (selectedRows.isEmpty())
	{
		QMessageBox::information(this, "Size", "No items selected.");
		return;
	}
	QSet< QString > dirPaths;
	bool hasFile = false;
	for (const QModelIndex &index : selectedRows)
	{
		QModelIndex sourceIndex = customModel->mapToSource(index);
		QFileInfo info = leftModel->fileInfo(sourceIndex);
		if (info.isDir())
		{
			dirPaths.insert(info.absoluteFilePath());
		}
		else
		{
			hasFile = true;
		}
	}
	if (dirPaths.isEmpty())
	{
		QModelIndex current = currentIndex();
		if (current.isValid())
		{
			QModelIndex sourceIndex = customModel->mapToSource(current);
			if (sourceIndex.isValid())
			{
				QFileInfo info = leftModel->fileInfo(sourceIndex);
				if (info.isDir())
				{
					dirPaths.insert(info.absoluteFilePath());
				}
				else
				{
					hasFile = true;
				}
			}
		}
	}
	if (dirPaths.isEmpty())
	{
		if (hasFile)
		{
			QMessageBox::warning(this, "Error", "Size can only be calculated for directories.");
		}
		return;
	}
	QStringList results;
	for (const QString &dirPath : dirPaths)
	{
		quint64 total = calculate(dirPath);
		results << QString("%1: %2 byte").arg(dirPath).arg(QLocale().toString(total));
	}
	QMessageBox::information(this, "Size directory", results.join("\n"));
}

quint64 LeftPanel::calculate(const QString &dirPath)
{
	QDir dir(dirPath);
	quint64 totalSize = 0;
	if (!dir.exists())
	{
		return 0;
	}
	QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
	for (const QFileInfo &fileInfo : files)
	{
		if (fileInfo.isDir())
		{
			totalSize += calculate(fileInfo.absoluteFilePath());
		}
		else
		{
			totalSize += fileInfo.size();
		}
	}
	return totalSize;
}

void LeftPanel::moveHome()
{
#ifdef Q_OS_WIN
	QString rootPath = "";
#else
	QString rootPath = "/";
#endif
	QModelIndex root = leftModel->index(rootPath);
	if (root.isValid())
	{
		setRootIndex(customModel->mapFromSource(root));
	}
}
