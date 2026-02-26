#include "include/fatmodel.h"

#include <QMessageBox>

FatModel::FatModel(QObject *parent) : QAbstractItemModel(parent), parser(nullptr), rootItem(nullptr) {}

FatModel::~FatModel()
{
	delete rootItem;
}

void FatModel::setParser(FatParser *newParser)
{
	beginResetModel();
	delete rootItem;
	rootItem = nullptr;
	parser = newParser;

	if (parser && parser->isValid())
	{
		FatDirEntry rootEntry;
		rootEntry.name = "/";
		rootEntry.isDirectory = true;
		rootEntry.isDeleted = false;
		rootEntry.startCluster = parser->getRootCluster();
		rootItem = new FatItem(rootEntry);
	}
	endResetModel();
}

QModelIndex FatModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!rootItem || row < 0 || column < 0 || column >= columnCount())
	{
		return QModelIndex();
	}
	FatItem *parentItem = itemFromIndex(parent);
	if (!parentItem)
	{
		return QModelIndex();
	}
	if (parentItem->entry.isDirectory && !parentItem->childrenLoaded)
	{
		loadChildren(parentItem);
	}
	return row < parentItem->children.size() ? createIndex(row, column, parentItem->children[row]) : QModelIndex();
}

QModelIndex FatModel::parent(const QModelIndex &index) const
{
	if (!index.isValid() || !rootItem)
		return QModelIndex();

	FatItem *childItem = static_cast< FatItem * >(index.internalPointer());
	FatItem *parentItem = childItem->parent;
	if (parentItem == rootItem)
		return QModelIndex();
	else
	{
		FatItem *grandParentItem = parentItem->parent;
		if (grandParentItem)
		{
			int row = grandParentItem->children.indexOf(parentItem);
			return createIndex(row, 0, parentItem);
		}
		else
		{
			return QModelIndex();
		}
	}
}

int FatModel::rowCount(const QModelIndex &parent) const
{
	if (!rootItem)
	{
		return 0;
	}
	FatItem *parentItem = itemFromIndex(parent);
	if (!parentItem)
	{
		return 0;
	}
	if (parentItem->entry.isDirectory && !parentItem->childrenLoaded)
	{
		loadChildren(parentItem);
	}
	return parentItem->children.size();
}

int FatModel::columnCount(const QModelIndex &) const
{
	return 3;
}

QVariant FatModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || !rootItem)
	{
		return QVariant();
	}
	const FatDirEntry &entry = static_cast< FatItem * >(index.internalPointer())->entry;

	if (role == Qt::DisplayRole)
	{
		switch (index.column())
		{
		case 0:
			return entry.isDeleted ? "Deleted: " + entry.name : entry.name;
		case 1:
			return entry.isDirectory ? "Dir" : QString::number(entry.size);
		case 2:
			return entry.modificationTime.isValid() ? entry.modificationTime.toString("yyyy-MM-dd HH:mm:ss") : QString();
		}
	}
	else if (role == Qt::UserRole && index.column() == 2)
	{
		return entry.modificationTime;
	}
	return QVariant();
}

void FatModel::loadChildren(FatItem *item) const
{
	if (!item->entry.isDirectory || item->childrenLoaded || !parser || !parser->isValid())
	{
		return;
	}
	if (item != rootItem && item->parent != nullptr)
	{
		FatDirEntry parentEntry;
		parentEntry.name = "..";
		parentEntry.isDirectory = true;
		parentEntry.isDeleted = false;
		parentEntry.startCluster = item->parent->entry.startCluster;
		item->children.append(new FatItem(parentEntry, item));
	}

	QList< FatDirEntry > entries = parser->readDirectory(item->entry.startCluster);
	if (entries.isEmpty() && !parser->isValid())
	{
		QMessageBox::warning(nullptr, "Error", "Cannot read directory contents.");
		return;
	}
	for (const FatDirEntry &entry : entries)
	{
		if (entry.name == "..")
		{
			continue;
		}
		item->children.append(new FatItem(entry, item));
	}
	item->childrenLoaded = true;
}

bool FatModel::isDirectory(const QModelIndex &index) const
{
	return index.isValid() && static_cast< FatItem * >(index.internalPointer())->entry.isDirectory;
}

FatModel::FatItem *FatModel::itemFromIndex(const QModelIndex &index) const
{
	return index.isValid() ? static_cast< FatItem * >(index.internalPointer()) : rootItem;
}
