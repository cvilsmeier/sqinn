#include "util.h"
#include "mem.h"
#include "conn.h"
#include "handler.h"
#include "loop.h"

void loop() {
    handler *hd = handler_new();
    dbuf *req = dbuf_new();
    dbuf *resp = dbuf_new();
    bool quit = FALSE;
    while (!quit) {
        dbuf_reset(req);
        dbuf_reset(resp);
        // read req
        {
            byte sz_buf[4];
            // DBG("waiting for sz bytes\n");
            size_t c = fread(sz_buf, 1, 4, stdin);
            if (c != 4) {
                INFO("EOF: expected 4 bytes, got %I64u\n", c);
                return;
            }
            int sz = decode_int32(sz_buf);
            // DBG("received 4 sz bytes, sz = %I64u\n", sz);
            if (sz) {
                byte *data_buf = MEM_MALLOC(sz * sizeof(byte));
                // DBG("waiting for %I64u data bytes\n", sz);
                c = fread(data_buf, 1, sz, stdin);
                if (c != sz) {
                    MEM_FREE(data_buf);
                    INFO("EOF: expected %d bytes, got %I64u\n", sz, c);
                    return;
                }
                dbuf_write_bytes(req, data_buf, sz);
                // TODO we could optimize here: since we already have the raw data in memory,
                // we should not malloc it by copying over to dbuf.
                MEM_FREE(data_buf);
            }
        }
        if (req->sz > 0) {
            handler_handle(hd, req, resp);
            // write resp
            byte sz_buf[4];
            encode_int32(resp->sz, sz_buf);
            fwrite(sz_buf, 1, 4, stdout);
            fwrite(resp->buf, 1, resp->sz, stdout);
            fflush(stdout);
        } else {
            quit = TRUE;
        }
    }
    dbuf_free(req);
    dbuf_free(resp);
    handler_free(hd);
}
