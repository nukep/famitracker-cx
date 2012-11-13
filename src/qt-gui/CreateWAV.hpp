#ifndef _CREATEWAV_HPP_
#define _CREATEWAV_HPP_

#include <QDialog>
#include "ui_createwav.h"

namespace gui
{
	class CreateWAVDialog : public QDialog, public Ui_CreateWAV
	{
		Q_OBJECT
	public:
		CreateWAVDialog(QWidget *parent);
		~CreateWAVDialog();
	};
}

#endif
