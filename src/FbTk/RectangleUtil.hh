#ifndef FBTK_RECTANGLEUTIL_HH
#define FBTK_RECTANGLEUTIL_HH

#include <vector>

namespace FbTk {
namespace RectangleUtil {

bool insideRectangle(int x, int y, int width, int height, int px, int py);

/*
 * Determines if a point is inside a rectangle-like object.
 * @param rect A rectangle-like object that has accessors for x, y, width, and
 *        height.
 * @returns true if (px, py) is inside [x, x + width) x [y, y + height)
 */
template <typename RectangleLike>
bool insideRectangle(const RectangleLike& rect, int px, int py) {
    return insideRectangle(rect.x(), rect.y(),
                           static_cast<int>(rect.width()),
                           static_cast<int>(rect.height()),
                           px, py);
}

/*
 * Determines if a point is inside a rectangle-like object's border.
 * As with an X window, the border lies outside width and height: the
 * object covers [x, x + width + 2*border) and the area inside its
 * border is [x + border, x + border + width), likewise for y.
 * @param rect A rectangle-like object that has accessors for x, y, width, and
 *        height.
 * @param x
 * @param y
 * @param border The size of the border.
 * @returns true if point is inside the rectangle-like object's border.
*/

template <typename RectangleLike>
bool insideBorder(const RectangleLike& rect,
                  int x, int y,
                  int border) {
    const int w = static_cast<int>(rect.width());
    const int h = static_cast<int>(rect.height());
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

bool overlapRectangles(
        int ax, int ay, int awidth, int aheight,
        int bx, int by, int bwidth, int bheight);


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

/*
 * The area of the region where rectangle 'a' overlaps rectangle 'b'
 * @returns the area, 0 if 'a' does not overlap 'b' (see overlapRectangles())
 */
int overlapArea(
        int ax, int ay, int awidth, int aheight,
        int bx, int by, int bwidth, int bheight);

/*
 * The area of the region where rectangle 'a' overlaps rectangle 'b'
 * @param a - rectangle a
 * @param b - rectangle b
 * @returns the area, 0 if 'a' does not overlap 'b'
 */
template <typename RectangleLikeA, typename RectangleLikeB>
int overlapArea(const RectangleLikeA& a, const RectangleLikeB& b) {

    return overlapArea(
            a.x(), a.y(), a.width(), a.height(),
            b.x(), b.y(), b.width(), b.height());
}

/// A plain rectangle, for callers without a rectangle-like type of their own.
struct Rect {
    int x, y, width, height;
};

/*
 * The largest rectangle (by area) that is free in a set of regions, e.g.
 * the heads of a screen: it lies inside the free part of some region and
 * in no region's non-free part.
 * @param regions - the regions, e.g. head rectangles
 * @param free_parts - free_parts[i] is the free part of regions[i], e.g.
 *        the head minus its struts; clamped to regions[i]
 * @returns the rectangle, or one with width == height == 0 if nothing is free
 */
Rect largestFreeRectangle(const std::vector<Rect> &regions,
                          const std::vector<Rect> &free_parts);

/*
 * One cell of 'area' split into rows x cols cells of the same size. The
 * division leaves a rest of up to cols - 1 pixels in width and rows - 1
 * in height; the cells of the last column and of the last row get it, so
 * the cells cover the area without gaps.
 * @param area - the rectangle to split
 * @param rows, cols - the number of rows and columns
 * @param row, col - the cell, counted from 0 at the top left
 * @returns the cell, or one with width == height == 0 if rows or cols is 0
 */
Rect gridCell(const Rect &area, unsigned int rows, unsigned int cols,
              unsigned int row, unsigned int col);

/*
 * The smallest rectangle containing all of 'rects', e.g. the area spanned
 * by several heads.
 * @returns the rectangle, or one with width == height == 0 if 'rects' is
 *          empty
 */
Rect boundingRectangle(const std::vector<Rect> &rects);

} // namespace RectangleUtil
} // namespace FbTk


#endif // FBTK_RECTANGLEUTIL_HH
