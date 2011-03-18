#include "RectangleUtil.hh"

#include <cstdio>

struct Rect {

    int x() const { return m_x; }
    int y() const { return m_y; }
    int width() const { return m_width; }
    int height() const { return m_height; }

    int m_x, m_y, m_width, m_height;
};




int test_insideBorder() {

    printf("testing RectangleUtil::insideBorder()\n");

    struct _t {
        struct Rect rect;
        int x;
        int y;
        int bw;
        int truth;
    };

    _t tests[] = {
        { { 0, 0, 10, 10 },  0,  0, 2, false }, // on the (outer) edge
        { { 0, 0, 10, 10 },  1,  1, 2, false }, // on the (inner) edge
        { { 0, 0, 10, 10 },  5,  5, 2, true },  // really inside
        { { 0, 0, 10, 10 }, -5,  0, 2, false }, // somewhere outside
        { { 0, 0, 10, 10 }, 20, 20, 2, false }  // outside for sure
    };

    int i;
    for (i = 0; i < sizeof(tests)/sizeof(_t); ++i) {
        const _t& t = tests[i];
        int result = RectangleUtil::insideBorder<Rect>(t.rect, t.x, t.y, t.bw);

        printf("  %d: is (%02d|%02d) inside [%d %d]-[%d %d] with border %d: %s, %s\n",
                i,
                t.x, t.y,
                t.rect.x(), t.rect.y(),
                t.rect.x() + (int)t.rect.width(),
                t.rect.y() + (int)t.rect.height(),
                t.bw,
                result ? "yes" : "no", result == t.truth ? "ok" : "failed");
    }

    printf("done.\n");

    return 0;
}




int main(int argc, char **argv) {

    test_insideBorder();

    return 0;
}
