#define NOMINMAX

#include "Misc.hpp"

#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <filesystem>
#include <Gdiplus.h>
#include "LR2HackBox/LR2HackBox.hpp"
#include "AnalogInput.hpp"

#include "ImGuiInjector/ImGuiInjector.hpp"
#include "safetyhook/safetyhook.hpp"
#include "minhook/include/MinHook.h"
#include "imgui/imgui.h"

#pragma comment(lib, "libSafetyhook.lib")

#pragma comment(lib, "Gdiplus.lib")

#if defined _M_X64
#pragma comment(lib, "libMinHook.x64.lib")
#elif defined _M_IX86
#pragma comment(lib, "libMinHook.x86.lib")
#endif

template <typename T>
inline MH_STATUS MH_CreateHookEx(LPVOID pTarget, LPVOID pDetour, T** ppOriginal)
{
	return MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
}

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

static LR2::SOUNDDATA metronomeMeasureFx;
static LR2::SOUNDDATA metronomeBeatFx;

void Misc::OnPlayISetSelecter(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);

	LR2::game& game = *LR2HackBox::Get().GetGame();

	if (!(game.procPhase == 2 || game.procPhase == 3)) return;

	typedef int(__cdecl* tReleaseSound)(LR2::AUDIO* aud, LR2::SOUNDDATA* sound);
	tReleaseSound ReleaseSound = (tReleaseSound)0x4B8040;

	ReleaseSound(&game.audio, &metronomeMeasureFx);
	ReleaseSound(&game.audio, &metronomeBeatFx);

	if (!misc.mIsRetryTweaks) return;

	if (game.gameplay.replay.status > 1) return;
	//if (game.gameplay.courseType != -1) return;
	if (game.gameplay.player[0].totalnotes <= game.gameplay.player[0].note_current) return;
	if (game.KeyInput.p1_buttonInput[12] == 1 || game.KeyInput.p2_buttonInput[12] == 1) {
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

		if (game.gameplay.courseType != -1 && game.gameplay.courseStageNow != 0) {
			game.gameplay.courseStageNow = 0;
			game.gameplay.bmsResourceLoaded = 0;
		}

		StopKeysounds();
	}
	else if (game.KeyInput.p1_buttonInput[13] == 1 || game.KeyInput.p2_buttonInput[13] == 1) {
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

		if (game.gameplay.courseType != -1 && game.gameplay.courseStageNow != 0) {
			game.gameplay.courseStageNow = 0;
			game.gameplay.bmsResourceLoaded = 0;
			game.gameplay.flag_retry = 0;
		}

		StopKeysounds();
	}
}

void Misc::OnInit(SafetyHookContext& regs) {
	LR2::game& game = *LR2HackBox::Get().GetGame();
}

void Misc::OnInitPlay(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);
	misc.OnInit(regs);
}

void Misc::OnInitRetry(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);
	misc.OnInit(regs);
}

void Misc::OnRandomMixInput(SafetyHookContext& regs) {
	LR2::game& game = *LR2HackBox::Get().GetGame();
	if (game.sSelect.bmsList[game.sSelect.cur_song].folderType != 9 && std::string(game.sSelect.bmsList[game.sSelect.cur_song].filepath.body) != "randomselect") return;
	regs.ecx = 0;

	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);
	if (!misc.mIsRandomSelect) return;

	misc.mRandSelCustomEntry = true;

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

