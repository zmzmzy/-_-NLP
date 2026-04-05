#include "../app_routes.h"

#include "../ai_insight_agent.h"
#include "../app_support.h"

#include <algorithm>
#include <ctime>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gm {

namespace {

struct InsightRequest {
    std::string scope{"all"};
    long long collegeId{0};
    long long majorId{0};
    std::string algorithmVersion;
    double alignmentThreshold{70.0};
    int minStudents{1};
    int smoothingK{20};
    int topN{5};
    int minGapCount{1};
    std::string focus{"balanced"};
    std::string tone{"neutral"};
    std::string customPrompt;
};

struct ScopeResolved {
    std::string scope{"all"};
    long long collegeId{0};
    long long majorId{0};
    std::string collegeName;
    std::string majorName;
    std::string scopeLabel{"全量专业"};
};

std::string nowLocalDateTime() {
    std::time_t now = std::time(nullptr);
    std::tm tmValue{};
#if defined(_WIN32)
    localtime_s(&tmValue, &now);
#else
    localtime_r(&now, &tmValue);
#endif
    char buffer[32] = {0};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tmValue);
    return buffer;
}

std::string jsonFieldAsString(const Json::Value &body, const char *key) {
    if (!body.isObject() || key == nullptr) {
        return "";
    }
    if (!body.isMember(key) || body[key].isNull()) {
        return "";
    }
    const auto &value = body[key];
    if (value.isString()) {
        return trimCopy(value.asString());
    }
    if (value.isBool()) {
        return value.asBool() ? "true" : "false";
    }
    if (value.isIntegral()) {
        return std::to_string(value.asLargestInt());
    }
    if (value.isDouble()) {
        return std::to_string(value.asDouble());
    }
    return "";
}

bool parseOptionalIntField(const Json::Value &body,
                           const char *key,
                           int minValue,
                           int maxValue,
                           int &target,
                           const DbCallback &callback) {
    if (!body.isMember(key) || body[key].isNull()) {
        return true;
    }
    int parsed = target;
    if (body[key].isIntegral()) {
        parsed = body[key].asInt();
    } else if (body[key].isString()) {
        if (!parseStrictInt(trimCopy(body[key].asString()), parsed)) {
            sendError(callback, drogon::k400BadRequest, std::string(key) + " must be an integer");
            return false;
        }
    } else {
        sendError(callback, drogon::k400BadRequest, std::string(key) + " must be an integer");
        return false;
    }
    if (parsed < minValue || parsed > maxValue) {
        sendError(
            callback,
            drogon::k400BadRequest,
            std::string(key) + " out of range [" + std::to_string(minValue) + ", " + std::to_string(maxValue) + "]");
        return false;
    }
    target = parsed;
    return true;
}

bool parseOptionalInt64Field(const Json::Value &body,
                             const char *key,
                             long long minValue,
                             long long maxValue,
                             long long &target,
                             const DbCallback &callback) {
    if (!body.isMember(key) || body[key].isNull()) {
        return true;
    }

    long long parsed = target;
    if (body[key].isIntegral()) {
        parsed = body[key].asInt64();
    } else if (body[key].isString()) {
        if (!parseStrictInt64(trimCopy(body[key].asString()), parsed)) {
            sendError(callback, drogon::k400BadRequest, std::string(key) + " must be an integer");
            return false;
        }
    } else {
        sendError(callback, drogon::k400BadRequest, std::string(key) + " must be an integer");
        return false;
    }

    if (parsed < minValue || parsed > maxValue) {
        sendError(
            callback,
            drogon::k400BadRequest,
            std::string(key) + " out of range [" + std::to_string(minValue) + ", " + std::to_string(maxValue) + "]");
        return false;
    }
    target = parsed;
    return true;
}

bool parseOptionalDoubleField(const Json::Value &body,
                              const char *key,
                              double minValue,
                              double maxValue,
                              double &target,
                              const DbCallback &callback) {
    if (!body.isMember(key) || body[key].isNull()) {
        return true;
    }

    double parsed = target;
    if (body[key].isNumeric()) {
        parsed = body[key].asDouble();
    } else if (body[key].isString()) {
        if (!parseStrictDouble(trimCopy(body[key].asString()), parsed)) {
            sendError(callback, drogon::k400BadRequest, std::string(key) + " must be a number");
            return false;
        }
    } else {
        sendError(callback, drogon::k400BadRequest, std::string(key) + " must be a number");
        return false;
    }

    if (parsed < minValue || parsed > maxValue) {
        sendError(
            callback,
            drogon::k400BadRequest,
            std::string(key) + " out of range [" + std::to_string(minValue) + ", " + std::to_string(maxValue) + "]");
        return false;
    }

    target = parsed;
    return true;
}

bool parseInsightRequest(const Json::Value &body, const DbCallback &callback, InsightRequest &request) {
    request = InsightRequest();
    if (!body.isObject()) {
        sendError(callback, drogon::k400BadRequest, "request body must be a JSON object");
        return false;
    }

    request.scope = toLowerAscii(jsonFieldAsString(body, "scope"));
    if (request.scope.empty()) {
        request.scope = "all";
    }
    static const std::unordered_set<std::string> kScopes = {"all", "college", "major"};
    if (!validateEnumValue("scope", request.scope, kScopes, callback)) {
        return false;
    }

    if (!parseOptionalInt64Field(body, "college_id", 0, 9223372036854775807LL, request.collegeId, callback)) {
        return false;
    }
    if (!parseOptionalInt64Field(body, "major_id", 0, 9223372036854775807LL, request.majorId, callback)) {
        return false;
    }
    if (request.scope == "college" && request.collegeId <= 0) {
        sendError(callback, drogon::k400BadRequest, "college_id is required when scope=college");
        return false;
    }
    if (request.scope == "major" && request.majorId <= 0) {
        sendError(callback, drogon::k400BadRequest, "major_id is required when scope=major");
        return false;
    }
    if (request.scope == "all") {
        request.collegeId = 0;
        request.majorId = 0;
    } else if (request.scope == "college") {
        request.majorId = 0;
    }

    request.algorithmVersion = jsonFieldAsString(body, "algorithm_version");
    if (request.algorithmVersion.size() > 64) {
        request.algorithmVersion = request.algorithmVersion.substr(0, 64);
    }

    if (!parseOptionalDoubleField(
            body,
            "alignment_threshold",
            0.0,
            100.0,
            request.alignmentThreshold,
            callback)) {
        return false;
    }
    if (!parseOptionalIntField(body, "min_students", 0, 1000000, request.minStudents, callback)) {
        return false;
    }
    if (!parseOptionalIntField(body, "smoothing_k", 1, 1000000, request.smoothingK, callback)) {
        return false;
    }
    if (!parseOptionalIntField(body, "top_n", 1, 20, request.topN, callback)) {
        return false;
    }
    if (!parseOptionalIntField(body, "min_gap_count", 1, 1000000, request.minGapCount, callback)) {
        return false;
    }

    request.focus = toLowerAscii(jsonFieldAsString(body, "focus"));
    if (request.focus.empty()) {
        request.focus = "balanced";
    }
    static const std::unordered_set<std::string> kFocus = {"balanced", "alignment", "gap", "risk", "employment"};
    if (!validateEnumValue("focus", request.focus, kFocus, callback)) {
        return false;
    }

    request.tone = toLowerAscii(jsonFieldAsString(body, "tone"));
    if (request.tone.empty()) {
        request.tone = "neutral";
    }
    static const std::unordered_set<std::string> kTone = {"neutral", "strict", "encourage", "executive"};
    if (!validateEnumValue("tone", request.tone, kTone, callback)) {
        return false;
    }

    request.customPrompt = jsonFieldAsString(body, "custom_prompt");
    if (request.customPrompt.size() > 4000) {
        sendError(callback, drogon::k400BadRequest, "custom_prompt is too long, max 4000 chars");
        return false;
    }
    return true;
}

