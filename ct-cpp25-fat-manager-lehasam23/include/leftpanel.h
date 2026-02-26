#ifndef LEFTPANEL_H
#define LEFTPANEL_H

#include "include/custommodel.h"

#include <QFileSystemModel>
#include <QMouseEvent>
#include <QTreeView>

class LeftPanel : public QTreeView
{
	Q_OBJECT

  public:
	explicit LeftPanel(QWidget *parent = nullptr);
	QString selectedPath() const;
	void calculateSize();
	void moveHome();
	CustomModel *getCustomModel() const { return customModel; }
	QFileSystemModel *getLeftModel() const { return leftModel; }

  protected:
	void mouseDoubleClickEvent(QMouseEvent *event) override;

  private:
	QFileSystemModel *leftModel;
	CustomModel *customModel;
	quint64 calculate(const QString &dirPath);
};

#endif	  // LEFTPANEL_H
