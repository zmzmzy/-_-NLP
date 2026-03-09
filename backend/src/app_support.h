#pragma once

#include <drogon/drogon.h>

#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gm {

using HttpRequestPtr = drogon::HttpRequestPtr;
using HttpResponsePtr = drogon::HttpResponsePtr;
using DrogonDbException = drogon::orm::DrogonDbException;
using DbCallback = std::function<void(const HttpResponsePtr &)>;

struct WeightedSkill {
    long long skillId{0};
    std::string name;
    double weight{1.0};
    bool mandatory{false};
};

struct SkillDictionaryEntry {
    long long skillId{0};
    std::string name;
    std::vector<std::string> aliases;
};

struct MatchComputationResult {
    bool ok{false};
    long long matchResultId{0};
    long long studentId{0};
    long long majorId{0};
    long long jobId{0};
    std::string algorithmVersion;
    double semanticScore{0.0};
    double skillScore{0.0};
    double constraintScore{1.0};
    double finalScore{0.0};
    Json::Value matchedSkills{Json::arrayValue};
    Json::Value missingSkills{Json::arrayValue};
    Json::Value constraintFailures{Json::arrayValue};
    std::string message;
    drogon::HttpStatusCode status{drogon::k200OK};
};

struct SessionUser {
    long long userId{0};
    std::string username;
    std::string role;
    long long expiresAtEpoch{0};
};

HttpResponsePtr makeJson(const Json::Value &body,
                         drogon::HttpStatusCode status = drogon::k200OK);
void sendError(const DbCallback &callback,
               drogon::HttpStatusCode status,
               const std::string &message,
               const std::string &errorCode = "api_error");
void sendDbError(const DrogonDbException &e, const DbCallback &callback);

bool validateEnumValue(const std::string &field,
                       const std::string &value,
                       const std::unordered_set<std::string> &allowedValues,
                       const DbCallback &callback);
bool parseStrictInt64(const std::string &text, long long &value);
bool parseStrictInt(const std::string &text, int &value);
bool parseStrictDouble(const std::string &text, double &value);

bool parsePagingParams(const HttpRequestPtr &req,
                       const DbCallback &callback,
                       int &page,
                       int &pageSize,
                       int defaultPage = 1,
                       int defaultPageSize = 20,
                       int maxPageSize = 100);
bool parseOptionalInt64Param(const HttpRequestPtr &req,
                             const std::string &name,
                             const DbCallback &callback,
                             long long &value);

std::string normalizeSortOrder(const std::string &raw);
std::string resolveSortField(const std::string &input,
                             const std::unordered_map<std::string, std::string> &fieldMap,
                             const std::string &defaultExpr);
void appendPagedMeta(Json::Value &body, Json::UInt64 count, int page, int pageSize);

bool requireJsonBody(const HttpRequestPtr &req,
                     const DbCallback &callback,
                     const Json::Value *&jsonPtr);
bool requireFields(const Json::Value &json,
                   const std::vector<std::string> &fields,
                   const DbCallback &callback);

Json::Value collegeRowToJson(const drogon::orm::Row &row);
Json::Value majorRowToJson(const drogon::orm::Row &row);
Json::Value majorProfileRowToJson(const drogon::orm::Row &row);
Json::Value companyRowToJson(const drogon::orm::Row &row);
Json::Value jobRowToJson(const drogon::orm::Row &row);
Json::Value majorImportBatchRowToJson(const drogon::orm::Row &row);
Json::Value studentRowToJson(const drogon::orm::Row &row);
Json::Value employmentRowToJson(const drogon::orm::Row &row);
Json::Value matchResultRowToJson(const drogon::orm::Row &row);
Json::Value majorJobMatchResultRowToJson(const drogon::orm::Row &row);
Json::Value systemLogRowToJson(const drogon::orm::Row &row);

std::string trimCopy(const std::string &text);
std::string toLowerAscii(const std::string &text);
std::string normalizeAlgorithmVersion(const std::string &raw);
bool equalsIgnoreCaseTrimmed(const std::string &left, const std::string &right);
std::string sha256Hex(const std::string &text);
bool verifyPassword(const std::string &inputPassword, const std::string &storedPasswordHash);

std::string extractAuthToken(const HttpRequestPtr &req);
std::string createSession(long long userId,
                          const std::string &username,
                          const std::string &role,
                          long long &expiresAtEpoch);
