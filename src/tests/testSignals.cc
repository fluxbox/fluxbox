#include <iostream>
using namespace std;

#include "../FbTk/Signal.hh"
#include "../FbTk/MemFun.hh"

#include <string>



struct NoArgument {
    void operator() () const {
        cout << "No Argument." << endl;
    }
};

struct OneArgument {
    void operator ()( int value ) {
        cout << "One argument = " << value << endl;
    }
};

struct TwoArguments {
    void operator ()( int value, const string& message ) {
        cout << "Two arguments, (1) = " << value << ", (2) = " << message << endl;
    }
};

struct ThreeArguments {
    void operator ()( int value, const string& message, double value2 ) {
        cout << "Two arguments, (1) = " << value << ", (2) = " << message
             << ", (3) = " << value2 << endl;
    }
};

struct FunctionClass {
    FunctionClass() {
        cout << "FunctionClass created." << endl;
    }
    ~FunctionClass() {
        cout << "FunctionClass deleted." << endl;
    }
    void print() {
        cout << "Printing." << endl;
    }

    void takeIt( string& str ) {
        cout << "takeIt( " << str << ")" << endl;
    }

    void showMessage( int value, const string& message ) {
        cout << "(" << value << "): " << message << endl;
    }
    void threeArgs( int value, const string& str, double pi ) {
        cout << "(" << value << "): " << str << ", pi = " << pi << endl;
    }
};

int main() {
    using FbTk::Signal;
    using FbTk::SignalTracker;

    Signal<void> no_arg;
    no_arg.connect( NoArgument() );

    Signal<void, int> one_arg;
    one_arg.connect( OneArgument() );

    Signal<void, int, const string&> two_args;
    two_args.connect( TwoArguments() );
    
    Signal<void, int, const string&, double> three_args;
    three_args.connect( ThreeArguments() );

    // emit test
    no_arg.emit();
    one_arg.emit( 10 );
    two_args.emit( 10, "Message" );
    three_args.emit( 10, "Three", 3.141592 );

    // test signal tracker
    {
        cout << "---- tracker ----" << endl;
        SignalTracker tracker;
        // setup two new slots and track them
        SignalTracker::TrackID id_no_arg = tracker.join( no_arg, NoArgument() );
        SignalTracker::TrackID id_one_arg = tracker.join( one_arg, OneArgument() );

        // two outputs each from these two signals
        no_arg.emit();
        one_arg.emit( 31 );

        // stop tracking id_one_arg, which should keep the slot after this scope,
        // the id_no_arg connection should be destroyed after this.
        tracker.leave( id_one_arg );
        cout << "---- tracker end ----" << endl;
    }

    // now we should have one output from no_arg and two outputs from one_arg
    no_arg.emit();
    one_arg.emit( 2 );

    using FbTk::MemFun;
    FunctionClass obj;
    no_arg.clear();
    no_arg.connect(MemFun(obj, &FunctionClass::print));
    no_arg.emit();

    string takeThis("Take this");
    Signal<void, string&> ref_arg;
    ref_arg.connect(MemFun(obj, &FunctionClass::takeIt));
    ref_arg.emit( takeThis );

    two_args.clear();
    two_args.connect(MemFun(obj, &FunctionClass::showMessage));
    two_args.emit(10, "This is a message");
    
    three_args.clear();
    three_args.connect(MemFun(obj, &FunctionClass::threeArgs));
    three_args.emit(9, "nine", 3.141592);
}