void Misc::OnDecideInit() {
	LR2::game& game = *LR2HackBox::Get().GetGame();
	void* sqlite = LR2HackBox::Get().GetSqlite();

	if (!mIsRandomSelect) return;
	if (!mRandSelCustomEntry) return;

	LR2::CSTR querry(0);
	std::string querryStr = std::format("{} {} ORDER BY random() LIMIT 1", game.sSelect.stack_query[game.sSelect.cur].body, game.sSelect.bmsList[game.sSelect.cur_song].tag.body);

	querry.assign(querryStr.c_str());

	typedef int(__cdecl* tSQL_Prepare)(LR2::CSTR queryStr, void* sql, void** ppStmt);
	tSQL_Prepare SQL_Prepare = (tSQL_Prepare)0x4443f0;

	typedef LR2::CSTR(__cdecl* tSQL_GetColumn)(int i, void* pStmt);
	tSQL_GetColumn SQL_GetColumn = (tSQL_GetColumn)0x4444F0;

	typedef int(__cdecl* tsqlite3_step)(void* stmt);
	tsqlite3_step sqlite3_step = (tsqlite3_step)0x48C480;

	typedef int(__cdecl* tsqlite3_finalize)(void* stmt);
	tsqlite3_finalize sqlite3_finalize = (tsqlite3_finalize)0x485FD0;

	void* pStmt;
	LR2::CSTR songhash;

	int prepareRes = SQL_Prepare(querry, sqlite, &pStmt);
	if (sqlite3_step(pStmt) == 100) {
		songhash.assign(SQL_GetColumn(0, pStmt).body);
	}
	sqlite3_finalize(pStmt);

	for (int i = 0; i < game.sSelect.bmsListCount; i++) {
		if (std::string_view(songhash.body) == game.sSelect.bmsList[i].hash.body) {
			game.sSelect.cur_song = i;
			break;
		}
		game.sSelect.cur_song = game.sSelect.bmsListCount - 1;
	}

	FillBMSMETA(game.sSelect.metaSelected, game.sSelect.bmsList[game.sSelect.cur_song]);
	UpdateSongdataStrings();

	mRandSelCustomEntry = false;
}

void Misc::OnPlayInit() {
	LR2::game& game = *LR2HackBox::Get().GetGame();
	mOrigGaugeType = game.config.play.gaugeOption[0];
	mCurrentDrawingLNObj = nullptr;

	if (game.config.play.autojudge == 3) game.config.play.judgetiming = mAutoadjustResetLastVal;

	mMetronomeLastPlayedBeat = 0;
	mMetronomePrevMeasureIdx = -1;
	if (mIsMetronome) {
		typedef int(__cdecl* tLoadSound)(LR2::AUDIO* aud, LR2::SOUNDDATA* sound, LR2::CSTR filepath, int loop, int disableDSP, int previewFlag);
		tLoadSound LoadSound = (tLoadSound)0x4B8BB0;

		LoadSound(&game.audio, &metronomeMeasureFx, LR2::CSTR("LR2files\\Sound\\LR2HackBox\\metronome-measure.wav"), 0, game.config.sound.disabledsp, 0);
		LoadSound(&game.audio, &metronomeBeatFx, LR2::CSTR("LR2files\\Sound\\LR2HackBox\\metronome-beat.wav"), 0, game.config.sound.disabledsp, 0);
	}
}

void Misc::OnSceneInitSwitch(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);

	switch (regs.eax) {
	case 3:
		misc.OnDecideInit();
		break;
	case 4:
		misc.OnPlayInit();
		break;
	}
}

void Misc::OnSelectExit() {
	LR2::game& game = *LR2HackBox::Get().GetGame();
	mAutoadjustResetLastVal = game.config.play.judgetiming;
}

void Misc::OnPlayExit() {
	LR2::game& game = *LR2HackBox::Get().GetGame();
	if (game.config.play.autojudge == 3) game.config.play.judgetiming = mAutoadjustResetLastVal;
}

void Misc::OnSceneExitSwitch(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);

	switch (regs.edi) {
	case 2:
		misc.OnSelectExit();
		break;
	case 4:
		misc.OnPlayExit();
		break;
	}
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

