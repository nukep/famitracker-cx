#ifndef _MODULEPROPERTIES_HPP_
#define _MODULEPROPERTIES_HPP_

#include <QDialog>
#include "ui_moduleproperties.h"

class FtmDocument;

namespace gui
{
	class App;
	class MainWindow;
	class ModulePropertiesDialog : public QDialog, public Ui_ModuleProperties
	{
		Q_OBJECT
	public:
		ModulePropertiesDialog(MainWindow *parent, App *a);
		~ModulePropertiesDialog();
	public slots:
		void accept();
		void changeTrackTitle();
		void selectTrack(int);

		void addTrack();
		void removeTrack();
		void moveUpTrack();
		void moveDownTrack();
		void importTrack();
	private:
		App *m_app;
		MainWindow *m_mw;
		FtmDocument *m_doc;

		void moveUpOrDown(bool up);
		void updateTracksList(int current);
	};
}

#endif

