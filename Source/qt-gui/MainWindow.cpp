#include <QFileDialog>
#include "MainWindow.hpp"
#include "gui.hpp"
#include "../linux/FtmDocument.hpp"

namespace gui
{
	MainWindow::MainWindow()
	{
		setupUi(this);
		QObject::connect(action_Open, SIGNAL(triggered()), this, SLOT(open()));
		QObject::connect(action_Save, SIGNAL(triggered()), this, SLOT(save()));
		QObject::connect(actionSave_As, SIGNAL(triggered()), this, SLOT(saveAs()));
		QObject::connect(actionE_xit, SIGNAL(triggered()), this, SLOT(quit()));

		QObject::connect(action_ViewToolbar, SIGNAL(toggled(bool)), this, SLOT(viewToolbar(bool)));
		QObject::connect(action_ViewStatusBar, SIGNAL(toggled(bool)), this, SLOT(viewStatusBar(bool)));
		QObject::connect(action_ViewControlpanel, SIGNAL(toggled(bool)), this, SLOT(viewControlpanel(bool)));
		QObject::connect(toolBar, SIGNAL(visibilityChanged(bool)), action_ViewToolbar, SLOT(setChecked(bool)));
		QObject::connect(controlPanel, SIGNAL(visibilityChanged(bool)), action_ViewControlpanel, SLOT(setChecked(bool)));

		QObject::connect(songs, SIGNAL(activated(int)), this, SLOT(setSong(int)));
	}

	void MainWindow::open()
	{
		QString path = QFileDialog::getOpenFileName(this, tr("Open"), QString(), tr("FamiTracker files (*.ftm);;All files (*.*)"), 0, 0);
		if (path.isEmpty())
			return;

		FileIO io(path, true);

		gui::openDocument(&io, true);

		FtmDocument *d = gui::activeDocument();

		title->setText(d->GetSongName());
		title->setCursorPosition(0);
		author->setText(d->GetSongArtist());
		author->setCursorPosition(0);
		copyright->setText(d->GetSongCopyright());
		copyright->setCursorPosition(0);

		songs->clear();
		int c = d->GetTrackCount();
		for (int i=0;i<c;i++)
		{
			QString s(d->GetTrackTitle(i));
			songs->addItem(s);
		}

		setSong(0);
	}
	void MainWindow::save()
	{
		saveAs();
	}
	void MainWindow::saveAs()
	{
		QFileDialog::getSaveFileName(this, tr("Save As"), QString(), tr("FamiTracker files (*.ftm);;All files (*.*)"), 0, 0);
	}

	void MainWindow::quit()
	{
		close();
	}

	void MainWindow::viewToolbar(bool v)
	{
		this->toolBar->setVisible(v);
	}
	void MainWindow::viewStatusBar(bool v)
	{
		this->statusbar->setVisible(v);
	}
	void MainWindow::viewControlpanel(bool v)
	{
		this->controlPanel->setVisible(v);
	}

	void MainWindow::setSong(int i)
	{
		FtmDocument *d = gui::activeDocument();

		d->SelectTrack(i);

		speed->setValue(d->GetSongSpeed());
		tempo->setValue(d->GetSongTempo());
		rows->setValue(d->GetPatternLength());
		frames->setValue(d->GetFrameCount());
	}
}
