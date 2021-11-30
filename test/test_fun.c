#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "CUnit/CUnit.h"
#include "CUnit/Automated.h"
#include "client.h"

/*---- test cases ------------------*/
void test_client_exit()
{
        CU_ASSERT_STRING_EQUAL(client_exit("GET http://www.cs.tufts.edu/comp/112/index.html HTTP/1.1\r\n\r\n",
                              "localhost", 9021, "before sending request"),"client exited without killing the proxy");
        CU_ASSERT_STRING_EQUAL(client_exit("GET http://www.cs.tufts.edu/comp/112/index.html HTTP/1.1\r\n\r\n", 
                              "localhost", 9021, "after sending request"),"client exited without killing the proxy");
        CU_ASSERT_STRING_EQUAL(client_exit("GET http://www.cs.tufts.edu/comp/112/index.html HTTP/1.1\r\n\r\n", 
                              "localhost", 90, "after receiving message"),"failed to make first connection to proxy");
        CU_ASSERT_STRING_EQUAL(client_exit("GET http://www.cs.tufts.edu/comp/112/index.html HTTP/1.1\r\n\r\n", 
                              "localhost", 90, "after receiving message"),"failed to make first connection to proxy");
        CU_ASSERT_STRING_EQUAL(client_exit("G http://www.cs.tufts HTTP/1.1", 
                              "localhost", 9021, "after receiving message"),"client exited without killing the proxy");
        CU_ASSERT_STRING_EQUAL(client_exit("G htHTT", 
                              "localhost", 9021, "after receiving message"),"client exited without killing the proxy");
}

void test_multiple_connections()
{
        CU_ASSERT_STRING_EQUAL(multiple_clients_make_connections_to_proxy(6, "localhost", 9021, "make connections at the same time"), "All clients successfully connect to proxy!");
        CU_ASSERT_STRING_EQUAL(multiple_clients_make_connections_to_proxy(4, "localhost", 9021, "make connections one by one"), "All clients successfully connect to proxy!");
        CU_ASSERT_STRING_EQUAL(multiple_clients_make_connections_to_proxy(5, "localhost", 90, "make connections one by one"),"At least one client failed to connect to proxy");
        CU_ASSERT_STRING_EQUAL(multiple_clients_make_connections_to_proxy(3, "loca", 90, "make connections at the same time"),"At least one client failed to connect to proxy");
}

void test_multiple_GET_requests(){
        char*requests[3]={"GET http://www.cs.tufts.edu/comp/112/index.html HTTP/1.1\r\n\r\nHost: www.cs.tufts.edu\r\n\r\n",
                          "GET http://www.cs.cmu.edu/~prs/bio.html HTTP/1.1\r\n\r\nHost: www.cs.cmu.edu\r\n\r\n",
                          "GET http://www.cs.cmu.edu/~dga/dga-headshot.jpg HTTP/1.1\r\n\r\nHost: www.cs.cmu.edu\r\n\r\n"};
        CU_ASSERT_STRING_EQUAL(multiple_clients_send_requests_to_server(3, requests,"localhost", 9021, "one by one"),
                               "All requests get correct respond from server");
        CU_ASSERT_STRING_EQUAL(multiple_clients_send_requests_to_server(3, requests,"localhost", 9021, "concurrent"),
                               "All requests get correct respond from server");
}

void test_multiple_CONNECT_requests(){
        char*requests[4]={"CONNECT www.google.com:443 HTTP/1.1\r\n\r\nHost: www.google.com:443\r\n\r\n",
                          "CONNECT en.wikipedia.org:443/wiki/Google HTTP/1.1\r\n\r\nHost: en.wikipedia.org:443\r\n\r\n",
                          "CONNECT www.youtube.com:443 HTTP/1.1\r\n\r\nHost: www.youtube.com:443\r\n\r\n",
                          "CONNECT www.facebook.com:443 HTTP/1.1\r\n\r\nHost: www.facebook.com:443\r\n\r\n"};
        CU_ASSERT_STRING_EQUAL(multiple_clients_send_requests_to_server(4, requests,"localhost", 9021, "one by one"),
                               "All requests get correct respond from server");
        CU_ASSERT_STRING_EQUAL(multiple_clients_send_requests_to_server(4, requests,"localhost", 9021, "concurrent"),
                               "All requests get correct respond from server");
}