std::string buildScopeLabel(const ScopeResolved &scope) {
    if (scope.scope == "college") {
        if (!scope.collegeName.empty()) {
            return "单学院：" + scope.collegeName;
        }
        return "单学院";
    }
    if (scope.scope == "major") {
        if (!scope.majorName.empty()) {
            return "单专业：" + scope.majorName;
        }
        return "单专业";
    }
    return "全量专业";
}

void resolveScopeAsync(const drogon::orm::DbClientPtr &db,
                       const InsightRequest &request,
                       const DbCallback &callback,
                       const std::function<void(const ScopeResolved &)> &onReady) {
    ScopeResolved resolved;
    resolved.scope = request.scope;
    resolved.collegeId = request.collegeId;
    resolved.majorId = request.majorId;
    resolved.scopeLabel = buildScopeLabel(resolved);

    if (request.scope == "all") {
        onReady(resolved);
        return;
    }

    if (request.scope == "college") {
        db->execSqlAsync(
            "SELECT id, name FROM colleges WHERE id = ? LIMIT 1",
            [callback, onReady, resolved](const drogon::orm::Result &result) mutable {
                if (result.empty()) {
                    sendError(callback, drogon::k404NotFound, "college not found");
                    return;
                }
                resolved.collegeName = result[0]["name"].isNull() ? "" : result[0]["name"].as<std::string>();
                resolved.scopeLabel = buildScopeLabel(resolved);
                onReady(resolved);
            },
            [callback](const DrogonDbException &e) {
                sendDbError(e, callback);
            },
            request.collegeId);
        return;
    }

    db->execSqlAsync(
        "SELECT m.id AS major_id, m.name AS major_name, m.college_id AS college_id, c.name AS college_name "
        "FROM majors m "
        "INNER JOIN colleges c ON c.id = m.college_id "
        "WHERE m.id = ? "
        "LIMIT 1",
        [callback, onReady, resolved, request](const drogon::orm::Result &result) mutable {
            if (result.empty()) {
                sendError(callback, drogon::k404NotFound, "major not found");
                return;
            }
            const auto rowCollegeId = result[0]["college_id"].as<long long>();
            if (request.collegeId > 0 && request.collegeId != rowCollegeId) {
                sendError(callback, drogon::k400BadRequest, "major_id does not belong to college_id");
                return;
            }

            resolved.majorId = result[0]["major_id"].as<long long>();
            resolved.majorName = result[0]["major_name"].isNull() ? "" : result[0]["major_name"].as<std::string>();
            resolved.collegeId = rowCollegeId;
            resolved.collegeName = result[0]["college_name"].isNull() ? "" : result[0]["college_name"].as<std::string>();
            resolved.scopeLabel = buildScopeLabel(resolved);
            onReady(resolved);
        },
        [callback](const DrogonDbException &e) {
            sendDbError(e, callback);
        },
        request.majorId);
}

Json::Value parseJsonColumn(const drogon::orm::Row &row,
                            const std::string &column,
                            Json::ValueType type) {
    if (!row[column].isNull()) {
        const auto parsed = parseJsonTextOrDefault(row[column].as<std::string>(), type);
        if ((type == Json::arrayValue && parsed.isArray()) || (type == Json::objectValue && parsed.isObject())) {
            return parsed;
        }
    }
    return Json::Value(type);
}

std::string scopeLabelFromRow(const drogon::orm::Row &row) {
    const auto scope = row["request_scope"].isNull() ? "all" : row["request_scope"].as<std::string>();
    if (scope == "major") {
        const auto majorName = row["major_name"].isNull() ? "" : row["major_name"].as<std::string>();
        if (!majorName.empty()) {
            return "单专业：" + majorName;
        }
        return "单专业";
    }
    if (scope == "college") {
        const auto collegeName = row["college_name"].isNull() ? "" : row["college_name"].as<std::string>();
        if (!collegeName.empty()) {
            return "单学院：" + collegeName;
        }
        return "单学院";
    }
    return "全量专业";
}

