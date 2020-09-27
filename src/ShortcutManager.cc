#include "ShortcutManager.hh"
#include "Debug.hh"

#include <iostream>

ShortcutManager::ShortcutManager() : m_last_placeholder_key(0) { }

void ShortcutManager::setLastPlaceHolderKey(unsigned int lastPlaceHolderKey_)
{
    m_last_placeholder_key = lastPlaceHolderKey_;
}

unsigned int ShortcutManager::getLastPlaceHolderKey()
{
    return m_last_placeholder_key;
}

void ShortcutManager::mapKeyToWindow(unsigned int key, FluxboxWindow* window)
{
    m_key_to_window_map[key] = window;
}

void ShortcutManager::removeWindow(FluxboxWindow* window)
{
    KeyToWindowMap::const_iterator it;
    for (it = m_key_to_window_map.begin(); it != m_key_to_window_map.end(); ++it) {
        if (it->second == window){
            fbdbg << "Remove mapping window[" << window
                  << "] key [" << it->first << "]" << std::endl;
            m_key_to_window_map.erase(it);
            return;
        }
    }
}

FluxboxWindow* ShortcutManager::getWindowForKey(unsigned int key)
{
    KeyToWindowMap::const_iterator it = m_key_to_window_map.find(key);
    if (it != m_key_to_window_map.end()) {
        return it->second;
    }
    else {
        return nullptr;
    }
}
