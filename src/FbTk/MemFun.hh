// MemFun.hh for FbTk, Fluxbox Toolkit
// Copyright (c) 2008 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef FBTK_MEM_FUN_HH
#define FBTK_MEM_FUN_HH

namespace FbTk {

/// No argument functor
template <typename ReturnType, typename Object>
class MemFun0 {
public:
    typedef ReturnType (Object:: *Action)();

    MemFun0(Object& obj, Action action):
        m_obj(obj),
        m_action(action) {
    }

    void operator ()() {
        (m_obj.*m_action)();
    }
    void call() {
        (m_obj.*m_action)();
    }
private:
    Object& m_obj;
    Action m_action;
};


template <typename ReturnType, typename Object>
MemFun0<ReturnType, Object>
MemFun( Object& obj, ReturnType (Object:: *action)() ) {
    return MemFun0<ReturnType, Object>(obj, action);
}

/// One argument functor
template <typename ReturnType, typename Object, typename Arg1>
class MemFun1 {
public:
    typedef ReturnType (Object:: *Action)(Arg1);

    MemFun1(Object& obj, Action action):
        m_obj(obj),
        m_action(action) {
    }

    void operator ()(Arg1 arg1) {
        (m_obj.*m_action)(arg1);
    }

private:
    Object& m_obj;
    Action m_action;
};

/// One argument functor helper function
template <typename ReturnType, typename Object, typename Arg1>
MemFun1<ReturnType, Object, Arg1>
MemFun( Object& obj, ReturnType (Object:: *action)(Arg1) ) {
    return MemFun1<ReturnType, Object, Arg1>(obj, action);
}

/// Two argument functor
template <typename ReturnType, typename Object, typename Arg1, typename Arg2>
class MemFun2 {
public:
    typedef ReturnType (Object:: *Action)(Arg1,Arg2);

    MemFun2(Object& obj, Action action):
        m_obj(obj),
        m_action(action) {
    }
    
    void operator ()(Arg1 arg1, Arg2 arg2) {
        (m_obj.*m_action)(arg1, arg2);
    }

private:
    Object& m_obj;
    Action m_action;
};

/// Two argument functor helper function
template <typename ReturnType, typename Object, typename Arg1, typename Arg2>
MemFun2<ReturnType, Object, Arg1, Arg2> 
MemFun( Object& obj, ReturnType (Object:: *action)(Arg1,Arg2) ) {
    return MemFun2<ReturnType, Object, Arg1, Arg2>(obj, action);
}

/// Three argument functor
template <typename ReturnType, typename Object,
          typename Arg1, typename Arg2, typename Arg3>
class MemFun3 {
public:
    typedef ReturnType (Object:: *Action)(Arg1,Arg2,Arg3);

    MemFun3(Object& obj, Action action):
        m_obj(obj),
        m_action(action) {
    }

    void operator ()(Arg1 arg1, Arg2 arg2, Arg3 arg3) {
        (m_obj.*m_action)(arg1, arg2, arg3);
    }

private:
    Object& m_obj;
    Action m_action;
};

/// Three argument functor helper
template <typename ReturnType, typename Object, typename Arg1, typename Arg2, typename Arg3>
MemFun3<ReturnType, Object, Arg1, Arg2, Arg3>
MemFun( Object& obj, ReturnType (Object:: *action)(Arg1, Arg2, Arg3) ) {
    return MemFun3<ReturnType, Object, Arg1, Arg2, Arg3>(obj, action);
}

/// Ignores all arguments
template <typename ReturnType, typename Object>
class MemFun0IgnoreArgs: public MemFun0<ReturnType, Object> {
public:
    typedef MemFun0<ReturnType, Object> BaseType;

    MemFun0IgnoreArgs(Object& obj,
                      typename BaseType::Action action):
        BaseType(obj, action) {
    }

    template <typename IgnoreType1, typename IgnoreType2, typename IgnoreType3>
    void operator ()(IgnoreType1&, IgnoreType2&, IgnoreType3&) {
        BaseType::operator ()();
    }
    
    template <typename IgnoreType1, typename IgnoreType2>
    void operator ()(IgnoreType1&, IgnoreType2&) {
        BaseType::operator ()();
    }

    template <typename IgnoreType1>
    void operator ()(IgnoreType1&) {
        BaseType::operator ()();
    }
};

/// Ignores second and third argument
template <typename ReturnType, typename Object, typename Arg1>
class MemFun1IgnoreArgs: public MemFun1<ReturnType, Object, Arg1> {
public:
    typedef MemFun1<ReturnType, Object, Arg1> BaseType;

    MemFun1IgnoreArgs(Object& obj, typename BaseType::Action& action):
        BaseType(obj, action) {
    }

    template <typename IgnoreType1, typename IgnoreType2>
    void operator ()(Arg1 arg1, IgnoreType1&, IgnoreType2&) {
        BaseType::operator ()(arg1);
    }

    template <typename IgnoreType>
    void operator ()(Arg1 arg1, IgnoreType&) {
        BaseType::operator ()(arg1);
    }
};

/// Takes two arguments but ignores the third
template <typename ReturnType, typename Object, typename Arg1, typename Arg2>
class MemFun2IgnoreArgs: public MemFun2<ReturnType, Object, Arg1, Arg2> {
public:
    typedef MemFun2<ReturnType, Object, Arg1, Arg2> BaseType;

    MemFun2IgnoreArgs(Object& obj, typename BaseType::Action& action):
        BaseType(obj, action) {
    }

    template < typename IgnoreType >
    void operator ()(Arg1 arg1, Arg2 arg2, IgnoreType&) {
        BaseType::operator ()(arg1, arg2);
    }
};

/// Creates functor that ignores all arguments.
template <typename ReturnType, typename Object>
MemFun0IgnoreArgs<ReturnType, Object>
MemFunIgnoreArgs( Object& obj, ReturnType (Object:: *action)() ) {
    return MemFun0IgnoreArgs<ReturnType, Object>(obj, action);
}

/// Creates functor that ignores second and third argument.
template <typename ReturnType, typename Object, typename Arg1>
MemFun1IgnoreArgs<ReturnType, Object, Arg1>
MemFunIgnoreArgs( Object& obj, ReturnType (Object:: *action)(Arg1) ) {
    return MemFun1IgnoreArgs<ReturnType, Object, Arg1>(obj, action);
}

/// Creates functor that ignores third argument. 
template <typename ReturnType, typename Object, typename Arg1, typename Arg2>
MemFun2IgnoreArgs<ReturnType, Object, Arg1, Arg2> 
MemFunIgnoreArgs( Object& obj, ReturnType (Object:: *action)(Arg1,Arg2) ) {
    return MemFun2IgnoreArgs<ReturnType, Object, Arg1, Arg2>(obj, action);
}


} // namespace FbTk

#endif // FBTK_MEM_FUN_HH