Json::Value runRowToJson(const drogon::orm::Row &row, bool includePayload) {
    Json::Value item(Json::objectValue);
    item["id"] = static_cast<Json::Int64>(row["id"].as<long long>());
    item["scope"] = row["request_scope"].isNull() ? "all" : row["request_scope"].as<std::string>();
    item["scope_label"] = scopeLabelFromRow(row);
    item["target_college_id"] =
        row["target_college_id"].isNull() ? Json::Value(Json::nullValue)
                                          : Json::Value(static_cast<Json::Int64>(row["target_college_id"].as<long long>()));
    item["target_major_id"] =
        row["target_major_id"].isNull() ? Json::Value(Json::nullValue)
                                        : Json::Value(static_cast<Json::Int64>(row["target_major_id"].as<long long>()));
    item["college_name"] = row["college_name"].isNull() ? "" : row["college_name"].as<std::string>();
    item["major_name"] = row["major_name"].isNull() ? "" : row["major_name"].as<std::string>();
    item["algorithm_version"] = row["algorithm_version"].isNull() ? "" : row["algorithm_version"].as<std::string>();
    item["profile_id"] = row["profile_id"].isNull() ? "" : row["profile_id"].as<std::string>();
    item["provider"] = row["provider"].isNull() ? "" : row["provider"].as<std::string>();
    item["model"] = row["model"].isNull() ? "" : row["model"].as<std::string>();
    item["focus"] = row["focus"].isNull() ? "" : row["focus"].as<std::string>();
    item["tone"] = row["tone"].isNull() ? "" : row["tone"].as<std::string>();
    item["status"] = row["status"].isNull() ? "failed" : row["status"].as<std::string>();
    item["latency_ms"] = row["latency_ms"].isNull() ? 0 : row["latency_ms"].as<int>();
    item["created_by_user_id"] =
        row["created_by_user_id"].isNull() ? Json::Value(Json::nullValue)
                                           : Json::Value(static_cast<Json::Int64>(row["created_by_user_id"].as<long long>()));
    item["created_at"] = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
    item["error_message"] = row["error_message"].isNull() ? "" : row["error_message"].as<std::string>();

    const auto insight = parseJsonColumn(row, "insight_json", Json::objectValue);
    item["summary"] = insight.isObject() && insight.isMember("summary") ? insight["summary"] : Json::Value("");
    item["confidence"] = insight.isObject() && insight.isMember("confidence") ? insight["confidence"] : Json::Value("");
    if (insight.isObject() && insight.isMember("evidence") && insight["evidence"].isObject()) {
        item["evidence"] = insight["evidence"];
    } else {
        item["evidence"] = Json::Value(Json::objectValue);
    }

    if (includePayload) {
        item["snapshot"] = parseJsonColumn(row, "snapshot_json", Json::objectValue);
        item["retrieved_context"] = parseJsonColumn(row, "retrieved_context_json", Json::arrayValue);
        item["insight"] = insight;
        item["prompt_text"] = row["prompt_text"].isNull() ? "" : row["prompt_text"].as<std::string>();
        item["raw_response_text"] = row["raw_response_text"].isNull() ? "" : row["raw_response_text"].as<std::string>();
    }
    return item;
}

Json::Value buildGroupedGapRows(const drogon::orm::Result &result, Json::Value &flatSkillsOut) {
    Json::Value grouped(Json::arrayValue);
    flatSkillsOut = Json::Value(Json::arrayValue);
    std::unordered_map<long long, Json::ArrayIndex> indexByMajor;

    for (const auto &row : result) {
        const auto majorId = row["major_id"].as<long long>();
        Json::ArrayIndex idx = 0;
        auto it = indexByMajor.find(majorId);
        if (it == indexByMajor.end()) {
            Json::Value major(Json::objectValue);
            major["major_id"] = static_cast<Json::Int64>(majorId);
            major["major_code"] = row["major_code"].isNull() ? "" : row["major_code"].as<std::string>();
            major["major_name"] = row["major_name"].isNull() ? "" : row["major_name"].as<std::string>();
            major["college_id"] = static_cast<Json::Int64>(row["college_id"].as<long long>());
            major["college_name"] = row["college_name"].isNull() ? "" : row["college_name"].as<std::string>();
            major["skills"] = Json::Value(Json::arrayValue);
            grouped.append(major);
            idx = grouped.size() - 1;
            indexByMajor[majorId] = idx;
        } else {
            idx = it->second;
        }

        Json::Value skill(Json::objectValue);
        skill["rank"] = row["rank_no"].isNull() ? 0 : row["rank_no"].as<int>();
        skill["skill_name"] = row["skill_name"].isNull() ? "" : row["skill_name"].as<std::string>();
        skill["gap_count"] = static_cast<Json::UInt64>(row["gap_count"].as<unsigned long long>());
        skill["student_count"] = static_cast<Json::UInt64>(row["student_count"].as<unsigned long long>());
        skill["mandatory_count"] = static_cast<Json::UInt64>(row["mandatory_count"].as<unsigned long long>());

        grouped[idx]["skills"].append(skill);

        Json::Value flatItem(skill);
        flatItem["major_id"] = grouped[idx]["major_id"];
        flatItem["major_name"] = grouped[idx]["major_name"];
        flatItem["college_name"] = grouped[idx]["college_name"];
        flatSkillsOut.append(flatItem);
    }
    return grouped;
}

