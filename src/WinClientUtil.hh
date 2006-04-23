#ifndef WINCLIENTUTIL_H
#define WINCLIENTUTIL_H

#include "Window.hh"

/// window client utilities
namespace WinClientUtil {

/**
 * Calculates the min of all maximum width/heights of all clients
 * @param clients the client list
 * @param width the return value of minimum of all max width of all clients
 * @param height the return value of mimimum of all max heights of all clients
 */
void maxSize(const FluxboxWindow::ClientList &clients,
             unsigned int &width, unsigned int &height);
}

#endif // WINCLIENTUTIL_H
