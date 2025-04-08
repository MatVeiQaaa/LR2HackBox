#include "Misc.hpp"

#include <iostream>
#include <unordered_map>
#include "LR2HackBox/LR2HackBox.hpp"

#include "safetyhook/safetyhook.hpp"
#include "imgui/imgui.h"

#pragma comment(lib, "libSafetyhook.lib")

void Misc::OnSetRetryFlag(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);
	if (!misc.mIsRetryTweaks) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();
	if ((game.KeyInput.p1_buttonInput[1] || game.KeyInput.p1_buttonInput[3] || game.KeyInput.p1_buttonInput[5] || game.KeyInput.p1_buttonInput[7] ||
		game.KeyInput.p2_buttonInput[1] || game.KeyInput.p2_buttonInput[3] || game.KeyInput.p2_buttonInput[5] || game.KeyInput.p2_buttonInput[7]) &&
		(game.KeyInput.p1_buttonInput[2] || game.KeyInput.p2_buttonInput[2])) 
	{
		game.gameplay.flag_retry = 0;
		game.gameplay.randomseed = 0;
		game.gameplay.bmsResourceLoaded = 1;

		game.config.play.gaugeOption[0] = misc.mOrigGaugeType;
	}
}

static void StopKeysounds() {
	typedef int(__cdecl* tStopSound)(LR2::AUDIO* aud, LR2::SOUNDDATA* sound);
	tStopSound StopSound = (tStopSound)0x4B8140;

	LR2::game& game = *LR2HackBox::Get().GetGame();
	for (int i = 0; i < 6480; i++) {
		StopSound(&game.audio, &game.gameplay.keysound[i]);
	}
}

void Misc::OnPlayISetSelecter(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);
	if (!misc.mIsRetryTweaks) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();
	if (!(game.procPhase == 2 || game.procPhase == 3)) return;
	if (game.gameplay.player[0].totalnotes <= game.gameplay.player[0].note_current) return;
	if (game.KeyInput.p1_buttonInput[2] || game.KeyInput.p2_buttonInput[2]) {
		game.procPhase = 0;
		game.procSelecter = 4;
		game.gameplay.flag_retry = 0;
		game.gameplay.randomseed = 0;
		game.gameplay.bmsResourceLoaded = 1;

		// Reset fast/slow stats.
		game.net.rankingData.clearPlayers[2] = 0;
		game.net.rankingData.clearPlayers[3] = 0;
		game.net.rankingData.clearPlayers[4] = 0;

		// Reset gauge type for GAS.
		game.config.play.gaugeOption[0] = misc.mOrigGaugeType;

		StopKeysounds();
	}
	else if (game.KeyInput.p1_buttonInput[4] || game.KeyInput.p2_buttonInput[4]) {
		game.procPhase = 0;
		game.procSelecter = 4;
		game.gameplay.flag_retry = 1;
		game.gameplay.bmsResourceLoaded = 1;

		// Reset fast/slow stats.
		game.net.rankingData.clearPlayers[2] = 0;
		game.net.rankingData.clearPlayers[3] = 0;
		game.net.rankingData.clearPlayers[4] = 0;

		// Reset gauge type for GAS.
		game.config.play.gaugeOption[0] = misc.mOrigGaugeType;

		StopKeysounds();
	}
}

void Misc::OnInit(SafetyHookContext& regs) {
	LR2::game& game = *LR2HackBox::Get().GetGame();
	mOrigGaugeType = game.config.play.gaugeOption[0];
}

void Misc::OnInitPlay(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);
	misc.OnInit(regs);
}

void Misc::OnInitRetry(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);
	misc.OnInit(regs);
}


static int realIdx = 0;
static int randIdx = 0;

