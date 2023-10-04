#include <ch/layer.hpp>

#include <sstream>
#include <string>

ch::layer::layer(const tinyxml2::XMLElement *layer_xml)
{
    width = layer_xml->Unsigned64Attribute("width");
    height = layer_xml->Unsigned64Attribute("height");

    const auto data_xml = layer_xml->FirstChildElement("data");
    if (data_xml)
    {
        const auto data = data_xml->GetText();
        if (data)
        {
            std::stringstream ss(data);
            std::string token;
            while (std::getline(ss, token, ','))
            {
                const std::int64_t gid = std::stoll(token);
                constexpr std::uint32_t h_flip_flag = 0x80000000;
                constexpr std::uint32_t v_flip_flag = 0x40000000;
                constexpr std::uint32_t d_flip_flag = 0x20000000;

                ch::layer_tile tile;
                tile.gid = gid & ~(h_flip_flag | v_flip_flag | d_flip_flag);
                tile.h_flip = gid & h_flip_flag;
                tile.v_flip = gid & v_flip_flag;
                tile.d_flip = gid & d_flip_flag;

                tiles.push_back(tile);
            }
        }
    }

    const auto properties_xml = layer_xml->FirstChildElement("properties");
    if (properties_xml)
    {
        for (auto property_xml = properties_xml->FirstChildElement("property"); property_xml; property_xml = property_xml->NextSiblingElement("property"))
        {
            const auto name = property_xml->Attribute("name");
            if (name)
            {
                const auto value = property_xml->Attribute("value");
                if (value)
                {
                    if (std::string(name) == "depth")
                    {
                        depth = std::string(value) == "true";
                    }
                }
            }
        }
    }
}

const ch::layer_tile *ch::layer::get_tile(const std::size_t x, const std::size_t y) const
{
    if (x < width && y < height)
    {
        const auto tile = &tiles.at(x + y * width);
        if (tile->gid)
        {
            return tile;
        }
    }

    return nullptr;
}
