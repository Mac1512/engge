#include "GGObject.h"
#include "Screen.h"

namespace gg
{
GGAnimation::GGAnimation(const sf::Texture &texture, const std::string &name)
    : _sprite(texture), _name(name), _fps(10), _index(0), _state(AnimState::Play)
{
}

GGAnimation::~GGAnimation() = default;

void GGAnimation::reset()
{ 
    _index = 0; 
    if (_rects.empty())
        return;
    auto &sourceRect = _sourceRects[_index];
    _sprite.setTextureRect(_rects[_index]);
    _sprite.setOrigin(-sourceRect.left, -sourceRect.top);
}

void GGAnimation::update(const sf::Time &elapsed)
{
    if (_state == AnimState::Pause)
        return;

    if (_rects.empty())
        return;

    _time += elapsed;
    if (_time.asSeconds() > (1.f / _fps))
    {
        _time = sf::seconds(0);
        _index = (_index + 1) % _rects.size();

        auto &sourceRect = _sourceRects[_index];
        _sprite.setTextureRect(_rects[_index]);
        _sprite.setOrigin(sf::Vector2f(-sourceRect.left, -sourceRect.top));
        // _sprite.setOrigin(sf::Vector2f(sourceRect.width / 2.f, sourceRect.height / 2.f));
    }
}

void GGAnimation::draw(sf::RenderWindow &window, const sf::RenderStates &states) const
{
    window.draw(_sprite, states);
}
} // namespace gg