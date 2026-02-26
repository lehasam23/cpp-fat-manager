#ifndef RIGHTPANEL_H
#define RIGHTPANEL_H

#include "fatmodel.h"
#include "include/custommodel.h"
#include "include/fatparser.h"

#include <QMouseEvent>
#include <QTreeView>

class RightPanel : public QTreeView
{
	Q_OBJECT

  public:
	explicit RightPanel(QWidget *parent = nullptr);
	void openFatImage(const QString &path);
	void calculateSize();
	CustomModel *getCustomModel() const;
	FatModel *getFatModel() const;
	FatParser *getParser() const;

  protected:
	void mouseDoubleClickEvent(QMouseEvent *event);

  private:
	FatModel *fatModel;
	CustomModel *customModel;
	FatParser *parser;
	quint64 calculateItemSize(FatModel::FatItem *item) const;
};

#endif	  // RIGHTPANEL_H
