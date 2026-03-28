#include <windows.h>
#include <commctrl.h>
#include <sapi.h>

#include <array>
#include <cmath>
#include <cwchar>
#include <string>

#include "../common/wp_festival_shared.h"

namespace {

enum ControlId : int {
    IDC_TITLE_TEXT = 900,
    IDC_SUBTITLE_TEXT,
    IDC_VARIANT_LABEL,
    IDC_VARIANT_COMBO,
    IDC_VARIANT_HELP_TEXT,
    IDC_SPEED_LABEL,
    IDC_SPEED_TRACK,
    IDC_SPEED_VALUE,
    IDC_PITCH_LABEL,
    IDC_PITCH_TRACK,
    IDC_PITCH_VALUE,
    IDC_VOLUME_LABEL,
    IDC_VOLUME_TRACK,
    IDC_VOLUME_VALUE,
    IDC_PARAMETERS_HELP_TEXT,
    IDC_PREVIEW_LABEL,
    IDC_PREVIEW_HELP_TEXT,
    IDC_PREVIEW_EDIT,
    IDC_PREVIEW_SPEAK_BUTTON,
    IDC_PREVIEW_STOP_BUTTON,
    IDC_SAVE_BUTTON,
    IDC_RESET_BUTTON,
    IDC_CLOSE_BUTTON,
};

struct SliderDef {
    int labelId;
    int trackId;
    int valueId;
    const wchar_t* label;
    const wchar_t* settingName;
    int defaultValue;
    int minValue;
    int maxValue;
};

struct UiSettings {
    std::wstring variantToken = kWpFestivalPresets.front().token;
    int speedPercent = kWpFestivalDefaultSpeedPercent;
    int pitchPercent = kWpFestivalDefaultPitchPercent;
    int volumePercent = kWpFestivalDefaultVolumePercent;
};

bool operator==(const UiSettings& left, const UiSettings& right) {
    return left.variantToken == right.variantToken &&
           left.speedPercent == right.speedPercent &&
           left.pitchPercent == right.pitchPercent &&
           left.volumePercent == right.volumePercent;
}

bool operator!=(const UiSettings& left, const UiSettings& right) {
    return !(left == right);
}

struct AppState {
    HBRUSH backgroundBrush = nullptr;
    HBRUSH editBrush = nullptr;
    HFONT baseFont = nullptr;
    HFONT titleFont = nullptr;
    HFONT sectionFont = nullptr;
    ISpVoice* previewVoice = nullptr;
    UiSettings originalSettings{};
    bool previewApplied = false;
};

constexpr COLORREF kWindowColor = RGB(10, 10, 10);
constexpr COLORREF kEditColor = RGB(20, 20, 20);
constexpr COLORREF kTextColor = RGB(242, 204, 52);
constexpr COLORREF kDimTextColor = RGB(186, 164, 90);
constexpr COLORREF kHintTextColor = RGB(150, 135, 80);
constexpr int kWindowWidth = 640;
constexpr int kWindowHeight = 700;

constexpr std::array<SliderDef, 3> kSliders = { {
    {
        IDC_SPEED_LABEL,
        IDC_SPEED_TRACK,
        IDC_SPEED_VALUE,
        L"Prędkość",
        kWpFestivalSpeedPercentValueName,
        kWpFestivalDefaultSpeedPercent,
        kWpFestivalMinSpeedPercent,
        kWpFestivalMaxSpeedPercent
    },
    {
        IDC_PITCH_LABEL,
        IDC_PITCH_TRACK,
        IDC_PITCH_VALUE,
        L"Wysokość",
        kWpFestivalPitchPercentValueName,
        kWpFestivalDefaultPitchPercent,
        kWpFestivalMinPitchPercent,
        kWpFestivalMaxPitchPercent
    },
    {
        IDC_VOLUME_LABEL,
        IDC_VOLUME_TRACK,
        IDC_VOLUME_VALUE,
        L"Głośność",
        kWpFestivalVolumePercentValueName,
        kWpFestivalDefaultVolumePercent,
        kWpFestivalMinVolumePercent,
        kWpFestivalMaxVolumePercent
    },
} };

int ClampInt(int value, int minValue, int maxValue) {
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

AppState* GetState(HWND hwnd) {
    return reinterpret_cast<AppState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}

HFONT CreateUiFont(int pointSize, bool bold) {
    HDC dc = GetDC(nullptr);
    const int dpi = dc ? GetDeviceCaps(dc, LOGPIXELSY) : 96;
    if (dc) {
        ReleaseDC(nullptr, dc);
    }
    return CreateFontW(
        -MulDiv(pointSize, dpi, 72),
        0,
        0,
        0,
        bold ? FW_BOLD : FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        L"Segoe UI"
    );
}

void ApplyFont(HWND hwnd, HFONT font) {
    SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

bool ReadSettingDword(const wchar_t* valueName, DWORD* outValue) {
    if (!valueName || !outValue) {
        return false;
    }
    DWORD value = 0;
    DWORD size = sizeof(value);
    const LONG rc = RegGetValueW(
        HKEY_CURRENT_USER,
        kWpFestivalSettingsSubKey,
        valueName,
        RRF_RT_REG_DWORD,
        nullptr,
        &value,
        &size
    );
    if (rc != ERROR_SUCCESS) {
        return false;
    }
    *outValue = value;
    return true;
}

bool ReadSettingString(const wchar_t* valueName, std::wstring* outValue) {
    if (!valueName || !outValue) {
        return false;
    }
    DWORD size = 0;
    LONG rc = RegGetValueW(HKEY_CURRENT_USER, kWpFestivalSettingsSubKey, valueName, RRF_RT_REG_SZ, nullptr, nullptr, &size);
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
    if (buffer.empty()) {
        return false;
    }
    *outValue = buffer;
    return true;
}

bool WriteSettingDword(const wchar_t* valueName, int value) {
    HKEY key = nullptr;
    const LONG rc = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        kWpFestivalSettingsSubKey,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        nullptr,
        &key,
        nullptr
    );
    if (rc != ERROR_SUCCESS) {
        return false;
    }
    const DWORD out = static_cast<DWORD>(value);
    const LONG setRc = RegSetValueExW(key, valueName, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&out), sizeof(out));
    RegCloseKey(key);
    return setRc == ERROR_SUCCESS;
}

bool WriteSettingString(const wchar_t* valueName, const std::wstring& value) {
    HKEY key = nullptr;
    const LONG rc = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        kWpFestivalSettingsSubKey,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        nullptr,
        &key,
        nullptr
    );
    if (rc != ERROR_SUCCESS) {
        return false;
    }
    const DWORD size = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
    const LONG setRc = RegSetValueExW(key, valueName, 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), size);
    RegCloseKey(key);
    return setRc == ERROR_SUCCESS;
}

