#include "squirrel.h"
#include "Actor.h"
#include "Camera.h"
#include "Engine.h"
#include "Lip.h"
#include "Locator.h"
#include "Logger.h"
#include "PathFinder.h"
#include "Preferences.h"
#include "ResourceManager.h"
#include "Room.h"
#include "RoomScaling.h"
#include "ScriptEngine.h"
#include "SoundId.h"
#include "SoundManager.h"
#include "_TalkingState.h"
#include "_Util.h"
#include "PathFinding/_Path.h"

namespace ng
{
struct Actor::Impl
{
    class WalkingState
    {
      public:
        WalkingState() = default;

        void setActor(Actor *pActor);
        void setDestination(const std::vector<sf::Vector2i> &path, std::optional<Facing> facing);
        void update(const sf::Time &elapsed);
        void stop();
        [[nodiscard]] bool isWalking() const { return _isWalking; }

      private:
        Facing getFacing();

      private:
        Actor *_pActor{nullptr};
        std::vector<sf::Vector2i> _path;
        std::optional<Facing> _facing{Facing::FACE_FRONT};
        bool _isWalking{false};
    };

    explicit Impl(Engine &engine)
        : _engine(engine), _settings(engine.getSettings()), _costume(engine.getTextureManager())
    {
        _talkingState.setEngine(&engine);
    }

    void setActor(Actor *pActor)
    {
        _pActor = pActor;
        _walkingState.setActor(pActor);
        _costume.setActor(pActor);
    }

    void drawHotspot(sf::RenderTarget &target, sf::RenderStates states) const
    {
        if (!_hotspotVisible)
            return;

        auto rect = _pActor->getHotspot();

        sf::RectangleShape s(sf::Vector2f(rect.width, rect.height));
        s.setPosition(rect.left, rect.top);
        s.setOutlineThickness(1);
        s.setOutlineColor(sf::Color::Red);
        s.setFillColor(sf::Color::Transparent);
        target.draw(s, states);

        // draw actor position
        sf::RectangleShape rectangle;
        rectangle.setFillColor(sf::Color::Red);
        rectangle.setSize(sf::Vector2f(2, 2));
        rectangle.setOrigin(sf::Vector2f(1, 1));
        target.draw(rectangle, states);
    }

