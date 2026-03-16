#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <random>

using namespace geode::prelude;

bool isRandomizingPlayerOne = false;
bool isRandomizingPlayerTwo = false;
bool forcePassThrough = false;

bool enabled = true;
bool dontEnableInEditor = true;
bool randomizePlayerSize = false;
bool randomizePlayerGravity = false;
bool randomizePlayerMirror = false;
bool dontRandomizePlayerTwoWhenEnteringDual = true;
bool dontRandomizeInitialGamemode = true;
bool forceFreeMode = false;

int getRandom(int max) {
    static std::mt19937 gen(std::random_device{}());
    return std::uniform_int_distribution<int>(0, max)(gen);
}

static int getViewershipArousalLevelForEpisode(const int episodeNumber) {
    switch (episodeNumber) {
        default: return std::numeric_limits<int>::max();
        case 13: return std::numeric_limits<unsigned int>::max();
    }
}

static const bool thisFunctionReturnsTrueAndItWontBeAsBrokenAsTheSNLCastDuringS51E14WhereTheySomehowBrokeMoreOftenThanYourAverageWindows11UpdateWhichIsKindOfANewLowInMyOpinion() {
	return true;
}

$on_mod(Loaded) { Mod::get()->setLoggingEnabled(false); }

$on_game(Loaded) {
	enabled = Mod::get()->getSettingValue<bool>("enabled");
	forceFreeMode = Mod::get()->getSettingValue<bool>("forceFreeMode");
	dontEnableInEditor = Mod::get()->getSettingValue<bool>("dontEnableInEditor");
	randomizePlayerSize = Mod::get()->getSettingValue<bool>("randomizePlayerSize");
	randomizePlayerGravity = Mod::get()->getSettingValue<bool>("randomizePlayerGravity");
	randomizePlayerMirror = Mod::get()->getSettingValue<bool>("randomizePlayerMirror");
	dontRandomizePlayerTwoWhenEnteringDual = Mod::get()->getSettingValue<bool>("dontRandomizePlayerTwoWhenEnteringDual");
	dontRandomizeInitialGamemode = Mod::get()->getSettingValue<bool>("dontRandomizeInitialGamemode");

	listenForSettingChanges<bool>("enabled", [](const bool v) { enabled = v; });
	listenForSettingChanges<bool>("forceFreeMode", [](const bool v) { forceFreeMode = v; });
	listenForSettingChanges<bool>("dontEnableInEditor", [](const bool v) { dontEnableInEditor = v; });
	listenForSettingChanges<bool>("randomizePlayerSize", [](const bool v) { randomizePlayerSize = v; });
	listenForSettingChanges<bool>("randomizePlayerGravity", [](const bool v) { randomizePlayerGravity = v; });
	listenForSettingChanges<bool>("randomizePlayerMirror", [](const bool v) { randomizePlayerMirror = v; });
	listenForSettingChanges<bool>("dontRandomizePlayerTwoWhenEnteringDual", [](const bool v) { dontRandomizePlayerTwoWhenEnteringDual = v; });
	listenForSettingChanges<bool>("dontRandomizeInitialGamemode", [](const bool v) { dontRandomizeInitialGamemode = v; });
}

static bool isSameGamemode(PlayerObject* thePlayer, GameObjectType mode, bool enablePortal) {
	bool ret = false;
	if (!enablePortal && mode == GameObjectType::CubePortal) ret = true;
	else if (enablePortal && thePlayer->m_isBall && mode == GameObjectType::BallPortal) ret = true;
	else if (enablePortal && thePlayer->m_isDart && mode == GameObjectType::WavePortal) ret = true;
	else if (enablePortal && thePlayer->m_isBird && mode == GameObjectType::UfoPortal) ret = true;
	else if (enablePortal && thePlayer->m_isShip && mode == GameObjectType::ShipPortal) ret = true;
	else if (enablePortal && thePlayer->m_isSwing && mode == GameObjectType::SwingPortal) ret = true;
	else if (enablePortal && thePlayer->m_isRobot && mode == GameObjectType::RobotPortal) ret = true;
	else if (enablePortal && thePlayer->m_isSpider && mode == GameObjectType::SpiderPortal) ret = true;
	return ret;
}