Json::Value buildSnapshot(const InsightRequest &request,
                          const ScopeResolved &scope,
                          const Json::Value &alignmentRows,
                          const Json::Value &groupedGapRows,
                          const Json::Value &flatGapSkills) {
    Json::Value snapshot(Json::objectValue);
    snapshot["generated_at"] = nowLocalDateTime();
    snapshot["scope"]["type"] = scope.scope;
    snapshot["scope"]["college_id"] = static_cast<Json::Int64>(scope.collegeId);
    snapshot["scope"]["major_id"] = static_cast<Json::Int64>(scope.majorId);
    snapshot["scope"]["college_name"] = scope.collegeName;
    snapshot["scope"]["major_name"] = scope.majorName;
    snapshot["scope"]["label"] = scope.scopeLabel;

    snapshot["params"]["algorithm_version"] = request.algorithmVersion;
    snapshot["params"]["alignment_threshold"] = request.alignmentThreshold;
    snapshot["params"]["min_students"] = request.minStudents;
    snapshot["params"]["smoothing_k"] = request.smoothingK;
    snapshot["params"]["top_n"] = request.topN;
    snapshot["params"]["min_gap_count"] = request.minGapCount;
    snapshot["params"]["focus"] = request.focus;
    snapshot["params"]["tone"] = request.tone;
    snapshot["params"]["custom_prompt"] = request.customPrompt;

    long long totalStudents = 0;
    long long employedStudents = 0;
    long long alignedStudents = 0;
    long long matchedStudents = 0;
    long long lowConfidenceCount = 0;
    double excellentWeighted = 0.0;
    long long excellentWeight = 0;

    Json::Value topMajors(Json::arrayValue);
    Json::Value riskMajors(Json::arrayValue);

    std::vector<Json::Value> riskCandidates;
    riskCandidates.reserve(alignmentRows.size());

    for (const auto &row : alignmentRows) {
        totalStudents += row.isMember("total_students") ? row["total_students"].asInt64() : 0;
        employedStudents += row.isMember("employed_students") ? row["employed_students"].asInt64() : 0;
        alignedStudents += row.isMember("aligned_students") ? row["aligned_students"].asInt64() : 0;
        matchedStudents += row.isMember("employed_with_match_students") ? row["employed_with_match_students"].asInt64() : 0;

        const auto employed = row.isMember("employed_students") ? row["employed_students"].asInt64() : 0;
        excellentWeighted += (row.isMember("excellent_graduate_score") ? row["excellent_graduate_score"].asDouble() : 0.0) *
                            static_cast<double>(employed);
        excellentWeight += employed;

        const auto confidence = row.isMember("confidence_level") ? toLowerAscii(row["confidence_level"].asString()) : "none";
        if (confidence == "low" || confidence == "none") {
            ++lowConfidenceCount;
        }
        riskCandidates.push_back(row);
    }

    std::vector<Json::Value> sortedTopRows;
    sortedTopRows.reserve(alignmentRows.size());
    for (const auto &item : alignmentRows) {
        sortedTopRows.push_back(item);
    }
    std::sort(sortedTopRows.begin(), sortedTopRows.end(), [](const Json::Value &a, const Json::Value &b) {
        if (a["excellent_graduate_score"].asDouble() != b["excellent_graduate_score"].asDouble()) {
            return a["excellent_graduate_score"].asDouble() > b["excellent_graduate_score"].asDouble();
        }
        return a["alignment_rate"].asDouble() > b["alignment_rate"].asDouble();
    });
    for (size_t i = 0; i < sortedTopRows.size() && i < 8; ++i) {
        topMajors.append(sortedTopRows[i]);
    }

    std::sort(riskCandidates.begin(), riskCandidates.end(), [](const Json::Value &a, const Json::Value &b) {
        if (a["alignment_rate"].asDouble() != b["alignment_rate"].asDouble()) {
            return a["alignment_rate"].asDouble() < b["alignment_rate"].asDouble();
        }
        if (a["excellent_graduate_score"].asDouble() != b["excellent_graduate_score"].asDouble()) {
            return a["excellent_graduate_score"].asDouble() < b["excellent_graduate_score"].asDouble();
        }
        return a["major_id"].asInt64() < b["major_id"].asInt64();
    });
    for (const auto &item : riskCandidates) {
        const bool isLowConfidence = [&]() {
            const auto confidence = item.isMember("confidence_level") ? toLowerAscii(item["confidence_level"].asString()) : "none";
            return confidence == "low" || confidence == "none";
        }();
        if (item["alignment_rate"].asDouble() < request.alignmentThreshold || isLowConfidence) {
            riskMajors.append(item);
        }
        if (riskMajors.size() >= 8) {
            break;
        }
    }
    if (riskMajors.empty()) {
        for (Json::ArrayIndex i = 0; i < static_cast<Json::ArrayIndex>(riskCandidates.size()) && i < 6; ++i) {
            riskMajors.append(riskCandidates[i]);
        }
    }

    std::vector<Json::Value> flatSkillVec;
    flatSkillVec.reserve(flatGapSkills.size());
    for (const auto &item : flatGapSkills) {
        flatSkillVec.push_back(item);
    }
    std::sort(flatSkillVec.begin(), flatSkillVec.end(), [](const Json::Value &a, const Json::Value &b) {
        const double wa =
            static_cast<double>(a["mandatory_count"].asInt64()) * 10.0 + static_cast<double>(a["gap_count"].asInt64());
        const double wb =
            static_cast<double>(b["mandatory_count"].asInt64()) * 10.0 + static_cast<double>(b["gap_count"].asInt64());
        if (wa != wb) {
            return wa > wb;
        }
        return a["skill_name"].asString() < b["skill_name"].asString();
    });
    Json::Value topGapSkills(Json::arrayValue);
    for (size_t i = 0; i < flatSkillVec.size() && i < 12; ++i) {
        topGapSkills.append(flatSkillVec[i]);
    }

    const auto majorCount = static_cast<long long>(alignmentRows.size());
    const auto employmentRate = totalStudents > 0
                                    ? (static_cast<double>(employedStudents) * 100.0) / static_cast<double>(totalStudents)
                                    : 0.0;
    const auto alignmentRate = employedStudents > 0
                                   ? (static_cast<double>(alignedStudents) * 100.0) / static_cast<double>(employedStudents)
                                   : 0.0;
    const auto coverageRate = employedStudents > 0
                                  ? (static_cast<double>(matchedStudents) * 100.0) / static_cast<double>(employedStudents)
                                  : 0.0;
    const auto avgExcellentScore = excellentWeight > 0
                                       ? excellentWeighted / static_cast<double>(excellentWeight)
                                       : 0.0;

    snapshot["metrics"]["major_count"] = static_cast<Json::Int64>(majorCount);
    snapshot["metrics"]["total_students"] = static_cast<Json::Int64>(totalStudents);
    snapshot["metrics"]["employed_students"] = static_cast<Json::Int64>(employedStudents);
    snapshot["metrics"]["aligned_students"] = static_cast<Json::Int64>(alignedStudents);
    snapshot["metrics"]["matched_students"] = static_cast<Json::Int64>(matchedStudents);
    snapshot["metrics"]["employment_rate"] = employmentRate;
    snapshot["metrics"]["alignment_rate"] = alignmentRate;
    snapshot["metrics"]["coverage_rate"] = coverageRate;
    snapshot["metrics"]["avg_excellent_score"] = avgExcellentScore;
    snapshot["metrics"]["low_confidence_count"] = static_cast<Json::Int64>(lowConfidenceCount);
    snapshot["metrics"]["top_gap_skill"] =
        topGapSkills.empty() ? Json::Value("暂无") : Json::Value(topGapSkills[0]["skill_name"]);

    snapshot["top_majors"] = topMajors;
    snapshot["risk_majors"] = riskMajors;
    snapshot["top_gap_skills"] = topGapSkills;

    Json::Value alignmentSampleTop(Json::arrayValue);
    Json::Value alignmentSampleBottom(Json::arrayValue);
    for (size_t i = 0; i < sortedTopRows.size() && i < 12; ++i) {
        alignmentSampleTop.append(sortedTopRows[i]);
    }
    if (sortedTopRows.size() > 12) {
        for (size_t i = sortedTopRows.size(); i > 0 && alignmentSampleBottom.size() < 8; --i) {
            alignmentSampleBottom.append(sortedTopRows[i - 1]);
        }
    }
    snapshot["alignment_sample_top"] = alignmentSampleTop;
    snapshot["alignment_sample_bottom"] = alignmentSampleBottom;
    snapshot["gap_groups"] = groupedGapRows;
    return snapshot;
}

