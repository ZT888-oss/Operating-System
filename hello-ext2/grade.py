#!/usr/bin/env python3

from test_lab6 import Lab6TestCase

import os
import pathlib
import unittest

def get_method_name(test_case):
    return test_case.id().rsplit('.', 1)[-1]

def get_weight(method_name):
    if method_name.startswith('test_fsck'):
        return 4
    elif method_name == 'test_hello':
        return 4
    elif method_name == 'test_hello_world':
        return 4
    return 1

if __name__ == '__main__':
    base_dir = pathlib.Path(__file__).resolve().parent
    os.chdir(base_dir)

    loader = unittest.TestLoader()
    suite = loader.loadTestsFromTestCase(Lab6TestCase)
    with open('/dev/null', 'w') as f:
        runner = unittest.TextTestRunner(stream=f)
        result = runner.run(suite)

    tests = []
    for test_case, string in result.failures:
        method_name = get_method_name(test_case)
        weight = get_weight(method_name)
        test = {'name': method_name, 'weight': weight, 'result': 'FAIL'}
        tests.append(test)

    for test_case, string in result.errors:
        method_name = get_method_name(test_case)
        weight = get_weight(method_name)
        test = {'name': method_name, 'weight': weight, 'result': 'ERROR'}
        tests.append(test)

    max_grade = 85
    assert(4 * 8 + result.testsRun == max_grade)
    assert(len(result.skipped) == 0)
    assert(len(result.expectedFailures) == 0)
    assert(len(result.unexpectedSuccesses) == 0)

    grade = max_grade
    for test in tests:
        grade -= test["weight"]

    if grade != 0:
        grade += 15
        tests.append({
            'name': 'successful_tests',
            'weight': grade,
            'result': 'OK',
        })

    print(grade)