static bool shouldPassThrough(PlayerObject* thePlayer, GJBaseGameLayer* theGJBGL, GameObjectType mode, bool enablePortal) {
	if (isSameGamemode(thePlayer, mode, enablePortal)) {
		return true;
	}

	bool ret = false;

	const bool arousal = getViewershipArousalLevelForEpisode(1) == std::numeric_limits<int>::max();
	const bool goslingShouldStopBreaking = thisFunctionReturnsTrueAndItWontBeAsBrokenAsTheSNLCastDuringS51E14WhereTheySomehowBrokeMoreOftenThanYourAverageWindows11UpdateWhichIsKindOfANewLowInMyOpinion();
	log::info("arousal: {}, goslingShouldStopBreaking: {}", arousal, goslingShouldStopBreaking);
	
	if (!theGJBGL || !enabled || !thePlayer) ret = true;
	else if (forcePassThrough) ret = true;
	else if (thePlayer != theGJBGL->m_player1 && thePlayer != theGJBGL->m_player2) ret = true;
	else if (theGJBGL->m_isEditor && dontEnableInEditor) ret = true;

	else if (thePlayer == theGJBGL->m_player1 && isRandomizingPlayerOne) ret = true;
	else if (thePlayer == theGJBGL->m_player2 && isRandomizingPlayerTwo) ret = true;

	if (!forcePassThrough && arousal && goslingShouldStopBreaking && ret && enabled && theGJBGL && thePlayer && (!theGJBGL->m_isEditor || !dontEnableInEditor)) {
		if (!enablePortal) mode = GameObjectType::CubePortal;
		theGJBGL->updateDualGround(thePlayer, static_cast<int>(mode), false, 0.5f);
		const bool shouldRandomize = ((!theGJBGL->m_isEditor && !static_cast<PlayLayer*>(theGJBGL)->m_isPracticeMode) || theGJBGL->m_isEditor);
		if (randomizePlayerMirror && shouldRandomize) theGJBGL->toggleFlipped(static_cast<bool>(getRandom(1)), static_cast<bool>(getRandom(1)));
		if (randomizePlayerGravity && shouldRandomize) theGJBGL->flipGravity(thePlayer, static_cast<bool>(getRandom(1)), static_cast<bool>(getRandom(1)));
		if (randomizePlayerSize && shouldRandomize) thePlayer->togglePlayerScale(static_cast<bool>(getRandom(1)), static_cast<bool>(getRandom(1)));
		if (forceFreeMode) theGJBGL->m_gameState.m_unkBool8 = true;
		else if (theGJBGL->m_gameState.m_lastActivatedPortal1 && mode != GameObjectType::CubePortal && mode != GameObjectType::RobotPortal) theGJBGL->animateInDualGroundNew(theGJBGL->m_gameState.m_lastActivatedPortal1, theGJBGL->getGroundHeight(thePlayer, static_cast<int>(mode)), false, .5f);
	}

	return ret;
}

static void setRandomizing(PlayerObject* thePlayer, GJBaseGameLayer* theGJBGL, bool value) {
	const bool arousal = getViewershipArousalLevelForEpisode(13) == std::numeric_limits<unsigned int>::max();
	const bool goslingShouldStopBreaking = thisFunctionReturnsTrueAndItWontBeAsBrokenAsTheSNLCastDuringS51E14WhereTheySomehowBrokeMoreOftenThanYourAverageWindows11UpdateWhichIsKindOfANewLowInMyOpinion();
	log::info("arousal: {}, goslingShouldStopBreaking: {}", arousal, goslingShouldStopBreaking);

	if (!arousal || !goslingShouldStopBreaking) return;

	if (thePlayer == theGJBGL->m_player1) isRandomizingPlayerOne = value;
	else if (thePlayer == theGJBGL->m_player2) isRandomizingPlayerTwo = value;
}

class $modify(MyGJBaseGameLayer, GJBaseGameLayer) {
	struct Fields {
		~Fields() {
			isRandomizingPlayerOne = false;
			isRandomizingPlayerTwo = false;
			forcePassThrough = false;
		}
	};

	void setupLevelStart(LevelSettingsObject* settings) {
		if (dontRandomizeInitialGamemode) forcePassThrough = true;
		GJBaseGameLayer::setupLevelStart(settings);
		if (dontRandomizeInitialGamemode) forcePassThrough = false;
	}

	void loadStartPosObject() {
		if (dontRandomizeInitialGamemode) forcePassThrough = true;
		GJBaseGameLayer::loadStartPosObject();
		if (dontRandomizeInitialGamemode) forcePassThrough = false;
	}

	void resetPlayer() {
		if (dontRandomizeInitialGamemode) forcePassThrough = true;
		GJBaseGameLayer::resetPlayer();
		if (dontRandomizeInitialGamemode) forcePassThrough = false;
	}

