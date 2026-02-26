#ifndef TOOLBAR_H
#define TOOLBAR_H

#include "include/leftpanel.h"
#include "include/rightpanel.h"

#include <QToolBar>

class ToolBar : public QToolBar
{
	Q_OBJECT

  public:
	ToolBar(QWidget *parent = nullptr, LeftPanel *leftPanel = nullptr, RightPanel *rightPanel = nullptr);

  private:
	void copyFiles(RightPanel *rightPanel, LeftPanel *leftPanel);
};

#endif	  // TOOLBAR_H
