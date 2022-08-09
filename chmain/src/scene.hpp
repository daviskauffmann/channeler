#ifndef SCENE_HPP
#define SCENE_HPP

union SDL_Event;
struct SDL_Renderer;

namespace ch
{
    class scene
    {
    public:
        scene(SDL_Renderer *renderer);
        virtual ~scene();

        virtual void handle_event(SDL_Event *event);
        virtual void update(float delta_time);
        virtual void render();

    protected:
        SDL_Renderer *renderer;
    };

}

#endif
