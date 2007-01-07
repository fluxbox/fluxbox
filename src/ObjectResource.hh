// ObjectResource.hh for Fluxbox
// Copyright (c) 2007 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id$

#ifndef OBJECTRESOURCE_HH
#define OBJECTRESOURCE_HH

/* This is a generic resource that can be used as an accessor to a value in an object.
   The constructors allow to select between:
     1a. giving an object of ObjectType, OR
     1b. giving a function returning an object of ObjectType

     2a. a function that sets a value
     2b. a function that toggles a value 
 */

template <typename ObjectType, typename ValueType>
class ObjectResource {
public:
    typedef ValueType (ObjectType::* getResourceType)() const;
    typedef void (ObjectType::* setResourceType)(ValueType);
    typedef void (ObjectType::* toggleResourceType)();
    typedef ObjectType* (*getResourceObject)();

    ObjectResource(ObjectType *object, getResourceType get, setResourceType set, ValueType a_default) :
        m_get(get), m_set(set), m_istoggle(false), m_object(object), 
        m_default(a_default), m_use_accessor(false) {
    }

    ObjectResource(ObjectType *object, getResourceType get, toggleResourceType toggle, ValueType a_default) :
        m_get(get), m_toggle(toggle), m_istoggle(true), m_object(object),
        m_default(a_default), m_use_accessor(false) {
    }

    ObjectResource(getResourceObject object_accessor, getResourceType get, setResourceType set, ValueType a_default) :
        m_get(get), m_set(set), m_istoggle(false), m_object_accessor(object_accessor),
        m_default(a_default), m_use_accessor(true) {
    }

    ObjectResource(getResourceObject object_accessor, getResourceType get, toggleResourceType toggle, ValueType a_default) :
        m_get(get), m_toggle(toggle), m_istoggle(true), m_object_accessor(object_accessor),
        m_default(a_default), m_use_accessor(true) {
    }
    
    ObjectResource<ObjectType, ValueType>& operator = (const ValueType newvalue) {
        ObjectType * obj = getObject();
        if (!obj)
            return *this;

        if (m_istoggle) {
            if (newvalue != (operator*)())
                (obj->*m_toggle)();
        } else {
            (obj->*m_set)(newvalue);
        }
        return *this;
    }

    ObjectResource<ObjectType, ValueType>& operator += (const ValueType newvalue) {
        ObjectType * obj = getObject();
        if (obj && !m_istoggle)
            (obj->*m_set)((operator*)()+newvalue);
        return *this;
    }

    ObjectResource<ObjectType, ValueType>& operator -= (const ValueType newvalue) {
        ObjectType * obj = getObject();
        if (obj && !m_istoggle)
            (obj->*m_set)((operator*)()-newvalue);
        return *this;
    }

    // this is a touch dirty, but it makes us compatible with FbTk::Resource<int> in IntResMenuItem 
    ObjectResource<ObjectType, ValueType>& get() {
        return *this;
    }

    ValueType operator*() {
        ObjectType * obj = getObject();
        if (!obj)
            return m_default;

        return (obj->*m_get)();
    }

    const ValueType operator*() const {
        ObjectType * obj = getObject();
        if (!obj)
            return m_default;

        return (obj->*m_get)();
    }

private:
    // choose one get and one set function

    ObjectType * getObject() {
        if (m_use_accessor) 
            return (*m_object_accessor)();
        else
            return m_object;
    }

    getResourceType m_get;

    union {
        setResourceType m_set;
        toggleResourceType m_toggle;
    };

    bool m_istoggle;

    union {
        ObjectType *m_object;
        getResourceObject m_object_accessor;
    };

    // default is only used when object isn't set (saves crashes)
    ValueType m_default; 

    bool m_use_accessor;
};


#endif // OBJECTRESOURCE_HH
