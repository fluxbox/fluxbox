// parsertest.cc a test app for Parser
// Copyright (c) 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)

#include "../FbMenuParser.hh"

#include <iostream>

using namespace std;


int main(int argc, char **argv) {
    if (argc < 2) {
        cerr<<"you must supply an argument!"<<endl;
        exit(0);
    }
    cerr<<"Loading: "<<argv[1]<<endl;
    Parser *p = new FbMenuParser(argv[1]);
    if (!p->isLoaded()) {
        cerr<<"Couldn't load file: "<<argv[1]<<endl;
        delete p;
        exit(0);
    }

    Parser::Item item, item2, item3;
    int args = 0;
    int last_line_num = 0;
    std::string last_line;
    std::string type, name, argument;
    while (!p->eof()) {
        (*p)>>item>>item2>>item3;
        cerr<<item.second<<","<<item2.second<<", "<<item3.second<<endl;
    }
    delete p;
}
