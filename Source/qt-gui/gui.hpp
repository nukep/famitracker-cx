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

	void updateFrameChannel();

	class DocInfo;

	unsigned int loadedDocuments();
	FtmDocument * activeDocument();
	DocInfo * activeDocInfo();
	void closeActiveDocument();
	class FileIO;
	void openDocument(FileIO *io, bool close_active);
	void newDocument(bool close_active);

	class DocInfo
	{
		friend void gui::closeActiveDocument();
	public:
		DocInfo(FtmDocument *d);
		FtmDocument * doc() const{ return m_doc; }
		void setCurrentFrame(unsigned int frame);
		void setCurrentChannel(unsigned int chan);
		void setCurrentRow(unsigned int row);
		unsigned int currentFrame() const{ return m_currentFrame; }
		unsigned int currentChannel() const{ return m_currentChannel; }
		unsigned int currentRow() const{ return m_currentRow; }
	protected:
		FtmDocument * m_doc;
		unsigned int m_currentFrame, m_currentChannel, m_currentRow;

		void destroy();
	};

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
