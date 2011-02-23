// testLayers.cc a test app for Layers
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)


#include "App.hh"
#include "FbWindow.hh"
#include "Color.hh"
#include "EventManager.hh"
#include <iostream>
#include "Layer.hh"
#include "LayerItem.hh"
#include "MultLayers.hh"

using namespace FbTk;
using namespace std;
int main() {
    FbTk::App app;
    int mask = ButtonPressMask;
    FbWindow win_red(0, 0, 0, 60, 60, mask, true);
    FbWindow win_redB(0, 0, 0, 60, 60, mask, true);
    FbWindow win_blue(0, 30, 0, 60, 60, mask, true);
    FbWindow win_green(0, 60, 0, 60, 60, mask, true);
    FbWindow win_red2(0, 0, 0, 60, 60, mask, true);
    FbWindow win_blue2(0, 30, 0, 60, 60, mask, true);
    FbWindow win_green2(0, 60, 0, 60, 60, mask, true);
    FbWindow win_red3(0, 0, 0, 60, 60, mask, true);
    FbWindow win_blue3(0, 30, 0, 60, 60, mask, true);
    FbWindow win_green3(0, 60, 0, 60, 60, mask, true);
    Color red("red", 0);
    Color blue("blue", 0);
    Color green("green", 0);
    Color red2("magenta", 0);
    Color blue2("cyan", 0);
    Color green2("yellow", 0);
    Color red3("pink", 0);
    Color blue3("navy", 0);
    Color green3("brown", 0);
    win_red.setBackgroundColor(red);
    win_red.clear();
    win_redB.setBackgroundColor(red);
    win_redB.clear();
    win_blue.setBackgroundColor(blue);
    win_blue.clear();
    win_green.setBackgroundColor(green);
    win_green.clear();

    win_red2.setBackgroundColor(red2);
    win_red2.clear();
    win_blue2.setBackgroundColor(blue2);
    win_blue2.clear();
    win_green2.setBackgroundColor(green2);
    win_green2.clear();

    win_red3.setBackgroundColor(red3);
    win_red3.clear();
    win_blue3.setBackgroundColor(blue3);
    win_blue3.clear();
    win_green3.setBackgroundColor(green3);
    win_green3.clear();

    win_red.show();
    win_red.move(10, 10);
    win_redB.show();
    win_redB.move(30, 30);
    win_blue.show();
    win_blue.move(40, 40);    
    win_green.show();
    win_green.move(50, 20);
    
    win_red2.show();
    win_red2.move(15, 15);
    win_blue2.show();
    win_blue2.move(45, 45);    
    win_green2.show();
    win_green2.move(45, 15);

    /*
    win_red3.show();
    win_red3.move(5, 5);
    win_blue3.show();
    win_blue3.move(35, 35);    
    win_green3.show();
    win_green3.move(45, 5);
    */

    EventManager &evm = *EventManager::instance();
    Display *disp = app.display();
    XEvent event;
    MultLayers *ml = new MultLayers(3);
    LayerItem *item_red = new XLayerItem(win_red.window());
    item_red->addWindow(win_redB.window());
    LayerItem *item_green = new LayerItem(win_green.window());
    LayerItem *item_blue = new LayerItem(win_blue.window());

    ml->addToTop(*item_blue, 0);
    ml->addToTop(*item_green, 0);
    ml->addToTop(*item_red, 0);

    LayerItem *item_red2 = new LayerItem(win_red2.window());
    LayerItem *item_green2 = new LayerItem(win_green2.window());
    LayerItem *item_blue2 = new LayerItem(win_blue2.window());

    ml->addToTop(*item_blue2, 1);
    ml->addToTop(*item_green2, 1);
    ml->addToTop(*item_red2, 1);

    LayerItem *item_red3 = new LayerItem(win_red3.window());
    LayerItem *item_green3 = new LayerItem(win_green3.window());
    LayerItem *item_blue3 = new LayerItem(win_blue3.window());

    ml->addToTop(*item_blue3, 2);
    ml->addToTop(*item_green3, 2);
    ml->addToTop(*item_red3, 2);

    //item_blue.setLayer(*layer);
    //item_green.setLayer(*layer);
    //item_red.setLayer(*layer);
    cerr<<"red: "<<hex<<win_red.window()<<endl;
    cerr<<"blue: "<<hex<<win_blue.window()<<endl;
    cerr<<"green: "<<hex<<win_green.window()<<endl;
    cerr<<"Left click on color to raise one step"<<endl;
    cerr<<"Right click on color to raise to next level up"<<endl;
    cerr<<"Middle click on color to lower to next level down"<<endl;
    XFlush(disp);
    //layer.restack();
    while (1) {
        while (XNextEvent(disp, &event))
            continue;
        if (event.xbutton.button == 1) {
            if (event.xany.window == win_red.window() || event.xany.window == win_redB.window()) {
                item_red->raise();
                cerr<<"Raise red"<<endl;
            } else if (event.xany.window == win_blue.window()) {
                item_blue->raise();
                cerr<<"Raise blue"<<endl;
            } else if (event.xany.window == win_green.window()) {
                item_green->raise();
                cerr<<"Raise green"<<endl;
            } else if (event.xany.window == win_red2.window()) {
                item_red2->raise();
                cerr<<"Raise red2"<<endl;
            } else if (event.xany.window == win_blue2.window()) {
                item_blue2->raise();
                cerr<<"Raise blue2"<<endl;
            } else if (event.xany.window == win_green2.window()) {
                item_green2->raise();
                cerr<<"Raise green2"<<endl;
            } else if (event.xany.window == win_red3.window()) {
                item_red3->raise();
                cerr<<"Raise red3"<<endl;
            } else if (event.xany.window == win_blue3.window()) {
                item_blue3->raise();
                cerr<<"Raise blue3"<<endl;
            } else if (event.xany.window == win_green3.window()) {
                item_green3->raise();
                cerr<<"Raise green3"<<endl;
            } else {
                cerr<<"Unknown window"<<endl;
            }

        } else if (event.xbutton.button == 3) {
            if (event.xany.window == win_red.window() || event.xany.window == win_redB.window()) {
                ml->raise(*item_red);
            } else if (event.xany.window == win_blue.window()) {
                ml->raise(*item_blue);
            } else if (event.xany.window == win_green.window()) {
                ml->raise(*item_green);
            } else if (event.xany.window == win_red2.window()) {
                ml->raise(*item_red2);
            } else if (event.xany.window == win_blue2.window()) {
                ml->raise(*item_blue2);
            } else if (event.xany.window == win_green2.window()) {
                ml->raise(*item_green2);
            } else if (event.xany.window == win_red3.window()) {
                ml->raise(*item_red3);
            } else if (event.xany.window == win_blue3.window()) {
                ml->raise(*item_blue3);
            } else if (event.xany.window == win_green3.window()) {
                ml->raise(*item_green3);
            } else {
                cerr<<"Unknown window"<<endl;
            }

        } else if (event.xbutton.button == 2) {
            if (event.xany.window == win_red.window() || event.xany.window == win_redB.window()) {
                ml->lower(*item_red);
            } else if (event.xany.window == win_blue.window()) {
                ml->lower(*item_blue);
            } else if (event.xany.window == win_green.window()) {
                ml->lower(*item_green);
            } else if (event.xany.window == win_red2.window()) {
                ml->lower(*item_red2);
            } else if (event.xany.window == win_blue2.window()) {
                ml->lower(*item_blue2);
            } else if (event.xany.window == win_green2.window()) {
                ml->lower(*item_green2);
            } else if (event.xany.window == win_red3.window()) {
                ml->lower(*item_red3);
            } else if (event.xany.window == win_blue3.window()) {
                ml->lower(*item_blue3);
            } else if (event.xany.window == win_green3.window()) {
                ml->lower(*item_green3);
            } else {
                cerr<<"Unknown window"<<endl;
            }

        } else {
            cerr<<"Button: "<<event.xbutton.button<<endl;
            if (event.xany.window == win_red.window() || event.xany.window == win_redB.window()) {
                item_red->raise();
                cerr<<"Raise red"<<endl;
            } else if (event.xany.window == win_blue.window()) {
                item_blue->raise();
                cerr<<"Raise blue"<<endl;
            } else if (event.xany.window == win_green.window()) {
                item_green->raise();
                cerr<<"Raise green"<<endl;
            } else {
                cerr<<"Unknown window"<<endl;
            }
            
        }

    }

}
