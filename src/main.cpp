#include <array>
#include <random>

#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

struct Settings {
  bool enabled = true;
  bool dontEnableInEditor = true;
  bool randomizePlayerSize = false;
  bool randomizePlayerGravity = false;
  bool randomizePlayerMirror = false;
  bool dontRandomizePlayerTwoWhenEnteringDual = true;
  bool dontRandomizeInitialGamemode = true;
} static s_settings;

$execute {
  auto *mod = Mod::get();

  s_settings.enabled = mod->getSettingValue<bool>("enabled");
  s_settings.dontEnableInEditor =
      mod->getSettingValue<bool>("dontEnableInEditor");
  s_settings.randomizePlayerSize =
      mod->getSettingValue<bool>("randomizePlayerSize");
  s_settings.randomizePlayerGravity =
      mod->getSettingValue<bool>("randomizePlayerGravity");
  s_settings.randomizePlayerMirror =
      mod->getSettingValue<bool>("randomizePlayerMirror");
  s_settings.dontRandomizePlayerTwoWhenEnteringDual =
      mod->getSettingValue<bool>("dontRandomizePlayerTwoWhenEnteringDual");
  s_settings.dontRandomizeInitialGamemode =
      mod->getSettingValue<bool>("dontRandomizeInitialGamemode");

  listenForSettingChanges<bool>("enabled",
                                [](bool v) { s_settings.enabled = v; });
  listenForSettingChanges<bool>(
      "dontEnableInEditor", [](bool v) { s_settings.dontEnableInEditor = v; });
  listenForSettingChanges<bool>("randomizePlayerSize", [](bool v) {
    s_settings.randomizePlayerSize = v;
  });
  listenForSettingChanges<bool>("randomizePlayerGravity", [](bool v) {
    s_settings.randomizePlayerGravity = v;
  });
  listenForSettingChanges<bool>("randomizePlayerMirror", [](bool v) {
    s_settings.randomizePlayerMirror = v;
  });
  listenForSettingChanges<bool>(
      "dontRandomizePlayerTwoWhenEnteringDual",
      [](bool v) { s_settings.dontRandomizePlayerTwoWhenEnteringDual = v; });
  listenForSettingChanges<bool>("dontRandomizeInitialGamemode", [](bool v) {
    s_settings.dontRandomizeInitialGamemode = v;
  });
}

static bool s_randomizingP1 = false;
static bool s_randomizingP2 = false;
static bool s_forcePassThrough = false;

static int getRandom(int max) {
  static std::mt19937 gen(std::random_device{}());
  return std::uniform_int_distribution<int>(0, max)(gen);
}

struct ModeEntry {
  GameObjectType portalType;
  void (*apply)(PlayerObject *pl, bool enable, bool noEffects);
};

static const std::array<ModeEntry, 7> k_modes = {{
    {GameObjectType::UfoPortal,
     [](PlayerObject *p, bool e, bool n) {
       p->PlayerObject::toggleBirdMode(e, n);
     }},
    {GameObjectType::WavePortal,
     [](PlayerObject *p, bool e, bool n) {
       p->PlayerObject::toggleDartMode(e, n);
     }},
    {GameObjectType::ShipPortal,
     [](PlayerObject *p, bool e, bool n) {
       p->PlayerObject::toggleFlyMode(e, n);
     }},
    {GameObjectType::RobotPortal,
     [](PlayerObject *p, bool e, bool n) {
       p->PlayerObject::toggleRobotMode(e, n);
     }},
    {GameObjectType::BallPortal,
     [](PlayerObject *p, bool e, bool n) {
       p->PlayerObject::toggleRollMode(e, n);
     }},
    {GameObjectType::SpiderPortal,
     [](PlayerObject *p, bool e, bool n) {
       p->PlayerObject::toggleSpiderMode(e, n);
     }},
    {GameObjectType::SwingPortal,
     [](PlayerObject *p, bool e, bool n) {
       p->PlayerObject::toggleSwingMode(e, n);
     }},
}};

static bool shouldPassThrough(PlayerObject *self, GJBaseGameLayer *layer) {
  return s_forcePassThrough || !s_settings.enabled || !layer || !self ||
         (self != layer->m_player1 && self != layer->m_player2) ||
         (layer->m_isEditor && s_settings.dontEnableInEditor) ||
         (self == layer->m_player1 && s_randomizingP1) ||
         (self == layer->m_player2 && s_randomizingP2);
}

static void setRandomizing(PlayerObject *self, GJBaseGameLayer *layer,
                           bool value) {
  if (self == layer->m_player1) {
    s_randomizingP1 = value;
  } else if (self == layer->m_player2) {
    s_randomizingP2 = value;
  }
}

class FrameDefer : public CCNode {
  std::function<void()> m_callback;

public:
  static FrameDefer *schedule(CCNode *parent, std::function<void()> callback) {
    auto *node = new FrameDefer();
    node->m_callback = std::move(callback);
    node->autorelease();
    node->scheduleOnce(schedule_selector(FrameDefer::fire), 0.f);
    parent->addChild(node);
    return node;
  }

