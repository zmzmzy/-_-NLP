#include "ai_insight_agent.h"

#include "app_support.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace gm {

namespace {

struct AgentProfile {
    std::string id;
    std::string displayName;
    std::string description;

    std::string provider;
    std::string model;
    std::string endpoint;

    std::string apiKeyEnv;
    std::string authHeader;
    std::string apiKeyPrefix;
    bool apiKeyInQuery{false};
    std::string apiKeyQueryName{"key"};
    bool apiKeyRequired{true};

    double temperature{0.2};
    int maxOutputTokens{1200};
    int timeoutMs{30000};

    std::string difyUser{"gm-dashboard"};
    std::string difyResponseMode{"blocking"};
    std::string difyOutputKey{"insight_json"};

    std::string systemPreamble;
    std::vector<std::string> worldBook;
    std::vector<std::string> presetRules;
    Json::Value roleCard{Json::objectValue};

    bool ragEnabled{false};
    std::string ragKnowledgeFile;
    int ragTopK{4};
    int ragMaxSnippets{4};
    int ragSnippetMaxChars{280};
};

struct AgentConfig {
    std::string defaultProfile;
    std::unordered_map<std::string, AgentProfile> profileMap;
    std::vector<std::string> profileOrder;
    std::filesystem::path configPath;
};

bool isGeminiProvider(const std::string &providerRaw) {
    return toLowerAscii(trimCopy(providerRaw)) == "gemini";
}

bool isLiteLlmProvider(const std::string &providerRaw) {
    const auto provider = toLowerAscii(trimCopy(providerRaw));
    return provider == "litellm" || provider == "litellm_gateway";
}

bool isDifyWorkflowProvider(const std::string &providerRaw) {
    return toLowerAscii(trimCopy(providerRaw)) == "dify_workflow";
}

bool isDifyChatProvider(const std::string &providerRaw) {
    return toLowerAscii(trimCopy(providerRaw)) == "dify_chat";
}

bool isDifyProvider(const std::string &providerRaw) {
    return isDifyWorkflowProvider(providerRaw) || isDifyChatProvider(providerRaw);
}

bool parseBoolJson(const Json::Value &node, bool defaultValue) {
    if (node.isBool()) {
        return node.asBool();
    }
    if (node.isString()) {
        bool out = defaultValue;
        if (parseBoolLike(node.asString(), out)) {
            return out;
        }
    }
    if (node.isInt()) {
        return node.asInt() != 0;
    }
    return defaultValue;
}

int parseIntJson(const Json::Value &node, int defaultValue, int minValue, int maxValue) {
    int out = defaultValue;
    if (node.isInt()) {
        out = node.asInt();
    } else if (node.isString()) {
        int parsed = defaultValue;
        if (parseStrictInt(node.asString(), parsed)) {
            out = parsed;
        }
    }
    out = std::max(minValue, std::min(maxValue, out));
    return out;
}

double parseDoubleJson(const Json::Value &node, double defaultValue, double minValue, double maxValue) {
    double out = defaultValue;
    if (node.isDouble() || node.isInt()) {
        out = node.asDouble();
    } else if (node.isString()) {
        double parsed = defaultValue;
        if (parseStrictDouble(node.asString(), parsed)) {
            out = parsed;
        }
    }
    out = std::max(minValue, std::min(maxValue, out));
    return out;
}

std::vector<std::string> readStringArray(const Json::Value &node) {
    std::vector<std::string> out;
    if (!node.isArray()) {
        return out;
    }
    for (const auto &item : node) {
        if (!item.isString()) {
            continue;
        }
        const auto text = trimCopy(item.asString());
        if (!text.empty()) {
            out.push_back(text);
        }
    }
    return out;
}

std::filesystem::path resolveAiAgentConfigPath() {
    const auto envPath = getEnvTrimmed("GM_AI_AGENT_CONFIG_PATH");
    if (!envPath.empty()) {
        return std::filesystem::path(envPath);
    }
    return std::filesystem::path("../config/ai_agent_profiles.json");
}

std::string replaceAll(std::string text, const std::string &from, const std::string &to) {
    if (from.empty()) {
        return text;
    }
    size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::string::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
    return text;
}

std::string trimTrailingSlash(const std::string &raw) {
    auto out = trimCopy(raw);
    while (out.size() > 1 && out.back() == '/') {
        out.pop_back();
    }
    return out;
}

std::string replaceEnvTokens(std::string text) {
    const std::string tokenPrefix = "{env:";
    size_t cursor = 0;
    while (true) {
        const auto start = text.find(tokenPrefix, cursor);
        if (start == std::string::npos) {
            break;
        }
        const auto end = text.find('}', start + tokenPrefix.size());
        if (end == std::string::npos) {
            break;
        }
        const auto envKey = trimCopy(text.substr(start + tokenPrefix.size(), end - (start + tokenPrefix.size())));
        const auto envValue = envKey.empty() ? std::string() : getEnvTrimmed(envKey.c_str());
        text.replace(start, end - start + 1, envValue);
        cursor = start + envValue.size();
    }
    return text;
}

std::string sanitizeEndpointForLog(const std::string &endpoint) {
    const auto qPos = endpoint.find('?');
    if (qPos == std::string::npos) {
        return endpoint;
    }
    return endpoint.substr(0, qPos) + "?<redacted>";
}

std::string urlEncode(const std::string &text) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase;
    for (unsigned char c : text) {
        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~') {
            oss << static_cast<char>(c);
        } else {
            oss << '%' << static_cast<int>(c >> 4) << static_cast<int>(c & 0x0F);
        }
    }
    return oss.str();
}

