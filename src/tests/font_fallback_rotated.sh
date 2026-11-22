#!/bin/sh
# font_fallback.sh for the rotated orientations: 90, 180, 270 degrees,
# one window after the other (Escape closes a window, the next one opens).
#
#   src/tests/font_fallback_rotated.sh [font]

dir=$(dirname "$0")
for o in 1 2 3; do
    echo "orient $o ($((o * 90)) degrees)"
    "$dir/font_fallback.sh" "${1:-sans-10}" "$o"
done
