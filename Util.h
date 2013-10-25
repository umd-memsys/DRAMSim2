#ifndef _UTIL_H_
#define _UTIL_H_
/**
 * An implementation of a "null stream" that simply throws away output in case logging is turned off
 *
 * NOTE: this file shamelessly taken from here -- http://stackoverflow.com/a/760353/476076
 */
#include <streambuf>
#include <ostream>
#include <string>
#include <sstream>

using std::string;
using std::stringstream;

namespace DRAMSim {

template <class cT, class traits = std::char_traits<cT> >
class basic_nullbuf: public std::basic_streambuf<cT, traits> {
	typename traits::int_type overflow(typename traits::int_type c)
	{
		return traits::not_eof(c); // indicate success
	}
};

template <class cT, class traits = std::char_traits<cT> >
class basic_onullstream: public std::basic_ostream<cT, traits> {
	public:
		basic_onullstream():
			std::basic_ios<cT, traits>(&m_sbuf),
			std::basic_ostream<cT, traits>(&m_sbuf)
	{
		this->init(&m_sbuf);
	}

	private:
		basic_nullbuf<cT, traits> m_sbuf;
};

typedef basic_onullstream<char> onullstream;


string FilenameWithNumberSuffix(const string &filename, const string &extension, unsigned maxNumber=100);
bool fileExists(string &path);

} // namespace
#endif