static void AddRandomSelectBar(const char* title, const char* tag = "") {
	LR2::game& game = *LR2HackBox::Get().GetGame();
	LR2::SONGDATA bar;
	typedef void(__cdecl* tSongDataInit)(LR2::SONGDATA* song);
	tSongDataInit SongDataInit = (tSongDataInit)0x444730;
	SongDataInit(&bar);
	bar.folderType = 9;
	bar.title.assign(title);
	bar.fulltitle.assign(title);
	bar.filepath.assign("randomselect");
	bar.tag.assign(tag);
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
		AddRandomSelectBar("RANDOM SELECT");
		AddRandomSelectBar("RANDOM SELECT UNPLAYED", "AND clear IS NULL");
		AddRandomSelectBar("RANDOM SELECT FAILED", "AND clear = 1");
		AddRandomSelectBar("RANDOM SELECT <HC", "AND (clear < 4 OR clear IS NULL)");
		AddRandomSelectBar("RANDOM SELECT <AAA", "AND (CAST((perfect * 2 + great) AS float) / (totalnotes * 2) * 100 < 88.88 OR perfect IS NULL)");
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

static double measureSize = 0;
static int beatsPerMeasure = 0;
void Misc::OnDrawNotesGetSongtimer(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);
	if (!misc.mIsMetronome) return;

	double songtimer = 0;
	__asm {
		fst songtimer
	}

	LR2::game& game = *LR2HackBox::Get().GetGame();
	int currentMeasureIdx = std::min(game.gameplay.bmsobj_line.count - 1, misc.mMetronomePrevMeasureIdx + 1);
	int nextMeasureIdx = std::min(game.gameplay.bmsobj_line.count - 1, currentMeasureIdx + 1);
	if (game.gameplay.bmsobj_line.notes[nextMeasureIdx].bmsTiming <= songtimer) {
		misc.mMetronomePrevMeasureIdx = currentMeasureIdx;
		currentMeasureIdx = std::min(game.gameplay.bmsobj_line.count - 1, misc.mMetronomePrevMeasureIdx + 1);
		nextMeasureIdx = std::min(game.gameplay.bmsobj_line.count - 1, currentMeasureIdx + 1);
	}

	const LR2::NoteStruct& currentMeasure = game.gameplay.bmsobj_line.notes[currentMeasureIdx];
	int currentBeat = (songtimer - currentMeasure.bmsTiming) / 480.f;

	typedef int(__cdecl* tGamePlaySound)(LR2::AUDIO* aud, LR2::SOUNDDATA* sound, LR2::FMOD_CHANNELGROUP* channelgroup, int stage);
	tGamePlaySound GamePlaySound = (tGamePlaySound)0x4B8F20;
	if (currentBeat != misc.mMetronomeLastPlayedBeat) {
		misc.mMetronomeLastPlayedBeat = currentBeat;
		if (currentBeat == 0) {
			LR2::SOUNDDATA* sfx = metronomeBeatFx.load == 1 ? &metronomeMeasureFx : &game.audio.sysSound.folder_open;
			GamePlaySound(&game.audio, sfx, game.audio.chnBgm, -1);
		}
		else {
			LR2::SOUNDDATA* sfx = metronomeBeatFx.load == 1 ? &metronomeBeatFx : &game.audio.sysSound.folder_close;
			GamePlaySound(&game.audio, sfx, game.audio.chnBgm, -1);
		}
	}
}

static std::wstring s2ws(const std::string& str)
{
	int size_needed = MultiByteToWideChar(CP_OEMCP, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_OEMCP, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

typedef int(__cdecl* tSaveDrawScreenToPNG)(int x1, int y1, int x2, int y2, const char* FileName, int CompressionLevel);
tSaveDrawScreenToPNG SaveDrawScreenToPNG = (tSaveDrawScreenToPNG)0x510060;
int Misc::OnSaveDrawScreenToPNG(int x1, int y1, int x2, int y2, const char* FileName, int CompressionLevel) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);
	std::string directory = "screenshots\\";
	std::string path = misc.mIsRerouteScreenshots ? directory + FileName : FileName;
	if (misc.mIsRerouteScreenshots && !std::filesystem::directory_entry(directory).exists())
		std::filesystem::create_directories(directory);
	int result = SaveDrawScreenToPNG(x1, y1, x2, y2, path.c_str(), CompressionLevel);

	if (misc.mIsScreenshotsCopybuffer) {
		ULONG_PTR gdiplusToken = NULL;
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Gdiplus::Ok) return result;

		Gdiplus::Status status;
		Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(s2ws(path).c_str());
		if (!bitmap) return result;

		HBITMAP hbitmap;
		status = bitmap->GetHBITMAP(0, &hbitmap);
		if (status != Gdiplus::Ok) return result;

		BITMAP bm;
		GetObject(hbitmap, sizeof bm, &bm);
		DIBSECTION ds;
		if (sizeof ds == GetObject(hbitmap, sizeof ds, &ds))
		{
			HDC hdc = GetDC(NULL);
			HBITMAP hbitmap_ddb = CreateDIBitmap(hdc, &ds.dsBmih, CBM_INIT,
				ds.dsBm.bmBits, (BITMAPINFO*)&ds.dsBmih, DIB_RGB_COLORS);
			ReleaseDC(NULL, hdc);
			if (OpenClipboard(ImGuiInjector::Get().GetWindowHandle()))
			{
				EmptyClipboard();
				SetClipboardData(CF_BITMAP, hbitmap_ddb);
				CloseClipboard();
			}
			DeleteObject(hbitmap_ddb);
		}
		DeleteObject(hbitmap);

		Gdiplus::GdiplusShutdown(gdiplusToken);
	}

	return result;
}

