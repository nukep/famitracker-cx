#ifndef _GUI_HPP_
#define _GUI_HPP_

#include "../linux/Document.hpp"

class QFile;
class QString;
class FtmDocument;

namespace gui
{
	void init(int &argc, char **argv);
	void destroy();
	void spin();

	unsigned int loadedDocuments();
	FtmDocument * activeDocument();
	void closeActiveDocument();
	class FileIO;
	void openDocument(FileIO *io, bool close_active);

	class FileIO : public IO
	{
	public:
		FileIO(const QString &name, bool reading);
		~FileIO();
		Quantity read(void *buf, Quantity sz);
		Quantity write(const void *buf, Quantity sz);
		bool seek(int offset, SeekOrigin o);
	private:
		QFile *f;
	};
}

#endif
