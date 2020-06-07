#include "util.h"
#include "conn.h"

#define MAX_ERRMSG 128

void _prepare_step_finalize(conn *con, const char *sql, char *errmsg, int maxerrmsg) {
    int err = conn_prepare(con, sql, errmsg, maxerrmsg);
    ASSERT(!err, "prepare err %s", errmsg);
    bool more;
    err = conn_step(con, &more, errmsg, maxerrmsg);
    ASSERT(!err, "step err '%s'", errmsg);
    ASSERT(!more, "wrong more %d", more);
    err = conn_finalize(con, errmsg, maxerrmsg);
    ASSERT(!err, "finalize err %s", errmsg);
}

void test_conn(const char *dbfile) {
    INFO("TEST %s\n", __func__);
    DEBUG("dbfile=%s\n", dbfile);
    char errmsg[MAX_ERRMSG+1];
    conn *con = conn_new();
    int err = conn_open(con, dbfile, errmsg, MAX_ERRMSG);
    ASSERT(!err, "expected !err but was %d", err);
    // prepare schema
    err = conn_prepare(con, "DROP TABLE IF EXISTS users", errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    err = conn_step(con, NULL, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    err = conn_finalize(con, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    err = conn_prepare(con, "CREATE TABLE users (id INTEGER PRIMARY KEY NOT NULL, name VARCHAR, age INTEGER, rating REAL)", errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    err = conn_step(con, NULL, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    err = conn_finalize(con, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    // insert users without parameters
    err = conn_prepare(con, "INSERT INTO users (id,name,age,rating) VALUES(1, 'Alice', 31, 0.1)", errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    err = conn_step(con, NULL, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    int changes;
    conn_changes(con, &changes);
    ASSERT(changes==1, "wrong changes %d", changes);
    err = conn_finalize(con, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    // insert users with parameters
    err = conn_prepare(con, "INSERT INTO users (id,name,age,rating) VALUES(?,?,?,?)", errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    int icol = 1;
    err = conn_bind_int(con, icol++, 2, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    err = conn_bind_text(con, icol++, "Bob", errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    err = conn_bind_int(con, icol++, 32, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    err = conn_bind_double(con, icol++, 0.2, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    err = conn_step(con, NULL, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    conn_changes(con, &changes);
    ASSERT(changes==1, "wrong changes %d", changes);
    err = conn_reset(con, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    icol = 1;
    err = conn_bind_int(con, icol++, 3, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    err = conn_bind_null(con, icol++, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    err = conn_bind_null(con, icol++, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    err = conn_bind_null(con, icol++, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    err = conn_step(con, NULL, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    conn_changes(con, &changes);
    ASSERT(changes==1, "wrong changes %d", changes);
    err = conn_reset(con, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    err = conn_finalize(con, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    // select users
    err = conn_prepare(con, "SELECT id,name,age,rating FROM users ORDER BY id", errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    bool more;
    err = conn_step(con, &more, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    size_t nrows = 0;
    while(more) {
        nrows++;
        bool set;
        int id=0;
        const char *name=NULL;
        int age=0;
        double rating=0;
        icol=0;
        conn_column_int(con, icol++, &set, &id);
        conn_column_text(con, icol++, &set, &name);
        conn_column_int(con, icol++, &set, &age);
        conn_column_double(con, icol++, &set, &rating);
        // DEBUG("id=%d, name='%s', age=%d, rating=%g\n", id, name, age, rating);
        err = conn_step(con, &more, errmsg, MAX_ERRMSG);
        ASSERT(!err, "wrong err %d", err);
    }
    ASSERT(nrows == 3, "want 3 rows but have %d", (int)nrows);
    err = conn_finalize(con, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    // delete users
    err = conn_prepare(con, "DELETE FROM users WHERE name = 'does_not_exist'", errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    err = conn_step(con, NULL, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    conn_changes(con, &changes);
    ASSERT(changes==0, "wrong changes %d", changes);
    err = conn_finalize(con, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    err = conn_prepare(con, "DELETE FROM users WHERE id >= 0", errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    err = conn_step(con, NULL, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    conn_changes(con, &changes);
    ASSERT(changes==3, "wrong changes %d", changes);
    err = conn_finalize(con, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    // close db
    err = conn_close(con, errmsg, MAX_ERRMSG);
    ASSERT(!err, "wrong err %d", err);
    conn_free(con);
    INFO("TEST %s OK\n", __func__);
}

void bench_conn_users(const char *dbfile, int nusers, bool bindRating) {
    INFO("BENCH %s\n", __func__);
    DEBUG("dbfile=%s, nusers=%d, bindRating=%d\n", dbfile, nusers, bindRating);
    char errmsg[MAX_ERRMSG+1];
    conn *con = conn_new();
    conn_open(con, dbfile, errmsg, MAX_ERRMSG);
    double t1 = mono_time();
    // prepare schema
    _prepare_step_finalize(con, "DROP TABLE IF EXISTS users", errmsg, MAX_ERRMSG);
    _prepare_step_finalize(con, "CREATE TABLE users (id INTEGER PRIMARY KEY NOT NULL, name VARCHAR, age INTEGER, rating REAL)", errmsg, MAX_ERRMSG);
    // insert users
    _prepare_step_finalize(con, "BEGIN TRANSACTION", errmsg, MAX_ERRMSG);
    conn_prepare(con, "INSERT INTO users (id, name, age, rating) VALUES(?,?,?,?)", errmsg, MAX_ERRMSG);
    for (int i=0 ; i<nusers ; i++) {
        int id = i+1;
        char name[32];
        sprintf(name, "User_%d", id);
        int age =31+i;
        double rating = 0.11*(i+1);
        conn_bind_int(con, 1, id, errmsg, MAX_ERRMSG);
        conn_bind_text(con, 2, name, errmsg, MAX_ERRMSG);
        conn_bind_int(con, 3, age, errmsg, MAX_ERRMSG);
        if (bindRating) {
            conn_bind_double(con, 4, rating, errmsg, MAX_ERRMSG);
        } else {
            conn_bind_null(con, 4, errmsg, MAX_ERRMSG);
        }
        conn_step(con, NULL, errmsg, MAX_ERRMSG);
        conn_reset(con, errmsg, MAX_ERRMSG);
    }
    conn_finalize(con, errmsg, MAX_ERRMSG);
    _prepare_step_finalize(con, "COMMIT", errmsg, MAX_ERRMSG);
    // DEBUG("inserted %d users\n", nusers);
    double t2 = mono_time();
    // select users
    conn_prepare(con, "SELECT id,name,age,rating FROM users ORDER BY id", errmsg, MAX_ERRMSG);
    bool more;
    conn_step(con, &more, errmsg, MAX_ERRMSG);
    int nrows = 0;
    while(more) {
        nrows++;
        bool set;
        int id;
        const char *name;
        int age;
        double rating;
        conn_column_int   (con, 0, &set, &id);
        conn_column_text  (con, 1, &set, &name);
        conn_column_int   (con, 2, &set, &age);
        conn_column_double(con, 3, &set, &rating);
        conn_step(con, &more, errmsg, MAX_ERRMSG);
    }
    // DEBUG("found %d users\n", nrows);
    ASSERT(nrows == nusers, "want %d rows but have %d", nusers, nrows);
    conn_finalize(con, errmsg, MAX_ERRMSG);
    double t3 = mono_time();
    // close db
    conn_close(con, errmsg, MAX_ERRMSG);
    conn_free(con);
    DEBUG("insert took %f s\n", mono_diff_sec(t1, t2));
    DEBUG("query took %f s\n", mono_diff_sec(t2, t3));
    INFO("BENCH %s OK\n", __func__);
}

void bench_conn_complex(const char *dbfile, int nprofiles, int nusers, int nlocations) {
    INFO("BENCH %s\n", __func__);
    DEBUG("dbfile=%s, nprofiles, nusers, nlocations = %d, %d, %d\n", dbfile, nprofiles, nusers, nlocations);
    char errmsg[MAX_ERRMSG+1];
    conn *con = conn_new();
    conn_open(con, dbfile, errmsg, MAX_ERRMSG);
    // create schema
    _prepare_step_finalize(con, "PRAGMA foreign_keys=1", errmsg, MAX_ERRMSG);
    _prepare_step_finalize(con, "DROP TABLE IF EXISTS locations", errmsg, MAX_ERRMSG);
    _prepare_step_finalize(con, "DROP TABLE IF EXISTS users", errmsg, MAX_ERRMSG);
    _prepare_step_finalize(con, "DROP TABLE IF EXISTS profiles", errmsg, MAX_ERRMSG);
    _prepare_step_finalize(con, "CREATE TABLE profiles (id VARCHAR PRIMARY KEY NOT NULL, name VARCHAR NOT NULL, active bool NOT NULL)", errmsg, MAX_ERRMSG);
    _prepare_step_finalize(con, "CREATE INDEX idx_profiles_name ON profiles(name);", errmsg, MAX_ERRMSG);
    //_prepare_step_finalize(con, "CREATE INDEX idx_profiles_active ON profiles(active);", errmsg, MAX_ERRMSG);
    _prepare_step_finalize(con, "CREATE TABLE users (id VARCHAR PRIMARY KEY NOT NULL, profileId VARCHAR NOT NULL, name VARCHAR NOT NULL, active bool NOT NULL, FOREIGN KEY (profileId) REFERENCES profiles(id))", errmsg, MAX_ERRMSG);
    _prepare_step_finalize(con, "CREATE INDEX idx_users_profileId ON users(profileId);", errmsg, MAX_ERRMSG);
    _prepare_step_finalize(con, "CREATE INDEX idx_users_name ON users(name);", errmsg, MAX_ERRMSG);
    //_prepare_step_finalize(con, "CREATE INDEX idx_users_active ON users(active);", errmsg, MAX_ERRMSG);
    _prepare_step_finalize(con, "CREATE TABLE locations (id VARCHAR PRIMARY KEY NOT NULL, userId VARCHAR NOT NULL, name VARCHAR NOT NULL, active bool NOT NULL, FOREIGN KEY (userId) REFERENCES users(id))", errmsg, MAX_ERRMSG);
    _prepare_step_finalize(con, "CREATE INDEX idx_locations_userId ON locations(userId);", errmsg, MAX_ERRMSG);
    _prepare_step_finalize(con, "CREATE INDEX idx_locations_name ON locations(name);", errmsg, MAX_ERRMSG);
    //_prepare_step_finalize(con, "CREATE INDEX idx_locations_active ON locations(active);", errmsg, MAX_ERRMSG);
    // insert profiles
    double t1 = mono_time();
    _prepare_step_finalize(con, "BEGIN TRANSACTION", errmsg, MAX_ERRMSG);
    conn_prepare(con, "INSERT INTO profiles (id, name, active) VALUES(?, ?, ?)", errmsg, MAX_ERRMSG);
    for (int p=0 ; p<nprofiles ; p++) {
        char profile_id[32];
        sprintf(profile_id, "profile_%d", p);
        char name[32];
        sprintf(name, "ProfileC %d", p);
        int active = p%2;
        conn_bind_text(con, 1, profile_id, errmsg, MAX_ERRMSG);
        conn_bind_text(con, 2, name, errmsg, MAX_ERRMSG);
        conn_bind_int(con, 3, active, errmsg, MAX_ERRMSG);
        conn_step(con, NULL, errmsg, MAX_ERRMSG);
        conn_reset(con, errmsg, MAX_ERRMSG);
    }
    conn_finalize(con, errmsg, MAX_ERRMSG);
    _prepare_step_finalize(con, "COMMIT", errmsg, MAX_ERRMSG);
    // DEBUG("inserted %d profiles\n", nprofiles);
    // insert users
    _prepare_step_finalize(con, "BEGIN TRANSACTION", errmsg, MAX_ERRMSG);
    conn_prepare(con, "INSERT INTO users (id, profileId, name, active) VALUES(?, ?, ?, ?)", errmsg, MAX_ERRMSG);
    for (int p=0 ; p<nprofiles ; p++) {
        char profile_id[32];
        sprintf(profile_id, "profile_%d", p);
        for (int u=0 ; u<nusers ; u++) {
            char user_id[32];
            sprintf(user_id, "user_%d_%d", p, u);
            char name[32];
            sprintf(name, "User %d %d", p, u);
            int active = u%2;
            conn_bind_text(con, 1, user_id, errmsg, MAX_ERRMSG);
            conn_bind_text(con, 2, profile_id, errmsg, MAX_ERRMSG);
            conn_bind_text(con, 3, name, errmsg, MAX_ERRMSG);
            conn_bind_int(con, 4, active, errmsg, MAX_ERRMSG);
            conn_step(con, NULL, errmsg, MAX_ERRMSG);
            conn_reset(con, errmsg, MAX_ERRMSG);
        }
    }
    conn_finalize(con, errmsg, MAX_ERRMSG);
    _prepare_step_finalize(con, "COMMIT", errmsg, MAX_ERRMSG);
    // DEBUG("inserted %d users\n", nprofiles*nusers);
    // insert locations
    _prepare_step_finalize(con, "BEGIN TRANSACTION", errmsg, MAX_ERRMSG);
    conn_prepare(con, "INSERT INTO locations (id, userId, name, active) VALUES(?, ?, ?, ?)", errmsg, MAX_ERRMSG);
    for (int p=0 ; p<nprofiles ; p++) {
        for (int u=0 ; u<nusers ; u++) {
            char user_id[32];
            sprintf(user_id, "user_%d_%d", p, u);
            for (int l=0 ; l<nlocations ; l++) {
                char location_id[32];
                sprintf(location_id, "location_%d_%d_%d", p, u, l);
                char name[32];
                sprintf(name, "Location %d %d %d", p, u, l);
                int active = l%2;
                conn_bind_text(con, 1, location_id, errmsg, MAX_ERRMSG);
                conn_bind_text(con, 2, user_id, errmsg, MAX_ERRMSG);
                conn_bind_text(con, 3, name, errmsg, MAX_ERRMSG);
                conn_bind_int(con, 4, active, errmsg, MAX_ERRMSG);
                conn_step(con, NULL, errmsg, MAX_ERRMSG);
                conn_reset(con, errmsg, MAX_ERRMSG);
            }
        }
    }
    conn_finalize(con, errmsg, MAX_ERRMSG);
    _prepare_step_finalize(con, "COMMIT", errmsg, MAX_ERRMSG);
    DEBUG("inserted %d locations\n", nprofiles*nusers*nlocations);
    double t2 = mono_time();
    DEBUG("insert took %f s\n", mono_diff_sec(t1, t2));
    const char *sql = "SELECT locations.id, locations.userId, locations.name, locations.active, users.id, users.profileId, users.name, users.active, profiles.id, profiles.name, profiles.active \
    FROM locations \
    LEFT JOIN users ON users.id = locations.userId \
    LEFT JOIN profiles ON profiles.id = users.profileId \
    WHERE locations.active = ? OR locations.active = ? \
    ORDER BY locations.name, locations.id, users.name, users.id, profiles.name, profiles.id";
    int err = conn_prepare(con, sql, errmsg, MAX_ERRMSG);
    ASSERT(!err, "err %s", errmsg);
    err = conn_bind_int(con, 1, 0, errmsg, MAX_ERRMSG);
    ASSERT(!err, "err %s", errmsg);
    err = conn_bind_int(con, 2, 1, errmsg, MAX_ERRMSG);
    ASSERT(!err, "err %s", errmsg);
    bool more;
    err = conn_step(con, &more, errmsg, MAX_ERRMSG);
    ASSERT(!err, "err %s", errmsg);
    int nrows = 0;
    bool set;
    while(more) {
        nrows++;
        const char *location_id;
        const char *location_user_id;
        const char *location_name;
        int location_active;
        const char *user_id;
        const char *user_profile_id;
        const char *user_name;
        int user_active;
        const char *profile_id;
        const char *profile_name;
        int profile_active;
        conn_column_text(con,  0, &set, &location_id);
        ASSERT(strncmp(location_id, "loc", 3)==0, "wrong location_id '%s'", location_id);
        conn_column_text(con,  1, &set, &location_user_id);
        ASSERT(strncmp(location_user_id, "use", 3)==0, "wrong location_user_id '%s'", location_user_id);
        conn_column_text(con,  2, &set, &location_name);
        ASSERT(strncmp(location_name, "Loc", 3)==0, "wrong location_name '%s'", location_name);
        conn_column_int (con,  3, &set, &location_active);
        ASSERT(location_active==0 || location_active==1, "wrong location_active %d", location_active);
        conn_column_text(con,  4, &set, &user_id);
        ASSERT(strncmp(user_id, "use", 3)==0, "wrong user_id '%s'", user_id);
        conn_column_text(con,  5, &set, &user_profile_id);
        ASSERT(strncmp(user_profile_id, "pro", 3)==0, "wrong user_profile_id '%s'", user_profile_id);
        conn_column_text(con,  6, &set, &user_name);
        ASSERT(strncmp(user_name, "Use", 3)==0, "wrong user_name '%s'", user_name);
        conn_column_int (con,  7, &set, &user_active);
        ASSERT(user_active==0 || user_active==1, "wrong user_active %d", user_active);
        conn_column_text(con,  8, &set, &profile_id);
        ASSERT(strncmp(profile_id, "pro", 3)==0, "wrong profile_id '%s'", profile_id);
        conn_column_text(con,  9, &set, &profile_name);
        ASSERT(strncmp(profile_name, "Pro", 3)==0, "wrong profile_name '%s'", profile_name);
        conn_column_int (con, 10, &set, &profile_active);
        ASSERT(profile_active==0 || profile_active==1, "wrong profile_active %d", profile_active);
        err = conn_step(con, &more, errmsg, MAX_ERRMSG);
        ASSERT(!err, "err %s", errmsg);
    }
    int exprows = nprofiles * nusers * nlocations;
    ASSERT(nrows==exprows, "expected %d rows but have %d", exprows, nrows);
    err = conn_finalize(con, errmsg, MAX_ERRMSG);
    ASSERT(!err, "err %s", errmsg);
    double t3 = mono_time();
    DEBUG("query took %f s\n", mono_diff_sec(t2, t3));
    // close db
    conn_close(con, errmsg, MAX_ERRMSG);
    conn_free(con);
    INFO("BENCH %s OK\n", __func__);
}