void Misc::SetMetronome(bool value) {
	mIsMetronome = value;
}

void Misc::MirrorGearshift(bool mirror) {
	const unsigned char* p1_buttonInputOffsets = (unsigned char*)0x1FD8C;

	uintptr_t* gearshiftDownButton1 = (uintptr_t*)0x4277AE;
	uintptr_t* gearshiftDownButton2 = (uintptr_t*)0x4277B7;
	uintptr_t* gearshiftDownButton3 = (uintptr_t*)0x4277C0;

	uintptr_t* gearshiftUpButton1 = (uintptr_t*)0x4277E5;
	uintptr_t* gearshiftUpButton2 = (uintptr_t*)0x4277EE;

	uintptr_t* lanecoverUpButton = (uintptr_t*)0x4278BC;
	uintptr_t* lanecoverDownButton = (uintptr_t*)0x4278DB;

	DWORD oldProtection = 0;
	BOOL hResult = VirtualProtect(gearshiftDownButton1, (uintptr_t)lanecoverDownButton - (uintptr_t)gearshiftDownButton1 + 4, PAGE_EXECUTE_READWRITE, &oldProtection);

	if (mirror) {
		*gearshiftDownButton1 = (uintptr_t)&p1_buttonInputOffsets[3];
		*gearshiftDownButton2 = (uintptr_t)&p1_buttonInputOffsets[5];
		*gearshiftDownButton3 = (uintptr_t)&p1_buttonInputOffsets[7];

		*gearshiftUpButton1 = (uintptr_t)&p1_buttonInputOffsets[4];
		*gearshiftUpButton2 = (uintptr_t)&p1_buttonInputOffsets[6];

		*lanecoverUpButton = (uintptr_t)&p1_buttonInputOffsets[2];
		*lanecoverDownButton = (uintptr_t)&p1_buttonInputOffsets[1];
	}
	else {
		*gearshiftDownButton1 = (uintptr_t)&p1_buttonInputOffsets[1];
		*gearshiftDownButton2 = (uintptr_t)&p1_buttonInputOffsets[3];
		*gearshiftDownButton3 = (uintptr_t)&p1_buttonInputOffsets[5];

		*gearshiftUpButton1 = (uintptr_t)&p1_buttonInputOffsets[2];
		*gearshiftUpButton2 = (uintptr_t)&p1_buttonInputOffsets[4];

		*lanecoverUpButton = (uintptr_t)&p1_buttonInputOffsets[6];
		*lanecoverDownButton = (uintptr_t)&p1_buttonInputOffsets[7];
	}

	DWORD discard = 0;
	hResult = VirtualProtect(gearshiftDownButton1, (uintptr_t)lanecoverDownButton - (uintptr_t)gearshiftDownButton1 + 4, oldProtection, &discard);
}

void Misc::OnBeforeAddDrawingBuffer_LN(SafetyHookContext& regs) {
	uintptr_t** offset1 = (uintptr_t**)(regs.esp + 0x68);
	uintptr_t* offset2 = (uintptr_t*)(regs.esp + 0xA0);
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);
	misc.mCurrentDrawingLNObj = (void*)(**offset1 + *offset2);
}