const char *kEnsureInsightRunsTableSql = R"SQL(
CREATE TABLE IF NOT EXISTS dashboard_insight_runs (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  request_scope ENUM('all', 'college', 'major') NOT NULL DEFAULT 'all',
  target_college_id BIGINT NULL,
  target_major_id BIGINT NULL,
  algorithm_version VARCHAR(64) NULL,
  profile_id VARCHAR(64) NOT NULL,
  provider VARCHAR(64) NULL,
  model VARCHAR(128) NULL,
  focus VARCHAR(32) NOT NULL DEFAULT 'balanced',
  tone VARCHAR(32) NOT NULL DEFAULT 'neutral',
  custom_prompt TEXT NULL,
  status ENUM('success', 'failed') NOT NULL DEFAULT 'failed',
  latency_ms INT NOT NULL DEFAULT 0,
  snapshot_json JSON NOT NULL,
  retrieved_context_json JSON NULL,
  insight_json JSON NULL,
  prompt_text MEDIUMTEXT NULL,
  raw_response_text MEDIUMTEXT NULL,
  error_message TEXT NULL,
  created_by_user_id BIGINT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_insight_runs_scope (request_scope),
  INDEX idx_insight_runs_status (status),
  INDEX idx_insight_runs_profile (profile_id),
  INDEX idx_insight_runs_created (created_at),
  CONSTRAINT fk_insight_runs_college
    FOREIGN KEY (target_college_id) REFERENCES colleges(id)
    ON DELETE SET NULL ON UPDATE CASCADE,
  CONSTRAINT fk_insight_runs_major
    FOREIGN KEY (target_major_id) REFERENCES majors(id)
    ON DELETE SET NULL ON UPDATE CASCADE,
  CONSTRAINT fk_insight_runs_user
    FOREIGN KEY (created_by_user_id) REFERENCES users(id)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB
)SQL";

void ensureInsightRunsTable(const drogon::orm::DbClientPtr &db,
                            const DbCallback &callback,
                            const std::function<void()> &onReady) {
    db->execSqlAsync(
        kEnsureInsightRunsTableSql,
        [onReady](const drogon::orm::Result &) {
            onReady();
        },
        [callback](const DrogonDbException &e) {
            sendDbError(e, callback);
        });
}

const char *kAlignmentSql = R"SQL(
WITH major_students AS (
    SELECT
        m.id AS major_id,
        m.code AS major_code,
        m.name AS major_name,
        m.college_id AS college_id,
        c.name AS college_name,
        s.id AS student_id
    FROM majors m
    INNER JOIN colleges c ON c.id = m.college_id
    LEFT JOIN students s ON s.major_id = m.id
    WHERE (? = 0 OR m.college_id = ?)
      AND (? = 0 OR m.id = ?)
),
latest_employment AS (
    SELECT er.student_id, er.job_id, er.employment_type
    FROM employment_records er
    INNER JOIN (
        SELECT student_id, MAX(id) AS latest_id
        FROM employment_records
        WHERE employment_type <> 'pending'
        GROUP BY student_id
    ) latest ON latest.latest_id = er.id
),
latest_match AS (
    SELECT mr.student_id, mr.job_id, mr.final_score, mr.constraint_score
    FROM match_results mr
    INNER JOIN (
        SELECT student_id, job_id, MAX(id) AS latest_id
        FROM match_results
        WHERE (? = '' OR algorithm_version = ?)
        GROUP BY student_id, job_id
    ) latest ON latest.latest_id = mr.id
),
major_stats AS (
    SELECT
        ms.major_id,
        ms.major_code,
        ms.major_name,
        ms.college_id,
        ms.college_name,
        COUNT(DISTINCT ms.student_id) AS total_students,
        COUNT(DISTINCT CASE WHEN le.student_id IS NOT NULL THEN ms.student_id END) AS employed_students,
        COUNT(DISTINCT CASE WHEN le.job_id IS NOT NULL AND lm.final_score IS NOT NULL THEN ms.student_id END)
            AS employed_with_match_students,
        COUNT(DISTINCT CASE WHEN le.job_id IS NOT NULL AND lm.final_score >= ? THEN ms.student_id END)
            AS aligned_students,
        AVG(CASE WHEN le.job_id IS NOT NULL AND lm.final_score IS NOT NULL THEN lm.final_score END) AS avg_match_score,
        SUM(
            CASE
                WHEN le.job_id IS NOT NULL AND lm.final_score IS NOT NULL THEN
                    lm.final_score * (
                        (CASE
                            WHEN le.employment_type = 'signed' THEN 1.0
                            WHEN le.employment_type = 'offer' THEN 0.9
                            WHEN le.employment_type = 'internship' THEN 0.75
                            ELSE 0.7
                        END) * (0.5 + 0.5 * COALESCE(lm.constraint_score, 1.0))
                    )
                ELSE 0.0
            END
        ) AS weighted_score_sum,
        SUM(
            CASE
                WHEN le.job_id IS NOT NULL AND lm.final_score IS NOT NULL THEN
                    (CASE
                        WHEN le.employment_type = 'signed' THEN 1.0
                        WHEN le.employment_type = 'offer' THEN 0.9
                        WHEN le.employment_type = 'internship' THEN 0.75
                        ELSE 0.7
                    END) * (0.5 + 0.5 * COALESCE(lm.constraint_score, 1.0))
                ELSE 0.0
            END
        ) AS weighted_score_weight
    FROM major_students ms
    LEFT JOIN latest_employment le ON le.student_id = ms.student_id
    LEFT JOIN latest_match lm ON lm.student_id = ms.student_id AND lm.job_id = le.job_id
    GROUP BY ms.major_id, ms.major_code, ms.major_name, ms.college_id, ms.college_name
),
metrics AS (
    SELECT
        major_id,
        major_code,
        major_name,
        college_id,
        college_name,
        total_students,
        employed_students,
        employed_with_match_students,
        aligned_students,
        COALESCE(ROUND(avg_match_score, 3), 0.0) AS avg_match_score,
        CASE
            WHEN weighted_score_weight <= 0 THEN 0.0
            ELSE ROUND(weighted_score_sum / weighted_score_weight, 3)
        END AS weighted_avg_match_score,
        CASE
            WHEN total_students <= 0 THEN 0.0
            ELSE ROUND(employed_students * 100.0 / total_students, 2)
        END AS employment_rate,
        CASE
            WHEN employed_students <= 0 THEN 0.0
            ELSE ROUND(aligned_students * 100.0 / employed_students, 2)
        END AS alignment_rate,
        CASE
            WHEN employed_students <= 0 THEN 0.0
            ELSE ROUND(employed_with_match_students * 100.0 / employed_students, 2)
        END AS coverage_rate
    FROM major_stats
    WHERE total_students >= ?
),
scored AS (
    SELECT
        metrics.*,
        CASE
            WHEN weighted_avg_match_score <= 0 THEN 0.0
            ELSE ROUND(
                0.5 * weighted_avg_match_score +
                0.2 * alignment_rate +
                0.2 * employment_rate +
                0.1 * coverage_rate,
                3
            )
        END AS raw_major_score
    FROM metrics
),
global_mean AS (
    SELECT COALESCE(ROUND(AVG(raw_major_score), 3), 0.0) AS score_mean
    FROM scored
    WHERE employed_students > 0
),
final_stats AS (
    SELECT
        s.major_id,
        s.major_code,
        s.major_name,
        s.college_id,
        s.college_name,
        s.total_students,
        s.employed_students,
        s.employed_with_match_students,
        s.aligned_students,
        s.avg_match_score,
        s.weighted_avg_match_score,
        s.raw_major_score,
        CASE
            WHEN s.employed_students <= 0 THEN 0.0
            ELSE ROUND(
                (s.employed_students * 1.0 / (s.employed_students + ?)) * s.raw_major_score +
                (? * 1.0 / (s.employed_students + ?)) * gm.score_mean,
                3
            )
        END AS excellent_graduate_score,
        s.employment_rate,
        s.alignment_rate,
        s.coverage_rate,
        CASE
            WHEN s.employed_students >= 20 THEN 'high'
            WHEN s.employed_students >= 10 THEN 'medium'
            WHEN s.employed_students > 0 THEN 'low'
            ELSE 'none'
        END AS confidence_level
    FROM scored s
    CROSS JOIN global_mean gm
)
SELECT
    major_id,
    major_code,
    major_name,
    college_id,
    college_name,
    total_students,
    employed_students,
    employed_with_match_students,
    aligned_students,
    avg_match_score,
    weighted_avg_match_score,
    raw_major_score,
    excellent_graduate_score,
    employment_rate,
    alignment_rate,
    coverage_rate,
    confidence_level