UiSettings LoadSettings() {
    UiSettings settings;

    DWORD dwordValue = 0;
    if (ReadSettingDword(kWpFestivalSpeedPercentValueName, &dwordValue)) {
        settings.speedPercent = ClampInt(static_cast<int>(dwordValue), kWpFestivalMinSpeedPercent, kWpFestivalMaxSpeedPercent);
    }
    if (ReadSettingDword(kWpFestivalPitchPercentValueName, &dwordValue)) {
        settings.pitchPercent = ClampInt(static_cast<int>(dwordValue), kWpFestivalMinPitchPercent, kWpFestivalMaxPitchPercent);
    }
    if (ReadSettingDword(kWpFestivalVolumePercentValueName, &dwordValue)) {
        settings.volumePercent = ClampInt(static_cast<int>(dwordValue), kWpFestivalMinVolumePercent, kWpFestivalMaxVolumePercent);
    }

    std::wstring variantToken;
    if (ReadSettingString(kWpFestivalVariantTokenValueName, &variantToken)) {
        settings.variantToken = variantToken;
    }
    return settings;
}

bool SaveSettings(const UiSettings& settings) {
    bool ok = WriteSettingString(kWpFestivalVariantTokenValueName, settings.variantToken);
    ok = WriteSettingDword(kWpFestivalSpeedPercentValueName, settings.speedPercent) && ok;
    ok = WriteSettingDword(kWpFestivalPitchPercentValueName, settings.pitchPercent) && ok;
    ok = WriteSettingDword(kWpFestivalVolumePercentValueName, settings.volumePercent) && ok;
    return ok;
}