typedef int (__cdecl* tAddDrawingBuffer_LN)(LR2::DrawingBuf* drb, LR2::SRCstruct* srcLs, LR2::SRCstruct* srcLe, LR2::SRCstruct* srcLb, LR2::DSTstruct* dst, LR2::Timer* T, float shiftX, float shiftY, float longY, int alpha, float sizeX, float sizeY);
tAddDrawingBuffer_LN AddDrawingBuffer_LN = (tAddDrawingBuffer_LN)0x49D240;
int Misc::OnAddDrawingBuffer_LN_Fixed(void* drbIn, void* srcLsIn, void* srcLeIn, void* srcLbIn, void* dstIn, void* TIn, float shiftX, float shiftY, float longY, int alpha, float sizeX, float sizeY, void* lnObjIn) {
	LR2::DrawingBuf* drb = (LR2::DrawingBuf*)drbIn;
	LR2::SRCstruct* srcLs = (LR2::SRCstruct*)srcLsIn;
	LR2::SRCstruct* srcLe = (LR2::SRCstruct*)srcLeIn;
	LR2::SRCstruct* srcLb = (LR2::SRCstruct*)srcLbIn;
	LR2::DSTstruct* dst = (LR2::DSTstruct*)dstIn;
	LR2::Timer* T = (LR2::Timer*)TIn;
	LR2::NoteStruct* lnObj = (LR2::NoteStruct*)lnObjIn;

	typedef LR2::DSTdraw(__cdecl* tSetDSTdrawByTime)(LR2::DSTstruct dst, double time);
	tSetDSTdrawByTime SetDSTdrawByTime = (tSetDSTdrawByTime)0x49A840;
	typedef double(__cdecl* tGetTimeLapse)(int timerID, LR2::Timer* T);
	tGetTimeLapse GetTimeLapse = (tGetTimeLapse)0x4B6B10;
	typedef int(__cdecl* tGetSRCcycleNow)(LR2::SRCstruct src, double time);
	tGetSRCcycleNow GetSRCcycleNow = (tGetSRCcycleNow)0x49ABF0;
	typedef int(__cdecl* tAddDrawingBuffer)(LR2::DrawingBuf* drb, int grHandle, LR2::DSTdraw* dstd);
	tAddDrawingBuffer AddDrawingBuffer = (tAddDrawingBuffer)0x49CEB0;

	LR2::DSTdraw tDstd;
	int grh;

	if (dst->dstCount <= 0 || dst->dataSize <= 0 || srcLs->graphcount <= 0 || srcLe->graphcount <= 0 || srcLb->graphcount <= 0) return 0;

	//body
	tDstd = SetDSTdrawByTime(*dst, GetTimeLapse(dst->timer, T));
	tDstd.w += sizeX;
	tDstd.h += sizeY;
	tDstd.x -= sizeX * 0.5;
	tDstd.y -= sizeY * 0.5;
	if (srcLb->timer == dst->timer) {
		if (lnObj->active > 0) {
			grh = srcLb->grHandles[GetSRCcycleNow(*srcLb, GetTimeLapse(srcLb->timer, T) - dst->draw->time)];
		}
		else {
			grh = srcLb->grHandles[0];
		}
	}
	else {
		if (lnObj->active > 0) {
			grh = srcLb->grHandles[GetSRCcycleNow(*srcLb, GetTimeLapse(srcLb->timer, T))];
		}
		else {
			grh = srcLb->grHandles[0];
		}
	}
	tDstd.x += shiftX;
	tDstd.subHandle = dst->n;
	tDstd.blend = 1;
	tDstd.a = alpha;
	tDstd.y += longY + tDstd.h;
	tDstd.h = shiftY - longY - tDstd.h;
	if (tDstd.time != -1 && grh != -1) AddDrawingBuffer(drb, grh, &tDstd);

	//start
	tDstd = SetDSTdrawByTime(*dst, GetTimeLapse(dst->timer, T));
	tDstd.w += sizeX;
	tDstd.h += sizeY;
	tDstd.x -= sizeX * 0.5;
	tDstd.y -= sizeY * 0.5;
	tDstd.sortID += 2;
	if (srcLs->timer == dst->timer) {
		if (lnObj->active > 0) {
			grh = srcLs->grHandles[GetSRCcycleNow(*srcLs, GetTimeLapse(srcLs->timer, T) - dst->draw->time)];
		}
		else {
			grh = srcLs->grHandles[0];
		}
	}
	else {
		if (lnObj->active > 0) {
			grh = srcLs->grHandles[GetSRCcycleNow(*srcLs, GetTimeLapse(srcLs->timer, T))];
		}
		else {
			grh = srcLs->grHandles[0];
		}
	}
	tDstd.x += shiftX;
	tDstd.subHandle = dst->n;
	tDstd.blend = 1;
	tDstd.a = alpha;
	tDstd.y += shiftY;
	if (tDstd.time != -1 && grh != -1) AddDrawingBuffer(drb, grh, &tDstd);

	//end
	tDstd = SetDSTdrawByTime(*dst, GetTimeLapse(dst->timer, T));
	tDstd.w += sizeX;
	tDstd.h += sizeY;
	tDstd.x -= sizeX * 0.5;
	tDstd.y -= sizeY * 0.5;
	tDstd.sortID += 1;
	if (srcLe->timer == dst->timer) {
		if (lnObj->active > 0) {
			grh = srcLe->grHandles[GetSRCcycleNow(*srcLe, GetTimeLapse(srcLe->timer, T) - dst->draw->time)];
		}
		else {
			grh = srcLe->grHandles[0];
		}
	}
	else {
		if (lnObj->active > 0) {
			grh = srcLe->grHandles[GetSRCcycleNow(*srcLe, GetTimeLapse(srcLe->timer, T))];
		}
		else {
			grh = srcLe->grHandles[0];
		}
	}
	tDstd.x += shiftX;
	tDstd.subHandle = dst->n;
	tDstd.blend = 1;
	tDstd.a = alpha;
	tDstd.y += longY;
	if (tDstd.time != -1 && grh != -1) AddDrawingBuffer(drb, grh, &tDstd);

	return 1;
}