void Misc::OnRandomMixInput(SafetyHookContext& regs) {
	LR2::game& game = *LR2HackBox::Get().GetGame();
	if (game.sSelect.bmsList[game.sSelect.cur_song].folderType != 9 && std::string(game.sSelect.bmsList[game.sSelect.cur_song].filepath.body) != "randomselect") return;
	regs.ecx = 0;

	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);
	if (!misc.mIsRandomSelect) return;
	
	typedef int(__cdecl* tGetRand)(int RandMax);
	tGetRand GetRand = (tGetRand)0x6C95E0;
	
	realIdx = game.sSelect.cur_song;
	while (game.sSelect.bmsListCount > 1) {
		randIdx = GetRand(game.sSelect.bmsListCount - 1);
		if (game.sSelect.bmsList[randIdx].keymode >= 5) break;
	}
	misc.mIsRandomSelectEntry = true;

	game.procSelecter = 3;
}

static void UpdateSongdataStrings() {
	LR2::game* g = LR2HackBox::Get().GetGame();

	typedef int(__cdecl* tSetObjectString)(uint32_t num, LR2::CSTR string, LR2::CSTR* objectList);
	tSetObjectString SetObjectString = (tSetObjectString)0x4B6C40;

	typedef int(__cdecl* tSetObjectStringInt)(int at, int val, LR2::CSTR* arr);
	tSetObjectStringInt SetObjectStringInt = (tSetObjectStringInt)0x4B6D00;

	SetObjectString(10, g->sSelect.bmsList[g->sSelect.cur_song].title, g->txtStruct.objectStr);
	SetObjectString(11, g->sSelect.bmsList[g->sSelect.cur_song].subtitle, g->txtStruct.objectStr);
	SetObjectString(12, g->sSelect.bmsList[g->sSelect.cur_song].fulltitle, g->txtStruct.objectStr);
	SetObjectString(13, g->sSelect.bmsList[g->sSelect.cur_song].genre, g->txtStruct.objectStr);
	SetObjectString(14, g->sSelect.bmsList[g->sSelect.cur_song].artist, g->txtStruct.objectStr);
	SetObjectString(15, g->sSelect.bmsList[g->sSelect.cur_song].subartist, g->txtStruct.objectStr);
	SetObjectString(16, g->sSelect.bmsList[g->sSelect.cur_song].tag, g->txtStruct.objectStr);
	SetObjectStringInt(17, g->sSelect.bmsList[g->sSelect.cur_song].level, g->txtStruct.objectStr);
	SetObjectStringInt(18, g->sSelect.bmsList[g->sSelect.cur_song].difficulty, g->txtStruct.objectStr);
	SetObjectStringInt(19, g->sSelect.bmsList[g->sSelect.cur_song].exlevel, g->txtStruct.objectStr);

	SetObjectString(20, g->sSelect.bmsList[g->sSelect.cur_song].title, g->txtStruct.objectStr);
	SetObjectString(21, g->sSelect.bmsList[g->sSelect.cur_song].subtitle, g->txtStruct.objectStr);
	SetObjectString(22, g->sSelect.bmsList[g->sSelect.cur_song].fulltitle, g->txtStruct.objectStr);
	SetObjectString(23, g->sSelect.bmsList[g->sSelect.cur_song].genre, g->txtStruct.objectStr);
	SetObjectString(24, g->sSelect.bmsList[g->sSelect.cur_song].artist, g->txtStruct.objectStr);
	SetObjectString(25, g->sSelect.bmsList[g->sSelect.cur_song].subartist, g->txtStruct.objectStr);
	SetObjectString(26, g->sSelect.bmsList[g->sSelect.cur_song].tag, g->txtStruct.objectStr);
	SetObjectStringInt(27, g->sSelect.bmsList[g->sSelect.cur_song].level, g->txtStruct.objectStr);
	SetObjectStringInt(28, g->sSelect.bmsList[g->sSelect.cur_song].difficulty, g->txtStruct.objectStr);
	SetObjectStringInt(29, g->sSelect.bmsList[g->sSelect.cur_song].exlevel, g->txtStruct.objectStr);
}

static void FillBMSMETA(LR2::BMSMETA& meta, const LR2::SONGDATA& song) {
	meta.artist.assign(song.artist.body);
	meta.filepath.assign(song.filepath.body);
	meta.subartist.assign(song.subartist.body);
	meta.title.assign(song.title.body);
	meta.subtitle.assign(song.title.body);

	meta.selLevel = song.level;
	meta.exlevel = song.exlevel;
	meta.difficulty = song.difficulty;
	meta.keymode = song.keymode;
}

