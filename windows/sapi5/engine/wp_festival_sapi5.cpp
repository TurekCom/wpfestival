#include <windows.h>
#include <shellapi.h>
#include <sapi.h>
#include <sapiddk.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cwctype>
#include <fstream>
#include <new>
#include <sstream>
#include <string>
#include <vector>

#include "../common/wp_festival_shared.h"

namespace {

#ifdef WP_FESTIVAL_PROFILE_FEMALE
const CLSID CLSID_WPFestivalSapi5 =
{ 0x22b4ce69, 0xfa37, 0x4a16, { 0x93, 0xde, 0x04, 0x16, 0x8f, 0xd7, 0x3a, 0x87 } };
const CLSID CLSID_WPFestivalSapi5Ui =
{ 0x51ec7519, 0x9e37, 0x4712, { 0xb8, 0x3f, 0x28, 0xe9, 0x93, 0x42, 0xc4, 0x9c } };
#else
const CLSID CLSID_WPFestivalSapi5 =
{ 0x7f5f0b2e, 0x4c3f, 0x4a3d, { 0x97, 0x74, 0x91, 0xd6, 0xd6, 0x03, 0xd4, 0x68 } };
const CLSID CLSID_WPFestivalSapi5Ui =
{ 0x8c37e6ad, 0xa71d, 0x4ef3, { 0x88, 0xa3, 0x57, 0xd3, 0xd5, 0xe1, 0x7a, 0x4c } };
#endif
const GUID kSpdfidWaveFormatEx =
{ 0xC31ADBAE, 0x527F, 0x4FF5, { 0xA2, 0x30, 0xF6, 0x2B, 0xB6, 0x1F, 0xF7, 0x0C } };

constexpr const wchar_t* kEngineName = kWpFestivalEngineDisplayName;
constexpr const wchar_t* kUiName = kWpFestivalUiDisplayName;
constexpr wchar_t kPerUserClsidRoot[] = L"Software\\Classes\\CLSID";
constexpr wchar_t kFestivalExeName[] = L"festival.exe";
constexpr wchar_t kRuntimeDirName[] = L"wp_runtime_lib";
constexpr const wchar_t* kConfigExeName = kWpFestivalConfigExeName;
constexpr DWORD kFestivalTimeoutMs = 60000;
constexpr uint32_t kSampleRate = 16000;
constexpr int kDefaultPitchMean = 105;
constexpr int kDefaultPitchStd = 14;
constexpr int kDefaultModelPitchMean = 170;
constexpr int kDefaultModelPitchStd = 34;

HMODULE g_module = nullptr;
std::atomic<ULONG> g_objects{0};
std::atomic<ULONG> g_locks{0};

bool IsSpeechAction(SPVACTIONS action) {
    return action == SPVA_Speak || action == SPVA_Pronounce || action == SPVA_SpellOut;
}

bool IsWhitespace(wchar_t ch) {
    return ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n';
}

std::wstring CollapseWhitespace(const std::wstring& text) {
    std::wstring out;
    out.reserve(text.size());
    bool pendingSpace = false;
    for (wchar_t ch : text) {
        wchar_t normalized = ch;
        if (ch == L'\r' || ch == L'\n' || ch == L'\t') {
            normalized = L' ';
        }
        if (IsWhitespace(normalized)) {
            pendingSpace = !out.empty();
            continue;
        }
        if (pendingSpace) {
            out.push_back(L' ');
            pendingSpace = false;
        }
        out.push_back(normalized);
    }
    return out;
}

wchar_t LowerPL(wchar_t ch) {
    switch (ch) {
    case L'Ą': return L'ą';
    case L'Ć': return L'ć';
    case L'Ę': return L'ę';
    case L'Ł': return L'ł';
    case L'Ń': return L'ń';
    case L'Ó': return L'ó';
    case L'Ś': return L'ś';
    case L'Ź': return L'ź';
    case L'Ż': return L'ż';
    default: return static_cast<wchar_t>(towlower(ch));
    }
}

std::wstring SpellOutNameForChar(wchar_t ch) {
    switch (LowerPL(ch)) {
    case L'a': return L"a";
    case L'ą': return L"a z ogonkiem";
    case L'b': return L"be";
    case L'c': return L"ce";
    case L'ć': return L"cie z kreską";
    case L'd': return L"de";
    case L'e': return L"e";
    case L'ę': return L"e z ogonkiem";
    case L'f': return L"ef";
    case L'g': return L"gie";
    case L'h': return L"ha";
    case L'i': return L"i";
    case L'j': return L"jot";
    case L'k': return L"ka";
    case L'l': return L"el";
    case L'ł': return L"eł przekreślone";
    case L'm': return L"em";
    case L'n': return L"en";
    case L'ń': return L"eń z kreską";
    case L'o': return L"o";
    case L'ó': return L"u z kreską";
    case L'p': return L"pe";
    case L'q': return L"ku";
    case L'r': return L"er";
    case L's': return L"es";
    case L'ś': return L"ś z kreską";
    case L't': return L"te";
    case L'u': return L"u";
    case L'v': return L"fał";
    case L'w': return L"wu";
    case L'x': return L"iks";
    case L'y': return L"igrek";
    case L'z': return L"zet";
    case L'ź': return L"zie z kreską";
    case L'ż': return L"zet z kropką";
    case L'0': return L"zero";
    case L'1': return L"jeden";
    case L'2': return L"dwa";
    case L'3': return L"trzy";
    case L'4': return L"cztery";
    case L'5': return L"pięć";
    case L'6': return L"sześć";
    case L'7': return L"siedem";
    case L'8': return L"osiem";
    case L'9': return L"dziewięć";
    case L' ': return L"spacja";
    case L'.': return L"kropka";
    case L',': return L"przecinek";
    case L':': return L"dwukropek";
    case L';': return L"średnik";
    case L'!': return L"wykrzyknik";
    case L'?': return L"znak zapytania";
    case L'-': return L"minus";
    case L'+': return L"plus";
    case L'/': return L"ukośnik";
    case L'\\': return L"ukośnik wsteczny";
    case L'"': return L"cudzysłów";
    case L'\'': return L"apostrof";
    default: return std::wstring(1, ch);
    }
}

std::wstring SpellOutText(const wchar_t* text, ULONG len) {
    std::wstring out;
    for (ULONG i = 0; i < len; ++i) {
        const std::wstring name = SpellOutNameForChar(text[i]);
        if (name.empty()) {
            continue;
        }
        if (!out.empty()) {
            out.push_back(L' ');
        }
        out += name;
    }
    return out;
}

std::wstring JoinSpeakText(const SPVTEXTFRAG* fragList) {
    std::wstring out;
    for (auto frag = fragList; frag; frag = frag->pNext) {
        if (frag->pTextStart && frag->ulTextLen > 0 && IsSpeechAction(frag->State.eAction)) {
            if (frag->State.eAction == SPVA_SpellOut) {
                out += SpellOutText(frag->pTextStart, frag->ulTextLen);
            } else {
                out.append(frag->pTextStart, frag->ulTextLen);
            }
            out.push_back(L' ');
        } else if (frag->State.eAction == SPVA_Silence) {
            out.push_back(L' ');
        }
    }
    return CollapseWhitespace(out);
}

HRESULT WriteRegString(HKEY root, const std::wstring& subKey, const wchar_t* name, const wchar_t* value) {
    HKEY key = nullptr;
    LONG rc = RegCreateKeyExW(root, subKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &key, nullptr);
    if (rc != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(rc);
    }
    const DWORD cb = static_cast<DWORD>((wcslen(value) + 1) * sizeof(wchar_t));
    rc = RegSetValueExW(key, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(value), cb);
    RegCloseKey(key);
    return (rc == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(rc);
}

HRESULT DeleteRegTree(HKEY root, const std::wstring& subKey) {
    const LONG rc = RegDeleteTreeW(root, subKey.c_str());
    if (rc == ERROR_FILE_NOT_FOUND || rc == ERROR_PATH_NOT_FOUND) {
        return S_OK;
    }
    return (rc == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(rc);
}

HRESULT RegisterClsidInHive(HKEY, REFCLSID clsidValue, const wchar_t* displayName, const wchar_t* modulePath) {
    if (!displayName || !modulePath) {
        return E_INVALIDARG;
    }
    wchar_t clsid[64] = {0};
    if (StringFromGUID2(clsidValue, clsid, static_cast<int>(std::size(clsid))) == 0) {
        return E_FAIL;
    }

    const std::wstring clsidKey = std::wstring(kPerUserClsidRoot) + L"\\" + clsid;
    HRESULT hr = WriteRegString(HKEY_CURRENT_USER, clsidKey, nullptr, displayName);
    if (FAILED(hr)) {
        return hr;
    }

    const std::wstring inprocKey = clsidKey + L"\\InprocServer32";
    hr = WriteRegString(HKEY_CURRENT_USER, inprocKey, nullptr, modulePath);
    if (FAILED(hr)) {
        return hr;
    }
    return WriteRegString(HKEY_CURRENT_USER, inprocKey, L"ThreadingModel", L"Both");
}

HRESULT UnregisterClsidInHive(HKEY root, REFCLSID clsidValue) {
    wchar_t clsid[64] = {0};
    if (StringFromGUID2(clsidValue, clsid, static_cast<int>(std::size(clsid))) == 0) {
        return E_FAIL;
    }
    const std::wstring clsidKey = std::wstring(kPerUserClsidRoot) + L"\\" + clsid;
    return DeleteRegTree(root, clsidKey);
}

std::wstring GetModulePath() {
    wchar_t path[MAX_PATH] = {0};
    if (GetModuleFileNameW(g_module, path, static_cast<DWORD>(std::size(path))) == 0) {
        return {};
    }
    return std::wstring(path);
}

std::wstring GetModuleDirectory() {
    const std::wstring full = GetModulePath();
    const size_t slash = full.find_last_of(L"\\/");
    return (slash == std::wstring::npos) ? std::wstring() : full.substr(0, slash);
}

std::wstring GetParentDirectory(const std::wstring& path) {
    const size_t slash = path.find_last_of(L"\\/");
    return (slash == std::wstring::npos) ? std::wstring() : path.substr(0, slash);
}

std::wstring JoinPath(const std::wstring& left, const std::wstring& right) {
    if (left.empty()) {
        return right;
    }
    if (left.back() == L'\\' || left.back() == L'/') {
        return left + right;
    }
    return left + L'\\' + right;
}

bool PathExists(const std::wstring& path) {
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool LaunchConfigUi(HWND parentWindow) {
    const std::wstring moduleDir = GetModuleDirectory();
    const std::wstring installDir = GetParentDirectory(moduleDir);
    const std::wstring configExe = JoinPath(installDir, kConfigExeName);
    if (!PathExists(configExe)) {
        return false;
    }

    std::wstring commandLine = L"\"" + configExe + L"\"";
    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWNORMAL;
    if (!CreateProcessW(
        nullptr,
        commandLine.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        installDir.c_str(),
        &si,
        &pi)) {
        return false;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 1;
    (void)GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return exitCode == 0;
}

std::wstring ForwardSlashPath(std::wstring path) {
    for (wchar_t& ch : path) {
        if (ch == L'\\') {
            ch = L'/';
        }
    }
    return path;
}

std::string WideToCodePage(const std::wstring& text, UINT codePage) {
    if (text.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(codePage, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, "?", nullptr);
    if (size <= 0) {
        return {};
    }
    std::string out(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(codePage, 0, text.data(), static_cast<int>(text.size()), out.data(), size, "?", nullptr);
    return out;
}

std::string EscapeSchemeString(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 32);
    for (unsigned char ch : text) {
        switch (ch) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\r':
        case '\n':
        case '\t': out.push_back(' '); break;
        default: out.push_back(static_cast<char>(ch)); break;
        }
    }
    return out;
}

struct TempFiles {
    std::wstring scriptPath;
    std::wstring wavPath;
    std::wstring logPath;
};

bool MakeTempFiles(TempFiles* files) {
    if (!files) {
        return false;
    }
    wchar_t tempDir[MAX_PATH] = {0};
    if (GetTempPathW(static_cast<DWORD>(std::size(tempDir)), tempDir) == 0) {
        return false;
    }
    wchar_t tempFile[MAX_PATH] = {0};
    if (GetTempFileNameW(tempDir, L"wpf", 0, tempFile) == 0) {
        return false;
    }
    DeleteFileW(tempFile);
    const std::wstring base = tempFile;
    files->scriptPath = base + L".scm";
    files->wavPath = base + L".wav";
    files->logPath = base + L".log";
    return true;
}

void CleanupTempFiles(const TempFiles& files) {
    if (!files.scriptPath.empty()) {
        DeleteFileW(files.scriptPath.c_str());
    }
    if (!files.wavPath.empty()) {
        DeleteFileW(files.wavPath.c_str());
    }
    if (!files.logPath.empty()) {
        DeleteFileW(files.logPath.c_str());
    }
}

bool WriteBinaryFile(const std::wstring& path, const std::vector<uint8_t>& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }
    out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return out.good();
}

std::vector<uint8_t> ReadBinaryFile(const std::wstring& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return {};
    }
    in.seekg(0, std::ios::end);
    const auto size = in.tellg();
    if (size <= 0) {
        return {};
    }
    in.seekg(0, std::ios::beg);
    std::vector<uint8_t> data(static_cast<size_t>(size));
    in.read(reinterpret_cast<char*>(data.data()), size);
    if (!in.good() && !in.eof()) {
        return {};
    }
    return data;
}

double RateAdjustToDurationStretch(long rateAdjust) {
    const double clamped = std::clamp(static_cast<double>(rateAdjust), -10.0, 10.0);
    return std::clamp(std::pow(2.0, -clamped / 10.0), 0.45, 2.20);
}

double SiteVolumeToTtsVolume(USHORT volume) {
    const double normalized = std::clamp(static_cast<double>(volume), 0.0, 100.0) / 100.0;
    return std::clamp(2.0 * normalized, 0.0, 2.5);
}

int ClampInt(int value, int lo, int hi) {
    return std::max(lo, std::min(value, hi));
}

struct VoicePreset {
    int baseRateAdjust = 0;
    int baseVolumePercent = 100;
    int pitchMean = kDefaultPitchMean;
    int pitchStd = kDefaultPitchStd;
};

struct UserSettings {
    std::wstring variantToken = kWpFestivalPresets.front().token;
    int speedPercent = kWpFestivalDefaultSpeedPercent;
    int pitchPercent = kWpFestivalDefaultPitchPercent;
    int volumePercent = kWpFestivalDefaultVolumePercent;
};

struct FragmentProsody {
    long rateAdjust = 0;
    long pitchMiddleAdj = 0;
    long pitchRangeAdj = 0;
};

std::wstring ExtractTokenLeafName(const std::wstring& tokenId) {
    const size_t slash = tokenId.find_last_of(L"\\/");
    return (slash == std::wstring::npos) ? tokenId : tokenId.substr(slash + 1);
}

const WpFestivalPresetDef* FindKnownPresetDef(const std::wstring& tokenName) {
    for (const auto& preset : kWpFestivalPresets) {
        if (_wcsicmp(tokenName.c_str(), preset.token) == 0) {
            return &preset;
        }
    }
    return nullptr;
}

VoicePreset VoicePresetFromDef(const WpFestivalPresetDef& presetDef) {
    VoicePreset preset;
    preset.baseRateAdjust = presetDef.baseRateAdjust;
    preset.baseVolumePercent = presetDef.baseVolumePercent;
    preset.pitchMean = presetDef.pitchMean;
    preset.pitchStd = presetDef.pitchStd;
    return preset;
}

bool ReadUserSettingDword(const wchar_t* valueName, DWORD* outValue) {
    if (!valueName || !outValue) {
        return false;
    }
    DWORD value = 0;
    DWORD size = sizeof(value);
    const LONG rc = RegGetValueW(HKEY_CURRENT_USER, kWpFestivalSettingsSubKey, valueName, RRF_RT_REG_DWORD, nullptr, &value, &size);
    if (rc != ERROR_SUCCESS) {
        return false;
    }
    *outValue = value;
    return true;
}

bool ReadUserSettingString(const wchar_t* valueName, std::wstring* outValue) {
    if (!valueName || !outValue) {
        return false;
    }
    DWORD type = 0;
    DWORD size = 0;
    LONG rc = RegGetValueW(HKEY_CURRENT_USER, kWpFestivalSettingsSubKey, valueName, RRF_RT_REG_SZ, &type, nullptr, &size);
    if (rc != ERROR_SUCCESS || size < sizeof(wchar_t)) {
        return false;
    }
    std::wstring buffer(size / sizeof(wchar_t), L'\0');
    rc = RegGetValueW(HKEY_CURRENT_USER, kWpFestivalSettingsSubKey, valueName, RRF_RT_REG_SZ, nullptr, buffer.data(), &size);
    if (rc != ERROR_SUCCESS) {
        return false;
    }
    const size_t terminator = buffer.find(L'\0');
    if (terminator != std::wstring::npos) {
        buffer.resize(terminator);
    }
    *outValue = buffer;
    return !outValue->empty();
}

int ReadUserPercentSetting(const wchar_t* valueName, int defaultValue, int minValue, int maxValue) {
    DWORD value = 0;
    if (!ReadUserSettingDword(valueName, &value)) {
        return defaultValue;
    }
    return ClampInt(static_cast<int>(value), minValue, maxValue);
}

UserSettings LoadUserSettings() {
    UserSettings settings;
    settings.speedPercent = ReadUserPercentSetting(
        kWpFestivalSpeedPercentValueName,
        kWpFestivalDefaultSpeedPercent,
        kWpFestivalMinSpeedPercent,
        kWpFestivalMaxSpeedPercent
    );
    settings.pitchPercent = ReadUserPercentSetting(
        kWpFestivalPitchPercentValueName,
        kWpFestivalDefaultPitchPercent,
        kWpFestivalMinPitchPercent,
        kWpFestivalMaxPitchPercent
    );
    settings.volumePercent = ReadUserPercentSetting(
        kWpFestivalVolumePercentValueName,
        kWpFestivalDefaultVolumePercent,
        kWpFestivalMinVolumePercent,
        kWpFestivalMaxVolumePercent
    );

    std::wstring variantToken;
    if (ReadUserSettingString(kWpFestivalVariantTokenValueName, &variantToken)) {
        variantToken = ExtractTokenLeafName(variantToken);
        if (FindKnownPresetDef(variantToken)) {
            settings.variantToken = variantToken;
        }
    }
    return settings;
}

int PercentToSignedAdjust(int percent) {
    return ClampInt(static_cast<int>(std::lround((static_cast<double>(percent) - 50.0) / 5.0)), -10, 10);
}

int PitchMiddleAdjustToMeanDelta(long middleAdj) {
    return static_cast<int>(std::lround(static_cast<double>(middleAdj) * 3.0));
}

int PitchRangeAdjustToStdDelta(long rangeAdj) {
    return static_cast<int>(std::lround(static_cast<double>(rangeAdj)));
}

FragmentProsody AnalyzeSpeechProsody(const SPVTEXTFRAG* fragList) {
    FragmentProsody prosody;
    long long rateSum = 0;
    long long pitchMiddleSum = 0;
    long long pitchRangeSum = 0;
    long long totalWeight = 0;

    for (auto frag = fragList; frag; frag = frag->pNext) {
        if (!frag->pTextStart || frag->ulTextLen == 0 || !IsSpeechAction(frag->State.eAction)) {
            continue;
        }
        const long long weight = std::max<long long>(1, frag->ulTextLen);
        totalWeight += weight;
        rateSum += static_cast<long long>(ClampInt(frag->State.RateAdj, -10, 10)) * weight;
        pitchMiddleSum += static_cast<long long>(ClampInt(frag->State.PitchAdj.MiddleAdj, -10, 10)) * weight;
        pitchRangeSum += static_cast<long long>(ClampInt(frag->State.PitchAdj.RangeAdj, -10, 10)) * weight;
    }

    if (totalWeight > 0) {
        prosody.rateAdjust = static_cast<long>(std::llround(static_cast<double>(rateSum) / static_cast<double>(totalWeight)));
        prosody.pitchMiddleAdj = static_cast<long>(std::llround(static_cast<double>(pitchMiddleSum) / static_cast<double>(totalWeight)));
        prosody.pitchRangeAdj = static_cast<long>(std::llround(static_cast<double>(pitchRangeSum) / static_cast<double>(totalWeight)));
    }
    return prosody;
}

VoicePreset BuildEffectivePreset(const VoicePreset& tokenPreset, const std::wstring& tokenName, const UserSettings& settings, const FragmentProsody& prosody) {
    VoicePreset preset = tokenPreset;

    if (_wcsicmp(tokenName.c_str(), kWpFestivalPresets.front().token) == 0) {
        if (const auto* variantPreset = FindKnownPresetDef(settings.variantToken)) {
            preset = VoicePresetFromDef(*variantPreset);
        }
    }

    preset.baseRateAdjust = ClampInt(preset.baseRateAdjust + PercentToSignedAdjust(settings.speedPercent), -10, 10);
    preset.baseVolumePercent = ClampInt(
        static_cast<int>(std::lround((static_cast<double>(preset.baseVolumePercent) * settings.volumePercent) / 100.0)),
        kWpFestivalMinVolumePercent,
        kWpFestivalMaxVolumePercent
    );

    const int totalPitchMiddleAdj = PercentToSignedAdjust(settings.pitchPercent) + static_cast<int>(prosody.pitchMiddleAdj);
    preset.pitchMean = ClampInt(preset.pitchMean + PitchMiddleAdjustToMeanDelta(totalPitchMiddleAdj), 70, 180);
    preset.pitchStd = ClampInt(preset.pitchStd + PitchRangeAdjustToStdDelta(prosody.pitchRangeAdj), 6, 40);
    return preset;
}

bool BuildSchemeScript(const std::wstring& text, long rateAdjust, USHORT volume, const VoicePreset& preset, const std::wstring& wavPath, std::vector<uint8_t>* outBytes) {
    if (!outBytes) {
        return false;
    }

    const std::string textCp1250 = EscapeSchemeString(WideToCodePage(text, 1250));
    const std::string wavUtf8 = EscapeSchemeString(WideToCodePage(ForwardSlashPath(wavPath), CP_UTF8));
    if (textCp1250.empty() || wavUtf8.empty()) {
        return false;
    }

    std::ostringstream script;
    const long finalRateAdjust = std::clamp<long>(rateAdjust + preset.baseRateAdjust, -10, 10);
    const int finalVolumePercent = ClampInt(static_cast<int>(std::lround((static_cast<double>(volume) * preset.baseVolumePercent) / 100.0)), 0, 150);

    script << "(voice_wp_pl_m1_diphone)\n";
    script << "(set! int_lr_params '((target_f0_mean " << preset.pitchMean << ") (target_f0_std " << preset.pitchStd
           << ") (model_f0_mean " << kDefaultModelPitchMean << ") (model_f0_std " << kDefaultModelPitchStd << ")))\n";
    script << "(set! wp_sapi_pitch_mean " << preset.pitchMean << ")\n";
    script << "(set! wp_sapi_pitch_std_scale " << (static_cast<double>(preset.pitchStd) / static_cast<double>(kDefaultPitchStd)) << ")\n";
    script << "(define (wp_sapi_pitch_target_wrap utt syl)\n";
    script << "  (mapcar\n";
    script << "    (lambda (target)\n";
    script << "      (let ((scaled (+ wp_sapi_pitch_mean (* (- (cadr target) " << kDefaultPitchMean << ") wp_sapi_pitch_std_scale))))\n";
    script << "        (list\n";
    script << "          (car target)\n";
    script << "          (if (< scaled 55) 55 (if (> scaled 220) 220 scaled)))))\n";
    script << "    (wp_pl_m1_targ_func1 utt syl)))\n";
    script << "(set! int_general_params (list (list 'targ_func wp_sapi_pitch_target_wrap)))\n";
    script << "(set! Set_Duration_Stretch " << RateAdjustToDurationStretch(finalRateAdjust) << ")\n";
    script << "(Parameter.set 'Duration_Stretch Set_Duration_Stretch)\n";
    script << "(set! tts_volume " << SiteVolumeToTtsVolume(static_cast<USHORT>(finalVolumePercent)) << ")\n";
    script << "(set! utt1 (Utterance Text \"" << textCp1250 << "\"))\n";
    script << "(utt.synth utt1)\n";
    script << "(utt.wave.rescale utt1 tts_volume nil)\n";
    script << "(utt.save.wave utt1 \"" << wavUtf8 << "\" 'riff)\n";
    script << "(exit)\n";

    const std::string bytes = script.str();
    outBytes->assign(bytes.begin(), bytes.end());
    return true;
}

bool RunFestival(const std::wstring& festivalExe, const std::wstring& runtimeDir, const TempFiles& files, ISpTTSEngineSite* site) {
    HANDLE logHandle = CreateFileW(
        files.logPath.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY,
        nullptr
    );
    if (logHandle == INVALID_HANDLE_VALUE) {
        return false;
    }

    const std::wstring command =
        L"\"" + festivalExe + L"\" --libdir \"" + runtimeDir + L"\" -b \"" + files.scriptPath + L"\"";

    STARTUPINFOW si = {};
    PROCESS_INFORMATION pi = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = logHandle;
    si.hStdError = logHandle;

    std::wstring mutableCommand = command;
    const BOOL ok = CreateProcessW(
        nullptr,
        mutableCommand.data(),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        GetModuleDirectory().c_str(),
        &si,
        &pi
    );
    CloseHandle(logHandle);
    if (!ok) {
        return false;
    }

    bool success = false;
    const DWORD started = GetTickCount();
    for (;;) {
        const DWORD wait = WaitForSingleObject(pi.hProcess, 100);
        if (wait == WAIT_OBJECT_0) {
            DWORD exitCode = 0;
            if (GetExitCodeProcess(pi.hProcess, &exitCode) && exitCode == 0) {
                success = true;
            }
            break;
        }
        if (wait != WAIT_TIMEOUT) {
            break;
        }
        if (site && (site->GetActions() & SPVES_ABORT) != 0) {
            TerminateProcess(pi.hProcess, 1);
            WaitForSingleObject(pi.hProcess, 1000);
            break;
        }
        if (GetTickCount() - started > kFestivalTimeoutMs) {
            TerminateProcess(pi.hProcess, 1);
            WaitForSingleObject(pi.hProcess, 1000);
            break;
        }
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return success;
}

bool ReadLe16(const std::vector<uint8_t>& data, size_t offset, uint16_t* value) {
    if (!value || offset + 2 > data.size()) {
        return false;
    }
    *value = static_cast<uint16_t>(data[offset] | (data[offset + 1] << 8));
    return true;
}

bool ReadLe32(const std::vector<uint8_t>& data, size_t offset, uint32_t* value) {
    if (!value || offset + 4 > data.size()) {
        return false;
    }
    *value =
        static_cast<uint32_t>(data[offset]) |
        (static_cast<uint32_t>(data[offset + 1]) << 8) |
        (static_cast<uint32_t>(data[offset + 2]) << 16) |
        (static_cast<uint32_t>(data[offset + 3]) << 24);
    return true;
}

bool ParseWavePcm16Mono(const std::vector<uint8_t>& wav, std::vector<uint8_t>* pcmOut) {
    if (!pcmOut || wav.size() < 44) {
        return false;
    }
    if (memcmp(wav.data(), "RIFF", 4) != 0 || memcmp(wav.data() + 8, "WAVE", 4) != 0) {
        return false;
    }

    uint16_t formatTag = 0;
    uint16_t channels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    size_t dataOffset = 0;
    uint32_t dataSize = 0;

    size_t cursor = 12;
    while (cursor + 8 <= wav.size()) {
        const char* chunkId = reinterpret_cast<const char*>(wav.data() + cursor);
        uint32_t chunkSize = 0;
        if (!ReadLe32(wav, cursor + 4, &chunkSize)) {
            return false;
        }
        const size_t chunkData = cursor + 8;
        const size_t nextChunk = chunkData + chunkSize + (chunkSize & 1U);
        if (nextChunk > wav.size()) {
            return false;
        }

        if (memcmp(chunkId, "fmt ", 4) == 0) {
            if (!ReadLe16(wav, chunkData + 0, &formatTag) ||
                !ReadLe16(wav, chunkData + 2, &channels) ||
                !ReadLe32(wav, chunkData + 4, &sampleRate) ||
                !ReadLe16(wav, chunkData + 14, &bitsPerSample)) {
                return false;
            }
        } else if (memcmp(chunkId, "data", 4) == 0) {
            dataOffset = chunkData;
            dataSize = chunkSize;
        }

        cursor = nextChunk;
    }

    if (formatTag != WAVE_FORMAT_PCM || channels != 1 || sampleRate != kSampleRate || bitsPerSample != 16) {
        return false;
    }
    if (dataOffset == 0 || dataOffset + dataSize > wav.size()) {
        return false;
    }

    pcmOut->assign(wav.begin() + static_cast<std::ptrdiff_t>(dataOffset), wav.begin() + static_cast<std::ptrdiff_t>(dataOffset + dataSize));
    return true;
}

bool SynthesizeWithFestival(const std::wstring& text, long rateAdjust, USHORT volume, const VoicePreset& preset, ISpTTSEngineSite* site, std::vector<uint8_t>* pcmOut) {
    if (!site || !pcmOut) {
        return false;
    }

    const std::wstring moduleDir = GetModuleDirectory();
    const std::wstring festivalExe = JoinPath(moduleDir, kFestivalExeName);
    const std::wstring runtimeDir = JoinPath(moduleDir, kRuntimeDirName);
    if (!PathExists(festivalExe) || !PathExists(runtimeDir)) {
        return false;
    }

    TempFiles files;
    if (!MakeTempFiles(&files)) {
        return false;
    }

    std::vector<uint8_t> scriptBytes;
    const bool built = BuildSchemeScript(text, rateAdjust, volume, preset, files.wavPath, &scriptBytes);
    if (!built || !WriteBinaryFile(files.scriptPath, scriptBytes)) {
        CleanupTempFiles(files);
        return false;
    }

    const bool ran = RunFestival(festivalExe, runtimeDir, files, site);
    if (!ran || !PathExists(files.wavPath)) {
        CleanupTempFiles(files);
        return false;
    }

    const std::vector<uint8_t> wavBytes = ReadBinaryFile(files.wavPath);
    const bool parsed = ParseWavePcm16Mono(wavBytes, pcmOut);
    CleanupTempFiles(files);
    return parsed;
}

bool IsPcm16Mono(const WAVEFORMATEX* fmt) {
    return fmt &&
           fmt->wFormatTag == WAVE_FORMAT_PCM &&
           fmt->nChannels == 1 &&
           fmt->wBitsPerSample == 16 &&
           fmt->nSamplesPerSec == kSampleRate;
}

bool ReadTokenInt(ISpObjectToken* token, const wchar_t* name, int minValue, int maxValue, int* outValue) {
    if (!token || !name || !outValue) {
        return false;
    }
    wchar_t* text = nullptr;
    if (SUCCEEDED(token->GetStringValue(name, &text)) && text) {
        *outValue = ClampInt(_wtoi(text), minValue, maxValue);
        CoTaskMemFree(text);
        return true;
    }
    ULONG dwordValue = 0;
    if (SUCCEEDED(token->GetDWORD(name, &dwordValue))) {
        *outValue = ClampInt(static_cast<int>(dwordValue), minValue, maxValue);
        return true;
    }
    return false;
}

class WPFestivalEngine final : public ISpTTSEngine, public ISpObjectWithToken {
public:
    WPFestivalEngine() : refCount_(1), token_(nullptr), tokenName_(kWpFestivalPresets.front().token) {
        g_objects.fetch_add(1, std::memory_order_relaxed);
    }

    ~WPFestivalEngine() {
        if (token_) {
            token_->Release();
            token_ = nullptr;
        }
        g_objects.fetch_sub(1, std::memory_order_relaxed);
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) {
            return E_POINTER;
        }
        *ppv = nullptr;
        if (riid == IID_IUnknown || riid == __uuidof(ISpTTSEngine)) {
            *ppv = static_cast<ISpTTSEngine*>(this);
        } else if (riid == __uuidof(ISpObjectWithToken)) {
            *ppv = static_cast<ISpObjectWithToken*>(this);
        } else {
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return static_cast<ULONG>(InterlockedIncrement(reinterpret_cast<LONG*>(&refCount_)));
    }

    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG value = static_cast<ULONG>(InterlockedDecrement(reinterpret_cast<LONG*>(&refCount_)));
        if (value == 0) {
            delete this;
        }
        return value;
    }

    HRESULT STDMETHODCALLTYPE SetObjectToken(ISpObjectToken* token) override {
        if (token_) {
            token_->Release();
            token_ = nullptr;
        }
        tokenName_ = kWpFestivalPresets.front().token;
        if (token) {
            token->AddRef();
            token_ = token;
        }
        tokenPreset_ = {};
        if (token_) {
            (void)ReadTokenInt(token_, L"BaseRateAdjust", -10, 10, &tokenPreset_.baseRateAdjust);
            (void)ReadTokenInt(token_, L"BaseVolume", 0, 150, &tokenPreset_.baseVolumePercent);
            (void)ReadTokenInt(token_, L"PitchMean", 70, 180, &tokenPreset_.pitchMean);
            (void)ReadTokenInt(token_, L"PitchStd", 6, 40, &tokenPreset_.pitchStd);

            wchar_t* tokenId = nullptr;
            if (SUCCEEDED(token_->GetId(&tokenId)) && tokenId) {
                tokenName_ = ExtractTokenLeafName(tokenId);
                CoTaskMemFree(tokenId);
            }
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetObjectToken(ISpObjectToken** ppToken) override {
        if (!ppToken) {
            return E_POINTER;
        }
        *ppToken = token_;
        if (token_) {
            token_->AddRef();
        }
        return token_ ? S_OK : S_FALSE;
    }

    HRESULT STDMETHODCALLTYPE Speak(DWORD, REFGUID, const WAVEFORMATEX*, const SPVTEXTFRAG* textFragList, ISpTTSEngineSite* outputSite) override {
        if (!outputSite) {
            return E_POINTER;
        }
        if ((outputSite->GetActions() & SPVES_ABORT) != 0) {
            return S_OK;
        }

        const std::wstring text = JoinSpeakText(textFragList);
        if (text.empty()) {
            return S_OK;
        }

        long rateAdjust = 0;
        (void)outputSite->GetRate(&rateAdjust);
        USHORT volume = 100;
        (void)outputSite->GetVolume(&volume);
        const FragmentProsody prosody = AnalyzeSpeechProsody(textFragList);
        const UserSettings userSettings = LoadUserSettings();
        const VoicePreset effectivePreset = BuildEffectivePreset(tokenPreset_, tokenName_, userSettings, prosody);
        rateAdjust = ClampInt(rateAdjust + static_cast<int>(prosody.rateAdjust), -10, 10);

        std::vector<uint8_t> pcm;
        if (!SynthesizeWithFestival(text, rateAdjust, volume, effectivePreset, outputSite, &pcm)) {
            return E_FAIL;
        }
        if (pcm.empty()) {
            return S_OK;
        }

        ULONG written = 0;
        return outputSite->Write(pcm.data(), static_cast<ULONG>(pcm.size()), &written);
    }

    HRESULT STDMETHODCALLTYPE GetOutputFormat(const GUID* targetFmtId, const WAVEFORMATEX* targetWaveFmt, GUID* outputFormatId, WAVEFORMATEX** ppCoMemOutputWaveFormatEx) override {
        if (!outputFormatId || !ppCoMemOutputWaveFormatEx) {
            return E_POINTER;
        }
        *ppCoMemOutputWaveFormatEx = nullptr;

        auto* fmt = static_cast<WAVEFORMATEX*>(CoTaskMemAlloc(sizeof(WAVEFORMATEX)));
        if (!fmt) {
            return E_OUTOFMEMORY;
        }

        if (targetFmtId && *targetFmtId == kSpdfidWaveFormatEx && IsPcm16Mono(targetWaveFmt)) {
            *fmt = *targetWaveFmt;
        } else {
            fmt->wFormatTag = WAVE_FORMAT_PCM;
            fmt->nChannels = 1;
            fmt->nSamplesPerSec = kSampleRate;
            fmt->wBitsPerSample = 16;
            fmt->nBlockAlign = static_cast<WORD>((fmt->nChannels * fmt->wBitsPerSample) / 8);
            fmt->nAvgBytesPerSec = fmt->nSamplesPerSec * fmt->nBlockAlign;
            fmt->cbSize = 0;
        }

        *outputFormatId = kSpdfidWaveFormatEx;
        *ppCoMemOutputWaveFormatEx = fmt;
        return S_OK;
    }

private:
    ULONG refCount_;
    ISpObjectToken* token_;
    std::wstring tokenName_;
    VoicePreset tokenPreset_;
};

class WPFestivalTokenUi final : public ISpTokenUI {
public:
    WPFestivalTokenUi() : refCount_(1) {
        g_objects.fetch_add(1, std::memory_order_relaxed);
    }

    ~WPFestivalTokenUi() {
        g_objects.fetch_sub(1, std::memory_order_relaxed);
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) {
            return E_POINTER;
        }
        *ppv = nullptr;
        if (riid == IID_IUnknown || riid == IID_ISpTokenUI) {
            *ppv = static_cast<ISpTokenUI*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return static_cast<ULONG>(InterlockedIncrement(reinterpret_cast<LONG*>(&refCount_)));
    }

    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG value = static_cast<ULONG>(InterlockedDecrement(reinterpret_cast<LONG*>(&refCount_)));
        if (value == 0) {
            delete this;
        }
        return value;
    }

    HRESULT STDMETHODCALLTYPE IsUISupported(
        const WCHAR* pszTypeOfUI,
        void*,
        ULONG,
        IUnknown*,
        BOOL* pfSupported) override {
        if (!pfSupported) {
            return E_POINTER;
        }
        *pfSupported = (pszTypeOfUI && wcscmp(pszTypeOfUI, SPDUI_EngineProperties) == 0) ? TRUE : FALSE;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE DisplayUI(
        HWND hwndParent,
        const WCHAR*,
        const WCHAR* pszTypeOfUI,
        void*,
        ULONG,
        ISpObjectToken*,
        IUnknown*) override {
        if (!pszTypeOfUI || wcscmp(pszTypeOfUI, SPDUI_EngineProperties) != 0) {
            return E_NOTIMPL;
        }
        return LaunchConfigUi(hwndParent) ? S_OK : E_FAIL;
    }

private:
    ULONG refCount_;
};

enum class FactoryKind {
    Engine,
    TokenUi
};

class ClassFactory final : public IClassFactory {
public:
    explicit ClassFactory(FactoryKind kind) : refCount_(1), kind_(kind) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) {
            return E_POINTER;
        }
        *ppv = nullptr;
        if (riid == IID_IUnknown || riid == IID_IClassFactory) {
            *ppv = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return static_cast<ULONG>(InterlockedIncrement(reinterpret_cast<LONG*>(&refCount_)));
    }

    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG value = static_cast<ULONG>(InterlockedDecrement(reinterpret_cast<LONG*>(&refCount_)));
        if (value == 0) {
            delete this;
        }
        return value;
    }

    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer, REFIID riid, void** ppv) override {
        if (outer != nullptr) {
            return CLASS_E_NOAGGREGATION;
        }
        if (kind_ == FactoryKind::Engine) {
            auto* engine = new (std::nothrow) WPFestivalEngine();
            if (!engine) {
                return E_OUTOFMEMORY;
            }
            const HRESULT hr = engine->QueryInterface(riid, ppv);
            engine->Release();
            return hr;
        }

        auto* tokenUi = new (std::nothrow) WPFestivalTokenUi();
        if (!tokenUi) {
            return E_OUTOFMEMORY;
        }
        const HRESULT hr = tokenUi->QueryInterface(riid, ppv);
        tokenUi->Release();
        return hr;
    }

    HRESULT STDMETHODCALLTYPE LockServer(BOOL lock) override {
        if (lock) {
            g_locks.fetch_add(1, std::memory_order_relaxed);
        } else {
            g_locks.fetch_sub(1, std::memory_order_relaxed);
        }
        return S_OK;
    }

private:
    ULONG refCount_;
    FactoryKind kind_;
};

} // namespace

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}