void test_GET_CONNECT_requests_combination(){
        char*requests[4]={"GET http://www.cs.tufts.edu/comp/112/index.html HTTP/1.1\r\n\r\nHost: www.cs.tufts.edu\r\n\r\n",
                          "CONNECT www.facebook.com:443 HTTP/1.1\r\n\r\nHost: www.facebook.com:443\r\n\r\n",
                          "GET http://www.cs.cmu.edu/~dga/dga-headshot.jpg HTTP/1.1\r\n\r\nHost: www.cs.cmu.edu\r\n\r\n",
                          "CONNECT en.wikipedia.org:443/wiki/Google HTTP/1.1\r\n\r\nHost: en.wikipedia.org:443\r\n\r\n"};
        CU_ASSERT_STRING_EQUAL(multiple_clients_send_requests_to_server(4, requests,"localhost", 9021, "one by one"),
                               "All requests get correct respond from server");
        CU_ASSERT_STRING_EQUAL(multiple_clients_send_requests_to_server(4, requests,"localhost", 9021, "concurrent"),
                               "All requests get correct respond from server");
}

void test_bad_requests(){
        char*requests[4]={"GrHost: ww",
                          "CONNECT .com:443 HTTP/1.1\r\n\r\nHost: www.facebook.com:443",
                          "GET http:// HTTP/1.1\r\n\r\nHost: www.cs.\r\n\r\n",
                          "CHTTP/1.1Host: en.wikipedia.org:443\r\n"
                          };
        CU_ASSERT_STRING_EQUAL(multiple_clients_send_requests_to_server(4, requests,"localhost", 9021, "one by one"),
                               "Proxy closed the connection because error(s) occur when forwarding");
        CU_ASSERT_STRING_EQUAL(multiple_clients_send_requests_to_server(4, requests,"localhost", 9021, "concurrent"),
                               "Proxy closed the connection because error(s) occur when forwarding");
}

CU_TestInfo testcases1[] = {
        {"Testing client's sudden exit:", test_client_exit},
        CU_TEST_INFO_NULL
};

CU_TestInfo testcases2[] = {
        {"Testing multiple connections:", test_multiple_connections},
        CU_TEST_INFO_NULL
};
CU_TestInfo testcases3[] = {
        {"Testing multiple GET requests:", test_multiple_GET_requests},
        CU_TEST_INFO_NULL
};
CU_TestInfo testcases4[] = {
        {"Testing multiple CONNECT requests:", test_multiple_CONNECT_requests},
        CU_TEST_INFO_NULL
};
CU_TestInfo testcases5[] = {
        {"Testing multiple bad requests:", test_bad_requests},
        CU_TEST_INFO_NULL
};
CU_TestInfo testcases6[] = {
        {"Testing multiple GET and CONNECT requests combinations:", test_GET_CONNECT_requests_combination},
        CU_TEST_INFO_NULL
};

/**//*---- test suites ------------------*/
int suite_success_init(void) { return 0; }
int suite_success_clean(void) { return 0; }

CU_SuiteInfo suites[] = {
        {"Testing client's sudden exit (before/after sending request, after receiving respond)", suite_success_init, suite_success_clean, NULL, NULL, testcases1},
        {"Testing multiple connections: (one by one, concurrent)",suite_success_init, suite_success_clean, NULL, NULL, testcases2},
        {"Testing multiple GET requests: (one by one, concurrent)",suite_success_init, suite_success_clean, NULL, NULL, testcases3},
        {"Testing multiple CONNECT requests: (one by one, concurrent)",suite_success_init, suite_success_clean, NULL, NULL, testcases4},
        {"Testing multiple Bad requests: (one by one, concurrent)", suite_success_init, suite_success_clean, NULL, NULL, testcases5},
        {"Testing multiple GET and CONNECT requests combinations: (one by one, concurrent)",suite_success_init, suite_success_clean, NULL, NULL, testcases6},
        CU_SUITE_INFO_NULL
};
/**//*---- setting enviroment -----------*/
void AddTests(void)
{
        assert(NULL != CU_get_registry());
        assert(!CU_is_test_running());
        /**//* shortcut regitry */

        if(CUE_SUCCESS != CU_register_suites(suites)){
                fprintf(stderr, "Register suites failed - %s ", CU_get_error_msg());
                exit(1);
        }
}