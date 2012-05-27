#ifndef _MAINWINDOW_HPP_
#define _MAINWINDOW_HPP_

#include <QMainWindow>
#include "ui_mainwindow.h"

namespace gui
{
	class MainWindow : public QMainWindow, Ui_MainWindow
	{
		Q_OBJECT
	public:
		MainWindow();

	public slots:
		void open();
		void save();
		void saveAs();
		void quit();
		void viewToolbar(bool);
		void viewStatusBar(bool);
		void viewControlpanel(bool);
		void setSong(int i);
	};
}

#endif

