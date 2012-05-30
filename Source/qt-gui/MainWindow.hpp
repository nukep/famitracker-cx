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

		void updateFrameChannel();
	private:
		void updateDocument();
	public slots:
		void newDoc();
		void open();
		void save();
		void saveAs();
		void quit();
		void viewToolbar(bool);
		void viewStatusBar(bool);
		void viewControlpanel(bool);
		void setSong(int i);
		void incrementPattern();
		void decrementPattern();
		void instrumentSelect();
		void instrumentNameChange(QString);
	};
}

#endif

