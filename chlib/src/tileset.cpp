#include <ch/tileset.hpp>

#include <spdlog/spdlog.h>
#include <tinyxml2.h>

ch::tileset::tileset(const std::string &filename)
{
    spdlog::info("Loading tileset {}", filename);

    tinyxml2::XMLDocument xml;
    if (xml.LoadFile(filename.c_str()) != tinyxml2::XML_SUCCESS)
    {
        throw std::runtime_error("Failed to load tileset");
    }

    const auto tileset_xml = xml.FirstChildElement("tileset");
    if (!tileset_xml)
    {
        throw std::runtime_error("Failed to load tileset");
    }

    columns = tileset_xml->Unsigned64Attribute("columns");

    const auto tile_count = tileset_xml->Unsigned64Attribute("tilecount");
    for (std::size_t i = 0; i < tile_count; ++i)
    {
        tiles.push_back({});
    }

    const auto image_xml = tileset_xml->FirstChildElement("image");
    if (image_xml)
    {
        const auto source_attr = image_xml->Attribute("source");
        if (source_attr)
        {
            this->image = "data/" + std::string(source_attr);
        }
    }

    auto tile_xml = tileset_xml->FirstChildElement("tile");

    if (tile_xml)
    {
        for (; tile_xml; tile_xml = tile_xml->NextSiblingElement("tile"))
        {
            const auto id = tile_xml->Unsigned64Attribute("id");

            auto tile = &tiles.at(id);

            tile->index = id;

            const auto tile_image_xml = tile_xml->FirstChildElement("image");
            if (tile_image_xml)
            {
                const auto source_attr = tile_image_xml->Attribute("source");
                if (source_attr)
                {
                    tile->image = "data/" + std::string(source_attr);
                }

                tile->width = tile_image_xml->Unsigned64Attribute("width");
                tile->height = tile_image_xml->Unsigned64Attribute("height");
            }

            const auto tile_properties_xml = tile_xml->FirstChildElement("properties");
            if (tile_properties_xml)
            {
                for (auto tile_property_xml = tile_properties_xml->FirstChildElement("property"); tile_property_xml; tile_property_xml = tile_property_xml->NextSiblingElement("property"))
                {
                    const auto name_attr = tile_property_xml->Attribute("name");
                    if (name_attr)
                    {
                        const auto value_attr = tile_property_xml->Attribute("value");
                        if (value_attr)
                        {
                            if (std::string(name_attr) == "solid")
                            {
                                tile->solid = std::string(value_attr) == "true";
                            }
                        }
                    }
                }
            }
        }
    }
}