int PercentToSignedAdjust(int percent) {
    return ClampInt(static_cast<int>(std::lround((static_cast<double>(percent) - 50.0) / 5.0)), -10, 10);
}

std::wstring FormatSliderValue(const SliderDef& slider, int value) {
    wchar_t buffer[48] = {0};
    if (slider.trackId == IDC_VOLUME_TRACK) {
        swprintf_s(buffer, L"%d%%", value);
    } else {
        swprintf_s(buffer, L"%d%% (%+d)", value, PercentToSignedAdjust(value));
    }
    return buffer;
}

std::wstring ReadWindowTextString(HWND hwnd) {
    const int length = GetWindowTextLengthW(hwnd);
    std::wstring value(static_cast<size_t>(length) + 1, L'\0');
    GetWindowTextW(hwnd, value.data(), length + 1);
    value.resize(static_cast<size_t>(length));
    return value;
}

std::wstring TrimmedText(std::wstring value) {
    const size_t first = value.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) {
        return {};
    }
    const size_t last = value.find_last_not_of(L" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::wstring ExtractTokenLeafName(const std::wstring& tokenId) {
    const size_t slash = tokenId.find_last_of(L"\\/");
    return (slash == std::wstring::npos) ? tokenId : tokenId.substr(slash + 1);
}

HWND CreateLabel(HWND parent, int id, const wchar_t* text, int x, int y, int w, int h, HFONT font) {
    HWND hwnd = CreateWindowExW(
        0,
        WC_STATICW,
        text,
        WS_CHILD | WS_VISIBLE,
        x,
        y,
        w,
        h,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        nullptr,
        nullptr
    );
    ApplyFont(hwnd, font);
    return hwnd;
}

HWND CreateButton(HWND parent, int id, const wchar_t* text, int x, int y, int w, int h, HFONT font) {
    const DWORD style = (id == IDC_SAVE_BUTTON)
        ? (WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON)
        : (WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON);
    HWND hwnd = CreateWindowExW(
        0,
        WC_BUTTONW,
        text,
        style,
        x,
        y,
        w,
        h,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        nullptr,
        nullptr
    );
    ApplyFont(hwnd, font);
    return hwnd;
}

HWND CreateVariantCombo(HWND parent, int x, int y, int w, int h, HFONT font) {
    HWND hwnd = CreateWindowExW(
        0,
        WC_COMBOBOXW,
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        x,
        y,
        w,
        h,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_VARIANT_COMBO)),
        nullptr,
        nullptr
    );
    ApplyFont(hwnd, font);
    return hwnd;
}

HWND CreateTrack(HWND parent, const SliderDef& slider, int x, int y, int w, int h, HFONT font) {
    HWND hwnd = CreateWindowExW(
        0,
        TRACKBAR_CLASSW,
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_AUTOTICKS,
        x,
        y,
        w,
        h,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(slider.trackId)),
        nullptr,
        nullptr
    );
    SendMessageW(hwnd, TBM_SETRANGEMIN, TRUE, slider.minValue);
    SendMessageW(hwnd, TBM_SETRANGEMAX, TRUE, slider.maxValue);
    SendMessageW(hwnd, TBM_SETTICFREQ, slider.trackId == IDC_VOLUME_TRACK ? 10 : 5, 0);
    SendMessageW(hwnd, TBM_SETPAGESIZE, 0, slider.trackId == IDC_VOLUME_TRACK ? 10 : 5);
    ApplyFont(hwnd, font);
    return hwnd;
}

HWND CreatePreviewEdit(HWND parent, int x, int y, int w, int h, HFONT font) {
    HWND hwnd = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        WC_EDITW,
        nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL,
        x,
        y,
        w,
        h,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_PREVIEW_EDIT)),
        nullptr,
        nullptr
    );
    ApplyFont(hwnd, font);
    return hwnd;
}

void FillVariantCombo(HWND combo) {
    for (const auto& preset : kWpFestivalPresets) {
        const LRESULT index = SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(preset.displayName));
        if (index >= 0) {
            SendMessageW(combo, CB_SETITEMDATA, index, reinterpret_cast<LPARAM>(preset.token));
        }
    }
}

