#ifndef _CONN_TEST_H
#define _CONN_TEST_H

void test_conn();
void bench_conn_users(int nusers, bool bindRating);
void bench_conn_complex(int nprofiles, int nusers, int nlocations);

#endif // _CONN_TEST_H
