#include <QApplication>
#include <QFile>
#include <vector>
#include "gui.hpp"
#include "MainWindow.hpp"
#include "../linux/FtmDocument.hpp"

namespace gui
{
	DocInfo::DocInfo(FtmDocument *d)
		: m_doc(d), m_currentChannel(0), m_currentFrame(0), m_currentRow(0)
	{
	}
	void DocInfo::destroy()
	{
		delete m_doc;
	}
	void DocInfo::setCurrentFrame(unsigned int frame)
	{
		if (frame >= doc()->GetFrameCount())
			frame = doc()->GetFrameCount()-1;

		m_currentFrame = frame;
	}
	void DocInfo::setCurrentChannel(unsigned int chan)
	{
		m_currentChannel = chan;
	}
	void DocInfo::setCurrentRow(unsigned int row)
	{
		m_currentRow = row;
	}

	typedef std::vector<DocInfo> DocsList;
	DocsList loaded_documents;
	int active_doc_index;

	QApplication *app;
	MainWindow *mw;
	void init(int &argc, char **argv)
	{
		active_doc_index = -1;

		newDocument(false);

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

	void updateFrameChannel()
	{
		mw->updateFrameChannel();
	}

	unsigned int loadedDocuments()
	{
		return loaded_documents.size();
	}
	FtmDocument * activeDocument()
	{
		if (active_doc_index < 0)
			return NULL;

		return loaded_documents[active_doc_index].doc();
	}
	DocInfo * activeDocInfo()
	{
		if (active_doc_index < 0)
			return NULL;

		return &loaded_documents[active_doc_index];
	}

	void closeActiveDocument()
	{
		if (active_doc_index < 0)
			return;

		loaded_documents[active_doc_index].destroy();
		loaded_documents.erase(loaded_documents.begin()+active_doc_index);

		active_doc_index--;
	}

	void openDocument(FileIO *io, bool close_active)
	{
		if (close_active)
			closeActiveDocument();

		FtmDocument *d = new FtmDocument;
		d->read(io);

		DocInfo a(d);

		loaded_documents.push_back(a);
		active_doc_index = loaded_documents.size()-1;
	}
	void newDocument(bool close_active)
	{
		if (close_active)
			closeActiveDocument();

		FtmDocument *d = new FtmDocument;
		d->createEmpty();

		DocInfo a(d);

		loaded_documents.push_back(a);
		active_doc_index = loaded_documents.size()-1;
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
	bool FileIO::isReadable()
	{
		return f->isReadable();
	}
	bool FileIO::isWritable()
	{
		return f->isWritable();
	}
}
