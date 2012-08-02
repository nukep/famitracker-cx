#include <stdlib.h>
#include "parse_arguments.hpp"

void ParseArguments::setFlagFields(const char * const flagfields[], int count)
{
	m_flagfields.clear();

	for (int i = 0; i < count; i++)
	{
		m_flagfields.insert(flagfields[i]);
	}
}

void ParseArguments::parse(const char * const argv[], int argc)
{
	m_map.clear();

	std::string currentfield;

	for (int i = 0; i < argc; i++)
	{
		if (currentfield.empty())
		{
			const char *s = argv[i];
			if (s[0] == '-')
			{
				// a flag
				std::string str(s+1);
				if (isFlag(str, m_flagfields))
				{
					m_flags.insert(str);
				}
				else
				{
					currentfield.swap(str);
				}
			}
			else
			{
				m_arguments.push_back(s);
			}
		}
		else
		{
			const char *s = argv[i];
			m_map[currentfield] = s;
			currentfield.clear();
		}
	}
}
int ParseArguments::integer(const std::string &field, int default_value)
{
	const std::string *str = findPair(field);
	if (str == NULL)
	{
		return default_value;
	}
	else
	{
		return atoi(str->c_str());
	}
}
const std::string & ParseArguments::string(const std::string &field, const std::string &default_value)
{
	const std::string *str = findPair(field);
	if (str == NULL)
	{
		return default_value;
	}
	else
	{
		return *str;
	}
}
bool ParseArguments::flag(const std::string &field)
{
	return isFlag(field, m_flags);
}

int ParseArguments::integer(int offset)
{
	const std::string &str = string(offset);

	return atoi(str.c_str());
}
const std::string & ParseArguments::string(int offset)
{
	static std::string empty;
	if (offset >= m_arguments.size())
		return empty;

	return m_arguments.at(offset);
}

const std::string * ParseArguments::findPair(const std::string &field) const
{
	Map::const_iterator it = m_map.find(field);
	if (it == m_map.end())
		return NULL;

	return &it->second;
}

bool ParseArguments::isFlag(const std::string &s, const std::set<std::string> &flagset)
{
	if (s[0] == 0)
		return false;

	std::set<std::string>::const_iterator it = flagset.find(s);
	return !(it == flagset.end());
}
