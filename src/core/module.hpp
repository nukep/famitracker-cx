#ifndef CORE_MODULE_HPP
#define CORE_MODULE_HPP

#include <vector>
#include "types.hpp"

namespace core
{
	typedef float SoundFormat;
	class IO;

	enum ModuleType
	{
		MODULE_TRACKER
	};

	class Module
	{
	public:
		virtual ~Module() = 0;
		virtual bool open(IO *io) = 0;
		virtual bool save(IO *io) = 0;

		virtual void soundRequest(ID output, u32 samples, SoundFormat *buf) = 0;
	private:
		ModuleType m_type;
	};

	enum ColumnType
	{
		COL_NOTE,
		COL_INST,
		COL_VOLUME,
		COL_EFFECT
	};

	struct column_entry_t
	{
		ColumnType type;
		int parameter_columns;
	};

	struct column_format_t
	{
		std::vector<ColumnType> column_groups;
	};

	class TrackerModule : public Module
	{
	public:
		virtual Quantity channels() const = 0;
		virtual Quantity channelColumnFormat(ID channel, column_format_t &c) const = 0;
	};
}

#endif
