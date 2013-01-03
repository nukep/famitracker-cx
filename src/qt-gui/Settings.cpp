#include <QApplication>
#include "Settings.hpp"
#include "config.h"

namespace gui
{
	QSettings *sett;
	QSettings * settings()
	{
		return sett;
	}

	void init_settings()
	{
#ifdef INSTALL_PORTABLE
		// for the portable build, the settings file should be in the application bundle
		const QString file = QApplication::applicationDirPath() + "/settings.conf";
		sett = new QSettings(file, QSettings::IniFormat);
#else
		// otherwise, make it a user setting
		QString on = qApp->applicationName();
		sett = new QSettings(QSettings::NativeFormat, QSettings::UserScope, on);
#endif
	}
	void destroy_settings()
	{
		delete sett;
	}
	static void setRecentFile(const QString &path, int idx)
	{
		settings()->setValue(QString("recent/%1").arg(idx+1), path);
	}

	void addRecentFile(const QString &path)
	{
		int capacity = recentFileCapacity();
		bool matchFound = false;
		// check if file is already in list
		int i;
		for (i = 0; i < capacity; i++)
		{
			QVariant v = recentFileVariant(i);
			if (!v.isValid())
				break;
			if (v.toString().compare(path) == 0)
			{
				matchFound = true;
				break;
			}
		}
		if (i == 0 && matchFound)
		{
			// already the first element. peace!
			return;
		}
		int count;
		if (matchFound)
		{
			count = i;
		}
		else
		{
			count = std::min(recentFileCount(), recentFileCapacity()-1);
		}

		// shift all previous files down
		for (int i = count-1; i >= 0; i--)
		{
			setRecentFile(recentFile(i), i+1);
		}
		// set the path on top of the list
		setRecentFile(path, 0);
	}

	QVariant recentFileVariant(int idx)
	{
		return settings()->value(QString("recent/%1").arg(idx+1));
	}
	QString recentFile(int idx)
	{
		return recentFileVariant(idx).toString();
	}
	bool recentFileEntry(int idx)
	{
		return recentFileVariant(idx).isValid();
	}

	int recentFileCount()
	{
		for (int i = recentFileCapacity()-1; i >= 0; i--)
		{
			if (recentFileEntry(i))
				return i+1;
		}
		return 0;
	}
	int recentFileCapacity()
	{
		return settings()->value("recentfile-capacity", 8).toInt();
	}
}
