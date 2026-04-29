////////////////////////////////////////////////////////////////////////////
//	Author      : Maxim Kornienko
//	Description : ibNumber unit tests — temporary MSBuild console-exe.
//                Mirrors enterprise/tests/test_number.cpp (gtest). Used while
//                the CMake gtest setup is incomplete; remove this exe after
//                cmake build with DB drivers is wired up.
////////////////////////////////////////////////////////////////////////////

#include <wx/wx.h>
#include <wx/app.h>
#include <wx/file.h>
#include <wx/textfile.h>

#include <climits>
#include <sstream>
#include <cstdio>

#include "backend/number.h"

namespace
{
	int g_passed = 0;
	int g_failed = 0;
	wxArrayString g_log;

	void Check(const wxString& name, bool ok, const wxString& detail = wxEmptyString)
	{
		const wxString line = (ok ? wxT("PASS  ") : wxT("FAIL  ")) + name
			+ (detail.IsEmpty() || ok ? wxString() : wxT(" — ") + detail);
		if (ok) ++g_passed; else ++g_failed;
		g_log.Add(line);
		std::printf("%s\n", static_cast<const char*>(line.utf8_str()));
		std::fflush(stdout);
	}

	wxString Hex16(const uint8_t buf[16])
	{
		wxString out;
		for (int i = 15; i >= 0; --i) out += wxString::Format(wxT("%02X"), buf[i]);
		return out;
	}

	void RunNumberTests()
	{
		// Layout
		Check(wxT("[L]  sizeof(ibNumber) == 8"), sizeof(ibNumber) == 8,
		      wxString::Format(wxT("got %zu"), sizeof(ibNumber)));
		Check(wxT("[L]  alignof(ibNumber) == 8"), alignof(ibNumber) == 8);

		// Constructors
		{
			ibNumber z;
			Check(wxT("[C]  default ctor → 0"), z.IsZero() && !z.IsHeap());
		}
		{
			ibNumber n(456);
			Check(wxT("[C]  ibNumber(int 456) immediate"), !n.IsHeap());
			Check(wxT("[C]  ibNumber(int 456) ToString '456'"), n.ToString() == wxT("456"));
		}
		{
			ibNumber n(static_cast<int>(-1));
			Check(wxT("[C]  ibNumber(int -1) ToString '-1'"), n.ToString() == wxT("-1"));
			Check(wxT("[C]  ibNumber(int -1) IsSign true"), n.IsSign());
		}
		{
			ibNumber n(static_cast<unsigned int>(4294967295u));
			Check(wxT("[C]  ibNumber(UINT_MAX) ToString"),
			      n.ToString() == wxT("4294967295"), n.ToString());
		}
		{
			ibNumber lo(static_cast<int64_t>(INT64_MIN));
			ibNumber hi(static_cast<int64_t>(INT64_MAX));
			Check(wxT("[C]  ibNumber(INT64_MIN) ToString"),
			      lo.ToString() == wxT("-9223372036854775808"), lo.ToString());
			Check(wxT("[C]  ibNumber(INT64_MAX) ToString"),
			      hi.ToString() == wxT("9223372036854775807"), hi.ToString());
			Check(wxT("[C]  INT64_MIN goes heap"),  lo.IsHeap());
			Check(wxT("[C]  INT64_MAX goes heap"),  hi.IsHeap());
		}
		{
			ibNumber u(static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFULL));
			Check(wxT("[C]  ibNumber(uint64_t MAX) ToString"),
			      u.ToString() == wxT("18446744073709551615"), u.ToString());
		}
		{
			const int64_t kImmMax = (1LL << 46) - 1;
			const int64_t kImmMin = -(1LL << 46);
			ibNumber maxImm(kImmMax);
			ibNumber minImm(kImmMin);
			Check(wxT("[C]  +2^46-1 immediate"), !maxImm.IsHeap());
			Check(wxT("[C]  -2^46  immediate"),  !minImm.IsHeap());
			ibNumber maxImmPlus1(kImmMax + 1);
			Check(wxT("[C]  +2^46 promotes to heap"), maxImmPlus1.IsHeap());
			Check(wxT("[C]  +2^46 ToString"), maxImmPlus1.ToString() == wxT("70368744177664"));
		}

