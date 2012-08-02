#ifndef PARSE_ARGUMENTS_HPP
#define PARSE_ARGUMENTS_HPP

#include <string>
#include <map>
#include <set>
#include <vector>

class ParseArguments
{
public:
	void setFlagFields(const char * const flagfields[], int count);
	void parse(const char * const argv[], int argc);

	int integer(const std::string &field, int default_value);
	const std::string & string(const std::string &field, const std::string &default_value);
	bool flag(const std::string &field);

	int integer(int offset);
	const std::string & string(int offset);
private:
	typedef std::map<std::string, std::string> Map;

	Map m_map;
	std::set<std::string> m_flagfields;
	std::set<std::string> m_flags;
	std::vector<std::string> m_arguments;

	const std::string * findPair(const std::string &field) const;
	static bool isFlag(const std::string &s, const std::set<std::string> &flagset);
};

#endif

