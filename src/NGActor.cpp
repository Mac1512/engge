#include "NGEngine.h"
#include "NGActor.h"
#include "NGRoom.h"

namespace ng
{
NGActor::WalkingState::WalkingState(NGActor &actor)
    : _actor(actor), _isWalking(false)
{
}

void NGActor::WalkingState::setDestination(const sf::Vector2f &destination, Facing facing)
{
    _destination = destination;
    _facing = facing;
    auto pos = _actor.getPosition();
    _actor.getCostume().setFacing(((_destination.x - pos.x) > 0) ? Facing::FACE_RIGHT : Facing::FACE_LEFT);
    _actor.getCostume().setState("walk");
    _actor.getCostume().getAnimation()->play(true);
    _isWalking = true;
}

void NGActor::WalkingState::update(const sf::Time &elapsed)
{
    if (!_isWalking)
        return;

    auto pos = _actor.getPosition();
    auto delta = (_destination - pos);
    auto speed = _actor.getWalkSpeed();
    auto offset = sf::Vector2f(speed) * elapsed.asSeconds();
    if (delta.x > 0)
    {
        if (offset.x > delta.x)
            offset.x = delta.x;
    }
    else
    {
        offset.x = -offset.x;
        if (offset.x < delta.x)
            offset.x = delta.x;
    }
    if (delta.y < 0)
    {
        offset.y = -offset.y;
        if (offset.y < delta.y)
            offset.y = delta.y;
    }
    else
    {
        if (offset.y > delta.y)
            offset.y = delta.y;
    }
    _actor.setPosition(pos + offset);
    if (fabs(_destination.x - pos.x) <= 1 && fabs(_destination.y - pos.y) <= 1)
    {
        _isWalking = false;
        std::cout << "Play anim stand" << std::endl;
        _actor.getCostume().setState("stand");
        _actor.getCostume().setFacing(_facing);
    };
}

NGActor::TalkingState::TalkingState(NGActor &actor)
    : _actor(actor), _isTalking(false),
      _talkColor(sf::Color::White), _index(0)
{
    _font.setSettings(&_actor._engine.getSettings());
    _font.setTextureManager(&_actor._engine.getTextureManager());
    _font.load("FontModernSheet");
}

static std::string str_toupper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::toupper(c); } // correct
    );
    return s;
}

void NGActor::TalkingState::say(int id)
{
    std::string name = str_toupper(_actor.getName()).append("_").append(std::to_string(id));
    auto soundDefinition = _actor._engine.defineSound(name + ".ogg");
    if (!soundDefinition)
        return;
    _actor._engine.playSound(*soundDefinition);

    std::string path;
    path.append(_actor._engine.getSettings().getGamePath()).append(name).append(".lip");
    std::cout << "load lip " << path << std::endl;
    _lip.load(path);

    _sayText = _actor._engine.getText(id);
    _isTalking = true;
    _clock.restart();
}

void NGActor::TalkingState::update(const sf::Time &elapsed)
{
    if (!_isTalking)
        return;

    auto time = _lip.getData()[_index + 1].time;
    if (_clock.getElapsedTime() > time)
    {
        _index = _index + 1;
    }
    if (_index == _lip.getData().size())
    {
        _isTalking = false;
        return;
    }
    auto index = _lip.getData()[_index].letter - 'A';
    // TODO: what is the correspondance between letter and head index ?
    _actor.getCostume().setHeadIndex(index % 6);
}

void NGActor::TalkingState::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
    sf::Transformable t;
    t.move((sf::Vector2f)-_talkOffset);
    states.transform *= t.getTransform();

    NGText text;
    text.setFont(_font);
    text.setColor(_talkColor);
    text.setText(_sayText);
    target.draw(text, states);
}

NGActor::NGActor(NGEngine &engine)
    : _engine(engine),
      _settings(engine.getSettings()),
      _costume(engine.getTextureManager()),
      _color(sf::Color::White),
      _zorder(0),
      _isVisible(true),
      _use(true),
      _pRoom(nullptr),
      _walkingState(*this),
      _talkingState(*this),
      _speed(30, 15)
{
}

NGActor::~NGActor() = default;

int NGActor::getZOrder() const
{
    return getRoom()->getRoomSize().y - getPosition().y;
}

void NGActor::setRoom(NGRoom *pRoom)
{
    _pRoom = pRoom;
    _pRoom->setAsParallaxLayer(this, 0);
}

void NGActor::move(const sf::Vector2f &offset)
{
    _transform.move(offset);
}

void NGActor::setPosition(const sf::Vector2f &pos)
{
    _transform.setPosition(pos);
}

void NGActor::setCostume(const std::string &name, const std::string &sheet)
{
    std::string path(_settings.getGamePath());
    path.append(name).append(".json");
    _costume.loadCostume(path, sheet);
}

void NGActor::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
    auto actorTransform = states.transform;
    auto transform = _transform;
    transform.move((sf::Vector2f)-_renderOffset);
    states.transform *= transform.getTransform();
    target.draw(_costume, states);
    if (!_talkingState.isTalking())
        return;

    states.transform = actorTransform * _transform.getTransform();
    _talkingState.draw(target, states);
}

void NGActor::update(const sf::Time &elapsed)
{
    _costume.update(elapsed);
    _walkingState.update(elapsed);
    _talkingState.update(elapsed);
}

void NGActor::walkTo(const sf::Vector2f &destination, Facing facing)
{
    _walkingState.setDestination(destination, facing);
}

void NGActor::walkTo(const sf::Vector2f &destination)
{
    _walkingState.setDestination(destination, getCostume().getFacing());
}

} // namespace ng