		// Parser
		{
			ibNumber n;
			Check(wxT("[P]  FromString('') returns false"), !n.FromString(wxEmptyString));
			Check(wxT("[P]  FromString('') leaves 0"), n.IsZero());
		}
		{
			ibNumber n;
			Check(wxT("[P]  FromString('-') fails"),  !n.FromString(wxString(wxT("-"))));
			Check(wxT("[P]  FromString('.') fails"),  !n.FromString(wxString(wxT("."))));
			Check(wxT("[P]  FromString('abc') fails"), !n.FromString(wxString(wxT("abc"))));
		}
		{
			ibNumber n;
			Check(wxT("[P]  FromString('+5') ok"),
			      n.FromString(wxString(wxT("+5"))) && n == ibNumber(5));
			Check(wxT("[P]  FromString('  42 ') ok"),
			      n.FromString(wxString(wxT("  42 "))) && n == ibNumber(42));
		}
		{
			ibNumber a(wxString(wxT("1.5e2")));
			Check(wxT("[P]  '1.5e2' == 150"), a == ibNumber(150), a.ToString());
			ibNumber b(wxString(wxT("1.5e-2")));
			Check(wxT("[P]  '1.5e-2' == 0.015"),
			      b == ibNumber(wxString(wxT("0.015"))), b.ToString());
		}
		{
			ibNumber n; bool ok = n.FromString(wxString(wxT("12x")));
			Check(wxT("[P]  '12x' parses prefix '12'"), ok && n == ibNumber(12));
		}

		// ToString
		{
			ibNumber n(0);
			Check(wxT("[S]  zero ToString '0'"), n.ToString() == wxT("0"));
		}
		{
			ibNumber n(wxString(wxT("0.01")));
			Check(wxT("[S]  '0.01' round-trip"), n.ToString() == wxT("0.01"));
		}
		{
			ibNumber n(wxString(wxT("-0.0001")));
			Check(wxT("[S]  '-0.0001' round-trip"), n.ToString() == wxT("-0.0001"));
		}
		{
			ibNumber n(wxString(wxT("1000000000000000000")));
			Check(wxT("[S]  10^18 round-trip"), n.ToString() == wxT("1000000000000000000"));
		}
		{
			const wxString s = wxT("765.3456754567765443343");
			ibNumber n(s);
			Check(wxT("[S]  22-digit heap round-trip"),
			      n.IsHeap() && n.ToString() == s, n.ToString());
		}
		{
			wxString s; s.reserve(202);
			for (int i = 0; i < 100; ++i) s += wxChar(wxT('1') + (i % 9));
			s += wxT('.');
			for (int i = 0; i < 100; ++i) s += wxChar(wxT('0') + ((i + 3) % 10));
			ibNumber n(s);
			Check(wxT("[S]  200-digit/100-frac round-trip"),
			      n.IsHeap() && n.ToString() == s,
			      wxString::Format(wxT("got len=%zu"), n.ToString().Length()));
		}
		{
			ibNumber n(wxString(wxT("-0")));
			Check(wxT("[S]  '-0' canonicalises to '0'"),
			      n.ToString() == wxT("0") && !n.IsSign());
		}
		{
			ibNumber n(wxString(wxT("00123")));
			Check(wxT("[S]  '00123' → '123' (leading zeros dropped)"),
			      n.ToString() == wxT("123"));
		}

