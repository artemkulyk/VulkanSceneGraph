#include <vsg/introspection/c_interface.h>


#include <vsg/core/Object.h>
#include <vsg/nodes/Group.h>
#include <vsg/nodes/LOD.h>
#include <vsg/core/Visitor.h>

#include <typeinfo>
#include <typeindex>
#include <map>
#include <unordered_map>
#include <iostream>
#include <functional>

namespace vsg
{



class TypeDescriptor : public vsg::Object
{
public:
    using Properties = std::map<std::string, ref_ptr<TypeDescriptor>>;
    using CreateFunction = std::function<Object*()>;

    std::string     className;
    CreateFunction  createFunction;
    Properties      properties;
};

class Introspection : public Object
{
public:

    Introspection() {

        std::cout<<"Introspection()"<<std::endl;

        add(std::type_index(typeid(vsg::Group)), "vsg::Group", [](){ return new vsg::Group; });
        add(std::type_index(typeid(vsg::LOD)), "vsg::LOD", [](){ return new vsg::LOD; });
        add(std::type_index(typeid(vsg::Node)), "vsg::Node", [](){ return new vsg::Node; });

    }
    virtual ~Introspection() { std::cout<<"~Introspection()"<<std::endl; }

    using IndexTypeDescriptorMap = std::unordered_map<std::type_index, ref_ptr<TypeDescriptor>>;
    using NameTypeDescriptorMap = std::unordered_map<std::string, ref_ptr<TypeDescriptor>>;

    IndexTypeDescriptorMap  _indexTypeDescriptorMap;
    NameTypeDescriptorMap   _nameTypeDescriptorMap;

    void add(std::type_index typeindex, const char* className, TypeDescriptor::CreateFunction createFunction)
    {
        TypeDescriptor* typeDescriptor = new TypeDescriptor;
        typeDescriptor->className = className;
        typeDescriptor->createFunction = createFunction;

        _indexTypeDescriptorMap[typeindex] = typeDescriptor;
        _nameTypeDescriptorMap[className] = typeDescriptor;

        std::cout<<"    add(..., "<<className<<", ,,)"<<std::endl;
    }

    static ref_ptr<Introspection>& instance()
    {
        static ref_ptr<Introspection> s_introspection = new Introspection;
        return s_introspection;
    }

    vsg::Object* create(const char* className) const
    {
        std::cout<<"create("<<className<<")"<<std::endl;
        NameTypeDescriptorMap::const_iterator itr = _nameTypeDescriptorMap.find(className);
        if (itr != _nameTypeDescriptorMap.end())
        {
            std::cout<<"  found TypeDescriptor()"<<std::endl;
            return (itr->second->createFunction)();
        }
        else
        {
            std::cout<<"  could not find TypeDescriptor()"<<std::endl;
            return nullptr;
        }
    }

    TypeDescriptor* typeDescriptor(const vsg::Object* object) const
    {
        IndexTypeDescriptorMap::const_iterator itr = _indexTypeDescriptorMap.find(std::type_index(typeid(*object)));
        return (itr != _indexTypeDescriptorMap.end()) ? itr->second : nullptr;
    }

};

}


extern "C"
{

void vsgRef(vsgObjectPtr object)
{
    if (object)
    {
        reinterpret_cast<vsg::Object*>(object)->ref();
    }
}

void vsgUnref(vsgObjectPtr object)
{
    if (object)
    {
        reinterpret_cast<vsg::Object*>(object)->unref();
    }
}

vsgObjectPtr vsgCreate(const char* className)
{
    return vsg::Introspection::instance()->create(className);
}

const char* vsgClassName(vsgObjectPtr object)
{
    vsg::TypeDescriptor* td = vsg::Introspection::instance()->typeDescriptor(reinterpret_cast<vsg::Object*>(object));
    if (td) return td->className.c_str();
    else return 0;
}

vsgObjectPtr vsgMethod(vsgObjectPtr /*object*/, const char* /*methodName*/)
{
    return 0;
}


class ObjectToPropertyVisitor : public vsg::Visitor
{
public:

    Property _property = {};

    void apply(vsg::Object& value) override { _property.type = Property::TYPE_Object; _property.value._object = &value; std::cout<<"apply(vsg::Object&)"<<std::endl;}
    void apply(vsg::boolValue& value) override { _property.type = Property::TYPE_bool; _property.value._bool = value.value(); std::cout<<"apply(vsg::boolValue&)"<<std::endl;}
    void apply(vsg::intValue& value) override { _property.type = Property::TYPE_int; _property.value._int = value.value(); std::cout<<"apply(vsg::intValue&)"<<std::endl;}
};

struct Property vsgGetProperty(vsgObjectPtr objectPtr, const char* propertyName)
{
    if (!objectPtr) return Property{Property::TYPE_undefined,{0}};

    vsg::Object* object = reinterpret_cast<vsg::Object*>(objectPtr);
    vsg::Object* propertyObject = object->getObject(propertyName);
    if (propertyObject)
    {
        ObjectToPropertyVisitor otpv;
        propertyObject->accept(otpv);

        std::cout<<"Return object"<<std::endl;
        return otpv._property;
    }
    else
    {
        std::cout<<"Return empty, fallbck to TYPE_undefined."<<std::endl;

        Property property;
        property.type = Property::TYPE_undefined;
        return property;
    }
}

void vsgSetProperty(vsgObjectPtr objectPtr, const char* propertyName, struct Property property)
{
    std::cout<<"vsgSetProperty("<<objectPtr<<", "<<propertyName<<", "<<property.type<<std::endl;
    if (!objectPtr) return;

    vsg::Object* object = reinterpret_cast<vsg::Object*>(objectPtr);
    switch(property.type)
    {
        case(Property::TYPE_Object) : object->setValue(propertyName, reinterpret_cast<vsg::Object*>(property.value._object)); break;
        case(Property::TYPE_bool) : object->setValue(propertyName, bool(property.value._bool!=0)); break;
        case(Property::TYPE_char) : object->setValue(propertyName, property.value._char); break;
        case(Property::TYPE_unsigned_char) : object->setValue(propertyName, property.value._unsigned_char); break;
        case(Property::TYPE_short) : object->setValue(propertyName, property.value._short); break;
        case(Property::TYPE_unsigned_short) : object->setValue(propertyName, property.value._unsigned_short); break;
        case(Property::TYPE_int) : object->setValue(propertyName, property.value._int); break;
        case(Property::TYPE_unsigned_int) : object->setValue(propertyName, property.value._unsigned_int); break;
        case(Property::TYPE_float) : object->setValue(propertyName, property.value._float); break;
        case(Property::TYPE_double) : object->setValue(propertyName, property.value._double); break;
        default: std::cout<<"Unhandling Property type"<<std::endl;
    }
}

unsigned int vsgGetNumProperties(vsgObjectPtr /*object*/)
{
    return 0;
}

const char* vsgGetPropertyName(vsgObjectPtr /*object*/, unsigned int /*index*/)
{
    return 0;
}

}
