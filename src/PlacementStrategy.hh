#ifndef PLACEMENTSTRATEGY_HH
#define PLACEMENTSTRATEGY_HH

#include <vector>
class FluxboxWindow;

struct PlacementStrategy {
    /**
     * Calculates a placement for @win and returns suggested placement in @place_x and @place_y
     * @param windowlist the windows that are on the same workspace
     * @param win the window that needs to be placed
     * @param place_x x placement of specific strategy
     * @param place_y y placement of specific strategy
     * @return true if the strategy found a placement for the window
     */
    virtual bool placeWindow(const std::vector<FluxboxWindow *> &windowlist,
                             const FluxboxWindow &win,
                             int &place_x, int &place_y) = 0;
};

#endif // PLACEMENTSTRATEGY_HH
