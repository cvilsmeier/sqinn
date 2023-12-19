#include "util.h"
#include "conn.h"

#define MAX_MESSAGE 128

void _prepare_step_finalize(conn *con, const char *sql, char *errmsg, int maxerrmsg) {
    int err = conn_prepare(con, sql, errmsg, maxerrmsg);
    ASSERT(!err, "prepare err %s", errmsg);
    bool more;
    err = conn_step(con, &more, errmsg, maxerrmsg);
    ASSERT(!err, "step err '%s'", errmsg);
    ASSERT(!more, "wrong more %d", more);
    conn_finalize(con);
}

void test_conn(const char *dbfile) {
    INFO("TEST %s\n", __func__);
    DEBUG("  dbfile=%s\n", dbfile);
    char errmsg[MAX_MESSAGE+1];
    conn *con = conn_new();
    int err = conn_open(con, dbfile, errmsg, MAX_MESSAGE);
    ASSERT(!err, "expected !err but was %d", err);
    // prepare schema
    err = conn_prepare(con, "DROP TABLE IF EXISTS users", errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    err = conn_step(con, NULL, errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    conn_finalize(con);
    err = conn_prepare(con, "CREATE TABLE users (id INTEGER PRIMARY KEY NOT NULL, name VARCHAR, age INTEGER, rating REAL)", errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    err = conn_step(con, NULL, errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    conn_finalize(con);
    // insert users without parameters
    err = conn_prepare(con, "INSERT INTO users (id,name,age,rating) VALUES(1, 'Alice', 31, 0.1)", errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    err = conn_step(con, NULL, errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    int changes;
    conn_changes(con, &changes);
    ASSERT(changes==1, "wrong changes %d", changes);
    conn_finalize(con);
    // insert users with parameters
    err = conn_prepare(con, "INSERT INTO users (id,name,age,rating) VALUES(?,?,?,?)", errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    int icol = 1;
    err = conn_bind_int(con, icol++, 2, errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    err = conn_bind_text(con, icol++, "Bob", errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    err = conn_bind_int(con, icol++, 32, errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    err = conn_bind_double(con, icol++, 0.2, errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    err = conn_step(con, NULL, errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    conn_changes(con, &changes);
    ASSERT(changes==1, "wrong changes %d", changes);
    err = conn_reset(con, errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    icol = 1;
    err = conn_bind_int(con, icol++, 3, errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    err = conn_bind_null(con, icol++, errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    err = conn_bind_null(con, icol++, errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    err = conn_bind_null(con, icol++, errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    err = conn_step(con, NULL, errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    conn_changes(con, &changes);
    ASSERT(changes==1, "wrong changes %d", changes);
    err = conn_reset(con, errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    conn_finalize(con);
    // select users
    err = conn_prepare(con, "SELECT id,name,age,rating FROM users ORDER BY id", errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    bool more;
    err = conn_step(con, &more, errmsg, MAX_MESSAGE);
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
        err = conn_step(con, &more, errmsg, MAX_MESSAGE);
        ASSERT(!err, "wrong err %d", err);
    }
    ASSERT(nrows == 3, "want 3 rows but have %d", (int)nrows);
    conn_finalize(con);
    // delete users
    err = conn_prepare(con, "DELETE FROM users WHERE name = 'does_not_exist'", errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    err = conn_step(con, NULL, errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    conn_changes(con, &changes);
    ASSERT(changes==0, "wrong changes %d", changes);
    conn_finalize(con);
    err = conn_prepare(con, "DELETE FROM users WHERE id >= 0", errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    err = conn_step(con, NULL, errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    conn_changes(con, &changes);
    ASSERT(changes==3, "wrong changes %d", changes);
    conn_finalize(con);
    // close db
    err = conn_close(con, errmsg, MAX_MESSAGE);
    ASSERT(!err, "wrong err %d", err);
    conn_free(con);
    INFO("TEST %s OK\n", __func__);
}

void bench_conn_users(const char *dbfile, int nusers) {
    INFO("BENCH %s\n", __func__);
    DEBUG("  dbfile=%s, nusers=%d\n", dbfile, nusers);
    char errmsg[MAX_MESSAGE+1];
    conn *con = conn_new();
    conn_open(con, dbfile, errmsg, MAX_MESSAGE);
    // prepare schema
    _prepare_step_finalize(con, "DROP TABLE IF EXISTS users", errmsg, MAX_MESSAGE);
    _prepare_step_finalize(con, "CREATE TABLE users (id INTEGER PRIMARY KEY NOT NULL, name VARCHAR, age INTEGER, rating REAL)", errmsg, MAX_MESSAGE);
    double tstart = mono_time();
    // insert
    _prepare_step_finalize(con, "BEGIN", errmsg, MAX_MESSAGE);
    conn_prepare(con, "INSERT INTO users (id, name, age, rating) VALUES(?,?,?,?)", errmsg, MAX_MESSAGE);
    for (int i=0 ; i<nusers ; i++) {
        int id = i+1;
        char name[32];
        sprintf(name, "User_%d", id);
        int age =31+i;
        double rating = 0.11*(i+1);
        conn_bind_int(con, 1, id, errmsg, MAX_MESSAGE);
        conn_bind_text(con, 2, name, errmsg, MAX_MESSAGE);
        conn_bind_int(con, 3, age, errmsg, MAX_MESSAGE);
        conn_bind_double(con, 4, rating, errmsg, MAX_MESSAGE);
        conn_step(con, NULL, errmsg, MAX_MESSAGE);
        conn_reset(con, errmsg, MAX_MESSAGE);
    }
    conn_finalize(con);
    _prepare_step_finalize(con, "COMMIT", errmsg, MAX_MESSAGE);
    DEBUG("  insert took %f s\n", mono_since(tstart));
    // select users
    tstart = mono_time();
    conn_prepare(con, "SELECT id,name,age,rating FROM users ORDER BY id", errmsg, MAX_MESSAGE);
    bool more;
    conn_step(con, &more, errmsg, MAX_MESSAGE);
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
        conn_step(con, &more, errmsg, MAX_MESSAGE);
    }
    ASSERT(nrows == nusers, "want %d rows but have %d", nusers, nrows);
    conn_finalize(con);
    DEBUG("  query took %f s\n", mono_since(tstart));
    // close db
    conn_close(con, errmsg, MAX_MESSAGE);
    conn_free(con);
    INFO("BENCH %s OK\n", __func__);
}

void bench_conn_complex(const char *dbfile, int nprofiles, int ndevices, int nlocations) {
    INFO("BENCH %s\n", __func__);
    DEBUG("  dbfile=%s, nprofiles, ndevices, nlocations = %d, %d, %d\n", dbfile, nprofiles, ndevices, nlocations);
    char errmsg[MAX_MESSAGE+1];
    conn *con = conn_new();
    conn_open(con, dbfile, errmsg, MAX_MESSAGE);
    // create schema
    _prepare_step_finalize(con, "PRAGMA foreign_keys=1", errmsg, MAX_MESSAGE);
    _prepare_step_finalize(con, "DROP TABLE IF EXISTS locations", errmsg, MAX_MESSAGE);
    _prepare_step_finalize(con, "DROP TABLE IF EXISTS devices", errmsg, MAX_MESSAGE);
    _prepare_step_finalize(con, "DROP TABLE IF EXISTS profiles", errmsg, MAX_MESSAGE);
    _prepare_step_finalize(con, "CREATE TABLE profiles (id VARCHAR PRIMARY KEY NOT NULL, name VARCHAR NOT NULL, active bool NOT NULL)", errmsg, MAX_MESSAGE);
    _prepare_step_finalize(con, "CREATE INDEX idx_profiles_name ON profiles(name);", errmsg, MAX_MESSAGE);
    _prepare_step_finalize(con, "CREATE INDEX idx_profiles_active ON profiles(active);", errmsg, MAX_MESSAGE);
    _prepare_step_finalize(con, "CREATE TABLE devices (id VARCHAR PRIMARY KEY NOT NULL, profileId VARCHAR NOT NULL, name VARCHAR NOT NULL, active bool NOT NULL, FOREIGN KEY (profileId) REFERENCES profiles(id))", errmsg, MAX_MESSAGE);
    _prepare_step_finalize(con, "CREATE INDEX idx_devices_profileId ON devices(profileId);", errmsg, MAX_MESSAGE);
    _prepare_step_finalize(con, "CREATE INDEX idx_devices_name ON devices(name);", errmsg, MAX_MESSAGE);
    _prepare_step_finalize(con, "CREATE INDEX idx_devices_active ON devices(active);", errmsg, MAX_MESSAGE);
    _prepare_step_finalize(con, "CREATE TABLE locations (id VARCHAR PRIMARY KEY NOT NULL, deviceId VARCHAR NOT NULL, name VARCHAR NOT NULL, active bool NOT NULL, FOREIGN KEY (deviceId) REFERENCES devices(id))", errmsg, MAX_MESSAGE);
    _prepare_step_finalize(con, "CREATE INDEX idx_locations_deviceId ON locations(deviceId);", errmsg, MAX_MESSAGE);
    _prepare_step_finalize(con, "CREATE INDEX idx_locations_name ON locations(name);", errmsg, MAX_MESSAGE);
    _prepare_step_finalize(con, "CREATE INDEX idx_locations_active ON locations(active);", errmsg, MAX_MESSAGE);
    // insert profiles
    double tstart = mono_time();
    _prepare_step_finalize(con, "BEGIN", errmsg, MAX_MESSAGE);
    conn_prepare(con, "INSERT INTO profiles (id, name, active) VALUES(?, ?, ?)", errmsg, MAX_MESSAGE);
    for (int p=0 ; p<nprofiles ; p++) {
        char profile_id[32];
        sprintf(profile_id, "profile_%d", p);
        char name[32];
        sprintf(name, "Profile %d", p);
        int active = p%2;
        conn_bind_text(con, 1, profile_id, errmsg, MAX_MESSAGE);
        conn_bind_text(con, 2, name, errmsg, MAX_MESSAGE);
        conn_bind_int(con, 3, active, errmsg, MAX_MESSAGE);
        conn_step(con, NULL, errmsg, MAX_MESSAGE);
        conn_reset(con, errmsg, MAX_MESSAGE);
    }
    conn_finalize(con);
    _prepare_step_finalize(con, "COMMIT", errmsg, MAX_MESSAGE);
    // insert users
    _prepare_step_finalize(con, "BEGIN", errmsg, MAX_MESSAGE);
    conn_prepare(con, "INSERT INTO devices (id, profileId, name, active) VALUES(?, ?, ?, ?)", errmsg, MAX_MESSAGE);
    for (int p=0 ; p<nprofiles ; p++) {
        char profile_id[32];
        sprintf(profile_id, "profile_%d", p);
        for (int d=0 ; d<ndevices ; d++) {
            char device_id[32];
            sprintf(device_id, "device_%d_%d", p, d);
            char name[32];
            sprintf(name, "Device %d %d", p, d);
            int active = d%2;
            conn_bind_text(con, 1, device_id, errmsg, MAX_MESSAGE);
            conn_bind_text(con, 2, profile_id, errmsg, MAX_MESSAGE);
            conn_bind_text(con, 3, name, errmsg, MAX_MESSAGE);
            conn_bind_int(con, 4, active, errmsg, MAX_MESSAGE);
            conn_step(con, NULL, errmsg, MAX_MESSAGE);
            conn_reset(con, errmsg, MAX_MESSAGE);
        }
    }
    conn_finalize(con);
    _prepare_step_finalize(con, "COMMIT", errmsg, MAX_MESSAGE);
    // insert locations
    _prepare_step_finalize(con, "BEGIN", errmsg, MAX_MESSAGE);
    conn_prepare(con, "INSERT INTO locations (id, deviceId, name, active) VALUES(?, ?, ?, ?)", errmsg, MAX_MESSAGE);
    for (int p=0 ; p<nprofiles ; p++) {
        for (int d=0 ; d<ndevices ; d++) {
            char device_id[32];
            sprintf(device_id, "device_%d_%d", p, d);
            for (int l=0 ; l<nlocations ; l++) {
                char location_id[64];
                sprintf(location_id, "location_%d_%d_%d", p, d, l);
                char name[64];
                sprintf(name, "Location %d %d %d", p, d, l);
                int active = l%2;
                conn_bind_text(con, 1, location_id, errmsg, MAX_MESSAGE);
                conn_bind_text(con, 2, device_id, errmsg, MAX_MESSAGE);
                conn_bind_text(con, 3, name, errmsg, MAX_MESSAGE);
                conn_bind_int(con, 4, active, errmsg, MAX_MESSAGE);
                conn_step(con, NULL, errmsg, MAX_MESSAGE);
                conn_reset(con, errmsg, MAX_MESSAGE);
            }
        }
    }
    conn_finalize(con);
    _prepare_step_finalize(con, "COMMIT", errmsg, MAX_MESSAGE);
    DEBUG("  insert took %f s\n", mono_since(tstart));
    // query
    tstart = mono_time();
    const char *sql = "SELECT \
      locations.id, locations.deviceId, locations.name, locations.active, \
      devices.id, devices.profileId, devices.name, devices.active, \
      profiles.id, profiles.name, profiles.active \
    FROM locations \
    LEFT JOIN devices ON devices.id = locations.deviceId \
    LEFT JOIN profiles ON profiles.id = devices.profileId \
    WHERE locations.active = ? OR locations.active = ? \
    ORDER BY locations.name, locations.id, devices.name, devices.id, profiles.name, profiles.id";
    int err = conn_prepare(con, sql, errmsg, MAX_MESSAGE);
    ASSERT(!err, "err %s", errmsg);
    err = conn_bind_int(con, 1, 0, errmsg, MAX_MESSAGE);
    ASSERT(!err, "err %s", errmsg);
    err = conn_bind_int(con, 2, 1, errmsg, MAX_MESSAGE);
    ASSERT(!err, "err %s", errmsg);
    bool more;
    err = conn_step(con, &more, errmsg, MAX_MESSAGE);
    ASSERT(!err, "err %s", errmsg);
    int nrows = 0;
    bool set;
    while(more) {
        nrows++;
        const char *location_id;
        const char *location_device_id;
        const char *location_name;
        int location_active;
        const char *device_id;
        const char *device_profile_id;
        const char *device_name;
        int device_active;
        const char *profile_id;
        const char *profile_name;
        int profile_active;
        conn_column_text(con,  0, &set, &location_id);
        ASSERT(strncmp(location_id, "loc", 3)==0, "wrong location_id '%s'", location_id);
        conn_column_text(con,  1, &set, &location_device_id);
        ASSERT(strncmp(location_device_id, "dev", 3)==0, "wrong location_device_id '%s'", location_device_id);
        conn_column_text(con,  2, &set, &location_name);
        ASSERT(strncmp(location_name, "Loc", 3)==0, "wrong location_name '%s'", location_name);
        conn_column_int (con,  3, &set, &location_active);
        ASSERT(location_active==0 || location_active==1, "wrong location_active %d", location_active);
        conn_column_text(con,  4, &set, &device_id);
        ASSERT(strncmp(device_id, "dev", 3)==0, "wrong device_id '%s'", device_id);
        conn_column_text(con,  5, &set, &device_profile_id);
        ASSERT(strncmp(device_profile_id, "pro", 3)==0, "wrong device_profile_id '%s'", device_profile_id);
        conn_column_text(con,  6, &set, &device_name);
        ASSERT(strncmp(device_name, "Dev", 3)==0, "wrong device_name '%s'", device_name);
        conn_column_int (con,  7, &set, &device_active);
        ASSERT(device_active==0 || device_active==1, "wrong device_active %d", device_active);
        conn_column_text(con,  8, &set, &profile_id);
        ASSERT(strncmp(profile_id, "pro", 3)==0, "wrong profile_id '%s'", profile_id);
        conn_column_text(con,  9, &set, &profile_name);
        ASSERT(strncmp(profile_name, "Pro", 3)==0, "wrong profile_name '%s'", profile_name);
        conn_column_int (con, 10, &set, &profile_active);
        ASSERT(profile_active==0 || profile_active==1, "wrong profile_active %d", profile_active);
        err = conn_step(con, &more, errmsg, MAX_MESSAGE);
        ASSERT(!err, "err %s", errmsg);
    }
    int exprows = nprofiles * ndevices * nlocations;
    ASSERT(nrows==exprows, "expected %d rows but have %d", exprows, nrows);
    conn_finalize(con);
    DEBUG("  query took %f s\n", mono_since(tstart));
    // close db
    conn_close(con, errmsg, MAX_MESSAGE);
    conn_free(con);
    INFO("BENCH %s OK\n", __func__);
}
