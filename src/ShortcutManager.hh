#ifndef SHORTCUTMANAGER_HH
#define SHORTCUTMANAGER_HH

#include <map>

class FluxboxWindow;

class ShortcutManager {

public:

    ShortcutManager();

    void setLastPlaceHolderKey(unsigned int lastPlaceHolderKey_);

    unsigned int getLastPlaceHolderKey();

    void mapKeyToWindow(unsigned int key, FluxboxWindow* window);

    void removeWindow(FluxboxWindow* window);

    FluxboxWindow* getWindowForKey(unsigned int key);

private:

    typedef std::map<unsigned int, FluxboxWindow*> KeyToWindowMap;

    unsigned int m_last_placeholder_key;
    KeyToWindowMap m_key_to_window_map;
};

#endif