int Misc::OnAddDrawingBuffer_LN(void* drb, void* srcLs, void* srcLe, void* srcLb, void* dst, void* T, float shiftX, float shiftY, float longY, int alpha, float sizeX, float sizeY) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);
	if (misc.mIsLNAnimFix && misc.mCurrentDrawingLNObj) {
		void* lnObj = misc.mCurrentDrawingLNObj;
		misc.mCurrentDrawingLNObj = nullptr;
		return misc.OnAddDrawingBuffer_LN_Fixed((LR2::DrawingBuf*)drb, (LR2::SRCstruct*)srcLs, (LR2::SRCstruct*)srcLe, (LR2::SRCstruct*)srcLb, (LR2::DSTstruct*)dst, (LR2::Timer*)T, shiftX, shiftY, longY, alpha, sizeX, sizeY, lnObj);
	}
	return AddDrawingBuffer_LN((LR2::DrawingBuf*)drb, (LR2::SRCstruct*)srcLs, (LR2::SRCstruct*)srcLe, (LR2::SRCstruct*)srcLb, (LR2::DSTstruct*)dst, (LR2::Timer*)T, shiftX, shiftY, longY, alpha, sizeX, sizeY);
}

void Misc::OnAutoadjustDec(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);
	if (!misc.mIsAutoadjustClamp) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();
	if (game.gameplay.autojudge_midsum == 0) return;
	if (game.config.play.judgetiming <= misc.mAutoadjustClampMin) game.config.play.judgetiming++;
}

void Misc::OnAutoadjustInc(SafetyHookContext& regs) {
	Misc& misc = *(Misc*)(LR2HackBox::Get().mMisc);
	if (!misc.mIsAutoadjustClamp) return;

	LR2::game& game = *LR2HackBox::Get().GetGame();
	if (game.config.play.judgetiming >= misc.mAutoadjustClampMax) game.config.play.judgetiming--;
}

void Misc::SetAutoadjustReset(bool enable) {
	char* autoadjustModeUpperLimit = (char*)0x425007;

	DWORD oldProtection = 0;
	BOOL hResult = VirtualProtect(autoadjustModeUpperLimit, 1, PAGE_EXECUTE_READWRITE, &oldProtection);

	*autoadjustModeUpperLimit = enable ? 3 : 2;

	DWORD discard = 0;
	hResult = VirtualProtect(autoadjustModeUpperLimit, 1, oldProtection, &discard);
}

typedef int(__cdecl* tSetObjectString)(unsigned int num, LR2::CSTR string, LR2::CSTR* objectList);
tSetObjectString SetObjectString = (tSetObjectString)0x4B6C40;
int Misc::OnSetObjectString(unsigned int num, void* string, void** objectList) {
	LR2::game& game = *LR2HackBox::Get().GetGame();
	if (num == 80 && game.config.play.autojudge == 3) {
		return SetObjectString(80, "RESET", game.txtStruct.objectStr);
	}
	return SetObjectString(num, (LR2::CSTR&)string, (LR2::CSTR*)objectList);
}

