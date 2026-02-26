#include "include/mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	MainWindow fatmanager;
	fatmanager.show();
	return app.exec();
}