std::string appendQuery(const std::string &url,
                        const std::string &key,
                        const std::string &value) {
    if (key.empty()) {
        return url;
    }
    const auto sep = (url.find('?') == std::string::npos) ? '?' : '&';
    return url + sep + urlEncode(key) + "=" + urlEncode(value);
}

bool splitUrlHostPath(const std::string &url,
                      std::string &host,
                      std::string &path,
                      std::string &error) {
    host.clear();
    path.clear();
    error.clear();
    const auto schemePos = url.find("://");
    if (schemePos == std::string::npos) {
        error = "invalid endpoint url (missing scheme)";
        return false;
    }
    const auto pathPos = url.find('/', schemePos + 3);
    if (pathPos == std::string::npos) {
        host = url;
        path = "/";
    } else {
        host = url.substr(0, pathPos);
        path = url.substr(pathPos);
    }
    return true;
}

std::string reqResultToText(drogon::ReqResult result) {
    switch (result) {
        case drogon::ReqResult::Ok:
            return "ok";
        case drogon::ReqResult::BadResponse:
            return "bad_response";
        case drogon::ReqResult::NetworkFailure:
            return "network_failure";
        case drogon::ReqResult::Timeout:
            return "timeout";
        case drogon::ReqResult::BadServerAddress:
            return "bad_server_address";
        case drogon::ReqResult::HandshakeError:
            return "tls_handshake_error";
        case drogon::ReqResult::InvalidCertificate:
            return "invalid_certificate";
        case drogon::ReqResult::EncryptionFailure:
            return "encryption_failure";
        default:
            return "unknown";
    }
}

std::string safeText(const Json::Value &node) {
    if (node.isString()) {
        return node.asString();
    }
    if (node.isBool()) {
        return node.asBool() ? "true" : "false";
    }
    if (node.isInt64()) {
        return std::to_string(node.asInt64());
    }
    if (node.isUInt64()) {
        return std::to_string(node.asUInt64());
    }
    if (node.isDouble()) {
        std::ostringstream oss;
        oss << node.asDouble();
        return oss.str();
    }
    return "";
}

std::string safeTextOrJson(const Json::Value &node) {
    const auto plain = trimCopy(safeText(node));
    if (!plain.empty()) {
        return plain;
    }
    if (node.isObject() || node.isArray()) {
        return jsonToCompactString(node);
    }
    return "";
}

std::string extractOpenAiLikeText(const Json::Value &response) {
    if (!response.isObject() || !response.isMember("choices") || !response["choices"].isArray() ||
        response["choices"].empty()) {
        return "";
    }
    const auto &firstChoice = response["choices"][0];
    if (!firstChoice.isObject() || !firstChoice.isMember("message")) {
        return "";
    }
    const auto &message = firstChoice["message"];
    if (!message.isObject() || !message.isMember("content")) {
        return "";
    }

    const auto &content = message["content"];
    if (content.isString()) {
        return content.asString();
    }
    if (content.isArray()) {
        std::ostringstream oss;
        for (const auto &part : content) {
            if (!part.isObject()) {
                continue;
            }
            if (part.isMember("text") && part["text"].isString()) {
                if (oss.tellp() > 0) {
                    oss << "\n";
                }
                oss << part["text"].asString();
            } else if (part.isMember("type") && part["type"].isString() &&
                       part["type"].asString() == "output_text" && part.isMember("value") && part["value"].isString()) {
                if (oss.tellp() > 0) {
                    oss << "\n";
                }
                oss << part["value"].asString();
            }
        }
        return oss.str();
    }
    return "";
}

std::string extractDifyOutputsText(const Json::Value &outputs,
                                   const AgentProfile &profile) {
    if (!outputs.isObject()) {
        return "";
    }

    if (!profile.difyOutputKey.empty() && outputs.isMember(profile.difyOutputKey)) {
        const auto text = trimCopy(safeTextOrJson(outputs[profile.difyOutputKey]));
        if (!text.empty()) {
            return text;
        }
    }

    static const std::vector<std::string> kFallbackKeys = {
        "insight_json",
        "answer",
        "text",
        "result",
        "output",
        "json"
    };
    for (const auto &key : kFallbackKeys) {
        if (!outputs.isMember(key)) {
            continue;
        }
        const auto text = trimCopy(safeTextOrJson(outputs[key]));
        if (!text.empty()) {
            return text;
        }
    }

    const auto members = outputs.getMemberNames();
    for (const auto &name : members) {
        const auto text = trimCopy(safeTextOrJson(outputs[name]));
        if (!text.empty()) {
            return text;
        }
    }
    return "";
}

