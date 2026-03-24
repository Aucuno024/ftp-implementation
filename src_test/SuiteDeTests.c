#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include "logs.h"
#include "CuTest.h"
#include "response.h"





int test_from_null()
{
    log_t *log = NULL;
    if(add(&log, GET, "test1"))
    {
        free_log(log);
        return 0;
    }
    if(add(&log, GET, "test"))
    {
        free_log(log);
        return 0;
    }
    if(!follow(log))
    {
        free_log(log);
        return 0;
    }
    free_log(log);
    return 1;
}

int test_follow()
{
    log_t *log = NULL;
    if(add(&log, GET, "test1"))
    {
        free_log(log);
        return 0;
    }
    if(add(&log, GET, "test")) 
    {
        free_log(log);
        return 0;
    }
    if(!follow(log))
    {
        free_log(log);
        return 0;
    }
    if(add(&log, GET,"tessst"))
    {
        free_log(log);
        return 0;
    }
    if(!follow(follow(log)))
    {
        free_log(log);
        return 0;
    }
    free_log(log);
    return 1;
}

int test_add_from_follow()
{
    log_t *log = NULL;
    if(add(&log, GET, "test1"))
    {
        free_log(log);
        return 0;
    }
    if(add(&log, GET, "test")) 
    {
        free_log(log);
        return 0;
    }
    if(!follow(log))
    {
        free_log(log);
        return 0;
    }
    if(add(&log, GET,"tessst"))
    {
        free_log(log);
        return 0;
    }
    log_t *fol = follow(log);
    if(!follow(fol))
    {
        free_log(log);
        return 0;
    }
    if(add(&fol, GET, "bah"))
    {
        free_log(log);
        return 0;
    }
    free_log(log);
    return 1;
}

void test1(CuTest *tc)
{
    CuAssertTrue(tc, test_from_null());
}
void test2(CuTest *tc)
{
    CuAssertTrue(tc, test_follow());
}
void test3(CuTest *tc)
{
    CuAssertTrue(tc, test_add_from_follow());
}
CuSuite *MaTestSuite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test1);
    SUITE_ADD_TEST(suite, test2);
    SUITE_ADD_TEST(suite, test2);

    // Ajouter les tests unitaires pour com
    return suite;
}



