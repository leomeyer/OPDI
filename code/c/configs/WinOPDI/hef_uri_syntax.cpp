
#include "hef_uri_syntax.h"
#include <stdio.h>
#include <cstdlib>
#include <cstring>

/*
	Code of this file is based on Poco library, file: URI.cpp.
	Original copyright is included below.

	//
	// URI.cpp
	//
	// $Id: //poco/1.4/Foundation/src/URI.cpp#5 $
	//
	// Library: Foundation
	// Package: URI
	// Module:  URI
	//
	// Copyright (c) 2004-2006, Applied Informatics Software Engineering GmbH.
	// and Contributors.
	//
	// Permission is hereby granted, free of charge, to any person or organization
	// obtaining a copy of the software and accompanying documentation covered by
	// this license (the "Software") to use, reproduce, display, distribute,
	// execute, and transmit the Software, and to prepare derivative works of the
	// Software, and to permit third-parties to whom the Software is furnished to
	// do so, all subject to the following:
	//
	// The copyright notices in the Software and this entire statement, including
	// the above license grant, this restriction and the following disclaimer,
	// must be included in all copies of the Software, in whole or in part, and
	// all derivative works of the Software, unless such copies or derivative
	// works are solely in the form of machine-executable object code generated by
	// a source language processor.
	//
	// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
	// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
	// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
	// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	// DEALINGS IN THE SOFTWARE.
	//
*/