std::string extractProviderBodyText(const Json::Value &response,
                                    const AgentProfile &profile) {
    const auto mode = toLowerAscii(trimCopy(profile.provider));
    if (mode == "gemini") {
        if (!response.isObject() || !response.isMember("candidates") || !response["candidates"].isArray() ||
            response["candidates"].empty()) {
            return "";
        }
        const auto &candidate = response["candidates"][0];
        if (!candidate.isObject() || !candidate.isMember("content")) {
            return "";
        }
        const auto &content = candidate["content"];
        if (!content.isObject() || !content.isMember("parts") || !content["parts"].isArray()) {
            return "";
        }
        std::ostringstream oss;
        for (const auto &part : content["parts"]) {
            if (part.isObject() && part.isMember("text") && part["text"].isString()) {
                if (oss.tellp() > 0) {
                    oss << "\n";
                }
                oss << part["text"].asString();
            }
        }
        return oss.str();
    }

    if (isDifyProvider(mode)) {
        if (response.isObject()) {
            if (response.isMember("answer")) {
                const auto text = trimCopy(safeTextOrJson(response["answer"]));
                if (!text.empty()) {
                    return text;
                }
            }
            if (response.isMember("data") && response["data"].isObject()) {
                const auto &data = response["data"];
                if (data.isMember("outputs")) {
                    const auto outputText = extractDifyOutputsText(data["outputs"], profile);
                    if (!outputText.empty()) {
                        return outputText;
                    }
                }
                if (data.isMember("output")) {
                    const auto text = trimCopy(safeTextOrJson(data["output"]));
                    if (!text.empty()) {
                        return text;
                    }
                }
            }
            if (response.isMember("outputs")) {
                const auto outputText = extractDifyOutputsText(response["outputs"], profile);
                if (!outputText.empty()) {
                    return outputText;
                }
            }
        }
        return "";
    }

    return extractOpenAiLikeText(response);
}

std::string extractProviderErrorMessage(const Json::Value &response) {
    if (!response.isObject()) {
        return "";
    }
    if (response.isMember("error")) {
        const auto &error = response["error"];
        if (error.isObject()) {
            if (error.isMember("message")) {
                const auto message = trimCopy(safeTextOrJson(error["message"]));
                if (!message.empty()) {
                    return message;
                }
            }
            const auto fallback = trimCopy(safeTextOrJson(error));
            if (!fallback.empty() && fallback != "{}") {
                return fallback;
            }
        } else {
            const auto message = trimCopy(safeTextOrJson(error));
            if (!message.empty()) {
                return message;
            }
        }
    }

    if (response.isMember("message")) {
        const auto message = trimCopy(safeTextOrJson(response["message"]));
        if (!message.empty()) {
            return message;
        }
    }

    if (response.isMember("detail")) {
        const auto message = trimCopy(safeTextOrJson(response["detail"]));
        if (!message.empty()) {
            return message;
        }
    }

    return "";
}

std::string stripCodeFence(const std::string &raw) {
    const auto trimmed = trimCopy(raw);
    if (trimmed.rfind("```", 0) != 0) {
        return trimmed;
    }
    const auto firstNewLine = trimmed.find('\n');
    if (firstNewLine == std::string::npos) {
        return trimmed;
    }
    const auto lastFence = trimmed.rfind("```");
    if (lastFence == std::string::npos || lastFence <= firstNewLine) {
        return trimmed;
    }
    return trimCopy(trimmed.substr(firstNewLine + 1, lastFence - firstNewLine - 1));
}

Json::Value readStringListNormalized(const Json::Value &node) {
    Json::Value out(Json::arrayValue);
    if (node.isArray()) {
        for (const auto &item : node) {
            const auto text = trimCopy(safeText(item));
            if (!text.empty()) {
                out.append(text);
            }
        }
        return out;
    }
    const auto text = trimCopy(safeText(node));
    if (!text.empty()) {
        out.append(text);
    }
    return out;
}

Json::Value normalizeInsightJson(const Json::Value &parsed,
                                 const Json::Value &snapshot,
                                 const std::string &fallbackSummary) {
    Json::Value out(Json::objectValue);
    const auto metrics = snapshot.isObject() && snapshot.isMember("metrics") && snapshot["metrics"].isObject()
                             ? snapshot["metrics"]
                             : Json::Value(Json::objectValue);

    std::string summary;
    if (parsed.isObject() && parsed.isMember("summary")) {
        summary = trimCopy(safeText(parsed["summary"]));
    }
    if (summary.empty()) {
        summary = trimCopy(fallbackSummary);
    }
    if (summary.empty()) {
        summary = "模型未返回结构化摘要，建议检查配置后重试。";
    }
    out["summary"] = summary;

    Json::Value highlights = parsed.isObject() ? readStringListNormalized(parsed["highlights"])
                                                : Json::Value(Json::arrayValue);
    Json::Value risks = parsed.isObject() ? readStringListNormalized(parsed["risks"])
                                          : Json::Value(Json::arrayValue);
    Json::Value actions = parsed.isObject() ? readStringListNormalized(parsed["actions"])
                                            : Json::Value(Json::arrayValue);

    if (highlights.empty()) {
        std::ostringstream oss;
        oss << "覆盖专业 " << safeText(metrics["major_count"]) << " 个，样本学生 "
            << safeText(metrics["total_students"]) << " 人。";
        highlights.append(oss.str());
    }
    if (risks.empty()) {
        std::ostringstream oss;
        oss << "高风险缺口技能：" << safeText(metrics["top_gap_skill"]) << "。";
        risks.append(oss.str());
    }
    if (actions.empty()) {
        actions.append("建议先处理高频必备缺口技能，并持续校准课程与岗位能力映射。");
    }

    out["highlights"] = highlights;
    out["risks"] = risks;
    out["actions"] = actions;

    std::string confidence;
    if (parsed.isObject() && parsed.isMember("confidence")) {
        confidence = toLowerAscii(trimCopy(safeText(parsed["confidence"])));
    }
    if (confidence != "high" && confidence != "medium" && confidence != "low") {
        confidence = "medium";
    }
    out["confidence"] = confidence;

    if (parsed.isObject() && parsed.isMember("evidence") && parsed["evidence"].isObject()) {
        out["evidence"] = parsed["evidence"];
    } else {
        Json::Value evidence(Json::objectValue);
        evidence["major_count"] = metrics["major_count"];
        evidence["total_students"] = metrics["total_students"];
        evidence["employment_rate"] = metrics["employment_rate"];
        evidence["alignment_rate"] = metrics["alignment_rate"];
        evidence["avg_excellent_score"] = metrics["avg_excellent_score"];
        evidence["top_gap_skill"] = metrics["top_gap_skill"];
        out["evidence"] = evidence;
    }

    return out;
}

