#include <ch/map.hpp>

#include <algorithm>
#include <ch/tileset.hpp>
#include <ch/world.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <tinyxml2.h>

ch::map::map(const std::string &filename, ch::world &world)
    : filename(filename)
{
    spdlog::info("Loading map {}", filename);

    tinyxml2::XMLDocument map_xml;
    if (map_xml.LoadFile(filename.c_str()) != tinyxml2::XML_SUCCESS)
    {
        throw std::runtime_error("Failed to load map");
    }

    const auto root = map_xml.FirstChildElement("map");
    if (!root)
    {
        throw std::runtime_error("Failed to load map");
    }

    width = root->Unsigned64Attribute("width");
    height = root->Unsigned64Attribute("height");
    tile_width = root->Unsigned64Attribute("tilewidth");
    tile_height = root->Unsigned64Attribute("tileheight");

    std::size_t index = 0;
    for (auto tileset_xml = root->FirstChildElement("tileset"); tileset_xml; tileset_xml = tileset_xml->NextSiblingElement("tileset"))
    {
        tilesets.push_back({tileset_xml, index++, world});
    }

    for (auto layer_xml = root->FirstChildElement("layer"); layer_xml; layer_xml = layer_xml->NextSiblingElement("layer"))
    {
        layers.push_back({layer_xml});
    }

    for (auto objectgroup_xml = root->FirstChildElement("objectgroup"); objectgroup_xml; objectgroup_xml = objectgroup_xml->NextSiblingElement("objectgroup"))
    {
        objectgroups.push_back({objectgroup_xml});
    }

    b2_world = std::make_unique<b2World>(b2Vec2(0.0f, 0.0f));

    for (std::size_t y = 0; y < height; ++y)
    {
        for (std::size_t x = 0; x < width; ++x)
        {
            if (is_solid(x, y))
            {
                b2BodyDef body_def;
                body_def.type = b2_staticBody;
                body_def.position.Set(
                    static_cast<float>(x * tile_width),
                    static_cast<float>(y * tile_height));

                b2PolygonShape shape;
                shape.SetAsBox(tile_width / 2.0f, tile_height / 2.0f);

                auto body = b2_world->CreateBody(&body_def);
                body->CreateFixture(&shape, 0.0f);
            }
        }
    }
}

const ch::map_tileset &ch::map::get_tileset(const size_t gid) const
{
    return *std::max_element(
        tilesets.begin(),
        tilesets.end(),
        [gid](const auto &a, const auto &b)
        {
            return a.first_gid < b.first_gid && b.first_gid <= gid;
        });
}

bool ch::map::is_solid(const std::size_t x, const std::size_t y) const
{
    const auto layer = std::find_if(
        layers.begin(),
        layers.end(),
        [this, x, y](const auto &layer)
        {
            const auto tile = layer.get_tile(x, y);
            return tile && get_tileset(tile->gid).get_tile(tile->gid).solid;
        });

    return layer != layers.end();
}

void ch::map::update(const float delta_time)
{
    constexpr auto physics_delta_time = 1.0f / 60.0f;
    physics_time_accumulator += delta_time;
    while (physics_time_accumulator >= physics_delta_time)
    {
        physics_time_accumulator -= physics_delta_time;

        b2_world->Step(physics_delta_time, 8, 3);
    }
}