		// Arithmetic
		{
			ibNumber a(wxString(wxT("0.1")));
			ibNumber b(wxString(wxT("0.2")));
			Check(wxT("[A]  0.1 + 0.2 == 0.3 (exact)"),
			      (a + b) == ibNumber(wxString(wxT("0.3"))), (a + b).ToString());
		}
		{
			ibNumber a(0), b(0);
			Check(wxT("[A]  0 + 0 == 0"), (a + b).IsZero());
			Check(wxT("[A]  0 - 0 == 0"), (a - b).IsZero());
			Check(wxT("[A]  0 * 0 == 0"), (a * b).IsZero());
		}
		{
			Check(wxT("[A]  -3 * -4 == 12"), ibNumber(-3) * ibNumber(-4) == ibNumber(12));
			Check(wxT("[A]  -3 *  4 == -12"), ibNumber(-3) * ibNumber(4) == ibNumber(-12));
			Check(wxT("[A]   3 * -4 == -12"), ibNumber(3) * ibNumber(-4) == ibNumber(-12));
			Check(wxT("[A]   0 * -7 == 0"),   (ibNumber(0) * ibNumber(-7)).IsZero());
		}
		{
			ibNumber a(wxString(wxT("4294967295")));
			Check(wxT("[A]  2^32-1 + 1 == 2^32"),
			      (a + ibNumber(1)) == ibNumber(wxString(wxT("4294967296"))));
		}
		{
			ibNumber a(wxString(wxT("18446744073709551616")));
			Check(wxT("[A]  2^64 + 2^64 == 2*2^64"),
			      (a + a) == ibNumber(wxString(wxT("36893488147419103232"))));
		}
		{
			ibNumber a(wxString(wxT("123456789012345")));
			Check(wxT("[A]  big - big == 0"), (a - a).IsZero());
		}
		{
			Check(wxT("[A]  12.5 * 8 == 100"),
			      (ibNumber(wxString(wxT("12.5"))) * ibNumber(8)) == ibNumber(100));
		}
		{
			ibNumber c = ibNumber(1) / ibNumber(3);
			const wxString s = c.ToString();
			Check(wxT("[A]  1/3 starts with '0.3333333333'"),
			      s.StartsWith(wxT("0.3333333333")), s);
		}
		{
			bool threw = false;
			try { ibNumber c = ibNumber(5) / ibNumber(0); (void)c; }
			catch (...) { threw = true; }
			Check(wxT("[A]  divide-by-zero throws"), threw);
		}
		{
			Check(wxT("[A]  10 % 3 == 1"),  (ibNumber(10) % ibNumber(3)) == ibNumber(1));
			Check(wxT("[A]  -10 % 3 == -1"), (ibNumber(-10) % ibNumber(3)) == ibNumber(-1));
		}
		{
			ibNumber a(5);
			Check(wxT("[A]  ++a yields 6 (pre)"),  (++a) == ibNumber(6));
			Check(wxT("[A]  ++a side-effect"),    a == ibNumber(6));
			ibNumber b(5);
			Check(wxT("[A]  b++ yields 5 (post)"), (b++) == ibNumber(5));
			Check(wxT("[A]  b++ side-effect"),    b == ibNumber(6));
		}

		// Compare
		{
			ibNumber tiny(5);
			ibNumber bigPos(wxString(wxT("123456789012345678901234567890")));
			ibNumber bigNeg(wxString(wxT("-123456789012345678901234567890")));
			Check(wxT("[CMP] tiny(5) < bigPos"), tiny < bigPos);
			Check(wxT("[CMP] bigPos > tiny"),    bigPos > tiny);
			Check(wxT("[CMP] bigNeg < tiny"),    bigNeg < tiny);
			Check(wxT("[CMP] bigNeg < bigPos"),  bigNeg < bigPos);
			Check(wxT("[CMP] -bigPos == bigNeg"), -bigPos == bigNeg);
		}
		{
			Check(wxT("[CMP] 1 == '1.0' (exp normalize)"),
			      ibNumber(1) == ibNumber(wxString(wxT("1.0"))));
			Check(wxT("[CMP] 0 == -0"),
			      ibNumber(0) == ibNumber(wxString(wxT("-0"))));
		}

		// Rounding
		{
			Check(wxT("[R]  Round of 1.5 → 2"),  ibNumber(wxString(wxT("1.5"))).Round() == ibNumber(2));
			Check(wxT("[R]  Round of 2.5 → 3"),  ibNumber(wxString(wxT("2.5"))).Round() == ibNumber(3));
			Check(wxT("[R]  Round of -1.5 → -2"), ibNumber(wxString(wxT("-1.5"))).Round() == ibNumber(-2));
			Check(wxT("[R]  Round of 0.4 → 0"),  ibNumber(wxString(wxT("0.4"))).Round().IsZero());
			Check(wxT("[R]  Round of 0.6 → 1"),  ibNumber(wxString(wxT("0.6"))).Round() == ibNumber(1));
			Check(wxT("[R]  Round(2) of 1.234 → 1.23"),
			      ibNumber(wxString(wxT("1.234"))).Round(2) == ibNumber(wxString(wxT("1.23"))));
			Check(wxT("[R]  Round(2) of 1.235 → 1.24"),
			      ibNumber(wxString(wxT("1.235"))).Round(2) == ibNumber(wxString(wxT("1.24"))));
		}
		{
			Check(wxT("[R]  Trunc 1.99 → 1"),  ibNumber(wxString(wxT("1.99"))).Trunc() == ibNumber(1));
			Check(wxT("[R]  Trunc -1.99 → -1"), ibNumber(wxString(wxT("-1.99"))).Trunc() == ibNumber(-1));
			Check(wxT("[R]  Trunc 0.5 → 0"),   ibNumber(wxString(wxT("0.5"))).Trunc().IsZero());
		}