std::string buildRoleCardText(const Json::Value &roleCard) {
    if (!roleCard.isObject()) {
        return "";
    }
    std::ostringstream oss;
    const auto name = trimCopy(safeText(roleCard["name"]));
    const auto style = trimCopy(safeText(roleCard["style"]));
    const auto tone = trimCopy(safeText(roleCard["tone"]));
    if (!name.empty()) {
        oss << "角色名: " << name << "\n";
    }
    if (!style.empty()) {
        oss << "角色风格: " << style << "\n";
    }
    if (!tone.empty()) {
        oss << "角色语气: " << tone << "\n";
    }

    if (roleCard.isMember("objectives")) {
        const auto objectives = readStringListNormalized(roleCard["objectives"]);
        if (!objectives.empty()) {
            oss << "角色目标:\n";
            for (const auto &item : objectives) {
                oss << "- " << item.asString() << "\n";
            }
        }
    }
    if (roleCard.isMember("constraints")) {
        const auto constraints = readStringListNormalized(roleCard["constraints"]);
        if (!constraints.empty()) {
            oss << "角色约束:\n";
            for (const auto &item : constraints) {
                oss << "- " << item.asString() << "\n";
            }
        }
    }
    return trimCopy(oss.str());
}

std::string buildRagQueryText(const Json::Value &requestBody,
                              const Json::Value &snapshot) {
    std::ostringstream oss;
    if (requestBody.isObject()) {
        oss << safeText(requestBody["scope"]) << " ";
        oss << safeText(requestBody["focus"]) << " ";
        oss << safeText(requestBody["tone"]) << " ";
        oss << safeText(requestBody["custom_prompt"]) << " ";
    }
    if (snapshot.isObject() && snapshot.isMember("scope") && snapshot["scope"].isObject()) {
        oss << safeText(snapshot["scope"]["label"]) << " ";
    }
    if (snapshot.isObject() && snapshot.isMember("top_gap_skills") && snapshot["top_gap_skills"].isArray()) {
        int count = 0;
        for (const auto &item : snapshot["top_gap_skills"]) {
            if (!item.isObject()) {
                continue;
            }
            oss << safeText(item["skill_name"]) << " ";
            if (++count >= 8) {
                break;
            }
        }
    }
    return trimCopy(oss.str());
}

Json::Value retrieveRagContext(const AgentProfile &profile,
                               const std::filesystem::path &configDir,
                               const std::string &queryText) {
    Json::Value out(Json::arrayValue);
    if (!profile.ragEnabled || profile.ragKnowledgeFile.empty()) {
        return out;
    }

    auto knowledgePath = std::filesystem::path(profile.ragKnowledgeFile);
    if (knowledgePath.is_relative()) {
        knowledgePath = configDir / knowledgePath;
    }

    Json::Value raw;
    std::string error;
    if (!readJsonFromFile(knowledgePath, raw, error) || !raw.isObject() || !raw.isMember("items") || !raw["items"].isArray()) {
        return out;
    }

    const auto queryTokens = buildTokenSet(queryText);

    struct ScoredItem {
        double score{0.0};
        Json::Value item{Json::objectValue};
    };
    std::vector<ScoredItem> scored;

    for (const auto &item : raw["items"]) {
        if (!item.isObject()) {
            continue;
        }
        const auto title = trimCopy(safeText(item["title"]));
        auto content = trimCopy(safeText(item["content"]));
        if (content.empty()) {
            continue;
        }
        if (static_cast<int>(content.size()) > profile.ragSnippetMaxChars) {
            content = content.substr(0, static_cast<size_t>(profile.ragSnippetMaxChars));
        }
        std::string tagsText;
        if (item.isMember("tags") && item["tags"].isArray()) {
            for (const auto &tag : item["tags"]) {
                const auto t = trimCopy(safeText(tag));
                if (!t.empty()) {
                    tagsText += t + " ";
                }
            }
        }

        const std::string corpus = title + " " + content + " " + tagsText;
        const auto itemTokens = buildTokenSet(corpus);

        double score = 0.0;
        if (!queryTokens.empty() && !itemTokens.empty()) {
            size_t overlap = 0;
            for (const auto &token : queryTokens) {
                if (itemTokens.find(token) != itemTokens.end()) {
                    ++overlap;
                }
            }
            score = static_cast<double>(overlap);
        }

        if (score <= 0.0 && queryText.empty()) {
            score = 0.1;
        }
        if (score <= 0.0) {
            continue;
        }

        Json::Value snippet(Json::objectValue);
        snippet["title"] = title;
        snippet["content"] = content;
        snippet["tags"] = item.isMember("tags") ? item["tags"] : Json::Value(Json::arrayValue);
        snippet["score"] = score;

        scored.push_back({score, snippet});
    }

    std::sort(scored.begin(), scored.end(), [](const ScoredItem &a, const ScoredItem &b) {
        if (std::abs(a.score - b.score) > 1e-9) {
            return a.score > b.score;
        }
        return safeText(a.item["title"]) < safeText(b.item["title"]);
    });

    const int topK = std::max(1, std::min(profile.ragTopK, profile.ragMaxSnippets));
    for (int i = 0; i < static_cast<int>(scored.size()) && i < topK; ++i) {
        out.append(scored[static_cast<size_t>(i)].item);
    }
    return out;
}

