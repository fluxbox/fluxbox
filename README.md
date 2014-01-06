The version in this branch adds following feature:

When Fluxbox creates system tray icons it adds them from right to left,
so in an autostart situation the order is effectively random.

With this version you can add following options to .fluxbox/init:

(o) session.screen0.systray.pinRight

(o) session.screen0.systray.pinLeft

which can be set to a comma seperated list of window classnames
(retrieveable with xprop). The system tray icons will then be sorted
accordingly and retain the order in which they are specified.

consider a system tray: A C B D E F
with pinRight = A, B and pinLeft = E, F it will look like
E F [C D E] A B while the icons in [] are un-ordered as usual.