FROM final_stats
ORDER BY excellent_graduate_score DESC,
         raw_major_score DESC,
         alignment_rate DESC,
         weighted_avg_match_score DESC,
         employed_students DESC,
         major_id ASC
)SQL";

const char *kGapSql = R"SQL(
WITH filtered_students AS (
    SELECT s.id AS student_id, s.major_id
    FROM students s
    INNER JOIN majors m ON m.id = s.major_id
    WHERE (? = 0 OR m.college_id = ?)
      AND (? = 0 OR m.id = ?)
),
latest_employment AS (
    SELECT er.student_id, er.job_id
    FROM employment_records er
    INNER JOIN (
        SELECT student_id, MAX(id) AS latest_id
        FROM employment_records
        WHERE employment_type <> 'pending' AND job_id IS NOT NULL
        GROUP BY student_id
    ) latest ON latest.latest_id = er.id
),
latest_match AS (
    SELECT mr.student_id, mr.job_id, mr.missing_skills_json
    FROM match_results mr
    INNER JOIN (
        SELECT student_id, job_id, MAX(id) AS latest_id
        FROM match_results
        WHERE (? = '' OR algorithm_version = ?)
          AND JSON_VALID(missing_skills_json)
          AND JSON_TYPE(missing_skills_json) = 'ARRAY'
        GROUP BY student_id, job_id
    ) latest ON latest.latest_id = mr.id
),
expanded_gaps AS (
    SELECT
        fs.major_id,
        fs.student_id,
        jt.skill_name,
        CASE
            WHEN jt.mandatory_raw IS NULL THEN 0
            WHEN LOWER(jt.mandatory_raw) IN ('1', 'true') THEN 1
            ELSE 0
        END AS mandatory
    FROM filtered_students fs
    INNER JOIN latest_employment le ON le.student_id = fs.student_id
    INNER JOIN latest_match lm ON lm.student_id = fs.student_id AND lm.job_id = le.job_id
    INNER JOIN JSON_TABLE(
        CAST(lm.missing_skills_json AS CHAR),
        '$[*]' COLUMNS (
            skill_name VARCHAR(128) PATH '$.skill_name' NULL ON EMPTY NULL ON ERROR,
            mandatory_raw VARCHAR(16) PATH '$.mandatory' NULL ON EMPTY NULL ON ERROR
        )
    ) AS jt
),
gap_stats AS (
    SELECT
        eg.major_id,
        eg.skill_name,
        COUNT(*) AS gap_count,
        COUNT(DISTINCT eg.student_id) AS student_count,
        SUM(CASE WHEN eg.mandatory = 1 THEN 1 ELSE 0 END) AS mandatory_count
    FROM expanded_gaps eg
    WHERE eg.skill_name IS NOT NULL AND eg.skill_name <> ''
    GROUP BY eg.major_id, eg.skill_name
    HAVING COUNT(*) >= ?
),
ranked AS (
    SELECT
        gs.major_id,
        gs.skill_name,
        gs.gap_count,
        gs.student_count,
        gs.mandatory_count,
        ROW_NUMBER() OVER (
            PARTITION BY gs.major_id
            ORDER BY gs.gap_count DESC, gs.mandatory_count DESC, gs.skill_name ASC
        ) AS rank_no
    FROM gap_stats gs
)
SELECT
    r.major_id,
    m.code AS major_code,
    m.name AS major_name,
    m.college_id AS college_id,
    c.name AS college_name,
    r.skill_name,
    r.gap_count,
    r.student_count,
    r.mandatory_count,
    r.rank_no
FROM ranked r
INNER JOIN majors m ON m.id = r.major_id
INNER JOIN colleges c ON c.id = m.college_id
WHERE r.rank_no <= ?
ORDER BY r.major_id ASC, r.rank_no ASC
)SQL";

}  // namespace