std::string buildSystemPrompt(const AgentProfile &profile,
                              const Json::Value &ragContext) {
    std::ostringstream oss;
    oss << "你是高校专业建设与就业质量分析助手。\n";
    oss << "你的目标是输出可执行、可追溯、避免空话的分析结论。\n";
    if (!profile.systemPreamble.empty()) {
        oss << "\n[系统预设]\n" << profile.systemPreamble << "\n";
    }
    if (!profile.worldBook.empty()) {
        oss << "\n[世界书]\n";
        for (const auto &line : profile.worldBook) {
            oss << "- " << line << "\n";
        }
    }
    if (!profile.presetRules.empty()) {
        oss << "\n[规则约束]\n";
        for (const auto &line : profile.presetRules) {
            oss << "- " << line << "\n";
        }
    }

    const auto roleCardText = buildRoleCardText(profile.roleCard);
    if (!roleCardText.empty()) {
        oss << "\n[角色卡]\n" << roleCardText << "\n";
    }

    if (ragContext.isArray() && !ragContext.empty()) {
        oss << "\n[RAG 检索上下文]\n";
        for (Json::ArrayIndex i = 0; i < ragContext.size(); ++i) {
            const auto &item = ragContext[i];
            oss << i + 1 << ". " << safeText(item["title"]) << "\n";
            oss << safeText(item["content"]) << "\n";
        }
    }

    oss << "\n[输出要求]\n";
    oss << "仅输出 JSON 对象，不要输出 markdown，不要输出额外说明。\n";
    oss << "字段必须包含: summary(string), highlights(string[]), risks(string[]), actions(string[]), confidence(high|medium|low), evidence(object)。\n";
    oss << "actions 至少 3 条，每条建议必须可执行。\n";
    return trimCopy(oss.str());
}

std::string buildUserPrompt(const Json::Value &requestBody,
                            const Json::Value &snapshot) {
    std::ostringstream oss;
    oss << "请基于以下决策看板数据生成结构化解读。\n";
    oss << "解读重点: " << safeText(requestBody["focus"]) << "\n";
    oss << "建议语气: " << safeText(requestBody["tone"]) << "\n";
    if (requestBody.isObject() && requestBody.isMember("custom_prompt") && requestBody["custom_prompt"].isString()) {
        const auto custom = trimCopy(requestBody["custom_prompt"].asString());
        if (!custom.empty()) {
            oss << "额外要求: " << custom << "\n";
        }
    }
    oss << "\n[看板快照JSON]\n";
    oss << jsonToCompactString(snapshot) << "\n";
    return trimCopy(oss.str());
}

