/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include <string>
#include <algorithm>
using namespace std;

#define SPACES " \r\n"
#define CRLF "\r\n"

inline string trim_right(const string &s, const string & t = SPACES)
{
	string d (s);
	string::size_type i(d.find_last_not_of (t));
	if (i == string::npos)
	return "";
	else
	return d.erase (d.find_last_not_of (t) + 1) ;
}

inline string remove_chars(const string &s, const string & t = CRLF, const string & w = " ")
{
	string d (s);
	string::size_type i;
	while ((i = d.find_first_of(t)) != string::npos)
			d.replace(i, 1, w) ;

	return d;
}




inline string ToUpper(const string &s)
{
	string d(s);

	transform (d.begin (), d.end (), d.begin (), (int(*)(int)) toupper);
	return d;
}


inline string ToLower(const string &s)
{
	string d(s);

	transform (d.begin (), d.end (), d.begin (), (int(*)(int)) tolower);
	return d;
}

