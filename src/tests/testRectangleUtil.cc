#include "FbTk/RectangleUtil.hh"

#include <cstdio>

struct Rect {

    int x() const { return m_x; }
    int y() const { return m_y; }
    int width() const { return m_width; }
    int height() const { return m_height; }

    int m_x, m_y, m_width, m_height;
};




int test_insideRectangle() {

    printf("testing FbTk::RectangleUtil::insideRectangle()\n");

    struct _t {
        struct Rect rect;
        int x;
        int y;
        int truth;
    };

    const _t tests[] = {
        { { 0, 0, 10, 10 },  0,  0, true },  // top left corner
        { { 0, 0, 10, 10 },  9,  9, true },  // bottom right corner
        { { 0, 0, 10, 10 }, 10,  5, false }, // one past the right edge
        { { 0, 0, 10, 10 },  5, 10, false }, // one past the bottom edge
        { { 5, 5, 10, 10 },  4,  5, false }, // moved, left of it
        { { 5, 5, 10, 10 }, 14, 14, true },  // moved, bottom right corner
        { { 0, 0,  0,  0 },  0,  0, false }  // empty
    };

    int failed = 0;
    for (unsigned int i = 0; i < sizeof(tests)/sizeof(_t); ++i) {
        const _t& t = tests[i];
        int result = FbTk::RectangleUtil::insideRectangle(t.rect, t.x, t.y);

        printf("  %u: is (%02d|%02d) inside [%d %d]-[%d %d]: %s, %s\n",
                i,
                t.x, t.y,
                t.rect.x(), t.rect.y(),
                t.rect.x() + t.rect.width(),
                t.rect.y() + t.rect.height(),
                result ? "yes" : "no", result == t.truth ? "ok" : "failed");
        if (result != t.truth) {
            ++failed;
        }
    }

    printf("done.\n");

    return failed;
}

int test_insideBorder() {

    printf("testing FbTk::RectangleUtil::insideBorder()\n");

    struct _t {
        struct Rect rect;
        int x;
        int y;
        int bw;
        int truth;
    };

    // as with an X window, the border lies outside width and height:
    // { 0, 0, 10, 10 } with border 2 covers [0, 14), its inside is [2, 12)
    _t tests[] = {
        { { 0, 0, 10, 10 },  0,  0, 2, false }, // on the (outer) edge
        { { 0, 0, 10, 10 },  1,  1, 2, false }, // on the (inner) edge
        { { 0, 0, 10, 10 },  2,  2, 2, true },  // first pixel inside
        { { 0, 0, 10, 10 },  5,  5, 2, true },  // really inside
        { { 0, 0, 10, 10 }, 11, 11, 2, true },  // last pixel inside
        { { 0, 0, 10, 10 }, 12,  5, 2, false }, // on the right border
        { { 0, 0, 10, 10 },  5, 12, 2, false }, // on the bottom border
        { { 5, 5, 10, 10 },  6,  6, 1, true },  // moved, first pixel inside
        { { 5, 5, 10, 10 }, 16,  6, 1, false }, // moved, on the right border
        { { 0, 0, 10, 10 }, -5,  0, 2, false }, // somewhere outside
        { { 0, 0, 10, 10 }, 20, 20, 2, false }  // outside for sure
    };

    int failed = 0;
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
        if (result != t.truth) {
            ++failed;
        }
    }

    printf("done.\n");

    return failed;
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
    int failed = 0;
    for (unsigned int i = 0; i < sizeof(tests)/sizeof(_t); ++i) {
        if (!test(tests[i].a, tests[i].b, tests[i].truth, i)) {
            ++failed;
        }
        if (!test(tests[i].b, tests[i].a, tests[i].truth, i)) {
            ++failed;
        }
    }

    printf("done.\n");

    return failed;
}