bool parseProfile(const Json::Value &node, AgentProfile &profile) {
    if (!node.isObject()) {
        return false;
    }

    profile = AgentProfile();
    profile.id = trimCopy(safeText(node["id"]));
    if (profile.id.empty()) {
        return false;
    }

    profile.displayName = trimCopy(safeText(node["display_name"]));
    if (profile.displayName.empty()) {
        profile.displayName = profile.id;
    }
    profile.description = trimCopy(safeText(node["description"]));

    profile.provider = toLowerAscii(trimCopy(safeText(node["provider"])));
    if (profile.provider.empty()) {
        profile.provider = "openai_compatible";
    }

    profile.model = trimCopy(safeText(node["model"]));
    if (profile.model.empty()) {
        if (isLiteLlmProvider(profile.provider)) {
            profile.model = "dashboard-default";
        } else if (isDifyProvider(profile.provider)) {
            profile.model = "dify-app";
        } else {
            profile.model = "gpt-4o-mini";
        }
    }

    profile.endpoint = trimCopy(safeText(node["endpoint"]));
    profile.apiKeyEnv = trimCopy(safeText(node["api_key_env"]));
    if (profile.apiKeyEnv.empty()) {
        if (isGeminiProvider(profile.provider)) {
            profile.apiKeyEnv = "GEMINI_API_KEY";
        } else if (isDifyProvider(profile.provider)) {
            profile.apiKeyEnv = "DIFY_API_KEY";
        } else if (isLiteLlmProvider(profile.provider)) {
            profile.apiKeyEnv = "LITELLM_MASTER_KEY";
        } else {
            profile.apiKeyEnv = "OPENAI_API_KEY";
        }
    }
    profile.apiKeyRequired = parseBoolJson(node["api_key_required"], true);

    profile.authHeader = trimCopy(safeText(node["auth_header"]));
    if (profile.authHeader.empty()) {
        profile.authHeader = isGeminiProvider(profile.provider) ? "x-goog-api-key" : "Authorization";
    }

    profile.apiKeyPrefix = safeText(node["api_key_prefix"]);
    if (profile.apiKeyPrefix.empty() && !isGeminiProvider(profile.provider)) {
        profile.apiKeyPrefix = "Bearer ";
    }

    profile.apiKeyInQuery = parseBoolJson(node["api_key_in_query"], isGeminiProvider(profile.provider));
    profile.apiKeyQueryName = trimCopy(safeText(node["api_key_query_name"]));
    if (profile.apiKeyQueryName.empty()) {
        profile.apiKeyQueryName = "key";
    }

    profile.temperature = parseDoubleJson(node["temperature"], 0.2, 0.0, 2.0);
    profile.maxOutputTokens = parseIntJson(node["max_output_tokens"], 1200, 64, 16000);
    profile.timeoutMs = parseIntJson(node["timeout_ms"], 30000, 1000, 120000);
    profile.difyUser = trimCopy(safeText(node["dify_user"]));
    if (profile.difyUser.empty()) {
        profile.difyUser = "gm-dashboard";
    }
    profile.difyResponseMode = toLowerAscii(trimCopy(safeText(node["dify_response_mode"])));
    if (profile.difyResponseMode != "streaming") {
        profile.difyResponseMode = "blocking";
    }
    profile.difyOutputKey = trimCopy(safeText(node["dify_output_key"]));
    if (profile.difyOutputKey.empty()) {
        profile.difyOutputKey = "insight_json";
    }

    profile.systemPreamble = trimCopy(safeText(node["system_preamble"]));
    profile.worldBook = readStringArray(node["world_book"]);
    profile.presetRules = readStringArray(node["preset_rules"]);
    profile.roleCard = node.isMember("role_card") && node["role_card"].isObject()
                           ? node["role_card"]
                           : Json::Value(Json::objectValue);

    if (node.isMember("rag") && node["rag"].isObject()) {
        const auto &rag = node["rag"];
        profile.ragEnabled = parseBoolJson(rag["enabled"], false);
        profile.ragKnowledgeFile = trimCopy(safeText(rag["knowledge_file"]));
        profile.ragTopK = parseIntJson(rag["top_k"], 4, 1, 10);
        profile.ragMaxSnippets = parseIntJson(rag["max_snippets"], 4, 1, 10);
        profile.ragSnippetMaxChars = parseIntJson(rag["snippet_max_chars"], 280, 60, 3000);
    }

    return true;
}

bool loadAgentConfig(AgentConfig &cfg, std::string &error) {
    cfg = AgentConfig();
    error.clear();

    cfg.configPath = resolveAiAgentConfigPath();

    Json::Value root;
    if (!readJsonFromFile(cfg.configPath, root, error) || !root.isObject()) {
        error = "failed to load ai agent config: " + error;
        return false;
    }

    cfg.defaultProfile = trimCopy(safeText(root["default_profile"]));

    if (!root.isMember("profiles") || !root["profiles"].isArray()) {
        error = "ai agent config missing profiles[]";
        return false;
    }

    for (const auto &item : root["profiles"]) {
        AgentProfile profile;
        if (!parseProfile(item, profile)) {
            continue;
        }
        cfg.profileOrder.push_back(profile.id);
        cfg.profileMap[profile.id] = profile;
    }

    if (cfg.profileMap.empty()) {
        error = "ai agent config has no valid profiles";
        return false;
    }

    if (cfg.defaultProfile.empty() || cfg.profileMap.find(cfg.defaultProfile) == cfg.profileMap.end()) {
        cfg.defaultProfile = cfg.profileOrder.front();
    }

    return true;
}

const AgentProfile *selectProfile(const AgentConfig &cfg,
                                  const Json::Value &requestBody,
                                  std::string &profileIdOut,
                                  std::string &error) {
    error.clear();
    std::string id;
    if (requestBody.isObject() && requestBody.isMember("profile_id") && requestBody["profile_id"].isString()) {
        id = trimCopy(requestBody["profile_id"].asString());
    }
    if (id.empty()) {
        id = cfg.defaultProfile;
    }

    auto it = cfg.profileMap.find(id);
    if (it == cfg.profileMap.end()) {
        error = "unknown profile_id: " + id;
        return nullptr;
    }
    profileIdOut = id;
    return &it->second;
}

