#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <functional>

namespace nxui {

class I18n {
public:
    using LanguageChangedCb = std::function<void()>;

    static I18n& instance();

    bool initialize(const std::string& basePath = "romfs:/i18n",
                    const std::string& fallbackTag = "en-US");

    bool setLanguageAuto();
    bool setLanguage(const std::string& tag);

    std::string tr(std::string_view key, std::string_view fallback = "") const;

    int  addLanguageChangedListener(LanguageChangedCb cb);
    void removeLanguageChangedListener(int id);

    const std::string& activeLanguageTag() const { return m_activeTag; }
    const std::string& fallbackLanguageTag() const { return m_fallbackTag; }

    static std::string detectSystemLanguageTag();
    static std::vector<std::string> supportedLanguageTags();

private:
    I18n() = default;

    static std::string normalizeTag(const std::string& tag);
    std::string buildLanguagePath(const std::string& normalizedTag) const;

    bool loadJsonMapFromFile(const std::string& path,
                             std::unordered_map<std::string, std::string>& out) const;
    void notifyLanguageChanged();

    std::string m_basePath = "romfs:/i18n";
    std::string m_fallbackTag = "en-US";
    std::string m_activeTag = "en-US";

    std::unordered_map<std::string, std::string> m_fallbackMap;
    std::unordered_map<std::string, std::string> m_activeMap;
    std::unordered_map<int, LanguageChangedCb> m_languageListeners;
    int m_nextListenerId = 1;
};

} // namespace nxui
