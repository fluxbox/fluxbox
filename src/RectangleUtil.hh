#ifndef RECTANGLEUTIL_HH
#define RECTANGLEUTIL_HH

namespace RectangleUtil {


/*
 * Determines if a point is inside a rectangle-like objects border.
 * @param rect A rectangle-like object that has accessors for x, y, width, and
 *        height.
 * @param x
 * @param y 
 * @param border_width The size of the border.
 * @returns true if point is inside the rectangle-like object.
*/
template <typename RectangleLike>
bool insideBorder(const RectangleLike& rect,
                  int x, int y,
                  int border_width) {
    return
        x >= rect.x() + border_width &&
        x < rect.x() + (int)rect.width() + border_width &&
        y >= rect.y() + border_width &&
        y < rect.y() + (int)rect.height() + border_width;
}

} // namespace RectangleUtil


#endif // RECTANGLEUTIL_HH