void SetVariantSelection(HWND hwnd, const std::wstring& variantToken) {
    const HWND combo = GetDlgItem(hwnd, IDC_VARIANT_COMBO);
    int selectedIndex = 0;
    for (int i = 0; i < static_cast<int>(kWpFestivalPresets.size()); ++i) {
        if (_wcsicmp(variantToken.c_str(), kWpFestivalPresets[static_cast<size_t>(i)].token) == 0) {
            selectedIndex = i;
            break;
        }
    }
    SendMessageW(combo, CB_SETCURSEL, selectedIndex, 0);
}

std::wstring GetSelectedVariantToken(HWND hwnd) {
    const HWND combo = GetDlgItem(hwnd, IDC_VARIANT_COMBO);
    const LRESULT index = SendMessageW(combo, CB_GETCURSEL, 0, 0);
    if (index == CB_ERR) {
        return kWpFestivalPresets.front().token;
    }
    const auto* token = reinterpret_cast<const wchar_t*>(SendMessageW(combo, CB_GETITEMDATA, index, 0));
    return token ? std::wstring(token) : std::wstring(kWpFestivalPresets.front().token);
}

void SetSliderValue(HWND hwnd, const SliderDef& slider, int value) {
    value = ClampInt(value, slider.minValue, slider.maxValue);
    SendDlgItemMessageW(hwnd, slider.trackId, TBM_SETPOS, TRUE, value);
    const std::wstring text = FormatSliderValue(slider, value);
    SetDlgItemTextW(hwnd, slider.valueId, text.c_str());
}

int GetSliderValue(HWND hwnd, const SliderDef& slider) {
    return static_cast<int>(SendDlgItemMessageW(hwnd, slider.trackId, TBM_GETPOS, 0, 0));
}

UiSettings ReadControls(HWND hwnd) {
    UiSettings settings;
    settings.variantToken = GetSelectedVariantToken(hwnd);
    settings.speedPercent = GetSliderValue(hwnd, kSliders[0]);
    settings.pitchPercent = GetSliderValue(hwnd, kSliders[1]);
    settings.volumePercent = GetSliderValue(hwnd, kSliders[2]);
    return settings;
}

void ApplySettingsToControls(HWND hwnd, const UiSettings& settings) {
    SetVariantSelection(hwnd, settings.variantToken);
    for (const auto& slider : kSliders) {
        int value = slider.defaultValue;
        if (slider.trackId == IDC_SPEED_TRACK) {
            value = settings.speedPercent;
        } else if (slider.trackId == IDC_PITCH_TRACK) {
            value = settings.pitchPercent;
        } else if (slider.trackId == IDC_VOLUME_TRACK) {
            value = settings.volumePercent;
        }
        SetSliderValue(hwnd, slider, value);
    }
}

const wchar_t* GetSamplePreviewText() {
#ifdef WP_FESTIVAL_PROFILE_FEMALE
    return L"To jest próbka żeńskiego głosu WP Festival. Ustaw wariant i odsłuchaj zmiany.";
#else
    return L"To jest próbka męskiego głosu WP Festival. Ustaw wariant i odsłuchaj zmiany.";
#endif
}

