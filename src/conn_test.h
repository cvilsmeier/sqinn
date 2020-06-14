#ifndef _CONN_TEST_H
#define _CONN_TEST_H

void test_conn(const char *dbfile);
void bench_conn_users(const char *dbfile, int nusers);
void bench_conn_complex(const char *dbfile, int nprofiles, int nusers, int nlocations);

#endif // _CONN_TEST_H
