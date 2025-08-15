#ifndef APP_H
#define APP_H

/* An App reads requests, processes them, and writes responses. */
typedef struct app_s App;
App *newApp(Db *db, Reader *r, Writer *w);
void App_free(App *this);
BOOL App_step(App *this); // TRUE if next, FALSE if not (must exit then)

//
// Test
//

void testApp();

#endif  // APP_H
