/**
 * @file control/params_validation.cpp
 * @brief Validation of ControlParams conversion functions
 *
 * This file runs runtime tests to ensure conversion functions work correctly.
 */

#include "params_test.hpp"
#include <iostream>

int main()
{
    bool all_passed = true;

    std::cout << "Running ControlParams conversion tests...\n\n";

    // Test 1: FromCFPARA
    if (control::test::test_from_cfpara())
    {
        std::cout << "✓ FromCFPARA test passed\n";
    }
    else
    {
        std::cout << "✗ FromCFPARA test FAILED\n";
        all_passed = false;
    }

    // Test 2: ToCFPARA
    if (control::test::test_to_cfpara())
    {
        std::cout << "✓ ToCFPARA test passed\n";
    }
    else
    {
        std::cout << "✗ ToCFPARA test FAILED\n";
        all_passed = false;
    }

    // Test 3: Round-trip conversion
    if (control::test::test_roundtrip())
    {
        std::cout << "✓ Round-trip conversion test passed\n";
    }
    else
    {
        std::cout << "✗ Round-trip conversion test FAILED\n";
        all_passed = false;
    }

    // Test 4: Default values
    if (control::test::test_default_values())
    {
        std::cout << "✓ Default values test passed\n";
    }
    else
    {
        std::cout << "✗ Default values test FAILED\n";
        all_passed = false;
    }

    std::cout << "\n";

    if (all_passed)
    {
        std::cout << "All tests passed successfully!\n";
        return 0;
    }
    else
    {
        std::cout << "Some tests failed.\n";
        return 1;
    }
}
