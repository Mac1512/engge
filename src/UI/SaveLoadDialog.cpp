#include <filesystem>
#include "UI/SaveLoadDialog.hpp"
#include "_ControlConstants.hpp"
#include "Engine/Engine.hpp"
#include "Font/FntFont.hpp"
#include "Graphics/Screen.hpp"
#include "Graphics/SpriteSheet.hpp"
#include "Graphics/Text.hpp"
#include "imgui.h"

namespace ng {
struct SaveLoadDialog::Impl {
  class _BackButton final : public sf::Drawable {
  private:
    inline static const int BackId = 99904;

  public:
    typedef std::function<void()> Callback;

  public:
    void setCallback(Callback callback) {
      _callback = std::move(callback);
    }

    void setEngine(Engine *pEngine) {
      _pEngine = pEngine;

      const FntFont &uiFontLarge = _pEngine->getTextureManager().getFntFont("UIFontLarge.fnt");
      _text.setFont(uiFontLarge);
      _text.setString(_pEngine->getText(BackId));
      auto textRect = _text.getGlobalBounds();
      _text.setOrigin(sf::Vector2f(textRect.width / 2.f, textRect.height / 2.f));
      _text.setPosition(sf::Vector2f(Screen::Width / 2.0f, 650.f));
    }

    void update(sf::Vector2f pos) {
      auto textRect = _text.getGlobalBounds();
      sf::Color color;
      if (textRect.contains((sf::Vector2f) pos)) {
        color = _ControlConstants::HoveColor;
        bool isDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
        ImGuiIO &io = ImGui::GetIO();
        if (!io.WantCaptureMouse && _wasMouseDown && !isDown && _callback) {
          _callback();
        }
        _wasMouseDown = isDown;
      } else {
        color = _ControlConstants::NormalColor;
      }
      _text.setFillColor(color);
    }

  private:
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override {
      target.draw(_text, states);
    }

  private:
    Engine *_pEngine{nullptr};
    bool _wasMouseDown{false};
    Callback _callback{nullptr};
    Text _text;
  };

  class _Slot final : public sf::Drawable, public sf::Transformable {
  private:
    inline static const int AutosaveId = 99901;

  public:
    void init(SavegameSlot& slot, SpriteSheet &spriteSheet, Engine& engine) {
      _index = slot.slot - 1;
      auto x = _index % 3;
      auto y = _index / 3;
      setPosition({168.f + 39.f * 4.f + 78.f * 4.f * x + 4.f * x, 92.f + 22.f * 4.f + 44.f * 4.f * y + 4.f * y});

      const auto &uiFontSmallBold = engine.getTextureManager().getFntFont("UIFontSmallBold.fnt");

      _gameTimeText.setString(slot.getGameTimeString());
      _gameTimeText.setFont(uiFontSmallBold);
      _gameTimeText.setFillColor(sf::Color::White);
      _gameTimeText.setPosition(0.f, -64.f);

      auto gameTimeSize = _gameTimeText.getGlobalBounds();
      _gameTimeText.setOrigin(static_cast<float>(gameTimeSize.width / 2), static_cast<float>(gameTimeSize.height / 2));

      sf::String saveTimeText;
      if(slot.slot == 1) {
        saveTimeText = engine.getText(AutosaveId);
      } else {
        saveTimeText = slot.getSaveTimeString();
      }
      _saveTimeText.setString(saveTimeText);
      _saveTimeText.setFont(uiFontSmallBold);
      _saveTimeText.setFillColor(sf::Color::White);
      _saveTimeText.setPosition(0.f, 64.f);

      auto saveTimeSize = _saveTimeText.getGlobalBounds();
      _saveTimeText.setOrigin(static_cast<float>(saveTimeSize.width / 2), static_cast<float>(saveTimeSize.height / 2));

      auto rect = spriteSheet.getRect("saveload_slot_frame");
      _sprite.setTexture(spriteSheet.getTexture());
      _sprite.setOrigin(static_cast<float>(rect.width / 2.f), static_cast<float>(rect.height / 2.f));
      _sprite.setScale(4, 4);
      _sprite.setTextureRect(rect);

      std::ostringstream s;
      s << "Savegame" << (_index + 1) << ".png";

      _isEmpty = !std::filesystem::exists(s.str());
      if (!_isEmpty) {
        _texture.loadFromFile(s.str());
        _spriteImg.setTexture(_texture, true);
        _spriteImg.setOrigin(160.f, 90.f);
        auto size = _texture.getSize();
        _spriteImg.setScale((rect.width * 4.f) / size.x, (rect.height * 4.f) / size.y);
        return;
      }

      auto saveslotRect = spriteSheet.getRect("saveload_slot");
      _spriteImg.setTextureRect(saveslotRect);
      _spriteImg.setTexture(spriteSheet.getTexture());
      _spriteImg.setOrigin(static_cast<float>(saveslotRect.width / 2.f), static_cast<float>(saveslotRect.height / 2.f));
      _spriteImg.setScale(4.f, 4.f);
    }

    bool contains(const sf::Vector2f &pos) const {
      auto trsf = getTransform();
      trsf.translate(-156.f, -88.f);
      return trsf.transformRect(_rect).contains(pos);
    }

    inline bool isEmpty() const {
      return _isEmpty;
    }

  private:
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override {
      states.transform *= getTransform();
      target.draw(_spriteImg, states);
      target.draw(_sprite, states);
      if(!_isEmpty) {
        target.draw(_gameTimeText, states);
        target.draw(_saveTimeText, states);
      }
    }

