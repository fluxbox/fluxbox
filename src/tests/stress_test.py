#!/usr/bin/env python

# This is a simple stress test:
# * resize
# * move
# * title change
# * create/delete window test

import gtk
import gobject
import random
import time


def rotate_left(values):
    """Rotate values left"""

    new_values = values[1:len(values)]
    new_values.append(values[0])
    return new_values

class App:
    def __init__(self):
        self.titles = [ "fluxbox",
                        ">>",
                        ">>"*4,
                        ">>"*8,
                        ">>"*16,
                        ">>"*24,
                        ">>"*32,
                        ">>"*64,
                        "more to come" ]

        seed = int(time.time())
        print "Seed:",seed
        random.seed(seed)

        self.window = None

        self.create_window()

        # Setup timers
        gobject.timeout_add(100, self.change_title)
        gobject.timeout_add(200, self.change_size)
        gobject.timeout_add(300, self.change_position)
        gobject.timeout_add(1000, self.create_window)

    def create_window(self):
        """Destroys the old window and creates a new window"""

        if self.window is not None:
            self.window.destroy()
        self.window = gtk.Window()
        self.window.connect("delete-event", gtk.main_quit)
        self.window.resize(300, 100)
        self.window.set_title("fluxbox")
        self.window.show()
        
        return True

    def change_title(self):
        """Changes the title of the window"""

        self.window.set_title(self.titles[0])
        self.titles = rotate_left(self.titles)

        return True

    def change_size(self):
        """Changes the size of the window"""

        self.window.resize(random.randrange(1, 1000),
                           random.randrange(1, 1000))
 
        return True

    def change_position(self):
        """Changes the position of the window"""

        self.window.move(random.randrange(-100, 1000),
                         random.randrange(-100, 1000))

        return True

if __name__ == "__main__":
    app = App()
    gtk.main()