bool Misc::Init(uintptr_t moduleBase) {
	Misc::mModuleBase = moduleBase;

	mIsRetryTweaks = LR2HackBox::Get().mConfig->ReadValue("bRetryTweaks") == "true" ? true : false;
	mIsRandomSelect = LR2HackBox::Get().mConfig->ReadValue("bRandomSelect") == "true" ? true : false;
	mIsMainBPM = LR2HackBox::Get().mConfig->ReadValue("bMainBPM") == "true" ? true : false;
	mIsRerouteScreenshots = LR2HackBox::Get().mConfig->ReadValue("bRerouteScreenshots") == "true" ? true : false;
	mIsScreenshotsCopybuffer = LR2HackBox::Get().mConfig->ReadValue("bScreenshotsCopybuffer") == "true" ? true : false;
	mIsMirrorGearshift = LR2HackBox::Get().mConfig->ReadValue("bMirrorGearshift") == "true" ? true : false;
	mIsAnalogInput = LR2HackBox::Get().mConfig->ReadValue("bAnalogInput") == "true" ? true : false;
	mIsLNAnimFix = LR2HackBox::Get().mConfig->ReadValue("bLNAnimFix") == "true" ? true : false;
	mIsAutoadjustClamp = LR2HackBox::Get().mConfig->ReadValue("bAutoadjustClamp") == "true" ? true : false;
	mIsAutoadjustReset = LR2HackBox::Get().mConfig->ReadValue("bAutoadjustReset") == "true" ? true : false;

	try {
		mAutoadjustClampMin = std::stoi(LR2HackBox::Get().mConfig->ReadValue("iAutoadjustClampMin"));
	}
	catch (...) {}
	try {
		mAutoadjustClampMax = std::stoi(LR2HackBox::Get().mConfig->ReadValue("iAutoadjustClampMax"));
	}
	catch (...) {}

	((AnalogInput*)LR2HackBox::Get().mAnalogInput)->SetEnabled(mIsAnalogInput);
	MirrorGearshift(mIsMirrorGearshift);
	SetAutoadjustReset(mIsAutoadjustReset);

	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x9573), OnSetRetryFlag));
	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x0198C1), OnPlayISetSelecter));
	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x02D36C), OnInitPlay));
	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x0AD24D), OnInitRetry));

	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x029AFA), OnRandomMixInput));
	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x031BB6), OnSceneInitSwitch));
	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x033830), OnSceneExitSwitch));
	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x01EE32), OnOpenFolderPlaySound));

	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x0B32AD), OnAddToAvgBpmSum));
	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x0B4366), OnCalcAvgSpeedmult));

	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x6D86), OnDrawNotesGetSongtimer));

	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x7A83), OnBeforeAddDrawingBuffer_LN));

	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x02C84F), OnAutoadjustInc));
	mMidHooks.push_back(safetyhook::create_mid((void*)(moduleBase + 0x02C854), OnAutoadjustDec));

	if (MH_CreateHookEx((LPVOID)SaveDrawScreenToPNG, &OnSaveDrawScreenToPNG, &SaveDrawScreenToPNG) != MH_OK)
	{
		std::cout << "Couldn't hook SaveDrawScreenToPNG" << std::endl;
		return false;
	}

	if (MH_CreateHookEx((LPVOID)AddDrawingBuffer_LN, &OnAddDrawingBuffer_LN, &AddDrawingBuffer_LN) != MH_OK)
	{
		std::cout << "Couldn't hook AddDrawingBuffer_LN" << std::endl;
		return false;
	}

	if (MH_CreateHookEx((LPVOID)SetObjectString, &OnSetObjectString, &SetObjectString) != MH_OK)
	{
		std::cout << "Couldn't hook SetObjectString" << std::endl;
		return false;
	}

	if (MH_QueueEnableHook(MH_ALL_HOOKS) || MH_ApplyQueued() != MH_OK)
	{
		std::cout << ("Couldn't enable misc hooks") << std::endl;
		return false;
	}

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
	HelpMarker("When enabled:\n  Play: 'START' on fade-out to restart with a new random,\n  'SELECT' to restart with the same random. \n\n  Result: <any white key>+2 on fade-out\n  to restart with a new random.");


	if (ImGui::Checkbox("Random Select", &mIsRandomSelect)) {
		LR2HackBox::Get().mConfig->WriteValue("bRandomSelect", mIsRandomSelect ? "true" : "false");
		LR2HackBox::Get().mConfig->SaveConfig();
	}
	ImGui::SameLine();
	HelpMarker("Adds an assortment of 'RANDOM SELECT' entries to song folder, which starts a random song matching the filter from it.\n\nFilters are 'UNPLAYED', 'FAILED', '<HC', '<AAA'.");

	if (ImGui::Checkbox("MainBPM hi-speed mode", &mIsMainBPM)) {
		LR2HackBox::Get().mConfig->WriteValue("bMainBPM", mIsMainBPM ? "true" : "false");
		LR2HackBox::Get().mConfig->SaveConfig();
	}
	ImGui::SameLine();
	HelpMarker("Replaces the effect of AverageBPM hi-speed mode to that of MainBPM");

	if (ImGui::Checkbox("Reroute Screenshots", &mIsRerouteScreenshots)) {
		LR2HackBox::Get().mConfig->WriteValue("bRerouteScreenshots", mIsRerouteScreenshots ? "true" : "false");
		LR2HackBox::Get().mConfig->SaveConfig();
	}
	ImGui::SameLine();
	HelpMarker("Reroutes screenshots to save in 'screenshots' folder");

	if (ImGui::Checkbox("Screenshots to Copybuffer", &mIsScreenshotsCopybuffer)) {
		LR2HackBox::Get().mConfig->WriteValue("bScreenshotsCopybuffer", mIsScreenshotsCopybuffer ? "true" : "false");
		LR2HackBox::Get().mConfig->SaveConfig();
	}
	ImGui::SameLine();
	HelpMarker("Puts the screenshots in the copybuffer, to later access them with CTRL+V");

	if (ImGui::Checkbox("Mirror Lanecover Buttons", &mIsMirrorGearshift)) {
		LR2HackBox::Get().mConfig->WriteValue("bMirrorGearshift", mIsMirrorGearshift ? "true" : "false");
		LR2HackBox::Get().mConfig->SaveConfig();

		MirrorGearshift(mIsMirrorGearshift);
	}
	ImGui::SameLine();
	HelpMarker("Mirrors controls for hi-speed and lanecover values, making lanecover on 1 and 2 instead of 6 and 7");

	if (ImGui::Checkbox("Fix LN Animation", &mIsLNAnimFix)) {
		LR2HackBox::Get().mConfig->WriteValue("bLNAnimFix", mIsLNAnimFix ? "true" : "false");
		LR2HackBox::Get().mConfig->SaveConfig();
	}
	ImGui::SameLine();
	HelpMarker("Fixes a bug, where all visible LNs of the same column would play the hold animation, instead of only the LN you are holding");

	if (ImGui::Checkbox("Autoadjust Clamp", &mIsAutoadjustClamp)) {
		LR2HackBox::Get().mConfig->WriteValue("bAutoadjustClamp", mIsAutoadjustClamp ? "true" : "false");
		LR2HackBox::Get().mConfig->SaveConfig();
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80.f);
	if (ImGui::InputInt2("Min-Max", &mAutoadjustClampMin)) {
		LR2HackBox::Get().mConfig->WriteValue("iAutoadjustClampMin", std::to_string(mAutoadjustClampMin));
		LR2HackBox::Get().mConfig->WriteValue("iAutoadjustClampMax", std::to_string(mAutoadjustClampMax));
		LR2HackBox::Get().mConfig->SaveConfig();
	}
	ImGui::SameLine();
	HelpMarker("Clamps the autoadjust to the bounds specified in 'Min-Max' boxes, not allowing it to adjust below min and above max");

	if (ImGui::Checkbox("Autoadjust Reset", &mIsAutoadjustReset)) {
		SetAutoadjustReset(mIsAutoadjustReset);
		LR2HackBox::Get().mConfig->WriteValue("bAutoadjustReset", mIsAutoadjustReset ? "true" : "false");
		LR2HackBox::Get().mConfig->SaveConfig();
	}
	ImGui::SameLine();
	HelpMarker("Adds new type for autoadjust, which can be selected through settings menu of the game. It's called 'RESET', and after each play it resets the adjust to value before you started playing");

	if (ImGui::Checkbox("Analog scratch support", &mIsAnalogInput)) {
		((AnalogInput*)LR2HackBox::Get().mAnalogInput)->SetEnabled(mIsAnalogInput);

		LR2HackBox::Get().mConfig->WriteValue("bAnalogInput", mIsAnalogInput ? "true" : "false");
		LR2HackBox::Get().mConfig->SaveConfig();
	}
	ImGui::SameLine();
	HelpMarker("Lets you select a device and axis to use for scratch input. While enabled, set axis should be unbound in LR2 input settings");

	if (mIsAnalogInput) {
		((AnalogInput*)LR2HackBox::Get().mAnalogInput)->Menu();
	}

	/*if (ImGui::Button("Start Random Song")) {
		StartRandomFromFolder();
	}*/

	ImGui::Unindent();
}