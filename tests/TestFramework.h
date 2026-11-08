#pragma once

#include <cmath>
#include <cstdio>
#include <string>

// Minimal test helpers shared by the test files.
namespace testing
{
inline int failures = 0;
inline int checks = 0;

inline void check (bool ok, const std::string& what)
{
    ++checks;
    if (! ok)
    {
        ++failures;
        std::printf ("  FAIL: %s\n", what.c_str());
    }
}

inline void section (const std::string& name) { std::printf ("\n== %s\n", name.c_str()); }

inline double db (double gain) { return 20.0 * std::log10 (gain + 1.0e-30); }
} // namespace testing