  void fire(float) {
    m_callback();
    removeFromParent();
  }
};

static void scheduleGroundAnimation(PlayerObject *self, GJBaseGameLayer *layer,
                                    GameObjectType pickedMode) {
  FrameDefer::schedule(layer, [layer, self, pickedMode]() {
    const bool isPractice =
        !layer->m_isEditor && static_cast<PlayLayer *>(layer)->m_isPracticeMode;

    if (!isPractice) {
      if (s_settings.randomizePlayerMirror) {
        layer->toggleFlipped(getRandom(1), getRandom(1));
      }
      if (s_settings.randomizePlayerGravity) {
        layer->flipGravity(self, getRandom(1), getRandom(1));
      }
      if (s_settings.randomizePlayerSize) {
        self->togglePlayerScale(getRandom(1), getRandom(1));
      }
    }

    const int modeInt = static_cast<int>(pickedMode);
    layer->updateDualGround(self, modeInt, false, 0.5f);

    const bool needsDualGround = pickedMode != GameObjectType::CubePortal &&
                                 pickedMode != GameObjectType::RobotPortal;

    if (needsDualGround && layer->m_gameState.m_lastActivatedPortal1) {
      layer->animateInDualGroundNew(layer->m_gameState.m_lastActivatedPortal1,
                                    layer->getGroundHeight(self, modeInt),
                                    false, 0.5f);
    }
  });
}

static void randomizeAndApply(PlayerObject *self, GJBaseGameLayer *layer,
                              int incomingModeIndex, bool noEffects) {
  setRandomizing(self, layer, true);
  const int pick = getRandom(7);

  GameObjectType picked;

  if (pick == 0) {
    k_modes[incomingModeIndex].apply(self, false, noEffects);
    picked = GameObjectType::CubePortal;
  } else {
    const auto &mode = k_modes[pick - 1];
    mode.apply(self, true, noEffects);
    picked = mode.portalType;
  }

  setRandomizing(self, layer, false);
  scheduleGroundAnimation(self, layer, picked);
}

class $modify(MyGJBaseGameLayer, GJBaseGameLayer) {
  struct Fields {
    ~Fields() {
      s_randomizingP1 = false;
      s_randomizingP2 = false;
      s_forcePassThrough = false;
    }
  };

  void resetPlayer() {
    if (s_settings.dontRandomizeInitialGamemode) {
      s_forcePassThrough = true;
    }

    GJBaseGameLayer::resetPlayer();
    s_forcePassThrough = false;
  }

  void toggleDualMode(GameObject *object, bool dual, PlayerObject *player,
                      bool noEffects) {
    if (s_settings.dontRandomizePlayerTwoWhenEnteringDual) {
      s_forcePassThrough = true;
    }

    GJBaseGameLayer::toggleDualMode(object, dual, player, noEffects);
    s_forcePassThrough = false;
  }
};

class $modify(MyPlayLayer, PlayLayer) {
  void loadFromCheckpoint(CheckpointObject *object) {
    s_forcePassThrough = true;
    PlayLayer::loadFromCheckpoint(object);
    s_forcePassThrough = false;
  }
};

class $modify(MyPlayerObject, PlayerObject) {
  void loadFromCheckpoint(PlayerCheckpoint *object) {
    s_forcePassThrough = true;
    PlayerObject::loadFromCheckpoint(object);
    s_forcePassThrough = false;
  }

  void toggleBirdMode(bool e, bool n) {
    if (shouldPassThrough(this, m_gameLayer)) {
      return PlayerObject::toggleBirdMode(e, n);
    }
    randomizeAndApply(this, m_gameLayer, 0, n);
  }

  void toggleDartMode(bool e, bool n) {
    if (shouldPassThrough(this, m_gameLayer)) {
      return PlayerObject::toggleDartMode(e, n);
    }
    randomizeAndApply(this, m_gameLayer, 1, n);
  }

  void toggleFlyMode(bool e, bool n) {
    if (shouldPassThrough(this, m_gameLayer)) {
      return PlayerObject::toggleFlyMode(e, n);
    }
    randomizeAndApply(this, m_gameLayer, 2, n);
  }

  void toggleRobotMode(bool e, bool n) {
    if (shouldPassThrough(this, m_gameLayer)) {
      return PlayerObject::toggleRobotMode(e, n);
    }
    randomizeAndApply(this, m_gameLayer, 3, n);
  }

  void toggleRollMode(bool e, bool n) {
    if (shouldPassThrough(this, m_gameLayer)) {
      return PlayerObject::toggleRollMode(e, n);
    }
    randomizeAndApply(this, m_gameLayer, 4, n);
  }

  void toggleSpiderMode(bool e, bool n) {
    if (shouldPassThrough(this, m_gameLayer)) {
      return PlayerObject::toggleSpiderMode(e, n);
    }
    randomizeAndApply(this, m_gameLayer, 5, n);
  }

  void toggleSwingMode(bool e, bool n) {
    if (shouldPassThrough(this, m_gameLayer)) {
      return PlayerObject::toggleSwingMode(e, n);
    }
    randomizeAndApply(this, m_gameLayer, 6, n);
  }
};
