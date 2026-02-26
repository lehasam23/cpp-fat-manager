#include "include/custommodel.h"

#include "include/fatmodel.h"

#include <QFileInfo>
#include <QFileSystemModel>
#include <QLocale>

CustomModel::CustomModel(QObject *parent) : QSortFilterProxyModel(parent) {}

QVariant CustomModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
	{
		return QSortFilterProxyModel::data(index, role);
	}

	QModelIndex sourceIndex = mapToSource(index);
	if (QFileSystemModel *fsModel = qobject_cast< QFileSystemModel * >(sourceModel()))
	{
		QFileInfo info = fsModel->fileInfo(sourceIndex);
		switch (index.column())
		{
		case 0:
			return (role == Qt::DisplayRole) ? info.fileName() : QVariant();
		case 1:
			return (role == Qt::DisplayRole) ? (info.isDir() ? "Dir" : QLocale().toString(info.size())) : QVariant();
		case 2:
			if (role == Qt::DisplayRole)
				return info.birthTime().isValid() ? info.birthTime().toString("yyyy-MM-dd HH:mm:ss") : QString();
			return QVariant();
		}
	}
	else if (FatModel *fatModel = qobject_cast< FatModel * >(sourceModel()))
	{
		return fatModel->data(sourceIndex, role);
	}
	return QSortFilterProxyModel::data(index, role);
}

int CustomModel::columnCount(const QModelIndex &) const
{
	return 3;
}

QVariant CustomModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		switch (section)
		{
		case 0:
			return "Name";
		case 1:
			return "Size";
		case 2:
			return "Time";
		default:
			return QVariant();
		}
	}
	return QSortFilterProxyModel::headerData(section, orientation, role);
}

QDateTime CustomModel::getTime(const QModelIndex &index) const
{
	QModelIndex sourceIndex = (index.model() == this) ? mapToSource(index) : index;

	if (QFileSystemModel *fsModel = qobject_cast< QFileSystemModel * >(sourceModel()))
	{
		QFileInfo info = fsModel->fileInfo(sourceIndex);
		return info.birthTime();
	}
	else if (FatModel *fatModel = qobject_cast< FatModel * >(sourceModel()))
	{
		QVariant timeVar = fatModel->data(sourceIndex, Qt::UserRole);
		return timeVar.toDateTime();
	}
	return QDateTime();
}

bool CustomModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	QFileSystemModel *fsModel = qobject_cast< QFileSystemModel * >(sourceModel());
	QString leftName, rightName;

	if (fsModel)
	{
		QFileInfo leftInfo = fsModel->fileInfo(left);
		QFileInfo rightInfo = fsModel->fileInfo(right);
		leftName = leftInfo.fileName();
		rightName = rightInfo.fileName();
		if (leftName == "..")
		{
			return true;
		}
		if (rightName == "..")
		{
			return false;
		}
	}
	if (left.column() == 2 && right.column() == 2)
	{
		QDateTime leftTime = getTime(left);
		QDateTime rightTime = getTime(right);

		if (leftTime.isValid() && rightTime.isValid())
		{
			return leftTime < rightTime;
		}
	}
	QVariant leftData = sourceModel()->data(left, Qt::DisplayRole);
	QVariant rightData = sourceModel()->data(right, Qt::DisplayRole);
	return QString::localeAwareCompare(leftData.toString(), rightData.toString()) < 0;
}