void Misc::OnSceneInitSwitch(SafetyHookContext& regs) {
	if (regs.eax != 3) return; // Before decide scene init.
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);
	if (!misc.mIsRandomSelect) return;
	if (!misc.mIsRandomSelectEntry) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();
	game.sSelect.cur_song = randIdx;
	FillBMSMETA(game.sSelect.metaSelected, game.sSelect.bmsList[game.sSelect.cur_song]);
	UpdateSongdataStrings();

	misc.mIsRandomSelectEntry = false;
}

void Misc::StartRandomFromFolder() {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);
	if (!misc.mIsRandomSelect) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();

	typedef int(__cdecl* tGetRand)(int RandMax);
	tGetRand GetRand = (tGetRand)0x6C95E0;
	int randIdx = GetRand(game.sSelect.bmsListCount - 1);

	typedef int(__cdecl* tParseBMSMETA)(LR2::BMSMETA* meta, LR2::CSTR filepath, char flag);
	tParseBMSMETA ParseBMSMETA = (tParseBMSMETA)0x4AA0B0;
	ParseBMSMETA(&game.sSelect.metaSelected, game.sSelect.bmsList[randIdx].filepath, 0);

	game.sSelect.nowBar += (randIdx - game.sSelect.cur_song) * 1000;
	game.sSelect.oldBar = game.sSelect.listTopbar;
	game.sSelect.prevCalculatedBar = game.sSelect.listCalculatedBar;
	game.sSelect.listCalculatedBar = game.sSelect.nowBar;
	game.sSelect.prevTopbar = game.sSelect.listTopbar;
	game.sSelect.listTopbar = game.sSelect.nowBar;
	game.sSelect.scrollDirection = 2;

	game.procSelecter = 3;
}

static void AddRandomSelectBar() {
	LR2::game& game = *LR2HackBox::Get().GetGame();
	LR2::SONGDATA bar;
	typedef void(__cdecl* tSongDataInit)(LR2::SONGDATA* song);
	tSongDataInit SongDataInit = (tSongDataInit)0x444730;
	SongDataInit(&bar);
	bar.folderType = 9;
	bar.title.assign("RANDOM SELECT");
	bar.fulltitle.assign("RANDOM SELECT");
	bar.filepath.assign("randomselect");
	//bar.hash.assign("randomcourse");
	typedef LR2::SONGDATA*(__thiscall* tSongDataAssign)(LR2::SONGDATA* pThis, LR2::SONGDATA* other);
	tSongDataAssign SongDataAssign = (tSongDataAssign)0x404FE0;
	SongDataAssign(&game.sSelect.bmsList[game.sSelect.bmsListCount], &bar);
	game.sSelect.bmsListCount++;
}

void Misc::OnOpenFolderPlaySound(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);
	if (!misc.mIsRandomSelect) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();
	if (game.sSelect.stack_isFolder[game.sSelect.cur] == 0) {
		AddRandomSelectBar();
	}
}


static std::unordered_map<double, int> bpmRefcount;
void Misc::OnAddToAvgBpmSum(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);
	if (!misc.mIsMainBPM) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();
	if (game.config.play.hsfix != 3) return;

	double bpm = 0;
	__asm {
		fld ST(1)
		fstp bpm
	}

	if (!bpmRefcount.contains(bpm)) {
		bpmRefcount[bpm] = 1;
	}
	else {
		bpmRefcount[bpm] = bpmRefcount[bpm] + 1;
	}
}