    Engine &_engine;
    Actor *_pActor{nullptr};
    const EngineSettings &_settings;
    Costume _costume;
    std::string _icon;
    bool _useWalkboxes{true};
    Room *_pRoom{nullptr};
    sf::IntRect _hotspot;
    std::vector<Object*> _objects;
    WalkingState _walkingState;
    _TalkingState _talkingState;
    sf::Vector2i _speed{30, 15};
    float _volume{1.f};
    std::shared_ptr<_Path> _path;
    HSQOBJECT _table{};
    bool _hotspotVisible{false};
    std::string _key;
    int _inventoryOffset{0};
    sf::Vector2i _talkOffset{0, 90};
};

std::wstring Actor::getTranslatedName() const
{
    return pImpl->_engine.getText(getName());
}

void Actor::setKey(const std::string &key) { pImpl->_key = key; }

const std::string &Actor::getKey() const { return pImpl->_key; }

void Actor::setIcon(const std::string &icon) { pImpl->_icon = icon; }

const std::string &Actor::getIcon() const { return pImpl->_icon; }

void Actor::useWalkboxes(bool useWalkboxes) { pImpl->_useWalkboxes = useWalkboxes; }

Costume &Actor::getCostume() { return pImpl->_costume; }

Costume &Actor::getCostume() const { return pImpl->_costume; }

void Actor::setTalkColor(sf::Color color) { pImpl->_talkingState.setTalkColor(color); }

sf::Color Actor::getTalkColor() const { return pImpl->_talkingState.getTalkColor(); }

void Actor::setTalkOffset(const sf::Vector2i &offset) { pImpl->_talkOffset = offset; }

void Actor::say(const std::string& text)
{ 
    pImpl->_talkingState.loadLip(text, this);
    auto pos = getRealPosition();
    auto at = pImpl->_engine.getCamera().getAt();
    pos.x = pos.x - at.x + pImpl->_talkOffset.x;
    pos.y = pos.y - at.y - pImpl->_talkOffset.y;
    pImpl->_talkingState.setPosition(pos);
}

void Actor::stopTalking() { pImpl->_talkingState.stop(); }

bool Actor::isTalking() const { return pImpl->_talkingState.isTalking(); }

Room *Actor::getRoom() { return pImpl->_pRoom; }

void Actor::setHotspot(const sf::IntRect &hotspot) { pImpl->_hotspot = hotspot; }

sf::IntRect Actor::getHotspot() const { return pImpl->_hotspot; }

void Actor::showHotspot(bool show) { pImpl->_hotspotVisible = show; }

bool Actor::isHotspotVisible() const { return pImpl->_hotspotVisible; }

bool Actor::contains(const sf::Vector2f &pos) const
{
    auto pAnim = pImpl->_costume.getAnimation();
    if (!pAnim)
        return false;

    auto size = pImpl->_pRoom->getRoomSize();
    auto scale = pImpl->_pRoom->getRoomScaling().getScaling(size.y - getRealPosition().y);
    auto transform = getTransform();
    transform.scale(scale, scale);
    transform.translate((sf::Vector2f)-getRenderOffset() * scale);
    auto t = transform.getInverse();
    auto pos2 = t.transformPoint(pos);
    return pAnim->contains(pos2);
}

void Actor::pickupObject(Object* pObject)
{ 
    pObject->setOwner(this);
    pImpl->_objects.push_back(pObject);
}

void Actor::pickupReplacementObject(Object* pObject1,Object* pObject2)
{ 
    pObject2->setOwner(this);
    auto srcIt = std::find_if(pImpl->_objects.begin(), pImpl->_objects.end(), [&pObject1](auto &pObj){return pObj == pObject1;});
    pImpl->_objects.erase(srcIt);
    pImpl->_objects.push_back(pObject2);
}

void Actor::giveTo(Object* pObject, Actor* pActor)
{
    if(!pObject||!pActor) return;
    pObject->setOwner(pActor);
    auto srcIt = std::find_if(pImpl->_objects.begin(), pImpl->_objects.end(), [&pObject](auto &pObj){return pObj == pObject;});
    std::move(srcIt, srcIt+1, std::inserter(pActor->pImpl->_objects, std::end(pActor->pImpl->_objects)));
    pImpl->_objects.erase(srcIt);
}

void Actor::removeInventory(Object* pObject)
{ 
    if(!pObject) return;
    pObject->setOwner(nullptr);
    pImpl->_objects.erase(std::remove_if(pImpl->_objects.begin(), pImpl->_objects.end(), [&pObject](auto &pObj){return pObj == pObject;}), pImpl->_objects.end());
}

void Actor::clearInventory()
{ 
    for(auto&& obj : pImpl->_objects)
    {
        obj->setOwner(nullptr);
    }
    pImpl->_objects.clear();
}

const std::vector<Object*> &Actor::getObjects() const { return pImpl->_objects; }

void Actor::setWalkSpeed(const sf::Vector2i &speed) { pImpl->_speed = speed; }

const sf::Vector2i &Actor::getWalkSpeed() const { return pImpl->_speed; }

void Actor::stopWalking() { pImpl->_walkingState.stop(); }

bool Actor::isWalking() const { return pImpl->_walkingState.isWalking(); }

void Actor::setVolume(float volume) { pImpl->_volume = volume; }

HSQOBJECT &Actor::getTable() { return pImpl->_table; }
HSQOBJECT &Actor::getTable() const { return pImpl->_table; }

bool Actor::isInventoryObject() const { return false; }

void Actor::Impl::WalkingState::setActor(Actor *pActor) { _pActor = pActor; }

void Actor::Impl::WalkingState::setDestination(const std::vector<sf::Vector2i> &path, std::optional<Facing> facing)
{
    _path = path;
    _facing = facing;
    _path.erase(_path.begin());
    _pActor->getCostume().setFacing(getFacing());
    _pActor->getCostume().setState("walk");
    _pActor->getCostume().getAnimation()->play(true);
    _isWalking = true;
    trace("{} go to : {},{}", _pActor->getName(), _path[0].x, _path[0].y);
}

void Actor::Impl::WalkingState::stop()
{ 
    _isWalking = false;
    ScriptEngine::call(_pActor, "postWalking");
}

Facing Actor::Impl::WalkingState::getFacing()
{
    auto pos = _pActor->getRealPosition();
    auto dx = _path[0].x - pos.x;
    auto dy = _path[0].y - pos.y;
    if (fabs(dx) > fabs(dy))
        return (dx > 0) ? Facing::FACE_RIGHT : Facing::FACE_LEFT;
    return (dy > 0) ? Facing::FACE_FRONT : Facing::FACE_BACK;
}

void Actor::Impl::WalkingState::update(const sf::Time &elapsed)
{
    if (!_isWalking)
        return;

    auto pos = _pActor->getRealPosition();
    auto delta = (_path[0] - (sf::Vector2i)pos);
    auto speed = _pActor->getWalkSpeed();
    if(_pActor->pImpl->_engine.actorShouldRun())
    {
        speed *= 4;
    }
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
    _pActor->setPosition(pos + offset);
    if (fabs(_path[0].x - pos.x) <= 1 && fabs(_path[0].y - pos.y) <= 1)
    {
        _path.erase(_path.begin());
        if (_path.empty())
        {
            stop();
            trace("Play anim stand");
            if (_facing.has_value())
            {
                _pActor->getCostume().setFacing(_facing.value());
            }
            _pActor->getCostume().setState("stand");
        }
        else
        {
            _pActor->getCostume().setFacing(getFacing());
            _pActor->getCostume().setState("walk");
            _pActor->getCostume().getAnimation()->play(true);
            trace("{} go to : {},{}", _pActor->getName(), _path[0].x, _path[0].y);
        }
    }
}

Actor::Actor(Engine &engine) : pImpl(std::make_unique<Impl>(engine))
{ 
    pImpl->setActor(this); 
    _id = Locator::getResourceManager().getActorId();
}

Actor::~Actor() = default;

const Room *Actor::getRoom() const { return pImpl->_pRoom; }

int Actor::getZOrder() const { return static_cast<int>(getRoom()->getRoomSize().y - getRealPosition().y); }

void Actor::setRoom(Room *pRoom)
{
    if (pImpl->_pRoom)
    {
        pImpl->_pRoom->removeEntity(this);
    }
    pImpl->_pRoom = pRoom;
    pImpl->_pRoom->setAsParallaxLayer(this, 0);
}

void Actor::setCostume(const std::string &name, const std::string &sheet)
{
    std::string path;
    path.append(name).append(".json");
    pImpl->_costume.loadCostume(path, sheet);
}

float Actor::getScale() const
{
    auto size = pImpl->_pRoom->getRoomSize();
    return pImpl->_pRoom->getRoomScaling().getScaling(size.y - getRealPosition().y);
}

void Actor::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
    if (!isVisible())
        return;

