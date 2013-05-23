#ifndef RECTANGLEUTIL_HH
#define RECTANGLEUTIL_HH

namespace RectangleUtil {

inline bool insideRectangle(int x, int y, int width, int height, int px, int py) {

    return
        px >= x &&
        px < (x + width) &&
        py >= y &&
        py < (y + height);
}

/*
 * Determines if a point is inside a rectangle-like objects border.
 * @param rect A rectangle-like object that has accessors for x, y, width, and
 *        height.
 * @param x
 * @param y 
 * @param border The size of the border.
 * @returns true if point is inside the rectangle-like object.
*/

template <typename RectangleLike>
bool insideBorder(const RectangleLike& rect,
                  int x, int y,
                  int border) {
    const int w = static_cast<int>(rect.width()) - border;
    const int h = static_cast<int>(rect.height()) - border;
    return insideRectangle(rect.x() + border, rect.y() + border, w, h, x, y);
}



/*
 * Determines if rectangle 'a' overlaps rectangle 'b'
 * @returns true if 'a' overlaps 'b'
 *
 *    outside              overlap situations
 *
 *  a----a            a----a         a--------a  b--------b
 *  |    | b----b     |  b-+--b      | b----b |  | a----a |
 *  a----a |    |     a--+-a  |      | |    | |  | |    | |
 *         b----b        b----b      | b----b |  | a----a |
 *                                   a--------a  b--------b
 *
 */

inline bool overlapRectangles(
        int ax, int ay, int awidth, int aheight,
        int bx, int by, int bwidth, int bheight) {

    bool do_not_overlap =
         ax > (bx + bwidth)
      || bx > (ax + awidth)
      || ay > (by + bheight)
      || by > (ay + aheight);

    return !do_not_overlap;
}


/*
 * Determines if rectangle 'a' overlaps rectangle 'b'
 * @param a - rectangle a
 * @param b - rectangle b
 * @returns true if 'a' overlaps 'b'
 */
template <typename RectangleLikeA, typename RectangleLikeB>
bool overlapRectangles(const RectangleLikeA& a, const RectangleLikeB& b) {

    return overlapRectangles(
            a.x(), a.y(), a.width(), a.height(),
            b.x(), b.y(), b.width(), b.height());
}

} // namespace RectangleUtil


#endif // RECTANGLEUTIL_HH
