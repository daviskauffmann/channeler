#include <ch/map_tileset.hpp>

#include <ch/tileset.hpp>
#include <ch/world.hpp>

ch::map_tileset::map_tileset(const tinyxml2::XMLElement *tileset_xml, const std::size_t index, ch::world &world)
    : index(index)
{
    first_gid = tileset_xml->Unsigned64Attribute("firstgid");

    const char *source;
    tileset_xml->QueryStringAttribute("source", &source);
    tileset = world.load_tileset("data/" + std::string(source));
}

const ch::tileset_tile &ch::map_tileset::get_tile(const std::size_t gid) const
{
    return tileset->tiles.at(gid - first_gid);
}
