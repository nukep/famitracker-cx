#ifndef _ABOUTDIALOG_HPP_
#define _ABOUTDIALOG_HPP_

#include <QDialog>
#include "ui_about.h"

namespace gui
{
	class AboutDialog : public QDialog, public Ui_About
	{
		Q_OBJECT
	public:
		AboutDialog(QWidget *parent);
		~AboutDialog();
	};
}

#endif

