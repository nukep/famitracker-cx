#ifndef _SETTINGS_HPP_
#define _SETTINGS_HPP_

#include <QSettings>

#define SETTINGS_WINDOWGEOMETRY "window-geometry"
#define SETTINGS_FTMPATH "ftm-path"
#define SETTINGS_WAVPATH "wav-path"

namespace gui
{
	QSettings * settings();
	void init_settings();
	void destroy_settings();

	void addRecentFile(const QString &path);
	QVariant recentFileVariant(int idx);
	QString recentFile(int idx);
	int recentFileCount();
	int recentFileCapacity();
}

#endif

