#pragma once

#include <array>

struct WpFestivalPresetDef {
    const wchar_t* token;
    const wchar_t* displayName;
    int baseRateAdjust;
    int baseVolumePercent;
    int pitchMean;
    int pitchStd;
};

#ifdef WP_FESTIVAL_PROFILE_FEMALE

constexpr wchar_t kWpFestivalSettingsSubKey[] = L"Software\\WPFestivalFemale\\SAPI5\\Settings";
constexpr wchar_t kWpFestivalVariantTokenValueName[] = L"VariantToken";
constexpr wchar_t kWpFestivalSpeedPercentValueName[] = L"SpeedPercent";
constexpr wchar_t kWpFestivalPitchPercentValueName[] = L"PitchPercent";
constexpr wchar_t kWpFestivalVolumePercentValueName[] = L"VolumePercent";

constexpr wchar_t kWpFestivalProductCaption[] = L"WP Festival SAPI5 Female";
constexpr wchar_t kWpFestivalWindowClass[] = L"WPFestivalSapi5FemaleConfigWindow";
constexpr wchar_t kWpFestivalWindowTitle[] = L"WP Festival SAPI5 Female - Ustawienia";
constexpr wchar_t kWpFestivalConfigExeName[] = L"WPFestivalSapi5FemaleConfig.exe";
constexpr wchar_t kWpFestivalEngineDisplayName[] = L"WP Festival SAPI5 Female Engine";
constexpr wchar_t kWpFestivalUiDisplayName[] = L"WP Festival SAPI5 Female UI";
constexpr wchar_t kWpFestivalAttributeName[] = L"WP Festival Female";
constexpr wchar_t kWpFestivalAttributeVendor[] = L"WP reverse-engineered runtime";
constexpr wchar_t kWpFestivalAttributeGender[] = L"Female";
constexpr wchar_t kWpFestivalStandardVoiceDisplayName[] = L"WP Festival Zenski (SAPI5)";

constexpr int kWpFestivalDefaultSpeedPercent = 50;
constexpr int kWpFestivalDefaultPitchPercent = 50;
constexpr int kWpFestivalDefaultVolumePercent = 100;
constexpr int kWpFestivalMinSpeedPercent = 0;
constexpr int kWpFestivalMaxSpeedPercent = 100;
constexpr int kWpFestivalMinPitchPercent = 0;
constexpr int kWpFestivalMaxPitchPercent = 100;
constexpr int kWpFestivalMinVolumePercent = 0;
constexpr int kWpFestivalMaxVolumePercent = 150;

constexpr std::array<WpFestivalPresetDef, 7> kWpFestivalPresets = {{
    {L"WPFestivalFemale.Standard", L"Standard", 0, 100, 145, 16},
    {L"WPFestivalFemale.Gleboki", L"Gleboki", -1, 106, 136, 14},
    {L"WPFestivalFemale.Jasny", L"Jasny", 0, 100, 156, 18},
    {L"WPFestivalFemale.Wolny", L"Wolny", -4, 100, 145, 16},
    {L"WPFestivalFemale.Szybki", L"Szybki", 4, 100, 145, 16},
    {L"WPFestivalFemale.Miekki", L"Miekki", -1, 94, 141, 14},
    {L"WPFestivalFemale.Mocny", L"Mocny", 1, 116, 148, 19},
}};

#else

constexpr wchar_t kWpFestivalSettingsSubKey[] = L"Software\\WPFestival\\SAPI5\\Settings";
constexpr wchar_t kWpFestivalVariantTokenValueName[] = L"VariantToken";
constexpr wchar_t kWpFestivalSpeedPercentValueName[] = L"SpeedPercent";
constexpr wchar_t kWpFestivalPitchPercentValueName[] = L"PitchPercent";
constexpr wchar_t kWpFestivalVolumePercentValueName[] = L"VolumePercent";

constexpr wchar_t kWpFestivalProductCaption[] = L"WP Festival SAPI5";
constexpr wchar_t kWpFestivalWindowClass[] = L"WPFestivalSapi5ConfigWindow";
constexpr wchar_t kWpFestivalWindowTitle[] = L"WP Festival SAPI5 - Ustawienia";
constexpr wchar_t kWpFestivalConfigExeName[] = L"WPFestivalSapi5Config.exe";
constexpr wchar_t kWpFestivalEngineDisplayName[] = L"WP Festival SAPI5 Engine";
constexpr wchar_t kWpFestivalUiDisplayName[] = L"WP Festival SAPI5 UI";
constexpr wchar_t kWpFestivalAttributeName[] = L"WP Festival";
constexpr wchar_t kWpFestivalAttributeVendor[] = L"WP reverse-engineered runtime";
constexpr wchar_t kWpFestivalAttributeGender[] = L"Male";
constexpr wchar_t kWpFestivalStandardVoiceDisplayName[] = L"WP Festival (SAPI5)";

constexpr int kWpFestivalDefaultSpeedPercent = 50;
constexpr int kWpFestivalDefaultPitchPercent = 50;
constexpr int kWpFestivalDefaultVolumePercent = 100;
constexpr int kWpFestivalMinSpeedPercent = 0;
constexpr int kWpFestivalMaxSpeedPercent = 100;
constexpr int kWpFestivalMinPitchPercent = 0;
constexpr int kWpFestivalMaxPitchPercent = 100;
constexpr int kWpFestivalMinVolumePercent = 0;
constexpr int kWpFestivalMaxVolumePercent = 150;

constexpr std::array<WpFestivalPresetDef, 7> kWpFestivalPresets = {{
    {L"WPFestival.Standard", L"Standard", 0, 100, 105, 14},
    {L"WPFestival.Gleboki", L"Gleboki", -1, 108, 92, 12},
    {L"WPFestival.Jasny", L"Jasny", 0, 100, 122, 16},
    {L"WPFestival.Wolny", L"Wolny", -4, 100, 105, 14},
    {L"WPFestival.Szybki", L"Szybki", 4, 100, 105, 14},
    {L"WPFestival.Miekki", L"Miekki", -1, 92, 98, 10},
    {L"WPFestival.Mocny", L"Mocny", 1, 118, 110, 18},
}};

#endif