    auto scale = getScale();
    auto transform = getTransform();
    transform.scale(scale, scale);
    transform.translate(getRenderOffset().x, -getRenderOffset().y);
    states.transform *= transform;
    target.draw(pImpl->_costume, states);

    pImpl->drawHotspot(target, states);
}

void Actor::drawForeground(sf::RenderTarget &target, sf::RenderStates states) const
{
    if (pImpl->_path && pImpl->_pRoom && pImpl->_engine.areDrawWalkboxesVisible())
    {
        target.draw(*pImpl->_path, states);
    }

    if (!pImpl->_talkingState.isTalking())
        return;

    sf::RenderStates s;
    target.draw(pImpl->_talkingState, s);
}

void Actor::update(const sf::Time &elapsed)
{
    Entity::update(elapsed);
    pImpl->_costume.update(elapsed);
    pImpl->_walkingState.update(elapsed);
    pImpl->_talkingState.update(elapsed);
}

void Actor::walkTo(const sf::Vector2f &destination, std::optional<Facing> facing)
{
    if (pImpl->_pRoom == nullptr)
        return;

    std::vector<sf::Vector2i> path;
    if(pImpl->_useWalkboxes)
    {
        path = pImpl->_pRoom->calculatePath((sf::Vector2i)getRealPosition(), (sf::Vector2i)destination);
        pImpl->_path = std::make_unique<_Path>(path);
        if (path.size() < 2)
            return;
    }
    else
    {
        path.push_back((sf::Vector2i)getRealPosition());
        path.push_back((sf::Vector2i)destination);
        pImpl->_path = std::make_unique<_Path>(path);
    }

    ScriptEngine::call(this, "preWalking");
    pImpl->_walkingState.setDestination(path, facing);
}

void Actor::trigSound(const std::string &name)
{
    auto soundId = pImpl->_engine.getSoundDefinition(name);
    if (!soundId)
        return;
    pImpl->_engine.getSoundManager().playSound(soundId);
}

void Actor::setFps(int fps)
{
    auto pAnim = pImpl->_costume.getAnimation();
    if (pAnim)
    {
        pAnim->setFps(fps);
    }
}

void Actor::setInventoryOffset(int offset) { pImpl->_inventoryOffset = offset; }

int Actor::getInventoryOffset() const{ return pImpl->_inventoryOffset; }

} // namespace ng
