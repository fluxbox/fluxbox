#include "WinClientUtil.hh"
#include "WinClient.hh"

#include <algorithm>

namespace WinClientUtil {

void maxSize(const FluxboxWindow::ClientList &clients,
             unsigned int &max_width, unsigned int &max_height) {
    FluxboxWindow::ClientList::const_iterator it = clients.begin();
    FluxboxWindow::ClientList::const_iterator it_end = clients.end();
    max_width = (unsigned int) ~0; // unlimited
    max_height = (unsigned int) ~0; // unlimited
    for (; it != it_end; ++it) {
        // special case for max height/width == 0
        // 0 indicates unlimited size, so we skip them
        // and set max size to 0 if max size == ~0 after the loop
        if ((*it)->maxHeight() != 0)
            max_height = std::min( (*it)->maxHeight(), max_height );
        if ((*it)->maxWidth() != 0)
            max_width = std::min( (*it)->maxWidth(), max_width );
    }

    if (max_width == (unsigned int) ~0)
        max_width = 0;
    if (max_height == (unsigned int) ~0)
        max_height = 0;
}

}


