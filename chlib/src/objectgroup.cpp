#include <ch/objectgroup.hpp>

ch::objectgroup::objectgroup(const tinyxml2::XMLElement *objectgroup_xml)
{
    for (auto object_xml = objectgroup_xml->FirstChildElement("object"); object_xml; object_xml = object_xml->NextSiblingElement("object"))
    {
        ch::objectgroup_object object;
        object.gid = object_xml->Unsigned64Attribute("gid");
        object.x = object_xml->FloatAttribute("x");
        object.y = object_xml->FloatAttribute("y");
        object.rotation = object_xml->FloatAttribute("rotation");

        objects.push_back(object);
    }
}
