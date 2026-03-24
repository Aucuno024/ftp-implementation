#include <stdio.h>
#include "CuTest.h"

CuSuite* MaTestSuite(void);

int main(void)
{
    int fail_count;
    CuString *output = CuStringNew();
    CuSuite* suite = MaTestSuite();

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);

    fail_count = suite->failCount;

    CuStringDelete(output);
    CuSuiteDelete(suite);
    return (fail_count == 0) ? 0 : 1;
}
