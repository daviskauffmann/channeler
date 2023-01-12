#include <ch/tileset.hpp>

#include <spdlog/spdlog.h>
#include <tinyxml2.h>

ch::tileset::tileset(const std::string &filename)
{
    spdlog::info("Loading tileset {}", filename);

    tinyxml2::XMLDocument tileset_xml;
    if (tileset_xml.LoadFile(filename.c_str()) != tinyxml2::XML_SUCCESS)
    {
        throw std::runtime_error("Failed to load tileset");
    }

    const auto root = tileset_xml.FirstChildElement("tileset");
    if (!root)
    {
        throw std::runtime_error("Failed to load tileset");
    }

    columns = root->Unsigned64Attribute("columns");

    const auto image_xml = root->FirstChildElement("image");
    if (image_xml)
    {
        const auto source_attr = image_xml->Attribute("source");
        if (source_attr)
        {
            this->image = "assets/" + std::string(source_attr);
        }
    }

    for (auto tile_xml = root->FirstChildElement("tile"); tile_xml; tile_xml = tile_xml->NextSiblingElement("tile"))
    {
        ch::tileset_tile tile;

        tile.index = tile_xml->Unsigned64Attribute("id");

        const auto tile_image_xml = tile_xml->FirstChildElement("image");
        if (tile_image_xml)
        {
            const auto source_attr = tile_image_xml->Attribute("source");
            if (source_attr)
            {
                tile.image = "assets/" + std::string(source_attr);
            }

            tile.width = tile_image_xml->Unsigned64Attribute("width");
            tile.height = tile_image_xml->Unsigned64Attribute("height");
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
                            tile.solid = std::string(value_attr) == "true";
                        }
                    }
                }
            }
        }

        tiles.push_back(tile);
    }
}
