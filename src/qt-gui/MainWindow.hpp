#ifndef _MAINWINDOW_HPP_
#define _MAINWINDOW_HPP_

#include <QMainWindow>
#include <QEvent>
#include <QMutex>
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

		void sendUpdateEvent();
		void updateEditMode();
		void setPlaying(bool playing);

		void refreshInstruments();
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
		void rowsChange(int);
		void framesChange(int);

		void play();
		void stop();
		void toggleEditMode();

		void addInstrument();
		void removeInstrument();
	private:
		int m_updateCount;	// number of queued up update events
		QMutex m_updateCountMutex;
	};
}

#endif