std::string resolveEndpoint(const AgentProfile &profile) {
    if (!profile.endpoint.empty()) {
        auto endpoint = replaceAll(profile.endpoint, "{model}", profile.model);
        endpoint = replaceEnvTokens(endpoint);
        return trimCopy(endpoint);
    }

    if (isGeminiProvider(profile.provider)) {
        return "https://generativelanguage.googleapis.com/v1beta/models/" + profile.model + ":generateContent";
    }
    if (isLiteLlmProvider(profile.provider)) {
        auto baseEnv = getEnvTrimmed("LITELLM_BASE_URL");
        if (baseEnv.empty()) {
            baseEnv = "http://127.0.0.1:4000";
        }
        const auto base = trimTrailingSlash(baseEnv);
        return base + "/v1/chat/completions";
    }
    if (isDifyWorkflowProvider(profile.provider)) {
        auto baseEnv = getEnvTrimmed("DIFY_BASE_URL");
        if (baseEnv.empty()) {
            baseEnv = "http://127.0.0.1:8080";
        }
        const auto base = trimTrailingSlash(baseEnv);
        return base + "/v1/workflows/run";
    }
    if (isDifyChatProvider(profile.provider)) {
        auto baseEnv = getEnvTrimmed("DIFY_BASE_URL");
        if (baseEnv.empty()) {
            baseEnv = "http://127.0.0.1:8080";
        }
        const auto base = trimTrailingSlash(baseEnv);
        return base + "/v1/chat-messages";
    }

    return "https://api.openai.com/v1/chat/completions";
}

Json::Value buildProviderRequest(const AgentProfile &profile,
                                 const std::string &systemPrompt,
                                 const std::string &userPrompt,
                                 const Json::Value &requestBody,
                                 const Json::Value &snapshot) {
    Json::Value req(Json::objectValue);
    if (isGeminiProvider(profile.provider)) {
        req["systemInstruction"]["parts"][0]["text"] = systemPrompt;
        req["contents"][0]["role"] = "user";
        req["contents"][0]["parts"][0]["text"] = userPrompt;
        req["generationConfig"]["temperature"] = profile.temperature;
        req["generationConfig"]["maxOutputTokens"] = profile.maxOutputTokens;
        return req;
    }

    if (isDifyProvider(profile.provider)) {
        Json::Value inputs(Json::objectValue);
        inputs["system_prompt"] = systemPrompt;
        inputs["user_prompt"] = userPrompt;
        inputs["snapshot_json"] = jsonToCompactString(snapshot);
        if (snapshot.isObject()) {
            inputs["snapshot"] = snapshot;
        }
        if (requestBody.isObject()) {
            if (requestBody.isMember("scope")) {
                inputs["scope"] = requestBody["scope"];
            }
            if (requestBody.isMember("focus")) {
                inputs["focus"] = requestBody["focus"];
            }
            if (requestBody.isMember("tone")) {
                inputs["tone"] = requestBody["tone"];
            }
            if (requestBody.isMember("custom_prompt")) {
                inputs["custom_prompt"] = requestBody["custom_prompt"];
            }
        }
        req["inputs"] = inputs;
        req["response_mode"] = profile.difyResponseMode;
        req["user"] = profile.difyUser;
        if (isDifyChatProvider(profile.provider)) {
            req["query"] = userPrompt;
        }
        return req;
    }

    req["model"] = profile.model;
    req["messages"] = Json::Value(Json::arrayValue);
    Json::Value sys(Json::objectValue);
    sys["role"] = "system";
    sys["content"] = systemPrompt;
    req["messages"].append(sys);

    Json::Value user(Json::objectValue);
    user["role"] = "user";
    user["content"] = userPrompt;
    req["messages"].append(user);

    req["temperature"] = profile.temperature;
    req["max_tokens"] = profile.maxOutputTokens;
    req["stream"] = false;
    return req;
}

}  // namespace

bool listAiInsightProfiles(Json::Value &profilesOut,
                           std::string &defaultProfile,
                           std::string &error) {
    profilesOut = Json::Value(Json::arrayValue);
    defaultProfile.clear();
    error.clear();

    AgentConfig cfg;
    if (!loadAgentConfig(cfg, error)) {
        return false;
    }

    defaultProfile = cfg.defaultProfile;
    for (const auto &id : cfg.profileOrder) {
        auto it = cfg.profileMap.find(id);
        if (it == cfg.profileMap.end()) {
            continue;
        }
        const auto &profile = it->second;
        Json::Value item(Json::objectValue);
        item["id"] = profile.id;
        item["display_name"] = profile.displayName;
        item["description"] = profile.description;
        item["provider"] = profile.provider;
        item["model"] = profile.model;
        item["rag_enabled"] = profile.ragEnabled;
        item["endpoint_preview"] = sanitizeEndpointForLog(resolveEndpoint(profile));
        item["api_key_env"] = profile.apiKeyEnv;
        item["is_default"] = (profile.id == cfg.defaultProfile);
        profilesOut.append(item);
    }
    return true;
}