bool getSessionByToken(const std::string &token, SessionUser &outUser, bool slidingTtl);
bool getSessionFromRequest(const HttpRequestPtr &req, SessionUser &outUser, bool slidingTtl = true);
bool revokeSessionByToken(const std::string &token);
void revokeSessionsByUserId(long long userId);
bool requireAuthUser(const HttpRequestPtr &req,
                     const DbCallback &callback,
                     SessionUser &outUser,
                     bool requireAdmin = false);
bool isPublicApiRoute(const HttpRequestPtr &req);

bool isSafeIdentifier(const std::string &text, size_t maxLen = 128);
bool readJsonFromFile(const std::filesystem::path &path, Json::Value &out, std::string &error);
bool writeJsonToFile(const std::filesystem::path &path, const Json::Value &value, std::string &error);
std::string getEnvTrimmed(const char *key);
std::filesystem::path resolveBackendConfigPath();
void applyDbConfigOverrides(Json::Value &config);

std::string normalizeJobTypeValue(const std::string &raw);
std::string normalizeDegreeValue(const std::string &raw);
bool parseBoolLike(const std::string &raw, bool &out);
std::string normalizeDateTimeValue(const std::string &raw);
std::string toUpperAscii(const std::string &text);

std::string normalizeImportCode(const std::string &raw, size_t maxLen = 64);
std::string normalizeImportText(const std::string &raw, size_t maxLen = 1024);
std::vector<std::string> parseCsvLineValues(const std::string &line);
std::string normalizeImportHeader(const std::string &raw);
bool parseCsvRowsFromText(const std::string &csvText,
                          std::vector<Json::Value> &rows,
                          std::string &error);
std::string jsonValueAsString(const Json::Value &obj,
                              const std::string &key,
                              const std::string &fallbackKey = "");
bool normalizeMajorImportRow(const Json::Value &rawRow,
                             Json::Value &normalizedRow,
                             std::string &error);

std::vector<std::string> splitPathSegments(const std::string &path);
void parseTargetByPath(const std::string &path,
                       std::string &targetType,
                       long long &targetIdOrZero);
void writeSystemLogAsync(const HttpRequestPtr &req, const HttpResponsePtr &resp);

std::unordered_set<std::string> buildTokenSet(const std::string &text);
std::string jsonToCompactString(const Json::Value &value);
Json::Value parseJsonTextOrDefault(const std::string &text, Json::ValueType expectedType);
std::vector<std::string> splitAliases(const std::string &text);

std::vector<SkillDictionaryEntry> loadSkillDictionary(const drogon::orm::DbClientPtr &db);
std::vector<WeightedSkill> loadMajorSkills(const drogon::orm::DbClientPtr &db, long long majorId);
std::vector<WeightedSkill> loadJobSkills(const drogon::orm::DbClientPtr &db, long long jobId);
std::unordered_map<std::string, WeightedSkill> toSkillMap(const std::vector<WeightedSkill> &skills);
std::unordered_map<std::string, WeightedSkill> inferSkillMapFromText(
    const std::vector<SkillDictionaryEntry> &dictionary,
    const std::string &text);

double computeSkillScore(
    const std::unordered_map<std::string, WeightedSkill> &majorSkills,
    const std::unordered_map<std::string, WeightedSkill> &jobSkills,
    Json::Value &matchedSkills,
    Json::Value &missingSkills,
    int &missingMandatoryCount);
MatchComputationResult computeBaselineMatch(const drogon::orm::DbClientPtr &db,
                                            long long studentId,
                                            long long jobId,
                                            const std::string &algorithmVersionRaw,
                                            const std::string &preferredProvince,
                                            const std::string &preferredCity);
MatchComputationResult computeMajorJobMatch(const drogon::orm::DbClientPtr &db,
                                            long long majorId,
                                            long long jobId,
                                            const std::string &algorithmVersionRaw);
bool persistMajorJobMatchResult(const drogon::orm::DbClientPtr &db,
                                const MatchComputationResult &result,
                                long long createdByUserId,
                                const std::string &runMode,
                                long long &resultId,
                                std::string &errorMessage);
bool persistMatchResult(const drogon::orm::DbClientPtr &db,
                        MatchComputationResult &result,
                        std::string &errorMessage);

}  // namespace gm