  private:
    int _index{0};
    bool _isEmpty{true};
    sf::Texture _texture;
    sf::Sprite _sprite, _spriteImg;
    Text _gameTimeText;
    Text _saveTimeText;
    sf::FloatRect _rect{0, 0, 78 * 4, 44 * 4};
  };

  inline static const int LoadGameId = 99910;
  inline static const int SaveGameId = 99911;

  Engine *_pEngine{nullptr};
  SpriteSheet _saveLoadSheet;
  Text _headingText;
  SaveLoadDialog::Impl::_BackButton _backButton;
  Callback _callback{nullptr};
  SlotCallback _slotCallback{nullptr};
  std::array<_Slot, 9> _slots;
  bool _wasMouseDown{false};
  bool _saveMode{false};

  void setHeading(bool saveMode) {
    _headingText.setString(_pEngine->getText(saveMode?SaveGameId : LoadGameId));
    auto textRect = _headingText.getGlobalBounds();
    _headingText.setOrigin(sf::Vector2f(textRect.width / 2.f, textRect.height / 2.f));
    _headingText.setPosition(sf::Vector2f(Screen::Width / 2.f, 54.f));
  }

  void updateState() {
    std::vector<SavegameSlot> slots;
    ng::Engine::getSlotSavegames(slots);

    for (int i = 0; i < static_cast<int>(_slots.size()); ++i) {
      _slots[i].init(slots[i], _saveLoadSheet, *_pEngine);
    }

    _wasMouseDown = false;
    setHeading(_saveMode);

    _backButton.setCallback([this]() {
      if (_callback)
        _callback();
    });

    _backButton.setEngine(_pEngine);
  }

  void setEngine(Engine *pEngine) {
    _pEngine = pEngine;
    if (!pEngine)
      return;

    ResourceManager &tm = pEngine->getTextureManager();
    _saveLoadSheet.setTextureManager(&tm);
    _saveLoadSheet.load("SaveLoadSheet");

    const FntFont &headingFont = _pEngine->getTextureManager().getFntFont("HeadingFont.fnt");
    _headingText.setFont(headingFont);
    _headingText.setFillColor(sf::Color::White);
  }

  void draw(sf::RenderTarget &target, sf::RenderStates) {
    const auto view = target.getView();
    auto viewRect = sf::FloatRect(0, 0, 320, 180);
    target.setView(sf::View(viewRect));

    sf::Color backColor{0, 0, 0, 128};
    sf::RectangleShape fadeShape;
    fadeShape.setSize(sf::Vector2f(viewRect.width, viewRect.height));
    fadeShape.setFillColor(backColor);
    target.draw(fadeShape);

    // draw background
    auto viewCenter = sf::Vector2f(viewRect.width / 2, viewRect.height / 2);
    auto rect = _saveLoadSheet.getRect("saveload");
    sf::Sprite sprite;
    sprite.setPosition(viewCenter);
    sprite.setTexture(_saveLoadSheet.getTexture());
    sprite.setOrigin(static_cast<float>(rect.width / 2.f), static_cast<float>(rect.height / 2.f));
    sprite.setTextureRect(rect);
    target.draw(sprite);

    viewRect = sf::FloatRect(0, 0, Screen::Width, Screen::Height);
    target.setView(sf::View(viewRect));

    // heading
    target.draw(_headingText);

    // slots
    for (auto &slot : _slots) {
      target.draw(slot);
    }

    // back button
    target.draw(_backButton);

    target.setView(view);
  }

  void update(const sf::Time &) {
    auto pos = (sf::Vector2f) _pEngine->getWindow().mapPixelToCoords(sf::Mouse::getPosition(_pEngine->getWindow()),
                                                                     sf::View(sf::FloatRect(0,
                                                                                            0,
                                                                                            Screen::Width,
                                                                                            Screen::Height)));
    _backButton.update(pos);

    bool isDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    const ImGuiIO &io = ImGui::GetIO();
    if (!io.WantCaptureMouse && _wasMouseDown && !isDown) {
      int i = 0;
      for (auto &slot : _slots) {
        if (slot.contains(pos)) {
          if ((_saveMode || !slot.isEmpty()) && _slotCallback) {
            _slotCallback(i + 1);
          }
          return;
        }
        i++;
      }
    }
    _wasMouseDown = isDown;
  }
};

SaveLoadDialog::SaveLoadDialog()
    : _pImpl(std::make_unique<Impl>()) {
}

SaveLoadDialog::~SaveLoadDialog() = default;

void SaveLoadDialog::setEngine(Engine *pEngine) { _pImpl->setEngine(pEngine); }

void SaveLoadDialog::draw(sf::RenderTarget &target, sf::RenderStates states) const {
  _pImpl->draw(target, states);
}

void SaveLoadDialog::update(const sf::Time &elapsed) {
  _pImpl->update(elapsed);
}

void SaveLoadDialog::setCallback(Callback callback) {
  _pImpl->_callback = std::move(callback);
}

void SaveLoadDialog::setSlotCallback(SlotCallback callback) {
  _pImpl->_slotCallback = std::move(callback);
}

void SaveLoadDialog::updateLanguage() {
  _pImpl->updateState();
}

void SaveLoadDialog::setSaveMode(bool saveMode) {
  _pImpl->_saveMode = saveMode;
  _pImpl->setHeading(saveMode);
}

bool SaveLoadDialog::getSaveMode() const { return _pImpl->_saveMode;}
}