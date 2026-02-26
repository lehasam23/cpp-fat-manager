#ifndef CUSTOMMODEL_H
#define CUSTOMMODEL_H

#include <QDateTime>
#include <QModelIndex>
#include <QSortFilterProxyModel>
#include <QVariant>

class CustomModel : public QSortFilterProxyModel
{
	Q_OBJECT

  public:
	explicit CustomModel(QObject* parent = nullptr);
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  protected:
	bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

  private:
	QDateTime getTime(const QModelIndex& index) const;
};
#endif	  // CUSTOMMODEL_H
