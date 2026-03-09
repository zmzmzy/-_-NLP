#include "app_bootstrap.h"

#include <drogon/drogon.h>

#include <filesystem>
#include <string>

#include "app_support.h"

namespace gm {

void configureApplication() {
    const auto configPath = resolveBackendConfigPath();
    Json::Value configJson;
    std::string configError;
    if (readJsonFromFile(configPath, configJson, configError) && configJson.isObject()) {
        applyDbConfigOverrides(configJson);
        const auto runtimeConfigPath =
            std::filesystem::temp_directory_path() / "graduate_match_backend.runtime.json";
        if (writeJsonToFile(runtimeConfigPath, configJson, configError)) {
            drogon::app().loadConfigFile(runtimeConfigPath.string());
        } else {
            LOG_WARN << "failed to write runtime config, fallback to original config: " << configError;
            drogon::app().loadConfigFile(configPath.string());
        }
    } else {
        LOG_WARN << "failed to load config json, fallback to default path: " << configPath.string();
        drogon::app().loadConfigFile(configPath.string());
    }
}

}  // namespace gm
