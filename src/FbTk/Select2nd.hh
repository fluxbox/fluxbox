#ifndef FBTK_SELECT2ND_HH
#define FBTK_SELECT2ND_HH

#include <functional>

namespace FbTk {

template <class A>
class Select2nd:public std::unary_function<A, typename A::second_type> {
public:
    typename A::second_type operator () (const A &arg) const {
        return arg.second;
    }
};

} // namespace FbTk

#endif // FBTK_SELECT2ND_HH
