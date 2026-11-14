#include "FbTk/RectangleUtil.hh"

#include <cstdio>

struct Rect {

    int x() const { return m_x; }
    int y() const { return m_y; }
    int width() const { return m_width; }
    int height() const { return m_height; }

    int m_x, m_y, m_width, m_height;
};




int test_insideBorder() {

    printf("testing FbTk::RectangleUtil::insideBorder()\n");

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

    for (unsigned int i = 0; i < sizeof(tests)/sizeof(_t); ++i) {
        const _t& t = tests[i];
        int result = FbTk::RectangleUtil::insideBorder<Rect>(t.rect, t.x, t.y, t.bw);

        printf("  %u: is (%02d|%02d) inside [%d %d]-[%d %d] with border %d: %s, %s\n",
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

int test_overlapRectangles() {

    printf("testing FbTk::RectangleUtil::overlapRectangles()\n");

    struct _t {
        struct Rect a;
        struct Rect b;
        int truth;
    };

    struct _test {
        bool operator()(const Rect& a, const Rect& b, int truth, unsigned int i) {

            int result = FbTk::RectangleUtil::overlapRectangles(a, b);

            printf("  %u: [%2d %2d]-[%2d %2d] %s [%2d %2d]-[%2d %2d]: %s\n",
                    i,
                    a.x(), a.y(),
                    a.x() + (int)a.width(),
                    a.y() + (int)a.height(),
                    result ? "overlaps" : "does not overlap",
                    b.x(), b.y(),
                    b.x() + (int)b.width(),
                    b.y() + (int)b.height(),
                    result == truth ? "ok" : "failed");
            return result == truth;
        }
    };

    const _t tests[] = {

        { { 0, 0, 8, 8 }, {  0, 0, 8, 8 }, true  }, // b equals a
        { { 0, 0, 8, 8 }, {  3, 3, 3, 3 }, true  }, // b completely inside a
        { { 0, 0, 8, 8 }, {  4, 4, 8, 8 }, true  }, // b overlaps a in one corner
        { { 0, 0, 8, 8 }, {  4,-1, 2, 9 }, true  }, // b overlaps a in the middle

        { { 0, 0, 8, 8 }, { -8, 0, 5, 8 }, false }, // b completely left from a
        { { 0, 0, 8, 8 }, {  9, 0, 5, 8 }, false }, // b completely right from a
        { { 0, 0, 8, 8 }, {  0,-9, 5, 8 }, false }, // b completely down below a
        { { 0, 0, 8, 8 }, {  0, 9, 5, 8 }, false }, // b completely up above a
        { { 0, 0, 8, 8 }, {  8, 0, 5, 8 }, false }, // b touches a's right edge
        { { 0, 0, 8, 8 }, {  0, 8, 5, 8 }, false }, // b touches a's bottom edge
    };



    _test test;
    for (unsigned int i = 0; i < sizeof(tests)/sizeof(_t); ++i) {
        test(tests[i].a, tests[i].b, tests[i].truth, i);
        test(tests[i].b, tests[i].a, tests[i].truth, i);
    }

    printf("done.\n");

    return 0;
}


int test_largestFreeRectangle() {

    printf("testing FbTk::RectangleUtil::largestFreeRectangle()\n");

    typedef FbTk::RectangleUtil::Rect R;
    typedef std::vector<R> V;

    struct _t {
        const char *what;
        V regions;
        V free_parts;
        R truth;
    };

    const _t tests[] = {
        { "no regions", V(), V(), { 0, 0, 0, 0 } },
        { "one head, no struts",
          V{ { 0, 0, 1920, 1080 } },
          V{ { 0, 0, 1920, 1080 } },
          { 0, 0, 1920, 1080 } },
        { "one head, left strut",
          V{ { 0, 0, 1920, 1080 } },
          V{ { 48, 0, 1872, 1080 } },
          { 48, 0, 1872, 1080 } },
        { "one head, all reserved",
          V{ { 0, 0, 1920, 1080 } },
          V{ { 0, 0, 0, 1080 } },
          { 0, 0, 0, 0 } },
        { "free part beyond its region is clamped",
          V{ { 0, 0, 100, 100 } },
          V{ { 0, 0, 200, 100 } },
          { 0, 0, 100, 100 } },
        { "two heads, no struts: spans both",
          V{ { 0, 0, 2560, 1440 }, { 2560, 0, 2560, 1440 } },
          V{ { 0, 0, 2560, 1440 }, { 2560, 0, 2560, 1440 } },
          { 0, 0, 5120, 1440 } },
        { "two heads, panel on the inner edge: the wider free head wins",
          V{ { 0, 0, 2560, 1440 }, { 2560, 0, 2560, 1440 } },
          V{ { 0, 0, 2560, 1440 }, { 2608, 0, 2512, 1440 } },
          { 0, 0, 2560, 1440 } },
        { "two heads, bottom panel on both: still spans both",
          V{ { 0, 0, 2560, 1440 }, { 2560, 0, 2560, 1440 } },
          V{ { 0, 0, 2560, 1410 }, { 2560, 0, 2560, 1410 } },
          { 0, 0, 5120, 1410 } },
        { "two heads, different heights: the wide strip wins",
          V{ { 0, 0, 1920, 1080 }, { 1920, 0, 2560, 1440 } },
          V{ { 0, 0, 1920, 1080 }, { 1920, 0, 2560, 1440 } },
          { 0, 0, 4480, 1080 } },
        { "cloned heads: one head's strut reserves for both",
          V{ { 0, 0, 1920, 1080 }, { 0, 0, 1920, 1080 } },
          V{ { 0, 0, 1920, 1080 }, { 0, 30, 1920, 1050 } },
          { 0, 30, 1920, 1050 } },
    };

    int failed = 0;
    for (unsigned int i = 0; i < sizeof(tests)/sizeof(_t); ++i) {
        const _t &t = tests[i];
        const R r = FbTk::RectangleUtil::largestFreeRectangle(t.regions, t.free_parts);
        const bool ok = r.x == t.truth.x && r.y == t.truth.y &&
                        r.width == t.truth.width && r.height == t.truth.height;
        printf("  %u: %s: %d,%d %dx%d: %s\n", i, t.what,
                r.x, r.y, r.width, r.height, ok ? "ok" : "failed");
        if (!ok) {
            ++failed;
        }
    }

    printf("done.\n");

    return failed;
}


int main(int argc, char **argv) {

    test_insideBorder();
    test_overlapRectangles();

    // the older tests only print; this one also fails the run
    return test_largestFreeRectangle() ? 1 : 0;
}