bool EnsurePreviewVoice(AppState* state) {
    if (!state) {
        return false;
    }
    if (state->previewVoice) {
        return true;
    }

    CLSID clsid{};
    HRESULT hr = CLSIDFromProgID(L"SAPI.SpVoice", &clsid);
    if (FAILED(hr)) {
        return false;
    }

    ISpVoice* voice = nullptr;
    hr = CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, IID_ISpVoice, reinterpret_cast<void**>(&voice));
    if (FAILED(hr) || !voice) {
        return false;
    }

    CLSID categoryClsid{};
    hr = CLSIDFromProgID(L"SAPI.SpObjectTokenCategory", &categoryClsid);
    if (FAILED(hr)) {
        voice->Release();
        return false;
    }

    ISpObjectTokenCategory* category = nullptr;
    hr = CoCreateInstance(
        categoryClsid,
        nullptr,
        CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
        IID_ISpObjectTokenCategory,
        reinterpret_cast<void**>(&category)
    );
    if (FAILED(hr) || !category) {
        voice->Release();
        return false;
    }

    hr = category->SetId(SPCAT_VOICES, FALSE);
    if (FAILED(hr)) {
        category->Release();
        voice->Release();
        return false;
    }

    IEnumSpObjectTokens* tokens = nullptr;
    hr = category->EnumTokens(nullptr, nullptr, &tokens);
    category->Release();
    if (FAILED(hr) || !tokens) {
        voice->Release();
        return false;
    }

    bool found = false;
    for (;;) {
        ISpObjectToken* token = nullptr;
        ULONG fetched = 0;
        hr = tokens->Next(1, &token, &fetched);
        if (hr != S_OK || !token) {
            break;
        }

        wchar_t* tokenId = nullptr;
        if (SUCCEEDED(token->GetId(&tokenId)) && tokenId) {
            const std::wstring leaf = ExtractTokenLeafName(tokenId);
            CoTaskMemFree(tokenId);
            if (_wcsicmp(leaf.c_str(), kWpFestivalPresets.front().token) == 0) {
                if (SUCCEEDED(voice->SetVoice(token))) {
                    found = true;
                }
                token->Release();
                break;
            }
        }
        token->Release();
    }

    tokens->Release();

    if (!found) {
        voice->Release();
        return false;
    }

    voice->SetRate(0);
    voice->SetVolume(100);
    state->previewVoice = voice;
    return true;
}

void StopPreview(AppState* state) {
    if (state && state->previewVoice) {
        state->previewVoice->Speak(L"", SPF_ASYNC | SPF_PURGEBEFORESPEAK, nullptr);
    }
}

bool ApplyPreviewSettings(AppState* state, const UiSettings& current) {
    if (!state) {
        return false;
    }
    if (current == state->originalSettings) {
        if (state->previewApplied) {
            if (!SaveSettings(state->originalSettings)) {
                return false;
            }
            state->previewApplied = false;
        }
        return true;
    }
    if (!SaveSettings(current)) {
        return false;
    }
    state->previewApplied = true;
    return true;
}

void RestoreOriginalSettings(AppState* state) {
    if (state && state->previewApplied) {
        (void)SaveSettings(state->originalSettings);
        state->previewApplied = false;
    }
}

void SaveAllSettings(HWND hwnd) {
    AppState* state = GetState(hwnd);
    const UiSettings current = ReadControls(hwnd);
    const bool ok = SaveSettings(current);
    if (ok && state) {
        state->originalSettings = current;
        state->previewApplied = false;
    }
    MessageBoxW(
        hwnd,
        ok ? L"Zapisano ustawienia. Głos użyje ich przy następnej wypowiedzi."
           : L"Nie udało się zapisać ustawień.",
        kWpFestivalProductCaption,
        ok ? MB_OK | MB_ICONINFORMATION : MB_OK | MB_ICONERROR
    );
}

void StartPreview(HWND hwnd) {
    AppState* state = GetState(hwnd);
    if (!state) {
        return;
    }

    const std::wstring text = TrimmedText(ReadWindowTextString(GetDlgItem(hwnd, IDC_PREVIEW_EDIT)));
    if (text.empty()) {
        MessageBoxW(hwnd, L"Wpisz tekst do odsłuchu.", kWpFestivalProductCaption, MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(hwnd, IDC_PREVIEW_EDIT));
        return;
    }

    const UiSettings current = ReadControls(hwnd);
    if (!ApplyPreviewSettings(state, current)) {
        MessageBoxW(hwnd, L"Nie udało się chwilowo zastosować bieżących ustawień.", kWpFestivalProductCaption, MB_OK | MB_ICONERROR);
        return;
    }

    if (!EnsurePreviewVoice(state)) {
        RestoreOriginalSettings(state);
        MessageBoxW(
            hwnd,
            L"Nie udało się uruchomić podglądu SAPI. Sprawdź, czy głos jest poprawnie zainstalowany.",
            kWpFestivalProductCaption,
            MB_OK | MB_ICONERROR
        );
        return;
    }

    StopPreview(state);
    const HRESULT hr = state->previewVoice->Speak(text.c_str(), SPF_ASYNC | SPF_IS_NOT_XML | SPF_PURGEBEFORESPEAK, nullptr);
    if (FAILED(hr)) {
        RestoreOriginalSettings(state);
        MessageBoxW(hwnd, L"Podgląd nie wystartował.", kWpFestivalProductCaption, MB_OK | MB_ICONERROR);
    }
}