void Misc::OnCalcAvgSpeedmult(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);
	if (!misc.mIsMainBPM) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();

	// Calculate mainBPM with longest duration bpm.
	/*std::unordered_map<double, double> bpmDuration;

	double lastNoteTime = 0;
	for (int i = 0; i < game.gameplay.bmsobj.count; i++) {
		if (10 <= game.gameplay.bmsobj.notes[i].op && game.gameplay.bmsobj.notes[i].op < 29)
			lastNoteTime = game.gameplay.bmsobj.notes[i].realTiming;
	}
	for (int i = 0; i < game.gameplay.bpmt_count - 1; i++) {
		if (game.gameplay.bpmt_data[i].realtime > lastNoteTime) break;
		double bpm = game.gameplay.bpmt_data[i].BPM;
		double timePointDuration = game.gameplay.bpmt_data[i + 1].realtime - game.gameplay.bpmt_data[i].realtime;
		if (!bpmDuration.contains(bpm)) {
			bpmDuration[bpm] = timePointDuration;
		}
		else {
			bpmDuration[bpm] = bpmDuration[bpm] + timePointDuration;
		}
	}

	double mainBpm = 0.f;
	double highestDuration = 0;
	for (auto& [bpm, duration] : bpmDuration) {
		if (duration > highestDuration) {
			highestDuration = duration;
			mainBpm = bpm;
		}
	}*/

	// Calculate mainBPM with bpm most notes use.
	double mainBpm = 0;
	int highestRefcount = 0;
	for (auto& [bpm, refcount] : bpmRefcount) {
		if (refcount > highestRefcount) {
			highestRefcount = refcount;
			mainBpm = bpm;
		}
	}
	bpmRefcount.clear();

	__asm {
		fld mainBpm
		fstp ST(1) // gameplay.speedmultiplier = 150.f / ST(0)
	}

	return;
}

bool Misc::Init(uintptr_t moduleBase) {
	Misc::mModuleBase = moduleBase;

	mIsRetryTweaks = LR2HackBox::Get().mConfig->ReadValue("bRetryTweaks") == "true" ? true : false;
	mIsRandomSelect = LR2HackBox::Get().mConfig->ReadValue("bRandomSelect") == "true" ? true : false;
	mIsMainBPM = LR2HackBox::Get().mConfig->ReadValue("bMainBPM") == "true" ? true : false;

	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x9573), OnSetRetryFlag));
	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x0198C1), OnPlayISetSelecter));
	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x02D36C), OnInitPlay));
	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x0AD24D), OnInitRetry));

	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x029AFA), OnRandomMixInput));
	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x031BB6), OnSceneInitSwitch));
	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x01EE32), OnOpenFolderPlaySound));

	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x0B32AD), OnAddToAvgBpmSum));
	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x0B4366), OnCalcAvgSpeedmult));

	return true;
}

bool Misc::Deinit() {
	return true;
}

static void HelpMarker(const char* desc) {
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

void Misc::Menu() {
	LR2::game* game = LR2HackBox::Get().GetGame();

	ImGui::Indent();

	if (ImGui::Checkbox("Restart Tweaks", &mIsRetryTweaks)) {
		LR2HackBox::Get().mConfig->WriteValue("bRetryTweaks", mIsRetryTweaks ? "true" : "false");
		LR2HackBox::Get().mConfig->SaveConfig();
	}
	ImGui::SameLine();
	HelpMarker("When enabled:\n  Play: 2 on fade-out to restart with a new random,\n  4 to restart with the same random. \n\n  Result: <any white key>+2 on fade-out\n  to restart with a new random.");


	if (ImGui::Checkbox("Random Select", &mIsRandomSelect)) {
		LR2HackBox::Get().mConfig->WriteValue("bRandomSelect", mIsRandomSelect ? "true" : "false");
		LR2HackBox::Get().mConfig->SaveConfig();
	}
	ImGui::SameLine();
	HelpMarker("Adds 'RANDOM SELECT' entry to song folder, which starts a random song from it");

	if (ImGui::Checkbox("MainBPM hi-speed mode", &mIsMainBPM)) {
		LR2HackBox::Get().mConfig->WriteValue("bMainBPM", mIsMainBPM ? "true" : "false");
		LR2HackBox::Get().mConfig->SaveConfig();
	}
	ImGui::SameLine();
	HelpMarker("Replaces the effect of AverageBPM hi-speed mode to that of MainBPM");

	/*if (ImGui::Button("Start Random Song")) {
		StartRandomFromFolder();
	}*/

	ImGui::Unindent();
}