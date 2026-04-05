#pragma once

#include <drogon/drogon.h>

#include <functional>
#include <string>

namespace gm {

struct AiInsightAgentResult {
    bool ok{false};
    drogon::HttpStatusCode status{drogon::k200OK};
    std::string error;

    std::string profileId;
    std::string provider;
    std::string model;

    int latencyMs{0};
    std::string promptText;
    std::string rawResponseText;
    Json::Value retrievedContext{Json::arrayValue};
    Json::Value insight{Json::objectValue};
};

using AiInsightAgentCallback = std::function<void(const AiInsightAgentResult &)>;

void runAiInsightAgentAsync(const Json::Value &requestBody,
                            const Json::Value &snapshot,
                            AiInsightAgentCallback callback);

bool listAiInsightProfiles(Json::Value &profilesOut,
                           std::string &defaultProfile,
                           std::string &error);

}  // namespace gm
