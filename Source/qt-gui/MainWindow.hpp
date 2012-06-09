#ifndef _MAINWINDOW_HPP_
#define _MAINWINDOW_HPP_

#include <QMainWindow>
#include <QEvent>
#include "ui_mainwindow.h"

namespace gui
{
#define UPDATEEVENT QEvent::User

	class UpdateEvent : public QEvent
	{
	public:
		UpdateEvent();
	};

	class MainWindow : public QMainWindow, Ui_MainWindow
	{
		Q_OBJECT
	public:
		MainWindow();

		void updateFrameChannel(bool modified=false);

	protected:
		bool event(QEvent *event);
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
		void speedTempoChange(int);

		void play();
		void stop();
	};
}

#endif

