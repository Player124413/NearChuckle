//////////////////////////////////////////////////////////////////////
//
//  Unit tests for the touch layout JSON parser (standalone).
//  Build & run:  g++ -std=c++17 -DTOUCHJSON_STANDALONE \
//                    -I ../SourceCode/CryGame tests/TouchJsonTest.cpp -o /tmp/touchtest && /tmp/touchtest
//
//////////////////////////////////////////////////////////////////////

#include "../SourceCode/CryGame/TouchJson.h"

#include <assert.h>
#include <stdio.h>
#include <string>
#include <math.h>

static int g_failures = 0;

#define CHECK(cond) do { \
	if (!(cond)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_failures++; } \
} while (0)

#define CHECK_NEAR(a, b) do { \
	float _a = (a), _b = (b); \
	if (fabsf(_a - _b) > 0.0001f) { printf("FAIL %s:%d: %f != %f\n", __FILE__, __LINE__, _a, _b); g_failures++; } \
} while (0)

int main()
{
	// --- basic types ---
	{
		TouchJson::Parser p("  \"hello world\" ");
		CHECK(p.ParseString() == "hello world");
		CHECK(p.Good());
	}
	{
		TouchJson::Parser p("\"esc \\\"quoted\\\" \\n line\"");
		CHECK(p.ParseString() == "esc \"quoted\" \n line");
		CHECK(p.Good());
	}
	{
		TouchJson::Parser p(" 3.14159 ");
		CHECK_NEAR(p.ParseNumber(), 3.14159f);
		CHECK(p.Good());
	}
	{
		TouchJson::Parser p("-12");
		CHECK_NEAR(p.ParseNumber(), -12.0f);
	}
	{
		TouchJson::Parser p("true");
		bool v = false;
		CHECK(p.ParseBool(v) && v);
	}
	{
		TouchJson::Parser p("false");
		bool v = true;
		CHECK(p.ParseBool(v) && !v);
	}
	{
		TouchJson::Parser p("null");
		p.SkipValue();
		CHECK(p.Good());
	}

	// --- escape round-trip ---
	{
		string src = string("a\"b\\c") + '\n' + 'd';
		string esc = TouchJson::Escape(src);
		string quoted = "\"" + esc + "\"";
		TouchJson::Parser p(quoted.c_str());
		CHECK(p.ParseString() == src);
	}

	// --- a full touch layout document ---
	{
		const char *doc =
		"{\n"
		"  \"version\": 1,\n"
		"  \"stick_dynamic\": false,\n"
		"  \"opacity\": 0.45,\n"
		"  \"elements\": [\n"
		"    { \"name\": \"fire\", \"label\": \"FIRE\", \"key\": \"mouse1\", \"x\": 0.86, \"y\": 0.7, \"size\": 0.068, \"visible\": true, \"round\": true, \"tap\": false },\n"
		"    { \"name\": \"edit\", \"label\": \"EDIT\", \"key\": \"none\", \"x\": 0.965, \"y\": 0.048, \"size\": 0.036, \"visible\": true, \"round\": false, \"tap\": true }\n"
		"  ]\n"
		"}\n";

		TouchJson::Parser p(doc);
		CHECK(p.Consume('{'));
		float fVersion = 0, opacity = 0;
		bool stickDyn = true;
		int nElementsParsed = 0;
		while (!p.Peek('}') && p.Good())
		{
			string key = p.ParseString();
			CHECK(p.Consume(':'));
			if (key == "version") fVersion = p.ParseNumber();
			else if (key == "stick_dynamic") p.ParseBool(stickDyn);
			else if (key == "opacity") opacity = p.ParseNumber();
			else if (key == "elements")
			{
				CHECK(p.Consume('['));
				while (!p.Peek(']') && p.Good())
				{
					CHECK(p.Consume('{'));
					while (!p.Peek('}') && p.Good())
					{
						string ek = p.ParseString();
						CHECK(p.Consume(':'));
						if (p.Peek('"')) { string v = p.ParseString(); (void)v; }
						else if (p.Peek('t') || p.Peek('f')) { bool b; p.ParseBool(b); }
						else p.ParseNumber();
						if (p.Peek(',')) p.Consume(','); else break;
					}
					CHECK(p.Consume('}'));
					nElementsParsed++;
					if (p.Peek(',')) p.Consume(','); else break;
				}
				CHECK(p.Consume(']'));
			}
			else p.SkipValue();
			if (p.Peek(',')) p.Consume(','); else break;
		}
		CHECK(p.Consume('}'));
		CHECK(p.Good());
		CHECK_NEAR(fVersion, 1.0f);
		CHECK_NEAR(opacity, 0.45f);
		CHECK(!stickDyn);
		CHECK(nElementsParsed == 2);
	}

	// --- SkipValue over nested structures ---
	{
		TouchJson::Parser p("{\"a\":{\"b\":[1,{\"c\":\"x],\"}],\"d\":2},\"e\":3}");
		CHECK(p.Consume('{'));
		string k1 = p.ParseString();
		CHECK(k1 == "a");
		CHECK(p.Consume(':'));
		p.SkipValue();
		CHECK(p.Good());
		CHECK(p.Peek(','));
		CHECK(p.Consume(','));
		string k2 = p.ParseString();
		CHECK(k2 == "e");
	}

	if (g_failures == 0)
		printf("ALL TOUCH JSON TESTS PASSED\n");
	else
		printf("%d FAILURES\n", g_failures);
	return g_failures == 0 ? 0 : 1;
}
