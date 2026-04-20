#include "BaseModels/ModFeature.hpp"

#include <LR2Mem/LR2Typedefs.hpp>
#include <safetyhook.hpp>

class ScoreCannon : public ModFeature {
public:
	struct Score {
		enum Lamp {
			NOPLAY,
			FAILED,
			EASYCLEAR,
			GROOVECLEAR,
			HARDCLEAR,
			FULLCOMBO,
		};
		enum Target {
			NOTARGET,
			MYBEST,
			RANKAAA,
			RANKAA,
			RANKA,
			PERCENT,
			IR_TOP,
			IR_NEXT,
			IR_AVERAGE,
			TARGETERROR
		};
		enum Random {
			NONRAN,
			MIRROR,
			RANDOM,
			SRANDOM,
			HRANDOM,
			ALLSCR
		};
		Score() = default;
		Score(const LR2::game& game);
		std::string md5;
		std::string folder;
		std::string title;
		std::string subtitle;
		Score::Lamp lamp = NONE;
		Score::Lamp lampBest = NONE;
		Score::Target target = NOTARGET;
		Score::Random random = NONRAN;
		std::string arrange = "1234567";
		int rseed = 0;
		int exScore = 0;
		int exScoreMax = 0;
		int exScoreBest = 0;
		int exScoreTarget = 0;
		int maxCombo = 0;
		int maxComboBest = 0;
		int missCount = 0;
		int missCountBest = 0;
		int irPosition = 0;
		int irPositionBest = 0;
		int irCount = 0;
	};
	bool Init(uintptr_t moduleBase);
	bool Deinit();
	void Menu();

	bool PostScore(const Score& score, const std::string& screenshotPath);

	bool mIsEnabled = false;
	bool mAlreadySent = false;

private:
	enum class MessageFormat {
		LAMP_ONLY,
		COMPACT,
		FULL
	};
	static std::string MessageFormatToString(MessageFormat format);
	static MessageFormat StringToMessageFormat(std::string_view string);
	std::string GetJsonString(const Score& score);

	std::vector<SafetyHookMid> mMidHooks;
	std::string mDiscordName;
	std::string mDiscordAvatarUrl;
	std::string mGameName;
	std::vector<std::string> mUrls;
	std::string mEditingUrl;
	MessageFormat mMessageFormat = MessageFormat::FULL;
};
