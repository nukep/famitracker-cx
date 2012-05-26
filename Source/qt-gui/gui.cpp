#include <QApplication>
#include "gui.hpp"
#include "MainWindow.hpp"

namespace gui
{
	QApplication *app;
	MainWindow *mw;
	void init(int &argc, char **argv)
	{
		app = new QApplication(argc, argv);
		app->setApplicationName(QApplication::tr("Famitracker"));

		mw = new MainWindow;
		mw->resize(1024, 768);
	}
	void destroy()
	{
		delete mw;
		delete app;
	}
	void spin()
	{
		mw->show();
		app->exec();
	}
}
