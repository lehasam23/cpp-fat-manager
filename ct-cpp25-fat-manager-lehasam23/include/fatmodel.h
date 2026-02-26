#ifndef FATMODEL_H
#define FATMODEL_H

#include "include/fatparser.h"

#include <QAbstractItemModel>

class FatModel : public QAbstractItemModel
{
	Q_OBJECT

  public:
	explicit FatModel(QObject *parent = nullptr);
	~FatModel();
	struct FatItem
	{
		FatDirEntry entry;
		QList< FatItem * > children;
		FatItem *parent;
		bool childrenLoaded;
		FatItem(const FatDirEntry &e, FatItem *p = nullptr) : entry(e), parent(p), childrenLoaded(false) {}
		~FatItem() { qDeleteAll(children); }
	};

	void loadChildren(FatItem *item) const;
	FatItem *itemFromIndex(const QModelIndex &index) const;
	void setParser(FatParser *newParser);
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool isDirectory(const QModelIndex &index) const;

  private:
	FatParser *parser;
	FatItem *rootItem;
};

#endif	  // FATMODEL_H
