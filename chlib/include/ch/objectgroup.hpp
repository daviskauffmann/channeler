#ifndef CH_OBJECTGROUP_HPP
#define CH_OBJECTGROUP_HPP

#include <tinyxml2.h>
#include <vector>

namespace ch
{
    struct objectgroup_object
    {
        std::size_t gid;
        float x;
        float y;
        float rotation;
    };

    struct objectgroup
    {
        std::vector<ch::objectgroup_object> objects;

        objectgroup(const tinyxml2::XMLElement *objectgroup_xml);
    };
}

#endif
