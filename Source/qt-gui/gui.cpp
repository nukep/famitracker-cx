#include <QApplication>
#include <QFile>
#include <list>
#include "gui.hpp"
#include "MainWindow.hpp"
#include "../linux/FtmDocument.hpp"

namespace gui
{
	typedef std::list<FtmDocument*> DocsList;
	DocsList loaded_documents;
	FtmDocument * active_document;

	QApplication *app;
	MainWindow *mw;
	void init(int &argc, char **argv)
	{
		active_document = NULL;

		app = new QApplication(argc, argv);
		app->setApplicationName(QApplication::tr("Famitracker"));

		mw = new MainWindow;
		mw->resize(1024, 768);
	}
	void destroy()
	{
		delete mw;
		delete app;
	}
	void spin()
	{
		mw->show();
		app->exec();
	}

	unsigned int loadedDocuments()
	{
		return loaded_documents.size();
	}
	FtmDocument * activeDocument()
	{
		return active_document;
	}
	void closeActiveDocument()
	{
		if (active_document == NULL)
			return;

		for (DocsList::iterator it=loaded_documents.begin();it!=loaded_documents.end();++it)
		{
			if (*it == active_document)
			{
				loaded_documents.erase(it);
				break;
			}
		}

		if (loaded_documents.empty())
		{
			active_document = NULL;
		}
		else
		{
			active_document = loaded_documents.back();
		}
	}

	void openDocument(FileIO *io, bool close_active)
	{
		if (close_active)
			closeActiveDocument();

		FtmDocument *d = new FtmDocument;
		d->read(io);

		loaded_documents.push_back(d);
		active_document = d;
	}

	FileIO::FileIO(const QString &name, bool reading)
	{
		f = new QFile(name);
		if (!f->open(reading ? QIODevice::ReadOnly : QIODevice::WriteOnly))
		{
			abort();
		}
	}
	FileIO::~FileIO()
	{
		delete f;
	}

	Quantity FileIO::read(void *buf, Quantity sz)
	{
		return f->read((char*)buf, sz);
	}
	Quantity FileIO::write(const void *buf, Quantity sz)
	{
		return f->write((const char*)buf, sz);
	}
	bool FileIO::seek(int offset, SeekOrigin o)
	{
		switch(o)
		{
		case IO_SEEK_SET:
			return f->seek(offset);
			break;
		case IO_SEEK_CUR:
			return f->seek(f->pos() + offset);
			break;
		case IO_SEEK_END:
			return f->seek(f->size());
			break;
		}
	}
}