int test_overlapArea() {

    printf("testing FbTk::RectangleUtil::overlapArea()\n");

    struct _t {
        struct Rect a;
        struct Rect b;
        int truth;
    };

    const _t tests[] = {
        { { 0, 0, 8, 8 }, {  0, 0, 8, 8 }, 64 }, // b equals a
        { { 0, 0, 8, 8 }, {  3, 3, 3, 3 },  9 }, // b completely inside a
        { { 0, 0, 8, 8 }, {  4, 4, 8, 8 }, 16 }, // b overlaps a in one corner
        { { 0, 0, 8, 8 }, {  4,-1, 2, 9 }, 16 }, // b overlaps a in the middle
        { { 0, 0, 8, 8 }, { -8, 0, 5, 8 },  0 }, // b completely left from a
        { { 0, 0, 8, 8 }, {  8, 0, 5, 8 },  0 }, // b touches a's right edge
        { { 0, 0, 8, 8 }, {  0, 8, 5, 8 },  0 }, // b touches a's bottom edge
        { { 0, 0, 8, 8 }, { 20,20, 5, 5 },  0 }, // b far off, both axes
    };

    int failed = 0;
    for (unsigned int i = 0; i < sizeof(tests)/sizeof(_t); ++i) {
        const _t& t = tests[i];
        const int ab = FbTk::RectangleUtil::overlapArea(t.a, t.b);
        const int ba = FbTk::RectangleUtil::overlapArea(t.b, t.a);
        const bool ok = ab == t.truth && ba == t.truth;

        printf("  %u: [%2d %2d]-[%2d %2d] and [%2d %2d]-[%2d %2d] overlap by %d|%d: %s\n",
                i,
                t.a.x(), t.a.y(),
                t.a.x() + t.a.width(),
                t.a.y() + t.a.height(),
                t.b.x(), t.b.y(),
                t.b.x() + t.b.width(),
                t.b.y() + t.b.height(),
                ab, ba, ok ? "ok" : "failed");
        if (!ok) {
            ++failed;
        }
    }

    printf("done.\n");

    return failed;
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


int test_gridCell() {

    printf("testing FbTk::RectangleUtil::gridCell()\n");

    typedef FbTk::RectangleUtil::Rect R;

    struct _t {
        const char *what;
        R area;
        unsigned int rows, cols, row, col;
        R truth;
    };

    const _t tests[] = {
        { "1x1 is the area",
          { 10, 20, 1030, 766 }, 1, 1, 0, 0, { 10, 20, 1030, 766 } },
        { "2 columns, even: left",
          { 0, 0, 1030, 766 }, 1, 2, 0, 0, { 0, 0, 515, 766 } },
        { "2 columns, even: right",
          { 0, 0, 1030, 766 }, 1, 2, 0, 1, { 515, 0, 515, 766 } },
        { "3 columns: first",
          { 0, 0, 1030, 766 }, 1, 3, 0, 0, { 0, 0, 343, 766 } },
        { "3 columns: middle",
          { 0, 0, 1030, 766 }, 1, 3, 0, 1, { 343, 0, 343, 766 } },
        { "3 columns: last gets the rest",
          { 0, 0, 1030, 766 }, 1, 3, 0, 2, { 686, 0, 344, 766 } },
        { "3 rows: last gets the rest",
          { 0, 0, 1030, 766 }, 3, 1, 2, 0, { 0, 510, 1030, 256 } },
        { "2x3, moved area: bottom right",
          { 100, 50, 1030, 766 }, 2, 3, 1, 2, { 786, 433, 344, 383 } },
        { "2x3, moved area: top middle",
          { 100, 50, 1030, 766 }, 2, 3, 0, 1, { 443, 50, 343, 383 } },
        { "more columns than pixels",
          { 0, 0, 2, 2 }, 1, 3, 0, 2, { 0, 0, 2, 2 } },
        { "no rows",
          { 0, 0, 1030, 766 }, 0, 3, 0, 0, { 0, 0, 0, 0 } },
        { "no columns",
          { 0, 0, 1030, 766 }, 3, 0, 0, 0, { 0, 0, 0, 0 } },
    };

    int failed = 0;
    for (unsigned int i = 0; i < sizeof(tests)/sizeof(_t); ++i) {
        const _t &t = tests[i];
        const R r = FbTk::RectangleUtil::gridCell(t.area, t.rows, t.cols, t.row, t.col);
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


int test_boundingRectangle() {

    printf("testing FbTk::RectangleUtil::boundingRectangle()\n");

    typedef FbTk::RectangleUtil::Rect R;
    typedef std::vector<R> V;

    struct _t {
        const char *what;
        V rects;
        R truth;
    };

    const _t tests[] = {
        { "nothing", V(), { 0, 0, 0, 0 } },
        { "one", V{ { 10, 20, 30, 40 } }, { 10, 20, 30, 40 } },
        { "two side by side", V{ { 0, 0, 2560, 1440 }, { 2560, 0, 2560, 1440 } },
          { 0, 0, 5120, 1440 } },
        { "same two, right one first", V{ { 2560, 0, 2560, 1440 }, { 0, 0, 2560, 1440 } },
          { 0, 0, 5120, 1440 } },
        { "different heights", V{ { 0, 0, 1920, 1080 }, { 1920, -200, 1080, 1920 } },
          { 0, -200, 3000, 1920 } },
        { "one inside the other", V{ { 0, 0, 100, 100 }, { 10, 10, 5, 5 } },
          { 0, 0, 100, 100 } },
    };

    int failed = 0;
    for (unsigned int i = 0; i < sizeof(tests)/sizeof(_t); ++i) {
        const _t &t = tests[i];
        const R r = FbTk::RectangleUtil::boundingRectangle(t.rects);
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

    int failed = 0;
    failed += test_insideRectangle();
    failed += test_insideBorder();
    failed += test_overlapRectangles();
    failed += test_overlapArea();
    failed += test_largestFreeRectangle();
    failed += test_gridCell();
    failed += test_boundingRectangle();

    return failed ? 1 : 0;
}
