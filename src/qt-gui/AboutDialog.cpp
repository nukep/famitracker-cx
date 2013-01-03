#include "AboutDialog.hpp"
#include "../version.hpp"
#include <QFile>
#include <QTextStream>
#include <QPixmap>

namespace gui
{
	AboutDialog::AboutDialog(QWidget *parent)
		: QDialog(parent)
	{
		setupUi(this);

		const int iconSize = 64;

		programIcon->resize(iconSize, iconSize);
		programIcon->setPixmap(qApp->windowIcon().pixmap(iconSize));

		QFile fInput(":/aboutBlurb");
		fInput.open(QIODevice::ReadOnly);

		QTextStream sin(&fInput);
		QString s = sin.readAll().arg(VERSION_STRING VERSION_STRING_MISC).arg("2013");
		fInput.close();

		blurb->setText(s);

		adjustSize();
	}
	AboutDialog::~AboutDialog()
	{

	}
}

