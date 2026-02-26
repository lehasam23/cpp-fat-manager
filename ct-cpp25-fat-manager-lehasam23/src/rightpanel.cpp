#include "include/rightpanel.h"

#include <QDebug>
#include <QMessageBox>
#include <QMouseEvent>

RightPanel::RightPanel(QWidget *parent) :
	QTreeView(parent), fatModel(new FatModel(this)), customModel(new CustomModel(this)), parser(nullptr)
{
	customModel->setSourceModel(fatModel);
	setModel(customModel);

	setSelectionMode(QAbstractItemView::ExtendedSelection);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSortingEnabled(true);
	setItemsExpandable(false);
	setExpandsOnDoubleClick(false);
}

void RightPanel::mouseDoubleClickEvent(QMouseEvent *event)
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

	FatModel::FatItem *item = fatModel->itemFromIndex(sourceIndex);
	if (!item)
	{
		QTreeView::mouseDoubleClickEvent(event);
		return;
	}

	if (item->entry.name == "..")
	{
		QModelIndex currentSource = customModel->mapToSource(rootIndex());
		QModelIndex parentSourceIndex = fatModel->parent(currentSource);
		setRootIndex(customModel->mapFromSource(parentSourceIndex));
		return;
	}

	if (item->entry.isDirectory)
	{
		setRootIndex(index);
		return;
	}

	QTreeView::mouseDoubleClickEvent(event);
}

void RightPanel::openFatImage(const QString &path)
{
	if (parser)
	{
		delete parser;
		parser = nullptr;
	}
	parser = new FatParser(path);
	if (!parser->isValid())
	{
		QMessageBox::warning(this, "Error", "The file is not a valid FAT16 or FAT32 system.");
		delete parser;
		parser = nullptr;
		fatModel->setParser(nullptr);
		return;
	}
	fatModel->setParser(parser);
	customModel->setSourceModel(fatModel);
	reset();
}

void RightPanel::calculateSize()
{
	QModelIndexList selectedRows = selectionModel()->selectedRows();
	if (selectedRows.isEmpty())
	{
		QMessageBox::information(this, "", "No items selected.");
		return;
	}
	if (!parser || !parser->isValid() || !fatModel || !customModel)
	{
		QMessageBox::warning(this, "Error", "No valid FAT system loaded.");
		return;
	}
	QSet< FatModel::FatItem * > dirItems;
	bool hasFile = false;
	for (const QModelIndex &index : selectedRows)
	{
		QModelIndex sourceIndex = customModel->mapToSource(index);
		if (!sourceIndex.isValid())
		{
			continue;
		}
		FatModel::FatItem *item = fatModel->itemFromIndex(sourceIndex);
		if (!item || !item->entry.isDirectory)
		{
			continue;
		}
		if (item->entry.isDirectory)
		{
			dirItems.insert(item);
		}
		else
		{
			hasFile = true;
		}
	}
	if (dirItems.isEmpty())
	{
		QModelIndex current = currentIndex();
		if (current.isValid())
		{
			QModelIndex sourceIndex = customModel->mapToSource(current);
			if (sourceIndex.isValid())
			{
				FatModel::FatItem *item = fatModel->itemFromIndex(sourceIndex);
				if (item && item->entry.isDirectory)
				{
					dirItems.insert(item);
				}
				else if (item && !item->entry.isDirectory)
				{
					hasFile = true;
				}
			}
		}
	}
	if (dirItems.isEmpty())
	{
		if (hasFile)
		{
			QMessageBox::warning(this, "Error", "Size can only be calculated for directories.");
		}
		return;
	}
	QStringList results;
	for (FatModel::FatItem *item : dirItems)
	{
		quint64 total = calculateItemSize(item);
		QString name = item->entry.name == "/" ? "root" : item->entry.name;
		results << QString("%1: %2 byte").arg(name).arg(QLocale().toString(total));
	}
	QMessageBox::information(this, "Size directory", results.join("\n"));
}

quint64 RightPanel::calculateItemSize(FatModel::FatItem *item) const
{
	if (!item || !parser || !parser->isValid())
	{
		return 0;
	}

	const FatDirEntry &entry = item->entry;
	if (!entry.isDirectory)
	{
		return entry.size;
	}
	quint64 res = 0;
	if (!item->childrenLoaded)
	{
		fatModel->loadChildren(item);
	}

	for (FatModel::FatItem *child : item->children)
	{
		if (child->entry.name == "..")
		{
			continue;
		}
		res += calculateItemSize(child);
	}
	return res;
}

CustomModel *RightPanel::getCustomModel() const
{
	return customModel;
}

FatModel *RightPanel::getFatModel() const
{
	return fatModel;
}

FatParser *RightPanel::getParser() const
{
	return parser;
}
