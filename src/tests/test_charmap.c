#include <assert.h>
#include "adv_string.h"

#define __THIS_FILE__       "test/test_charmap.c"


int main(void)
{
    charmap_t cm;
    //(void)fprintf(stderr, "%s\n", SRC2STR(PROJECT_ROOT));

    charmap_clean(&cm);
    assert(! charmap_isset(&cm, '\x20'));
    charmap_set(&cm, '\x20');
    charmap_set(&cm, '\t');
    charmap_set(&cm, '\r');
    charmap_set(&cm, '\n');
    assert(4 == charmap_count(&cm));
    assert(charmap_isset(&cm, '\n'));
    assert(charmap_isset(&cm, '\r'));
    assert(charmap_isset(&cm, '\t'));
    assert(charmap_isset(&cm, '\x20'));
    assert(4 == charmap_count(&cm));

    return 0;
}
