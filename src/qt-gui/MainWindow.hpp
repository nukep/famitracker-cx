#ifndef _MAINWINDOW_HPP_
#define _MAINWINDOW_HPP_

#include <QMainWindow>
#include <QEvent>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include "ui_mainwindow.h"

namespace gui
{
#define UPDATEEVENT QEvent::User
#define STOPPEDSONGEVENT (QEvent::Type)((int)QEvent::User+1)

	class MainWindow;
	class InstrumentEditor;

	typedef void(*stopsong_callback)(MainWindow*, void*);

	class UpdateEvent : public QEvent
	{
	public:
		UpdateEvent() : QEvent(UPDATEEVENT){}
	};
	class StoppedSongEvent : public QEvent
	{
	public:
		StoppedSongEvent() : QEvent(STOPPEDSONGEVENT){}
		stopsong_callback callback;
		void *callback_data;
	};

	class MainWindow : public QMainWindow, Ui_MainWindow
	{
		Q_OBJECT
	public:
		MainWindow();
		~MainWindow();

		void updateFrameChannel(bool modified=false);

		void sendUpdateEvent();
		void sendStoppedSongEvent(stopsong_callback, void *data);
		void updateEditMode();
		void setPlaying(bool playing);

		void refreshInstruments();

		void stopSongConcurrent(void(*mainthread_callback)(void *data));

		void updateDocument();
	protected:
		void closeEvent(QCloseEvent *);
		bool event(QEvent *event);
	private:

		void setSong_mw_cb();
		static void setSong_cb(MainWindow*, void*);

		static void newDoc_cb(MainWindow*, void*);
		static void open_cb(MainWindow*, void*);
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
		void instrumentDoubleClicked(QModelIndex);
		void speedTempoChange(int);
		void rowsChange(int);
		void framesChange(int);

		void play();
		void stop();
		void toggleEditMode();

		void addInstrument();
		void removeInstrument();
		void editInstrument();
	private:
		boost::mutex m_mtx_updateEvent;
		boost::condition m_cond_updateEvent;
		InstrumentEditor *m_instrumenteditor;
	};
}

#endif

