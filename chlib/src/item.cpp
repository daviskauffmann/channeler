#include <ch/item.hpp>
#include <ch/player.hpp>

ch::item::item(const nlohmann::json &item_json)
{
    name = item_json.at("name");
    sprite_filename = item_json.at("sprite_filename");
    width = item_json.at("width");
    height = item_json.at("height");

    const auto attack_positions_json = item_json.at("attack_positions");
    attack_positions.at(static_cast<std::size_t>(ch::direction::down)) = {
        attack_positions_json.at("down").at("x_offset"),
        attack_positions_json.at("down").at("y_offset"),
        attack_positions_json.at("down").at("angle")};
    attack_positions.at(static_cast<std::size_t>(ch::direction::up)) = {
        attack_positions_json.at("up").at("x_offset"),
        attack_positions_json.at("up").at("y_offset"),
        attack_positions_json.at("up").at("angle")};
    attack_positions.at(static_cast<std::size_t>(ch::direction::left)) = {
        attack_positions_json.at("left").at("x_offset"),
        attack_positions_json.at("left").at("y_offset"),
        attack_positions_json.at("left").at("angle")};
    attack_positions.at(static_cast<std::size_t>(ch::direction::right)) = {
        attack_positions_json.at("right").at("x_offset"),
        attack_positions_json.at("right").at("y_offset"),
        attack_positions_json.at("right").at("angle")};
    const std::string attack_sprite_filename_string = item_json.at("attack_sprite_filename");
    attack_sprite_filename = "assets/" + attack_sprite_filename_string;
    const std::string attack_sound_filename_string = item_json.at("attack_sound_filename");
    attack_sound_filename = "assets/" + attack_sound_filename_string;
}