extern "C" HRESULT __stdcall DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
    if (!ppv) {
        return E_POINTER;
    }
    *ppv = nullptr;
    FactoryKind kind;
    if (rclsid == CLSID_WPFestivalSapi5) {
        kind = FactoryKind::Engine;
    } else if (rclsid == CLSID_WPFestivalSapi5Ui) {
        kind = FactoryKind::TokenUi;
    } else {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    auto* factory = new (std::nothrow) ClassFactory(kind);
    if (!factory) {
        return E_OUTOFMEMORY;
    }
    const HRESULT hr = factory->QueryInterface(riid, ppv);
    factory->Release();
    return hr;
}

extern "C" HRESULT __stdcall DllCanUnloadNow(void) {
    return (g_objects.load(std::memory_order_relaxed) == 0 && g_locks.load(std::memory_order_relaxed) == 0) ? S_OK : S_FALSE;
}

extern "C" HRESULT __stdcall DllRegisterServer(void) {
    const std::wstring modulePath = GetModulePath();
    if (modulePath.empty()) {
        return E_FAIL;
    }

    HRESULT hr = RegisterClsidInHive(HKEY_CURRENT_USER, CLSID_WPFestivalSapi5, kEngineName, modulePath.c_str());
    if (FAILED(hr)) {
        return hr;
    }
    return RegisterClsidInHive(HKEY_CURRENT_USER, CLSID_WPFestivalSapi5Ui, kUiName, modulePath.c_str());
}

extern "C" HRESULT __stdcall DllUnregisterServer(void) {
    HRESULT hr = UnregisterClsidInHive(HKEY_CURRENT_USER, CLSID_WPFestivalSapi5);
    if (FAILED(hr)) {
        return hr;
    }
    return UnregisterClsidInHive(HKEY_CURRENT_USER, CLSID_WPFestivalSapi5Ui);
}