void registerDashboardInsightRoutes() {
    drogon::app().registerHandler(
        "/api/dashboard/insight/profiles",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            SessionUser user;
            if (!requireAuthUser(req, callback, user)) {
                return;
            }

            Json::Value profiles(Json::arrayValue);
            std::string defaultProfile;
            std::string error;
            if (!listAiInsightProfiles(profiles, defaultProfile, error)) {
                sendError(callback, drogon::k500InternalServerError, error, "service_error");
                return;
            }

            Json::Value body(Json::objectValue);
            body["data"] = profiles;
            body["meta"]["default_profile"] = defaultProfile;
            callback(makeJson(body));
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/dashboard/insight",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            SessionUser user;
            if (!requireAuthUser(req, callback, user)) {
                return;
            }

            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }

            InsightRequest insightRequest;
            if (!parseInsightRequest(*json, callback, insightRequest)) {
                return;
            }
            const Json::Value requestBody = *json;

            ensureInsightRunsTable(
                db,
                callback,
                [db, callback, user, requestBody, insightRequest]() {
                    resolveScopeAsync(
                        db,
                        insightRequest,
                        callback,
                        [db, callback, user, requestBody, insightRequest](const ScopeResolved &scopeResolved) {
                            db->execSqlAsync(
                                kAlignmentSql,
                                [db, callback, user, requestBody, insightRequest, scopeResolved]
                                (const drogon::orm::Result &alignmentResult) {
                                    Json::Value alignmentRows(Json::arrayValue);
                                    for (const auto &row : alignmentResult) {
                                        Json::Value item(Json::objectValue);
                                        item["major_id"] = static_cast<Json::Int64>(row["major_id"].as<long long>());
                                        item["major_code"] = row["major_code"].isNull() ? "" : row["major_code"].as<std::string>();
                                        item["major_name"] = row["major_name"].isNull() ? "" : row["major_name"].as<std::string>();
                                        item["college_id"] = static_cast<Json::Int64>(row["college_id"].as<long long>());
                                        item["college_name"] = row["college_name"].isNull() ? "" : row["college_name"].as<std::string>();
                                        item["total_students"] =
                                            static_cast<Json::UInt64>(row["total_students"].as<unsigned long long>());
                                        item["employed_students"] =
                                            static_cast<Json::UInt64>(row["employed_students"].as<unsigned long long>());
                                        item["employed_with_match_students"] =
                                            static_cast<Json::UInt64>(row["employed_with_match_students"].as<unsigned long long>());
                                        item["aligned_students"] =
                                            static_cast<Json::UInt64>(row["aligned_students"].as<unsigned long long>());
                                        item["avg_match_score"] = row["avg_match_score"].isNull() ? 0.0 : row["avg_match_score"].as<double>();
                                        item["weighted_avg_match_score"] =
                                            row["weighted_avg_match_score"].isNull() ? 0.0 : row["weighted_avg_match_score"].as<double>();
                                        item["raw_major_score"] =
                                            row["raw_major_score"].isNull() ? 0.0 : row["raw_major_score"].as<double>();
                                        item["excellent_graduate_score"] =
                                            row["excellent_graduate_score"].isNull() ? 0.0 : row["excellent_graduate_score"].as<double>();
                                        item["employment_rate"] = row["employment_rate"].isNull() ? 0.0 : row["employment_rate"].as<double>();
                                        item["alignment_rate"] = row["alignment_rate"].isNull() ? 0.0 : row["alignment_rate"].as<double>();
                                        item["coverage_rate"] = row["coverage_rate"].isNull() ? 0.0 : row["coverage_rate"].as<double>();
                                        item["confidence_level"] =
                                            row["confidence_level"].isNull() ? "none" : row["confidence_level"].as<std::string>();
                                        alignmentRows.append(item);
                                    }

                                    db->execSqlAsync(
                                        kGapSql,
                                        [db, callback, user, requestBody, insightRequest, scopeResolved, alignmentRows]
                                        (const drogon::orm::Result &gapResult) {
                                            Json::Value flatGapSkills(Json::arrayValue);
                                            const auto groupedGapRows = buildGroupedGapRows(gapResult, flatGapSkills);
                                            const auto snapshot =
                                                buildSnapshot(insightRequest, scopeResolved, alignmentRows, groupedGapRows, flatGapSkills);

                                            runAiInsightAgentAsync(
                                                requestBody,
                                                snapshot,
                                                [db, callback, user, requestBody, insightRequest, scopeResolved, snapshot]
                                                (const AiInsightAgentResult &agentResult) {
                                                    Json::Value body(Json::objectValue);
                                                    Json::Value data(Json::objectValue);
                                                    data["status"] = agentResult.ok ? "success" : "failed";
                                                    data["scope"] = scopeResolved.scope;
                                                    data["scope_label"] = scopeResolved.scopeLabel;
                                                    data["target_college_id"] = static_cast<Json::Int64>(scopeResolved.collegeId);
                                                    data["target_major_id"] = static_cast<Json::Int64>(scopeResolved.majorId);
                                                    data["profile_id"] = agentResult.profileId;
                                                    data["provider"] = agentResult.provider;
                                                    data["model"] = agentResult.model;
                                                    data["focus"] = insightRequest.focus;
                                                    data["tone"] = insightRequest.tone;
                                                    data["latency_ms"] = agentResult.latencyMs;
                                                    data["generated_at"] = nowLocalDateTime();
                                                    data["snapshot"] = snapshot;
                                                    data["retrieved_context"] = agentResult.retrievedContext;
                                                    data["insight"] = agentResult.insight;
                                                    data["error"] = agentResult.ok ? Json::Value("") : Json::Value(agentResult.error);
                                                    data["persisted"] = false;
                                                    body["data"] = data;

                                                    const auto snapshotText = jsonToCompactString(snapshot);
                                                    const auto contextText = jsonToCompactString(agentResult.retrievedContext);
                                                    const auto insightText = jsonToCompactString(agentResult.insight);
                                                    const std::string statusText = agentResult.ok ? "success" : "failed";
                                                    const auto requestedProfile = jsonFieldAsString(requestBody, "profile_id");
                                                    const std::string profileId = !agentResult.profileId.empty()
                                                                                      ? agentResult.profileId
                                                                                      : requestedProfile;

                                                    db->execSqlAsync(
                                                        "INSERT INTO dashboard_insight_runs ("
                                                        "request_scope, target_college_id, target_major_id, algorithm_version, "
                                                        "profile_id, provider, model, focus, tone, custom_prompt, status, latency_ms, "
                                                        "snapshot_json, retrieved_context_json, insight_json, prompt_text, raw_response_text, error_message, created_by_user_id"
                                                        ") VALUES ("
                                                        "?, NULLIF(?, 0), NULLIF(?, 0), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                                                        [callback, body, agentResult]
                                                        (const drogon::orm::Result &insertResult) mutable {
                                                            auto mutableBody = body;
                                                            auto &dataRef = mutableBody["data"];
                                                            dataRef["persisted"] = true;
                                                            dataRef["run_id"] = static_cast<Json::Int64>(insertResult.insertId());
                                                            const auto statusCode =
                                                                agentResult.ok ? drogon::k201Created : agentResult.status;
                                                            callback(makeJson(mutableBody, statusCode));
                                                        },
                                                        [callback, body, agentResult](const DrogonDbException &e) mutable {
                                                            auto mutableBody = body;
                                                            mutableBody["data"]["persist_warning"] =
                                                                "insight generated but failed to persist run: " + std::string(e.base().what());
                                                            const auto statusCode =
                                                                agentResult.ok ? drogon::k200OK : agentResult.status;
                                                            callback(makeJson(mutableBody, statusCode));
                                                        },
                                                        scopeResolved.scope,
                                                        scopeResolved.collegeId,
                                                        scopeResolved.majorId,
                                                        insightRequest.algorithmVersion,
                                                        profileId,
                                                        agentResult.provider,
                                                        agentResult.model,
                                                        insightRequest.focus,
                                                        insightRequest.tone,
                                                        insightRequest.customPrompt,
                                                        statusText,
                                                        agentResult.latencyMs,
                                                        snapshotText,
                                                        contextText,
                                                        insightText,
                                                        agentResult.promptText,
                                                        agentResult.rawResponseText,
                                                        agentResult.error,
                                                        user.userId);
                                                });
                                        },
                                        [callback](const DrogonDbException &e) {
                                            sendDbError(e, callback);
                                        },
                                        scopeResolved.collegeId,
                                        scopeResolved.collegeId,
                                        scopeResolved.majorId,
                                        scopeResolved.majorId,
                                        insightRequest.algorithmVersion,
                                        insightRequest.algorithmVersion,
                                        insightRequest.minGapCount,
                                        insightRequest.topN);
                                },
                                [callback](const DrogonDbException &e) {
                                    sendDbError(e, callback);
                                },
                                scopeResolved.collegeId,
                                scopeResolved.collegeId,
                                scopeResolved.majorId,
                                scopeResolved.majorId,
                                insightRequest.algorithmVersion,
                                insightRequest.algorithmVersion,
                                insightRequest.alignmentThreshold,
                                insightRequest.minStudents,
                                insightRequest.smoothingK,
                                insightRequest.smoothingK,
                                insightRequest.smoothingK);
                        });
                });
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/dashboard/insight/runs",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            SessionUser user;
            if (!requireAuthUser(req, callback, user)) {
                return;
            }

            int page = 1;
            int pageSize = 20;
            if (!parsePagingParams(req, callback, page, pageSize)) {
                return;
            }
            const int offset = (page - 1) * pageSize;

            const auto scope = toLowerAscii(trimCopy(req->getParameter("scope")));
            const auto status = toLowerAscii(trimCopy(req->getParameter("status")));
            const auto profileId = trimCopy(req->getParameter("profile_id"));
            const auto focus = toLowerAscii(trimCopy(req->getParameter("focus")));
            const auto keyword = trimCopy(req->getParameter("keyword"));
            const auto sortOrder = normalizeSortOrder(req->getParameter("sort_order"));
            const long long ownerUserId = user.role == "admin" ? 0 : user.userId;

            bool includePayload = true;
            const auto includePayloadRaw = trimCopy(req->getParameter("include_payload"));
            if (!includePayloadRaw.empty() && !parseBoolLike(includePayloadRaw, includePayload)) {
                sendError(callback, drogon::k400BadRequest, "invalid query param: include_payload");
                return;
            }

            static const std::unordered_set<std::string> kScopes = {"", "all", "college", "major"};
            if (!validateEnumValue("scope", scope, kScopes, callback)) {
                return;
            }
            static const std::unordered_set<std::string> kStatus = {"", "success", "failed"};
            if (!validateEnumValue("status", status, kStatus, callback)) {
                return;
            }
            static const std::unordered_set<std::string> kFocus = {"", "balanced", "alignment", "gap", "risk", "employment"};
            if (!validateEnumValue("focus", focus, kFocus, callback)) {
                return;
            }

            const auto keywordLike = keyword.empty() ? std::string("") : "%" + keyword + "%";

            const std::string baseFromSql =
                " FROM dashboard_insight_runs r "
                "LEFT JOIN colleges c ON c.id = r.target_college_id "
                "LEFT JOIN majors m ON m.id = r.target_major_id "
                "WHERE (? = '' OR r.request_scope = ?) "
                "  AND (? = '' OR r.status = ?) "
                "  AND (? = '' OR r.profile_id = ?) "
                "  AND (? = '' OR r.focus = ?) "
                "  AND (? = 0 OR r.created_by_user_id = ?) "
                "  AND (? = '' "
                "       OR JSON_UNQUOTE(JSON_EXTRACT(r.insight_json, '$.summary')) LIKE ? "
                "       OR r.error_message LIKE ? "
                "       OR r.custom_prompt LIKE ?) ";

            ensureInsightRunsTable(
                db,
                callback,
                [db, callback, baseFromSql, scope, status, profileId, focus, keywordLike, ownerUserId, sortOrder, page, pageSize, offset, includePayload]() {
                    db->execSqlAsync(
                        "SELECT COUNT(*) AS total " + baseFromSql,
                        [db, callback, baseFromSql, scope, status, profileId, focus, keywordLike, ownerUserId, sortOrder, page, pageSize, offset, includePayload]
                        (const drogon::orm::Result &countResult) {
                            const auto total = static_cast<Json::UInt64>(countResult[0]["total"].as<unsigned long long>());
                            db->execSqlAsync(
                                "SELECT "
                                "r.id, r.request_scope, r.target_college_id, r.target_major_id, "
                                "r.algorithm_version, r.profile_id, r.provider, r.model, "
                                "r.focus, r.tone, r.status, r.latency_ms, r.snapshot_json, r.retrieved_context_json, "
                                "r.insight_json, r.prompt_text, r.raw_response_text, r.error_message, r.created_by_user_id, r.created_at, "
                                "c.name AS college_name, m.name AS major_name "
                                + baseFromSql +
                                "ORDER BY r.created_at " + sortOrder + ", r.id " + sortOrder + " LIMIT ? OFFSET ?",
                                [callback, total, page, pageSize, includePayload](const drogon::orm::Result &result) {
                                    Json::Value rows(Json::arrayValue);
                                    for (const auto &row : result) {
                                        rows.append(runRowToJson(row, includePayload));
                                    }
                                    Json::Value body(Json::objectValue);
                                    body["data"] = rows;
                                    appendPagedMeta(body, total, page, pageSize);
                                    callback(makeJson(body));
                                },
                                [callback](const DrogonDbException &e) {
                                    sendDbError(e, callback);
                                },
                                scope,
                                scope,
                                status,
                                status,
                                profileId,
                                profileId,
                                focus,
                                focus,
                                ownerUserId,
                                ownerUserId,
                                keywordLike,
                                keywordLike,
                                keywordLike,
                                keywordLike,
                                pageSize,
                                offset);
                        },
                        [callback](const DrogonDbException &e) {
                            sendDbError(e, callback);
                        },
                        scope,
                        scope,
                        status,
                        status,
                        profileId,
                        profileId,
                        focus,
                        focus,
                        ownerUserId,
                        ownerUserId,
                        keywordLike,
                        keywordLike,
                        keywordLike,
                        keywordLike);
                });
        },
        {drogon::Get});
}

}  // namespace gm