		// Conversions
		{
			Check(wxT("[CV] ToInt(456) == 456"),  ibNumber(456).ToInt() == 456);
			Check(wxT("[CV] ToInt(-456) == -456"), ibNumber(-456).ToInt() == -456);
			Check(wxT("[CV] ToInt(1.99 trunc) == 1"),
			      ibNumber(wxString(wxT("1.99"))).ToInt() == 1);
			Check(wxT("[CV] ToInt overflow clamps to INT_MAX"),
			      ibNumber(static_cast<int64_t>(INT64_MAX)).ToInt() == INT_MAX);
		}
		{
			Check(wxT("[CV] ToUInt(-5) == 0"), ibNumber(-5).ToUInt() == 0u);
			Check(wxT("[CV] ToUInt(456) == 456"), ibNumber(456).ToUInt() == 456u);
		}
		{
			int64_t out = -1;
			int rc = ibNumber(static_cast<int64_t>(INT64_MAX)).ToInt(out);
			Check(wxT("[CV] ToInt(int64&) of INT64_MAX"), rc == 0 && out == INT64_MAX);
			rc = ibNumber(wxString(wxT("99999999999999999999"))).ToInt(out);
			Check(wxT("[CV] ToInt(int64&) of 20-digit returns overflow"), rc != 0);
		}
		{
			double d = ibNumber(wxString(wxT("3.14"))).ToDouble();
			Check(wxT("[CV] ToDouble 3.14 ≈ 3.14"), d > 3.139 && d < 3.141);
			float f = ibNumber(wxString(wxT("2.5"))).ToFloat();
			Check(wxT("[CV] ToFloat 2.5 == 2.5"), f == 2.5f);
		}

		// Math compat
		{
			Check(wxT("[M]  2^10 == 1024"),  ibNumber(2).Pow(10) == ibNumber(1024));
			Check(wxT("[M]  2^32 == 4294967296"),
			      ibNumber(2).Pow(32) == ibNumber(static_cast<int64_t>(4294967296LL)));
			Check(wxT("[M]  Pow(0) == 1"),   ibNumber(7).Pow(0) == ibNumber(1));
			Check(wxT("[M]  Pow(-2) of 4 == 0.0625"),
			      ibNumber(4).Pow(-2) == ibNumber(wxString(wxT("0.0625"))));
		}
		{
			ibNumber s = ibNumber(16).Sqrt();
			Check(wxT("[M]  Sqrt(16) ≈ 4"),
			      (s - ibNumber(4)).Abs() < ibNumber(wxString(wxT("0.01"))));
		}
		{
			ibNumber a(wxString(wxT("-7.5")));
			Check(wxT("[M]  Abs(-7.5) == 7.5"), a.Abs() == ibNumber(wxString(wxT("7.5"))));
			ibNumber b(wxString(wxT("3")));
			b.ChangeSign();
			Check(wxT("[M]  ChangeSign(3) == -3"), b == ibNumber(-3));
			Check(wxT("[M]  IsSign(-3) true"), ibNumber(-3).IsSign());
			Check(wxT("[M]  IsSign(0) false"), !ibNumber(0).IsSign());
			Check(wxT("[M]  IsSign(3) false"), !ibNumber(3).IsSign());
		}