void CreateInterface(HWND hwnd) {
    AppState* state = GetState(hwnd);
    if (!state) {
        return;
    }

    const HFONT baseFont = state->baseFont;
    const HFONT titleFont = state->titleFont;
    const HFONT sectionFont = state->sectionFont;

    CreateLabel(hwnd, IDC_TITLE_TEXT, kWpFestivalProductCaption, 20, 16, 580, 38, titleFont);

    std::wstring subtitle = std::wstring(L"Ustawienia dla głosu \"") + kWpFestivalStandardVoiceDisplayName +
        L"\". Układ jest wzorowany na ekranie BlackBox Android.";
    CreateLabel(hwnd, IDC_SUBTITLE_TEXT, subtitle.c_str(), 20, 58, 580, 42, baseFont);

    CreateLabel(hwnd, IDC_VARIANT_LABEL, L"Wariant głosu", 20, 112, 580, 24, sectionFont);
    const HWND combo = CreateVariantCombo(hwnd, 20, 142, 280, 280, baseFont);
    FillVariantCombo(combo);
    CreateLabel(
        hwnd,
        IDC_VARIANT_HELP_TEXT,
        L"Wariant dotyczy głosu standardowego. Nazwane presety nadal pozostają osobnymi pozycjami w hostach SAPI.",
        20,
        178,
        580,
        36,
        baseFont
    );

    int y = 230;
    for (const auto& slider : kSliders) {
        CreateLabel(hwnd, slider.labelId, slider.label, 20, y, 200, 22, sectionFont);
        CreateTrack(hwnd, slider, 20, y + 26, 430, 36, baseFont);
        CreateLabel(hwnd, slider.valueId, L"", 468, y + 26, 120, 22, baseFont);
        y += 88;
    }

    CreateLabel(
        hwnd,
        IDC_PARAMETERS_HELP_TEXT,
        L"Prędkość i wysokość są offsetami ponad wybrany wariant. Podgląd działa na bieżących wartościach bez zamykania okna.",
        20,
        y - 6,
        580,
        40,
        baseFont
    );

    CreateLabel(hwnd, IDC_PREVIEW_LABEL, L"Podgląd", 20, y + 48, 580, 24, sectionFont);
    CreateLabel(
        hwnd,
        IDC_PREVIEW_HELP_TEXT,
        L"Zmiany odsłuchasz przyciskiem Mów. Jeśli zamkniesz okno bez zapisu, podgląd cofnie niezapisane ustawienia.",
        20,
        y + 78,
        580,
        36,
        baseFont
    );

    const HWND previewEdit = CreatePreviewEdit(hwnd, 20, y + 120, 580, 124, baseFont);
    SetWindowTextW(previewEdit, GetSamplePreviewText());

    CreateButton(hwnd, IDC_PREVIEW_SPEAK_BUTTON, L"Mów", 20, y + 260, 120, 34, baseFont);
    CreateButton(hwnd, IDC_PREVIEW_STOP_BUTTON, L"Stop", 152, y + 260, 120, 34, baseFont);
    CreateButton(hwnd, IDC_SAVE_BUTTON, L"Zapisz", 284, y + 260, 100, 34, baseFont);
    CreateButton(hwnd, IDC_RESET_BUTTON, L"Domyślne", 396, y + 260, 100, 34, baseFont);
    CreateButton(hwnd, IDC_CLOSE_BUTTON, L"Zamknij", 500, y + 260, 100, 34, baseFont);

    ApplySettingsToControls(hwnd, state->originalSettings);
    PostMessageW(hwnd, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(combo), TRUE);
}