void runAiInsightAgentAsync(const Json::Value &requestBody,
                            const Json::Value &snapshot,
                            AiInsightAgentCallback callback) {
    AiInsightAgentResult precheck;

    AgentConfig cfg;
    std::string configError;
    if (!loadAgentConfig(cfg, configError)) {
        precheck.ok = false;
        precheck.status = drogon::k500InternalServerError;
        precheck.error = configError;
        callback(precheck);
        return;
    }

    std::string profileId;
    std::string profileError;
    const auto *selected = selectProfile(cfg, requestBody, profileId, profileError);
    if (selected == nullptr) {
        precheck.ok = false;
        precheck.status = drogon::k400BadRequest;
        precheck.error = profileError;
        callback(precheck);
        return;
    }

    const auto &profile = *selected;
    const auto apiKey = getEnvTrimmed(profile.apiKeyEnv.c_str());
    if (profile.apiKeyRequired && apiKey.empty()) {
        precheck.ok = false;
        precheck.status = drogon::k503ServiceUnavailable;
        precheck.error = "missing api key env: " + profile.apiKeyEnv;
        precheck.profileId = profile.id;
        precheck.provider = profile.provider;
        precheck.model = profile.model;
        callback(precheck);
        return;
    }

    const auto queryText = buildRagQueryText(requestBody, snapshot);
    const auto ragContext = retrieveRagContext(profile, cfg.configPath.parent_path(), queryText);
    const auto systemPrompt = buildSystemPrompt(profile, ragContext);
    const auto userPrompt = buildUserPrompt(requestBody, snapshot);
    const auto requestJson = buildProviderRequest(profile, systemPrompt, userPrompt, requestBody, snapshot);

    auto endpoint = resolveEndpoint(profile);
    if (profile.apiKeyInQuery && !apiKey.empty()) {
        endpoint = appendQuery(endpoint, profile.apiKeyQueryName, apiKey);
    }
    const auto endpointForLog = sanitizeEndpointForLog(endpoint);

    std::string host;
    std::string path;
    std::string endpointError;
    if (!splitUrlHostPath(endpoint, host, path, endpointError)) {
        precheck.ok = false;
        precheck.status = drogon::k500InternalServerError;
        precheck.error = endpointError;
        precheck.profileId = profile.id;
        precheck.provider = profile.provider;
        precheck.model = profile.model;
        callback(precheck);
        return;
    }

    auto client = drogon::HttpClient::newHttpClient(host);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);
    req->setPath(path);
    req->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    req->setBody(jsonToCompactString(requestJson));

    if (!profile.apiKeyInQuery && !apiKey.empty()) {
        req->addHeader(profile.authHeader, profile.apiKeyPrefix + apiKey);
    }

    const auto start = std::chrono::steady_clock::now();
    const double timeoutSec = static_cast<double>(profile.timeoutMs) / 1000.0;

    client->sendRequest(
        req,
        [callback = std::move(callback),
         profile,
         profileId,
         ragContext,
         snapshot,
         systemPrompt,
         userPrompt,
         endpointForLog,
         start](drogon::ReqResult result, const drogon::HttpResponsePtr &resp) {
            AiInsightAgentResult out;
            out.profileId = profileId;
            out.provider = profile.provider;
            out.model = profile.model;
            out.retrievedContext = ragContext;
            out.promptText = systemPrompt + "\n\n" + userPrompt;

            const auto cost = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  std::chrono::steady_clock::now() - start)
                                  .count();
            out.latencyMs = static_cast<int>(std::max<long long>(0, cost));

            if (result != drogon::ReqResult::Ok || !resp) {
                out.ok = false;
                out.status = drogon::k502BadGateway;
                out.error = "llm request failed: " + reqResultToText(result) + " (" + endpointForLog + ")";
                callback(out);
                return;
            }

            out.rawResponseText = resp->getBody();
            const auto parsedResp = parseJsonTextOrDefault(out.rawResponseText, Json::objectValue);
            if (resp->statusCode() < drogon::k200OK || resp->statusCode() >= drogon::k300MultipleChoices) {
                out.ok = false;
                out.status = drogon::k502BadGateway;
                auto reason = extractProviderErrorMessage(parsedResp);
                if (reason.size() > 220) {
                    reason = reason.substr(0, 220) + "...";
                }
                out.error = "llm endpoint returned status " + std::to_string(static_cast<int>(resp->statusCode()));
                if (!reason.empty()) {
                    out.error += ": " + reason;
                }
                callback(out);
                return;
            }

            if (!parsedResp.isObject() || parsedResp.empty()) {
                out.ok = false;
                out.status = drogon::k502BadGateway;
                out.error = "invalid llm response body";
                callback(out);
                return;
            }

            const auto modelText = extractProviderBodyText(parsedResp, profile);
            const auto normalizedText = stripCodeFence(modelText);
            Json::Value parsedInsight = parseJsonTextOrDefault(normalizedText, Json::objectValue);

            if (!parsedInsight.isObject() || parsedInsight.empty()) {
                const auto firstBrace = normalizedText.find('{');
                const auto lastBrace = normalizedText.rfind('}');
                if (firstBrace != std::string::npos && lastBrace != std::string::npos && lastBrace > firstBrace) {
                    parsedInsight = parseJsonTextOrDefault(
                        normalizedText.substr(firstBrace, lastBrace - firstBrace + 1),
                        Json::objectValue);
                }
            }

            out.insight = normalizeInsightJson(parsedInsight, snapshot, normalizedText);
            out.ok = true;
            out.status = drogon::k200OK;
            callback(out);
        },
        timeoutSec);
}

}  // namespace gm
