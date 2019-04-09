//
// Created by skv on 03.09.2018.
//

#include "TemplateDataSource.hpp"

namespace protogen {

void TemplateDataSource::initForMessage(protogen::Parser& p, const std::string& messageName)
{
    using namespace protogen;

    const Message& msg = p.getMessage(messageName);
    setVar("message.name", messageName);
    setVar("message.versionMajor", std::to_string(msg.majorVersion));
    setVar("message.versionMinor", std::to_string(msg.minorVersion));
    fillPackage("message.package", msg.pkg);

    setBool("message.havetag", msg.haveTag);
    if(msg.haveTag)
    {
        setVar("message.tag", std::to_string(msg.tag));
    }

    setBool("message.haveparent", !msg.parent.empty());
    if(!msg.parent.empty())
    {
        setVar("message.parent", msg.parent);
        const Message& pmsg = p.getMessage(msg.parent);
        if(!pmsg.pkg.empty())
        {
            //varMap["message.parentpackage"]=pmsg.pkg;
            fillPackage("message.parentpackage", pmsg.pkg);
        }
    }


    Loop& msgFields = createLoop("field");
    fillFields(p, msgFields, msg.fields);
    {
        const Message* pmsg = msg.parent.empty() ? nullptr : &p.getMessage(msg.parent);
        Loop& msgParentFields = createLoop("parent.field");
        msgParentFields.name = "field";
        while(pmsg)
        {
            fillFields(p, msgParentFields, pmsg->fields);
            pmsg = pmsg->parent.empty() ? nullptr : &p.getMessage(pmsg->parent);
        }
    }

    for(const auto& prop : msg.properties)
    {
        for(const auto& field : prop.fields)
        {
            std::string n = "message.";
            n += field.name;
            if(field.pt == ptString)
            {
                setVar(n, field.strValue);
            }
            else if(field.pt == ptBool)
            {
                setBool(n, field.boolValue);
            }
            else
            {
                setVar(n, std::to_string(field.intValue));
            }
        }
    }

}

void TemplateDataSource::initForProtocol(const protogen::Parser& p, const std::string& protoName)
{
    using namespace protogen;
    const Protocol& proto = p.getProtocol(protoName);
    setVar("protocol.name", protoName);
    fillPackage("protocol.package", proto.pkg);
    typedef protogen::Protocol::MessagesVector MsgVector;
    Loop& ld = createLoop("message");
    for(const auto& message : proto.messages)
    {
        const Message& msg = p.getMessage(message.msgName);
        Namespace& mn = ld.newItem();
        mn.addVar("message.name", msg.name);
        mn.addBool("message.havetag", msg.haveTag);
        mn.addBool("message.haveparent", !msg.parent.empty());
        if(!msg.parent.empty())
        {
            mn.addVar("message.parent", msg.parent);
        }

        if(msg.haveTag)
        {
            mn.addVar("message.tag", std::to_string(msg.tag));
        }
        for(const auto& prop : msg.properties)
        {
            for(const auto& field : prop.fields)
            {
                if(field.pt == ptBool)
                {
                    mn.addBool("message." + field.name, field.boolValue);
                }
                else if(field.pt == ptString)
                {
                    mn.addVar("message." + field.name, field.strValue);
                }
                else if(field.pt == ptInt)
                {
                    mn.addVar("message." + field.name, std::to_string(field.intValue));
                }
            }
        }
        for(const auto& prop : message.props)
        {
            for(const auto& field : prop.fields)
            {
                if(field.pt == ptBool)
                {
                    mn.addBool("message." + field.name, field.boolValue);
                }
                else if(field.pt == ptString)
                {
                    mn.addVar("message." + field.name, field.strValue);
                }
                else if(field.pt == ptInt)
                {
                    mn.addVar("message." + field.name, std::to_string(field.intValue));
                }
            }
        }
    }
}

void TemplateDataSource::initForEnum(protogen::Parser& p, const std::string& enumName)
{
    using namespace protogen;
    const Enum& e = p.getEnum(enumName);
    setVar("enum.name", e.name);
    setVar("enum.type", e.typeName);
    Loop& ld = createLoop("item");
    for(auto& val : e.values)
    {
        Namespace& en = ld.newItem();
        en.addVar("item.name", val.name);
        if(e.vt == Enum::vtString)
        {
            en.addVar("item.value", val.strVal);
        }
        else
        {
            en.addVar("item.value", std::to_string(val.intVal));
        }
    }

    for(const auto& prop : e.properties)
    {
        for(const auto& field : prop.fields)
        {
            std::string n = "enum." + field.name;
            if(field.pt == ptBool)
            {
                setBool(n, field.boolValue);
            }
            else if(field.pt == ptString)
            {
                setVar(n, field.strValue);
            }
            else if(field.pt == ptInt)
            {
                setVar(n, std::to_string(field.intValue));
            }
        }
    }
}

void TemplateDataSource::fillFields(protogen::Parser& p, Loop& ld, const protogen::FieldsVector& fields)
{
    using namespace protogen;
    FieldsVector::const_iterator it, end;
    it = fields.begin();
    end = fields.end();
    for(; it != end; it++)
    {
        Namespace& fn = ld.newItem();
        fn.addVar("name", it->name);
        fn.addVar("tag", std::to_string(it->tag));
        if(it->ft.fk == fkNested)
        {
            fn.addVar("type", "nested");
            fn.addVar("typename", it->ft.typeName);
            const Message& fmsg = p.getMessage(it->ft.typeName);
            if(!fmsg.pkg.empty())
            {
                fn.addVar("typepackage", fmsg.pkg);
            }
        }
        else if(it->ft.fk == fkEnum)
        {
            fn.addVar("type", "enum");
            fn.addVar("typename", it->ft.typeName);
            const Enum& fenum = p.getEnum(it->ft.typeName);
            fn.addVar("valuetype", fenum.typeName);
            if(!fenum.pkg.empty())
            {
                fn.addVar("typepackage", fenum.pkg);
            }
        }
        else
        {
            const FieldType& ft = it->ft;
            fn.addVar("type", ft.typeName);
            for(const auto& prop : ft.properties)
            {
                for(const auto& field : prop.fields)
                {
                    if(field.pt == ptBool)
                    {
                        fn.addBool(field.name, field.boolValue);
                    }
                    else if(field.pt == ptInt)
                    {
                        fn.addVar(field.name, std::to_string(field.intValue));
                    }
                    else
                    {
                        fn.addVar(field.name, field.strValue);
                    }
                }
            }
        }
        if(!it->fsname.empty())
        {
            fn.addVar("fieldset", it->fsname);
            const FieldSet& fs = p.getFieldset(it->fsname);
            if(!fs.pkg.empty())
            {
                fn.addVar("package", fs.pkg);
            }
        }
        for(const auto& prop2 : it->properties)
        {
            for(const auto& field : prop2.fields)
            {
                if(field.pt == ptString)
                {
                    fn.addVar(field.name, field.strValue);
                }
                else if(field.pt == ptBool)
                {
                    fn.addBool(field.name, field.boolValue);
                }
                else
                {
                    fn.addVar(field.name, std::to_string(field.intValue));
                }
            }
        }
    }
}

} // namespace protogen