COLORREF GetStaticTextColor(int controlId) {
    switch (controlId) {
    case IDC_SUBTITLE_TEXT:
    case IDC_VARIANT_HELP_TEXT:
    case IDC_PARAMETERS_HELP_TEXT:
    case IDC_PREVIEW_HELP_TEXT:
    case IDC_SPEED_VALUE:
    case IDC_PITCH_VALUE:
    case IDC_VOLUME_VALUE:
        return kDimTextColor;
    default:
        return kTextColor;
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_NCCREATE: {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* state = reinterpret_cast<AppState*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        return TRUE;
    }
    case WM_CREATE: {
        AppState* state = GetState(hwnd);
        if (!state) {
            return -1;
        }
        state->backgroundBrush = CreateSolidBrush(kWindowColor);
        state->editBrush = CreateSolidBrush(kEditColor);
        state->baseFont = CreateUiFont(10, false);
        state->titleFont = CreateUiFont(22, true);
        state->sectionFont = CreateUiFont(12, true);
        state->originalSettings = LoadSettings();
        CreateInterface(hwnd);
        return 0;
    }
    case WM_HSCROLL: {
        const HWND source = reinterpret_cast<HWND>(lParam);
        for (const auto& slider : kSliders) {
            if (source == GetDlgItem(hwnd, slider.trackId)) {
                SetSliderValue(hwnd, slider, GetSliderValue(hwnd, slider));
                break;
            }
        }
        return 0;
    }
    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        HWND control = reinterpret_cast<HWND>(lParam);
        SetTextColor(dc, GetStaticTextColor(GetDlgCtrlID(control)));
        SetBkMode(dc, TRANSPARENT);
        AppState* state = GetState(hwnd);
        return reinterpret_cast<INT_PTR>(state ? state->backgroundBrush : GetStockObject(BLACK_BRUSH));
    }
    case WM_CTLCOLOREDIT: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, kTextColor);
        SetBkColor(dc, kEditColor);
        AppState* state = GetState(hwnd);
        return reinterpret_cast<INT_PTR>(state ? state->editBrush : GetStockObject(BLACK_BRUSH));
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_PREVIEW_SPEAK_BUTTON:
            StartPreview(hwnd);
            return 0;
        case IDC_PREVIEW_STOP_BUTTON:
            StopPreview(GetState(hwnd));
            return 0;
        case IDC_SAVE_BUTTON:
            SaveAllSettings(hwnd);
            return 0;
        case IDC_RESET_BUTTON:
            ApplySettingsToControls(hwnd, UiSettings{});
            return 0;
        case IDC_CLOSE_BUTTON:
            DestroyWindow(hwnd);
            return 0;
        default:
            break;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY: {
        AppState* state = GetState(hwnd);
        if (state) {
            StopPreview(state);
            RestoreOriginalSettings(state);
            if (state->previewVoice) {
                state->previewVoice->Release();
                state->previewVoice = nullptr;
            }
            if (state->backgroundBrush) {
                DeleteObject(state->backgroundBrush);
            }
            if (state->editBrush) {
                DeleteObject(state->editBrush);
            }
            if (state->baseFont) {
                DeleteObject(state->baseFont);
            }
            if (state->titleFont) {
                DeleteObject(state->titleFont);
            }
            if (state->sectionFont) {
                DeleteObject(state->sectionFont);
            }
            delete state;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        }
        PostQuitMessage(0);
        return 0;
    }
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool uninitializeCom = SUCCEEDED(hr);

    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kWpFestivalWindowClass;
    RegisterClassW(&wc);

    auto* state = new AppState();
    HWND hwnd = CreateWindowExW(
        WS_EX_CONTROLPARENT,
        kWpFestivalWindowClass,
        kWpFestivalWindowTitle,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        kWindowWidth,
        kWindowHeight,
        nullptr,
        nullptr,
        instance,
        state
    );

    if (!hwnd) {
        delete state;
        if (uninitializeCom) {
            CoUninitialize();
        }
        return 1;
    }

    ShowWindow(hwnd, showCommand == 0 ? SW_SHOWDEFAULT : showCommand);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (IsDialogMessageW(hwnd, &msg)) {
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (uninitializeCom) {
        CoUninitialize();
    }
    return static_cast<int>(msg.wParam);
}