namespace hef{
;

const std::string HfURISyntax::RESERVED_PATH     = "?#";
const std::string HfURISyntax::RESERVED_QUERY    = "#";
const std::string HfURISyntax::RESERVED_FRAGMENT = "";
const std::string HfURISyntax::ILLEGAL           = "%<>{}|\\\"^`";

HfURISyntax::HfURISyntax():
	_port(0), _flags(0)
{
}

HfURISyntax::HfURISyntax(const std::string& uri):
	_port(0), _flags(0)
{
	parse(uri);
}


HfURISyntax::HfURISyntax(const char* uri):
	_port(0), _flags(0)
{
	parse(std::string(uri));
}


HfURISyntax::HfURISyntax(const std::string& scheme, const std::string& pathEtc):
	_scheme(scheme),
	_port(0), _flags(0)
{
	toLowerInPlace3(_scheme);
	_port = getWellKnownPort();
	//_portStr = intToStr(_port);	//rm_a
	std::string::const_iterator beg = pathEtc.begin();
	std::string::const_iterator end = pathEtc.end();
	parsePathEtc(beg, end);
}


HfURISyntax::HfURISyntax(const std::string& scheme, const std::string& authority, const std::string& pathEtc):
	_scheme(scheme)
{
	toLowerInPlace3(_scheme);
	std::string::const_iterator beg = authority.begin();
	std::string::const_iterator end = authority.end();
	parseAuthority(beg, end);
	beg = pathEtc.begin();
	end = pathEtc.end();
	parsePathEtc(beg, end);
}


HfURISyntax::HfURISyntax(const std::string& scheme, const std::string& authority, const std::string& path, const std::string& query):
	_scheme(scheme),
	_path(path),
	_query(query)
{
	toLowerInPlace3(_scheme);
	std::string::const_iterator beg = authority.begin();
	std::string::const_iterator end = authority.end();
	parseAuthority(beg, end);
}


HfURISyntax::HfURISyntax(const std::string& scheme, const std::string& authority, const std::string& path, const std::string& query, const std::string& fragment):
	_scheme(scheme),
	_path(path),
	_query(query),
	_fragment(fragment)
{
	toLowerInPlace3(_scheme);
	std::string::const_iterator beg = authority.begin();
	std::string::const_iterator end = authority.end();
	parseAuthority(beg, end);
}


HfURISyntax::HfURISyntax(const HfURISyntax& uri):
	_scheme(uri._scheme),
	_userInfo(uri._userInfo),
	_host(uri._host),
	_port(uri._port),
	_portStr(uri._portStr),
	_path(uri._path),
	_query(uri._query),
	_fragment(uri._fragment),
	_flags(0)
{
}


HfURISyntax::HfURISyntax(const HfURISyntax& baseURI, const std::string& relativeURI):
	_scheme(baseURI._scheme),
	_userInfo(baseURI._userInfo),
	_host(baseURI._host),
	_port(baseURI._port),
	_portStr(baseURI._portStr),
	_path(baseURI._path),
	_query(baseURI._query),
	_fragment(baseURI._fragment),
	_flags(0)
{
	resolve(relativeURI);
}


HfURISyntax::~HfURISyntax()
{
}


HfURISyntax& HfURISyntax::operator = (const HfURISyntax& uri)
{
	if (&uri != this)
	{
		_scheme   = uri._scheme;
		_userInfo = uri._userInfo;
		_host     = uri._host;
		_port     = uri._port;
		_portStr  = uri._portStr;
		_path     = uri._path;
		_query    = uri._query;
		_fragment = uri._fragment;
		_flags    = uri._flags;
	}
	return *this;
}


HfURISyntax& HfURISyntax::operator = (const std::string& uri)
{
	clear();
	parse(uri);
	return *this;
}


HfURISyntax& HfURISyntax::operator = (const char* uri)
{
	clear();
	parse(std::string(uri));
	return *this;
}


void HfURISyntax::swap(HfURISyntax& uri)
{
	std::swap(_scheme, uri._scheme);
	std::swap(_userInfo, uri._userInfo);
	std::swap(_host, uri._host);
	std::swap(_port, uri._port);
	std::swap(_portStr, uri._portStr);
	std::swap(_path, uri._path);
	std::swap(_query, uri._query);
	std::swap(_fragment, uri._fragment);
	std::swap(_flags, uri._flags);
}


void HfURISyntax::clear()
{
	_scheme.clear();
	_userInfo.clear();
	_host.clear();
	_port = 0;
	_portStr.clear();
	_path.clear();
	_query.clear();
	_fragment.clear();
	_flags = 0;
}


std::string HfURISyntax::toString() const
{
	std::string uri;
	if (isRelative())
	{
		encode(_path, RESERVED_PATH, uri);
	}
	else
	{
		uri = _scheme;
		uri += ':';
		std::string auth = getAuthority();
		if (!auth.empty() || _scheme == "file")
		{
			uri.append("//");
			uri.append(auth);
		}
		if (!_path.empty())
		{
			if (!auth.empty() && _path[0] != '/')
				uri += '/';
			encode(_path, RESERVED_PATH, uri);
		}
		else if (!_query.empty() || !_fragment.empty())
		{
			uri += '/';
		}
	}
	if (!_query.empty())
	{
		uri += '?';
		uri.append(_query);
	}
	if (!_fragment.empty())
	{
		uri += '#';
		encode(_fragment, RESERVED_FRAGMENT, uri);
	}
	return uri;
}


void HfURISyntax::setScheme(const std::string& scheme)
{
	_scheme = scheme;
	toLowerInPlace3(_scheme);
	if (_port == 0){
		_port = getWellKnownPort();
		//_portStr = intToStr(_port);	//rm_a
	}
}


void HfURISyntax::setUserInfo(const std::string& userInfo)
{
	_userInfo.clear();
	decode(userInfo, _userInfo, _flags);
}


void HfURISyntax::setHost(const std::string& host)
{
	_host = host;
}


unsigned short HfURISyntax::getPort() const
{
	if (_port == 0)
		return getWellKnownPort();
	else
		return _port;
}
const std::string& HfURISyntax::getPortAsString() const
{
	return _portStr;
}


void HfURISyntax::setPort(unsigned short port)
{
	_port = port;
	_portStr = intToStr(port);
}


std::string HfURISyntax::getAuthority() const
{
	std::string auth;
	if (!_userInfo.empty())
	{
		auth.append(_userInfo);
		auth += '@';
	}
	if (_host.find(':') != std::string::npos)
	{
		auth += '[';
		auth += _host;
		auth += ']';
	}
	else auth.append(_host);
	if (_port && !isWellKnownPort())
	{
		auth += ':';
		auth += intToStr(_port);
	}
	return auth;
}


void HfURISyntax::setAuthority(const std::string& authority)
{
	_userInfo.clear();
	_host.clear();
	_port = 0;
	_portStr.clear();	// instead of ="0";
	std::string::const_iterator beg = authority.begin();
	std::string::const_iterator end = authority.end();
	parseAuthority(beg, end);
}


void HfURISyntax::setPath(const std::string& path)
{
	_path.clear();
	decode(path, _path, _flags);
}


void HfURISyntax::setRawQuery(const std::string& query)
{
	_query = query;
}


void HfURISyntax::setQuery(const std::string& query)
{
	_query.clear();
	encode(query, RESERVED_QUERY, _query);
}


std::string HfURISyntax::getQuery() const
{
	std::string query;
	decode(_query, query, _flags);
	return query;
}


void HfURISyntax::setFragment(const std::string& fragment)
{
	_fragment.clear();
	decode(fragment, _fragment, _flags);
}


void HfURISyntax::setPathEtc(const std::string& pathEtc)
{
	_path.clear();
	_query.clear();
	_fragment.clear();
	std::string::const_iterator beg = pathEtc.begin();
	std::string::const_iterator end = pathEtc.end();
	parsePathEtc(beg, end);
}


std::string HfURISyntax::getPathEtc() const
{
	std::string pathEtc;
	encode(_path, RESERVED_PATH, pathEtc);
	if (!_query.empty())
	{
		pathEtc += '?';
		pathEtc += _query;
	}
	if (!_fragment.empty())
	{
		pathEtc += '#';
		encode(_fragment, RESERVED_FRAGMENT, pathEtc);
	}
	return pathEtc;
}


std::string HfURISyntax::getPathAndQuery() const
{
	std::string pathAndQuery;
	encode(_path, RESERVED_PATH, pathAndQuery);
	if (!_query.empty())
	{
		pathAndQuery += '?';
		pathAndQuery += _query;
	}
	return pathAndQuery;
}


void HfURISyntax::resolve(const std::string& relativeURI)
{
	HfURISyntax parsedURI(relativeURI);
	resolve(parsedURI);
}


void HfURISyntax::resolve(const HfURISyntax& relativeURI)
{
	if (!relativeURI._scheme.empty())
	{
		_scheme   = relativeURI._scheme;
		_userInfo = relativeURI._userInfo;
		_host     = relativeURI._host;
		_port     = relativeURI._port;
		_portStr  = relativeURI._portStr;
		_path     = relativeURI._path;
		_query    = relativeURI._query;
		_flags    = relativeURI._flags;
		removeDotSegments();
	}
	else
	{
		if (!relativeURI._host.empty())
		{
			_userInfo = relativeURI._userInfo;
			_host     = relativeURI._host;
			_port     = relativeURI._port;
			_portStr  = relativeURI._portStr;
			_path     = relativeURI._path;
			_query    = relativeURI._query;
			_flags    = relativeURI._flags;
			removeDotSegments();
		}
		else
		{
			if (relativeURI._path.empty())
			{
				if (!relativeURI._query.empty())
					_query = relativeURI._query;
			}
			else
			{
				if (relativeURI._path[0] == '/')
				{
					_path = relativeURI._path;
					removeDotSegments();
				}
				else
				{
					mergePath(relativeURI._path);
				}
				_query = relativeURI._query;
			}
		}
	}
	_fragment = relativeURI._fragment;
}


bool HfURISyntax::isRelative() const
{
	return _scheme.empty();
}


bool HfURISyntax::empty() const
{
	return _scheme.empty() && _host.empty() && _path.empty() && _query.empty() && _fragment.empty();
}


bool HfURISyntax::operator == (const HfURISyntax& uri) const
{
	return equals(uri);
}


bool HfURISyntax::operator == (const std::string& uri) const
{
	HfURISyntax parsedURI(uri);
	return equals(parsedURI);
}


bool HfURISyntax::operator != (const HfURISyntax& uri) const
{
	return !equals(uri);
}


bool HfURISyntax::operator != (const std::string& uri) const
{
	HfURISyntax parsedURI(uri);
	return !equals(parsedURI);
}


bool HfURISyntax::equals(const HfURISyntax& uri) const
{
	return _scheme   == uri._scheme
	    && _userInfo == uri._userInfo
	    && _host     == uri._host
	    && getPort() == uri.getPort()
	    && _path     == uri._path
	    && _query    == uri._query
	    && _fragment == uri._fragment;
}


void HfURISyntax::normalize()
{
	removeDotSegments(!isRelative());
}


void HfURISyntax::removeDotSegments(bool removeLeading)
{
	if (_path.empty()) return;

	bool leadingSlash  = *(_path.begin()) == '/';
	bool trailingSlash = *(_path.rbegin()) == '/';
	std::vector<std::string> segments;
	std::vector<std::string> normalizedSegments;
	getPathSegments(segments);
	for (std::vector<std::string>::const_iterator it = segments.begin(); it != segments.end(); ++it)
	{
		if (*it == "..")
		{
			if (!normalizedSegments.empty())
			{
				if (normalizedSegments.back() == "..")
					normalizedSegments.push_back(*it);
				else
					normalizedSegments.pop_back();
			}
			else if (!removeLeading)
			{
				normalizedSegments.push_back(*it);
			}
		}
		else if (*it != ".")
		{
			normalizedSegments.push_back(*it);
		}
	}
	buildPath(normalizedSegments, leadingSlash, trailingSlash);
}


void HfURISyntax::getPathSegments(std::vector<std::string>& segments)
{
	getPathSegments(_path, segments);
}


void HfURISyntax::getPathSegments(const std::string& path, std::vector<std::string>& segments)
{
	std::string::const_iterator it  = path.begin();
	std::string::const_iterator end = path.end();
	std::string seg;
	while (it != end)
	{
		if (*it == '/')
		{
			if (!seg.empty())
			{
				segments.push_back(seg);
				seg.clear();
			}
		}
		else seg += *it;
		++it;
	}
	if (!seg.empty())
		segments.push_back(seg);
}


void HfURISyntax::encode(const std::string& str, const std::string& reserved, std::string& encodedStr)
{
	for (std::string::const_iterator it = str.begin(); it != str.end(); ++it)
	{
		char c = *it;
		if ((c >= 'a' && c <= 'z') ||
		    (c >= 'A' && c <= 'Z') ||
		    (c >= '0' && c <= '9') ||
		    c == '-' || c == '_' ||
		    c == '.' || c == '~')
		{
			encodedStr += c;
		}
		else if (c <= 0x20 || c >= 0x7F || ILLEGAL.find(c) != std::string::npos || reserved.find(c) != std::string::npos)
		{
			encodedStr += '%';
		//	encodedStr += NumberFormatter::formatHex( (unsigned) (unsigned char) c, 2 );
			char bfr[32];
			sprintf(bfr,"%02X", (unsigned int) (unsigned long) (unsigned char) c );
			encodedStr += bfr;
		}
		else encodedStr += c;
	}
}


void HfURISyntax::decode(const std::string& str, std::string& decodedStr, long _flags2 )
{
	std::string::const_iterator it  = str.begin();
	std::string::const_iterator end = str.end();
	while (it != end)
	{
		char c = *it++;
		if (c == '%')
		{
//			if (it == end) throw SyntaxException("URI encoding: no hex digit following percent sign", str);
			if (it == end){
				if( _flags2 & 0x1 )
					printf("ERROR: URI encoding: no hex digit following percent sign.\n");
				return;
			}
			char hi = *it++;
//			if (it == end) throw SyntaxException("URI encoding: two hex digits must follow percent sign", str);
			if (it == end){
				if( _flags2 & 0x1 )
					printf("ERROR: URI encoding: two hex digits must follow percent sign.\n");
				return;
			}
			char lo = *it++;
			if (hi >= '0' && hi <= '9')
				c = hi - '0';
			else if (hi >= 'A' && hi <= 'F')
				c = hi - 'A' + 10;
			else if (hi >= 'a' && hi <= 'f')
				c = hi - 'a' + 10;
			else{
//				throw SyntaxException("URI encoding: not a hex digit");
				if( _flags2 & 0x1 )
					printf("ERROR: URI encoding: not a hex digit.\n");
				return;
			}
			c *= 16;
			if (lo >= '0' && lo <= '9')
				c += lo - '0';
			else if (lo >= 'A' && lo <= 'F')
				c += lo - 'A' + 10;
			else if (lo >= 'a' && lo <= 'f')
				c += lo - 'a' + 10;
			else{
//				throw SyntaxException("URI encoding: not a hex digit");
				if( _flags2 & 0x1 )
					printf("ERROR: URI encoding: not a hex digit.\n");
				return;
			}
		}
		decodedStr += c;
	}
}


bool HfURISyntax::isWellKnownPort() const
{
	return _port == getWellKnownPort();
}


unsigned short HfURISyntax::getWellKnownPort() const
{
	if (_scheme == "ftp")
		return 21;
	else if (_scheme == "ssh")
		return 22;
	else if (_scheme == "telnet")
		return 23;
	else if (_scheme == "http")
		return 80;
	else if (_scheme == "nntp")
		return 119;
	else if (_scheme == "ldap")
		return 389;
	else if (_scheme == "https")
		return 443;
	else if (_scheme == "rtsp")
		return 554;
	else if (_scheme == "sip")
		return 5060;
	else if (_scheme == "sips")
		return 5061;
	else if (_scheme == "xmpp")
		return 5222;
	else
		return 0;
}


void HfURISyntax::parse(const std::string& uri)
{
	std::string::const_iterator it  = uri.begin();
	std::string::const_iterator end = uri.end();
	if (it == end) return;
	if (*it != '/' && *it != '.' && *it != '?' && *it != '#')
	{
		std::string scheme;
		while (it != end && *it != ':' && *it != '?' && *it != '#' && *it != '/') scheme += *it++;
		if (it != end && *it == ':')
		{
			++it;
//			if (it == end) throw SyntaxException("URI scheme must be followed by authority or path", uri);
			if (it == end){
				if( _flags & 0x1 )
					printf("ERROR: URI scheme must be followed by authority or path.\n");
				return;
			}
			setScheme(scheme);
			if (*it == '/')
			{
				++it;
				if (it != end && *it == '/')
				{
					++it;
					parseAuthority(it, end);
				}
				else --it;
			}
			parsePathEtc(it, end);
		}
		else
		{
			it = uri.begin();
			parsePathEtc(it, end);
		}
	}
	else parsePathEtc(it, end);
}


void HfURISyntax::parseAuthority(std::string::const_iterator& it, const std::string::const_iterator& end)
{
	std::string userInfo;
	std::string part;
	while (it != end && *it != '/' && *it != '?' && *it != '#')
	{
		if (*it == '@')
		{
			userInfo = part;
			part.clear();
		}
		else part += *it;
		++it;
	}
	std::string::const_iterator pbeg = part.begin();
	std::string::const_iterator pend = part.end();
	parseHostAndPort(pbeg, pend);
	_userInfo = userInfo;
}


void HfURISyntax::parseHostAndPort(std::string::const_iterator& it, const std::string::const_iterator& end)
{
	if (it == end) return;
	std::string host;
	if (*it == '[')
	{
		// IPv6 address
		++it;
		while (it != end && *it != ']') host += *it++;
//		if (it == end) throw SyntaxException("unterminated IPv6 address");
		if (it == end){
			if( _flags & 0x1 )
				printf("ERROR: unterminated IPv6 address.\n");
			return;
		}
		++it;
	}
	else
	{
		while (it != end && *it != ':') host += *it++;
	}
	if (it != end && *it == ':')
	{
		++it;
		std::string port;
		while (it != end) port += *it++;
		if (!port.empty())
		{
			int nport = 0;
			//if (NumberParser::tryParse(port, nport) && nport > 0 && nport < 65536)
			//	_port = (unsigned short) nport;
			//else{
			//	throw SyntaxException("bad or invalid port number", port);
			//}
			_portStr = port.c_str();
			nport = atoi( port.c_str() );
			if( nport < 0 ){
				_port = 0;
			}else if( nport > 65535 ){
				_port = 65535;
			}else{
				_port = (unsigned short) nport;
			}
		}else{
			_port = getWellKnownPort();
			//_portStr = intToStr( _port );	//rm_a
		}
	}else{
		_port = getWellKnownPort();
		//_portStr = intToStr(_port);	//rm_a
	}
	_host = host;
	toLowerInPlace3(_host);
}


void HfURISyntax::parsePath(std::string::const_iterator& it, const std::string::const_iterator& end)
{
	std::string path;
	while (it != end && *it != '?' && *it != '#') path += *it++;
	decode(path, _path, _flags);
}


void HfURISyntax::parsePathEtc(std::string::const_iterator& it, const std::string::const_iterator& end)
{
	if (it == end) return;
	if (*it != '?' && *it != '#')
		parsePath(it, end);
	if (it != end && *it == '?')
	{
		++it;
		parseQuery(it, end);
	}
	if (it != end && *it == '#')
	{
		++it;
		parseFragment(it, end);
	}
}


void HfURISyntax::parseQuery(std::string::const_iterator& it, const std::string::const_iterator& end)
{
	_query.clear();
	while (it != end && *it != '#') _query += *it++;
}


void HfURISyntax::parseFragment(std::string::const_iterator& it, const std::string::const_iterator& end)
{
	std::string fragment;
	while (it != end) fragment += *it++;
	decode(fragment, _fragment, _flags);
}


void HfURISyntax::mergePath(const std::string& path)
{
	std::vector<std::string> segments;
	std::vector<std::string> normalizedSegments;
	bool addLeadingSlash = false;
	if (!_path.empty())
	{
		getPathSegments(segments);
		bool endsWithSlash = *(_path.rbegin()) == '/';
		if (!endsWithSlash && !segments.empty())
			segments.pop_back();
		addLeadingSlash = _path[0] == '/';
	}
	getPathSegments(path, segments);
	addLeadingSlash = addLeadingSlash || (!path.empty() && path[0] == '/');
	bool hasTrailingSlash = (!path.empty() && *(path.rbegin()) == '/');
	bool addTrailingSlash = false;
	for (std::vector<std::string>::const_iterator it = segments.begin(); it != segments.end(); ++it)
	{
		if (*it == "..")
		{
			addTrailingSlash = true;
			if (!normalizedSegments.empty())
				normalizedSegments.pop_back();
		}
		else if (*it != ".")
		{
			addTrailingSlash = false;
			normalizedSegments.push_back(*it);
		}
		else addTrailingSlash = true;
	}
	buildPath(normalizedSegments, addLeadingSlash, hasTrailingSlash || addTrailingSlash);
}


void HfURISyntax::buildPath(const std::vector<std::string>& segments, bool leadingSlash, bool trailingSlash)
{
	_path.clear();
	bool first = true;
	for (std::vector<std::string>::const_iterator it = segments.begin(); it != segments.end(); ++it)
	{
		if (first)
		{
			first = false;
			if (leadingSlash)
				_path += '/';
			else if (_scheme.empty() && (*it).find(':') != std::string::npos)
				_path.append("./");
		}
		else _path += '/';
		_path.append(*it);
	}
	if (trailingSlash)
		_path += '/';
}

bool HfURISyntax::isIPAddress( const std::string& str )
{
	const char* str2 = str.c_str();
	const char* ptr = strrchr( str2, (int)'.' );
	if( ptr && *ptr == '.' ){
		++ptr;
		if( strchr("0123456789", *ptr ) )
			return 1;
	}
	return 0;
}
void HfURISyntax::setPrintErrors( bool in )
{
	if(in){
		_flags |= 0x1;
	}else{
		_flags &= ~0x1;
	}
}
std::string HfURISyntax::intToStr( int a )
{
	char bfr[32];
	sprintf(bfr,"%d",a);
	return bfr;
}

} // end namespace hef
