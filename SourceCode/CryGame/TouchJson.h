//////////////////////////////////////////////////////////////////////
//
//  Touch layout JSON mini-parser.
//  Restricted JSON: objects, arrays, strings, numbers, bools.
//  Header-only, no dependencies, usable from tests too.
//
//////////////////////////////////////////////////////////////////////

#ifndef _TOUCH_JSON_H_
#define _TOUCH_JSON_H_

#include <string.h>
#include <stdlib.h>

// In engine builds 'string' is CryString (provided by platform.h).
// Standalone (unit tests): fall back to std::string.
#ifdef TOUCHJSON_STANDALONE
#include <string>
using std::string;
#define TJSON_STRNICMP strncasecmp
#include <strings.h>
#else
#if defined(_WIN32)
#define TJSON_STRNICMP _strnicmp
#else
#define TJSON_STRNICMP strncasecmp
#include <strings.h>
#endif
#endif

namespace TouchJson
{
	struct Parser
	{
		const char *p;
		bool ok;

		explicit Parser(const char *s) : p(s), ok(true) {}
		void SkipWs() { while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++; }
		bool Consume(char c) { SkipWs(); if (*p == c) { p++; return true; } ok = false; return false; }
		bool Peek(char c) { SkipWs(); return *p == c; }
		bool Good() const { return ok; }

		string ParseString()
		{
			string res;
			SkipWs();
			if (*p != '"') { ok = false; return res; }
			p++;
			while (*p && *p != '"')
			{
				if (*p == '\\' && p[1])
				{
					p++;
					switch (*p)
					{
					case 'n': res += '\n'; break;
					case 't': res += '\t'; break;
					case 'r': res += '\r'; break;
					case '\\': res += '\\'; break;
					case '"': res += '"'; break;
					case '/': res += '/'; break;
					default: res += *p; break;
					}
				}
				else
					res += *p;
				p++;
			}
			if (*p != '"') { ok = false; return res; }
			p++;
			return res;
		}

		float ParseNumber()
		{
			SkipWs();
			char *end = 0;
			float v = (float)strtod(p, &end);
			if (end == p) { ok = false; return 0; }
			p = end;
			return v;
		}

		bool ParseBool(bool &val)
		{
			SkipWs();
			if (!TJSON_STRNICMP(p, "true", 4)) { p += 4; val = true; return true; }
			if (!TJSON_STRNICMP(p, "false", 5)) { p += 5; val = false; return true; }
			ok = false;
			return false;
		}

		// skip any value (string, number, bool, array, object)
		void SkipValue()
		{
			SkipWs();
			if (*p == '"') { ParseString(); return; }
			if (*p == '[' || *p == '{')
			{
				char open = *p;
				char close = (open == '[') ? ']' : '}';
				int depth = 0;
				while (*p)
				{
					if (*p == open) depth++;
					else if (*p == close) { depth--; p++; if (depth <= 0) return; continue; }
					else if (*p == '"') { ParseString(); continue; }
					p++;
				}
				ok = false;
				return;
			}
			if (*p == 't' || *p == 'f') { bool b; ParseBool(b); return; }
			if (*p == 'n' && !TJSON_STRNICMP(p, "null", 4)) { p += 4; return; }
			ParseNumber();
		}
	};

	static string Escape(const string &s)
	{
		string res;
		for (unsigned i = 0; i < s.size(); i++)
		{
			char c = s[i];
			if (c == '"' || c == '\\') { res += '\\'; res += c; }
			else if (c == '\n') res += "\\n";
			else if (c == '\t') res += "\\t";
			else res += c;
		}
		return res;
	}
}

#endif // _TOUCH_JSON_H_