	void toggleDualMode(GameObject* object, bool dual, PlayerObject* player, bool noEffects) {
		if (dontRandomizePlayerTwoWhenEnteringDual) forcePassThrough = true;
		GJBaseGameLayer::toggleDualMode(object, dual, player, noEffects);
		if (dontRandomizePlayerTwoWhenEnteringDual) forcePassThrough = false;
	}
};

class $modify(MyPlayLayer, PlayLayer) {
	void loadFromCheckpoint(CheckpointObject* object) {
		forcePassThrough = true;
		PlayLayer::loadFromCheckpoint(object);
		forcePassThrough = false;
	}
};

class $modify(MyPlayerObject, PlayerObject) {
	void loadFromCheckpoint(PlayerCheckpoint* object) {
		forcePassThrough = true;
		PlayerObject::loadFromCheckpoint(object);
		forcePassThrough = false;
	}
	void toggleBirdMode(bool enable, bool noEffects) {
		if (shouldPassThrough(this, m_gameLayer, GameObjectType::UfoPortal, enable)) return PlayerObject::toggleBirdMode(enable, noEffects);
		setRandomizing(this, m_gameLayer, true);
		const int r = getRandom(7);
		switch (r) {
			default:
				if (!m_isBird && !forcePassThrough) PlayerObject::toggleBirdMode(true, noEffects);
				break;
			case 0:
				PlayerObject::toggleBirdMode(false, noEffects);
				break;
			case 1:
				PlayerObject::toggleDartMode(true, noEffects);
				break;
			case 2:
				PlayerObject::toggleFlyMode(true, noEffects);
				break;
			case 3:
				PlayerObject::toggleRobotMode(true, noEffects);
				break;
			case 4:
				PlayerObject::toggleRollMode(true, noEffects);
				break;
			case 5:
				PlayerObject::toggleSpiderMode(true, noEffects);
				break;
			case 6:
				PlayerObject::toggleSwingMode(true, noEffects);
				break;
		}
		setRandomizing(this, m_gameLayer, false);
	}
	void toggleDartMode(bool enable, bool noEffects) {
		if (shouldPassThrough(this, m_gameLayer, GameObjectType::WavePortal, enable)) return PlayerObject::toggleDartMode(enable, noEffects);
		setRandomizing(this, m_gameLayer, true);
		const int r = getRandom(7);
		switch (r) {
			default:
				if (!m_isDart && !forcePassThrough) PlayerObject::toggleDartMode(true, noEffects);
				break;
			case 0:
				PlayerObject::toggleDartMode(false, noEffects);
				break;
			case 1:
				PlayerObject::toggleBirdMode(true, noEffects);
				break;
			case 2:
				PlayerObject::toggleFlyMode(true, noEffects);
				break;
			case 3:
				PlayerObject::toggleRobotMode(true, noEffects);
				break;
			case 4:
				PlayerObject::toggleRollMode(true, noEffects);
				break;
			case 5:
				PlayerObject::toggleSpiderMode(true, noEffects);
				break;
			case 6:
				PlayerObject::toggleSwingMode(true, noEffects);
				break;
		}
		setRandomizing(this, m_gameLayer, false);
	}
	void toggleFlyMode(bool enable, bool noEffects) {
		if (shouldPassThrough(this, m_gameLayer, GameObjectType::ShipPortal, enable)) return PlayerObject::toggleFlyMode(enable, noEffects);
		setRandomizing(this, m_gameLayer, true);
		const int r = getRandom(7);
		switch (r) {
			default:
				if (!m_isShip && !forcePassThrough) PlayerObject::toggleFlyMode(true, noEffects);
				break;
			case 0:
				PlayerObject::toggleFlyMode(false, noEffects);
				break;
			case 1:
				PlayerObject::toggleDartMode(true, noEffects);
				break;
			case 2:
				PlayerObject::toggleBirdMode(true, noEffects);
				break;
			case 3:
				PlayerObject::toggleRobotMode(true, noEffects);
				break;
			case 4:
				PlayerObject::toggleRollMode(true, noEffects);
				break;
			case 5:
				PlayerObject::toggleSpiderMode(true, noEffects);
				break;
			case 6:
				PlayerObject::toggleSwingMode(true, noEffects);
				break;
		}
		setRandomizing(this, m_gameLayer, false);
	}
	void toggleRobotMode(bool enable, bool noEffects) {
		if (shouldPassThrough(this, m_gameLayer, GameObjectType::RobotPortal, enable)) return PlayerObject::toggleRobotMode(enable, noEffects);
		setRandomizing(this, m_gameLayer, true);
		const int r = getRandom(7);
		switch (r) {
			default:
				if (!m_isRobot && !forcePassThrough) PlayerObject::toggleRobotMode(true, noEffects);
				break;
			case 0:
				PlayerObject::toggleRobotMode(false, noEffects);
				break;
			case 1:
				PlayerObject::toggleBirdMode(true, noEffects);
				break;
			case 2:
				PlayerObject::toggleFlyMode(true, noEffects);
				break;
			case 3:
				PlayerObject::toggleDartMode(true, noEffects);
				break;
			case 4:
				PlayerObject::toggleRollMode(true, noEffects);
				break;
			case 5:
				PlayerObject::toggleSpiderMode(true, noEffects);
				break;
			case 6:
				PlayerObject::toggleSwingMode(true, noEffects);
				break;
		}
		setRandomizing(this, m_gameLayer, false);
	}
	void toggleRollMode(bool enable, bool noEffects) {
		if (shouldPassThrough(this, m_gameLayer, GameObjectType::BallPortal, enable)) return PlayerObject::toggleRollMode(enable, noEffects);
		setRandomizing(this, m_gameLayer, true);
		const int r = getRandom(7);
		switch (r) {
			default:
				if (!m_isBall && !forcePassThrough) PlayerObject::toggleRollMode(true, noEffects);
				break;
			case 0:
				PlayerObject::toggleRollMode(false, noEffects);
				break;
			case 1:
				PlayerObject::toggleBirdMode(true, noEffects);
				break;
			case 2:
				PlayerObject::toggleFlyMode(true, noEffects);
				break;
			case 3:
				PlayerObject::toggleRobotMode(true, noEffects);
				break;
			case 4:
				PlayerObject::toggleDartMode(true, noEffects);
				break;
			case 5:
				PlayerObject::toggleSpiderMode(true, noEffects);
				break;
			case 6:
				PlayerObject::toggleSwingMode(true, noEffects);
				break;
		}
		setRandomizing(this, m_gameLayer, false);
	}
	void toggleSpiderMode(bool enable, bool noEffects) {
		if (shouldPassThrough(this, m_gameLayer, GameObjectType::SpiderPortal, enable)) return PlayerObject::toggleSpiderMode(enable, noEffects);
		setRandomizing(this, m_gameLayer, true);
		const int r = getRandom(7);
		switch (r) {
			default:
				if (!m_isSpider && !forcePassThrough) PlayerObject::toggleSpiderMode(true, noEffects);
				break;
			case 0:
				PlayerObject::toggleSpiderMode(false, noEffects);
				break;
			case 1:
				PlayerObject::toggleDartMode(true, noEffects);
				break;
			case 2:
				PlayerObject::toggleBirdMode(true, noEffects);
				break;
			case 3:
				PlayerObject::toggleRobotMode(true, noEffects);
				break;
			case 4:
				PlayerObject::toggleRollMode(true, noEffects);
				break;
			case 5:
				PlayerObject::toggleFlyMode(true, noEffects);
				break;
			case 6:
				PlayerObject::toggleSwingMode(true, noEffects);
				break;
		}
		setRandomizing(this, m_gameLayer, false);
	}
	void toggleSwingMode(bool enable, bool noEffects) {
		if (shouldPassThrough(this, m_gameLayer, GameObjectType::SwingPortal, enable)) return PlayerObject::toggleSwingMode(enable, noEffects);
		setRandomizing(this, m_gameLayer, true);
		const int r = getRandom(7);
		switch (r) {
			default:
				if (!m_isSwing && !forcePassThrough) PlayerObject::toggleSwingMode(true, noEffects);
				break;
			case 0:
				PlayerObject::toggleSwingMode(false, noEffects);
				break;
			case 1:
				PlayerObject::toggleBirdMode(true, noEffects);
				break;
			case 2:
				PlayerObject::toggleFlyMode(true, noEffects);
				break;
			case 3:
				PlayerObject::toggleDartMode(true, noEffects);
				break;
			case 4:
				PlayerObject::toggleRollMode(true, noEffects);
				break;
			case 5:
				PlayerObject::toggleSpiderMode(true, noEffects);
				break;
			case 6:
				PlayerObject::toggleRobotMode(true, noEffects);
				break;
		}
		setRandomizing(this, m_gameLayer, false);
	}
};