		// Get/SetBuffer — zero is encoded as an empty buffer (no header bytes).
		{
			ibNumber a;
			wxMemoryBuffer blob = a.GetBuffer();
			Check(wxT("[BUF] GetBuffer(0) size == 0 (compact-zero)"),
			      blob.GetDataLen() == 0);
			ibNumber b(7);
			Check(wxT("[BUF] SetBuffer(empty) → 0"), b.SetBuffer(blob));
			Check(wxT("[BUF] result IsZero"), b.IsZero());
		}
		{
			const wxString s = wxT("-7654321.0987654321");
			ibNumber a(s);
			wxMemoryBuffer blob = a.GetBuffer();
			ibNumber b;
			Check(wxT("[BUF] SetBuffer ok"), b.SetBuffer(blob));
			Check(wxT("[BUF] round-trip equals original"), b == a);
		}
		{
			const wxString s = wxT("12345678901234567890123456789012345678901234567890");
			ibNumber a(s);
			wxMemoryBuffer blob = a.GetBuffer();
			ibNumber b;
			b.SetBuffer(blob);
			Check(wxT("[BUF] 50-digit round-trip"), b == a, b.ToString());
		}
		{
			// Length 0 — compact-zero encoding, succeeds regardless of pointer.
			ibNumber b(42);
			Check(wxT("[BUF] SetBuffer(null, 0) → zero (compact)"),
			      b.SetBuffer(nullptr, 0) && b.IsZero());
		}
		{
			// Bad short buffer (< 9 bytes but > 0) is malformed — must fail.
			ibNumber b(42);
			uint8_t junk[3] = { 0x55, 0xAA, 0x00 };
			Check(wxT("[BUF] SetBuffer(short malformed) → false"),
			      !b.SetBuffer(junk, sizeof(junk)));
		}
		{
			wxMemoryBuffer mb;
			ibNumber(42).GetBuffer(mb);
			const size_t firstLen = mb.GetDataLen();
			ibNumber(100).GetBuffer(mb);
			Check(wxT("[BUF] reuse buffer — second GetBuffer overwrites"),
			      mb.GetDataLen() == firstLen);
			ibNumber check;
			check.SetBuffer(mb);
			Check(wxT("[BUF] reused buffer reads back as second value"),
			      check == ibNumber(100));
		}
		{
			// Compact-zero through reuse: a non-zero value followed by zero
			// shrinks the buffer to length 0.
			wxMemoryBuffer mb;
			ibNumber(42).GetBuffer(mb);
			Check(wxT("[BUF] non-zero produces non-empty buffer"),
			      mb.GetDataLen() > 0);
			ibNumber(0).GetBuffer(mb);
			Check(wxT("[BUF] zero shrinks buffer to length 0"),
			      mb.GetDataLen() == 0);
		}

		// 128-byte API
		{
			ibNumber a(0);
			uint8_t buf[16] = {0};
			a.To128Bytes(buf);
			Check(wxT("[128] To128Bytes(0) all zero"),
			      Hex16(buf) == wxT("00000000000000000000000000000000"));
		}
		{
			ibNumber a(static_cast<int64_t>(-1));
			uint8_t buf[16];
			a.To128Bytes(buf);
			Check(wxT("[128] To128Bytes(-1) all 0xFF"),
			      Hex16(buf) == wxT("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"));
			ibNumber b;
			b.From128Bytes(buf);
			Check(wxT("[128] From128Bytes(-1) round-trip"), b == a);
		}
		{
			ibNumber a(wxString(wxT("123456789012345678901234567890")));
			uint8_t buf[16];
			a.To128Bytes(buf);
			ibNumber b;
			b.From128Bytes(buf);
			Check(wxT("[128] 30-digit positive round-trip"), b == a, b.ToString());
		}
		{
			ibNumber a(wxString(wxT("-123456789012345678901234567890")));
			uint8_t buf[16];
			a.To128Bytes(buf);
			ibNumber b;
			b.From128Bytes(buf);
			Check(wxT("[128] 30-digit negative round-trip"), b == a, b.ToString());
		}

		// Stream operators
		{
			std::stringstream ss;
			ss << ibNumber(wxString(wxT("3.14")));
			Check(wxT("[ST] ostream << 3.14 → '3.14'"), ss.str() == "3.14",
			      wxString::FromAscii(ss.str().c_str()));
		}
		{
			std::stringstream ss;
			ss << ibNumber(-42);
			Check(wxT("[ST] ostream << -42 → '-42'"), ss.str() == "-42");
		}
	}

	void WriteResults()
	{
		wxString path = wxT("numberTest.log");
		wxTextFile f(path);
		if (f.Exists()) f.Open(); else f.Create();
		f.Clear();
		for (const wxString& line : g_log) f.AddLine(line);
		f.AddLine(wxString::Format(wxT("---- summary: %d passed, %d failed ----"),
		                            g_passed, g_failed));
		f.Write();
		f.Close();
	}
}

int main(int argc, char** argv)
{
	if (!wxEntryStart(argc, argv)) {
		std::fprintf(stderr, "wxEntryStart failed\n");
		return 2;
	}

	RunNumberTests();
	WriteResults();

	std::printf("---- summary: %d passed, %d failed ----\n", g_passed, g_failed);
	std::fflush(stdout);

	wxEntryCleanup();
	return g_failed == 0 ? 0 : 1;
}
