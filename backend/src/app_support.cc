#include <drogon/drogon.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <openssl/sha.h>

using drogon::HttpRequestPtr;
using drogon::HttpResponse;
using drogon::HttpResponsePtr;
using drogon::orm::DrogonDbException;
using DbCallback = std::function<void(const HttpResponsePtr &)>;

namespace gm {

std::string jsonToCompactString(const Json::Value &value);

HttpResponsePtr makeJson(const Json::Value &body,
                         drogon::HttpStatusCode status = drogon::k200OK) {
    auto resp = HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(status);
    return resp;
}

void sendError(const DbCallback &callback,
               drogon::HttpStatusCode status,
               const std::string &message,
               const std::string &errorCode = "api_error") {
    Json::Value body;
    body["success"] = false;
    body["error"] = message;
    body["error_code"] = errorCode;
    Json::Value detail;
    detail["code"] = errorCode;
    detail["message"] = message;
    body["error_detail"] = detail;
    callback(makeJson(body, status));
}

void sendDbError(const DrogonDbException &e, const DbCallback &callback) {
    LOG_ERROR << "Database error: " << e.base().what();
    sendError(callback, drogon::k500InternalServerError, "database operation failed", "database_error");
}

bool validateEnumValue(const std::string &field,
                       const std::string &value,
                       const std::unordered_set<std::string> &allowedValues,
                       const DbCallback &callback) {
    if (allowedValues.find(value) != allowedValues.end()) {
        return true;
    }
    sendError(callback,
              drogon::k400BadRequest,
              "invalid enum value for field: " + field,
              "validation_error");
    return false;
}

bool parseStrictInt64(const std::string &text, long long &value) {
    if (text.empty()) {
        return false;
    }
    try {
        size_t pos = 0;
        const auto parsed = std::stoll(text, &pos);
        if (pos != text.size()) {
            return false;
        }
        value = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

bool parseStrictInt(const std::string &text, int &value) {
    long long tmp = 0;
    if (!parseStrictInt64(text, tmp)) {
        return false;
    }
    if (tmp < std::numeric_limits<int>::min() || tmp > std::numeric_limits<int>::max()) {
        return false;
    }
    value = static_cast<int>(tmp);
    return true;
}

bool parseStrictDouble(const std::string &text, double &value) {
    if (text.empty()) {
        return false;
    }
    try {
        size_t pos = 0;
        const auto parsed = std::stod(text, &pos);
        if (pos != text.size()) {
            return false;
        }
        value = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

bool parsePagingParams(const HttpRequestPtr &req,
                       const DbCallback &callback,
                       int &page,
                       int &pageSize,
                       int defaultPage = 1,
                       int defaultPageSize = 20,
                       int maxPageSize = 100) {
    page = defaultPage;
    pageSize = defaultPageSize;

    const auto pageParam = req->getParameter("page");
    if (!pageParam.empty()) {
        int parsed = 0;
        if (!parseStrictInt(pageParam, parsed) || parsed <= 0) {
            sendError(callback, drogon::k400BadRequest, "invalid query param: page");
            return false;
        }
        page = parsed;
    }

    const auto pageSizeParam = req->getParameter("page_size");
    if (!pageSizeParam.empty()) {
        int parsed = 0;
        if (!parseStrictInt(pageSizeParam, parsed) || parsed <= 0) {
            sendError(callback, drogon::k400BadRequest, "invalid query param: page_size");
            return false;
        }
        pageSize = std::min(parsed, maxPageSize);
    }
    return true;
}

bool parseOptionalInt64Param(const HttpRequestPtr &req,
                             const std::string &name,
                             const DbCallback &callback,
                             long long &value) {
    value = 0;
    const auto text = req->getParameter(name);
    if (text.empty()) {
        return true;
    }
    long long parsed = 0;
    if (!parseStrictInt64(text, parsed)) {
        sendError(callback, drogon::k400BadRequest, "invalid query param: " + name);
        return false;
    }
    value = parsed;
    return true;
}

std::string normalizeSortOrder(const std::string &raw) {
    if (raw.empty()) {
        return "DESC";
    }
    std::string order = raw;
    std::transform(order.begin(), order.end(), order.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return order == "asc" ? "ASC" : "DESC";
}

std::string resolveSortField(const std::string &input,
                             const std::unordered_map<std::string, std::string> &fieldMap,
                             const std::string &defaultExpr) {
    auto it = fieldMap.find(input);
    if (it == fieldMap.end()) {
        return defaultExpr;
    }
    return it->second;
}

void appendPagedMeta(Json::Value &body, Json::UInt64 count, int page, int pageSize) {
    body["count"] = count;
    body["page"] = page;
    body["page_size"] = pageSize;
    const Json::UInt64 pages = pageSize <= 0 ? 0 : (count + static_cast<Json::UInt64>(pageSize) - 1) /
                                                   static_cast<Json::UInt64>(pageSize);
    body["total_pages"] = pages;
}

bool requireJsonBody(const HttpRequestPtr &req,
                     const DbCallback &callback,
                     const Json::Value *&jsonPtr) {
    const auto parsed = req->getJsonObject();
    if (!parsed) {
        sendError(callback, drogon::k400BadRequest, "invalid or missing JSON body");
        return false;
    }
    jsonPtr = parsed.get();
    return true;
}

bool requireFields(const Json::Value &json,
                   const std::vector<std::string> &fields,
                   const DbCallback &callback) {
    for (const auto &f : fields) {
        if (!json.isMember(f) || json[f].isNull()) {
            sendError(callback, drogon::k400BadRequest, "missing field: " + f);
            return false;
        }
    }
    return true;
}

Json::Value collegeRowToJson(const drogon::orm::Row &row) {
    Json::Value item;
    item["id"] = static_cast<Json::Int64>(row["id"].as<long long>());
    item["code"] = row["code"].as<std::string>();
    item["name"] = row["name"].as<std::string>();
    item["description"] = row["description"].isNull() ? "" : row["description"].as<std::string>();
    item["created_at"] = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
    item["updated_at"] = row["updated_at"].isNull() ? "" : row["updated_at"].as<std::string>();
    return item;
}

Json::Value majorRowToJson(const drogon::orm::Row &row) {
    Json::Value item;
    item["id"] = static_cast<Json::Int64>(row["id"].as<long long>());
    item["college_id"] = static_cast<Json::Int64>(row["college_id"].as<long long>());
    item["college_name"] = row["college_name"].isNull() ? "" : row["college_name"].as<std::string>();
    item["code"] = row["code"].as<std::string>();
    item["name"] = row["name"].as<std::string>();
    item["description"] = row["description"].isNull() ? "" : row["description"].as<std::string>();
    item["created_at"] = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
    item["updated_at"] = row["updated_at"].isNull() ? "" : row["updated_at"].as<std::string>();
    return item;
}

Json::Value majorProfileRowToJson(const drogon::orm::Row &row) {
    Json::Value item;
    item["id"] = static_cast<Json::Int64>(row["id"].as<long long>());
    item["major_id"] = static_cast<Json::Int64>(row["major_id"].as<long long>());
    item["major_name"] = row["major_name"].isNull() ? "" : row["major_name"].as<std::string>();
    item["core_courses_text"] = row["core_courses_text"].isNull() ? "" : row["core_courses_text"].as<std::string>();
    item["knowledge_skills_text"] = row["knowledge_skills_text"].isNull() ? "" : row["knowledge_skills_text"].as<std::string>();
    item["technical_platform_text"] = row["technical_platform_text"].isNull() ? "" : row["technical_platform_text"].as<std::string>();
    item["capability_text"] = row["capability_text"].isNull() ? "" : row["capability_text"].as<std::string>();
    item["updated_by_user_id"] = row["updated_by_user_id"].isNull()
                                     ? static_cast<Json::Int64>(0)
                                     : static_cast<Json::Int64>(row["updated_by_user_id"].as<long long>());
    item["updated_at"] = row["updated_at"].isNull() ? "" : row["updated_at"].as<std::string>();
    return item;
}

Json::Value companyRowToJson(const drogon::orm::Row &row) {
    Json::Value item;
    item["id"] = static_cast<Json::Int64>(row["id"].as<long long>());
    item["name"] = row["name"].as<std::string>();
    item["main_business"] = row["main_business"].isNull() ? "" : row["main_business"].as<std::string>();
    item["flagship_products"] = row["flagship_products"].isNull() ? "" : row["flagship_products"].as<std::string>();
    item["domain_area"] = row["domain_area"].isNull() ? "" : row["domain_area"].as<std::string>();
    item["application_industries"] = row["application_industries"].isNull() ? "" : row["application_industries"].as<std::string>();
    item["province"] = row["province"].isNull() ? "" : row["province"].as<std::string>();
    item["city"] = row["city"].isNull() ? "" : row["city"].as<std::string>();
    item["website"] = row["website"].isNull() ? "" : row["website"].as<std::string>();
    item["description"] = row["description"].isNull() ? "" : row["description"].as<std::string>();
    item["created_at"] = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
    item["updated_at"] = row["updated_at"].isNull() ? "" : row["updated_at"].as<std::string>();
    return item;
}

Json::Value jobRowToJson(const drogon::orm::Row &row) {
    Json::Value item;
    item["id"] = static_cast<Json::Int64>(row["id"].as<long long>());
    item["company_id"] = static_cast<Json::Int64>(row["company_id"].as<long long>());
    item["company_name"] = row["company_name"].isNull() ? "" : row["company_name"].as<std::string>();
    item["title"] = row["title"].as<std::string>();
    item["job_type"] = row["job_type"].isNull() ? "" : row["job_type"].as<std::string>();
    item["location_province"] = row["location_province"].isNull() ? "" : row["location_province"].as<std::string>();
    item["location_city"] = row["location_city"].isNull() ? "" : row["location_city"].as<std::string>();
    item["min_degree"] = row["min_degree"].isNull() ? "" : row["min_degree"].as<std::string>();
    item["salary_range"] = row["salary_range"].isNull() ? "" : row["salary_range"].as<std::string>();
    item["responsibilities_text"] = row["responsibilities_text"].isNull() ? "" : row["responsibilities_text"].as<std::string>();
    item["technical_projects_text"] = row["technical_projects_text"].isNull() ? "" : row["technical_projects_text"].as<std::string>();
    item["required_knowledge_skills_text"] = row["required_knowledge_skills_text"].isNull() ? "" : row["required_knowledge_skills_text"].as<std::string>();
    item["required_tools_platform_text"] = row["required_tools_platform_text"].isNull() ? "" : row["required_tools_platform_text"].as<std::string>();
    item["published_at"] = row["published_at"].isNull() ? "" : row["published_at"].as<std::string>();
    item["created_at"] = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
    item["updated_at"] = row["updated_at"].isNull() ? "" : row["updated_at"].as<std::string>();
    return item;
}

Json::Value majorImportBatchRowToJson(const drogon::orm::Row &row) {
    Json::Value item;
    item["id"] = static_cast<Json::Int64>(row["id"].as<long long>());
    item["source_type"] = row["source_type"].isNull() ? "" : row["source_type"].as<std::string>();
    item["source_filename"] = row["source_filename"].isNull() ? "" : row["source_filename"].as<std::string>();
    item["status"] = row["status"].isNull() ? "" : row["status"].as<std::string>();
    item["triggered_by_user_id"] = row["triggered_by_user_id"].isNull()
                                       ? static_cast<Json::Int64>(0)
                                       : static_cast<Json::Int64>(row["triggered_by_user_id"].as<long long>());
    item["triggered_by_username"] =
        row["triggered_by_username"].isNull() ? "" : row["triggered_by_username"].as<std::string>();
    item["total_rows"] = row["total_rows"].isNull() ? 0 : row["total_rows"].as<int>();
    item["valid_rows"] = row["valid_rows"].isNull() ? 0 : row["valid_rows"].as<int>();
    item["invalid_rows"] = row["invalid_rows"].isNull() ? 0 : row["invalid_rows"].as<int>();
    item["inserted_rows"] = row["inserted_rows"].isNull() ? 0 : row["inserted_rows"].as<int>();
    item["updated_rows"] = row["updated_rows"].isNull() ? 0 : row["updated_rows"].as<int>();
    item["skipped_rows"] = row["skipped_rows"].isNull() ? 0 : row["skipped_rows"].as<int>();
    item["error_rows"] = row["error_rows"].isNull() ? 0 : row["error_rows"].as<int>();
    item["started_at"] = row["started_at"].isNull() ? "" : row["started_at"].as<std::string>();
    item["finished_at"] = row["finished_at"].isNull() ? "" : row["finished_at"].as<std::string>();
    item["message"] = row["message"].isNull() ? "" : row["message"].as<std::string>();
    item["created_at"] = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
    return item;
}

Json::Value studentRowToJson(const drogon::orm::Row &row) {
    Json::Value item;
    item["id"] = static_cast<Json::Int64>(row["id"].as<long long>());
    item["student_no"] = row["student_no"].as<std::string>();
    item["full_name"] = row["full_name"].as<std::string>();
    item["gender"] = row["gender"].isNull() ? "unknown" : row["gender"].as<std::string>();
    item["college_id"] = static_cast<Json::Int64>(row["college_id"].as<long long>());
    item["college_name"] = row["college_name"].isNull() ? "" : row["college_name"].as<std::string>();
    item["major_id"] = static_cast<Json::Int64>(row["major_id"].as<long long>());
    item["major_name"] = row["major_name"].isNull() ? "" : row["major_name"].as<std::string>();
    item["graduation_year"] = row["graduation_year"].isNull() ? 0 : row["graduation_year"].as<int>();
    item["email"] = row["email"].isNull() ? "" : row["email"].as<std::string>();
    item["phone"] = row["phone"].isNull() ? "" : row["phone"].as<std::string>();
    item["resume_text"] = row["resume_text"].isNull() ? "" : row["resume_text"].as<std::string>();
    item["technical_projects_text"] =
        row["technical_projects_text"].isNull() ? "" : row["technical_projects_text"].as<std::string>();
    item["capability_summary_text"] =
        row["capability_summary_text"].isNull() ? "" : row["capability_summary_text"].as<std::string>();
    item["created_at"] = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
    item["updated_at"] = row["updated_at"].isNull() ? "" : row["updated_at"].as<std::string>();
    return item;
}

Json::Value employmentRowToJson(const drogon::orm::Row &row) {
    Json::Value item;
    item["id"] = static_cast<Json::Int64>(row["id"].as<long long>());
    item["student_id"] = static_cast<Json::Int64>(row["student_id"].as<long long>());
    item["student_name"] = row["student_name"].isNull() ? "" : row["student_name"].as<std::string>();
    item["company_id"] = static_cast<Json::Int64>(row["company_id"].as<long long>());
    item["company_name"] = row["company_name"].isNull() ? "" : row["company_name"].as<std::string>();
    item["job_id"] = row["job_id"].isNull() ? static_cast<Json::Int64>(0)
                                            : static_cast<Json::Int64>(row["job_id"].as<long long>());
    item["job_title"] = row["job_title"].isNull() ? "" : row["job_title"].as<std::string>();
    item["employment_type"] = row["employment_type"].isNull() ? "" : row["employment_type"].as<std::string>();
    item["employment_status"] = row["employment_status"].isNull() ? "" : row["employment_status"].as<std::string>();
    item["start_date"] = row["start_date"].isNull() ? "" : row["start_date"].as<std::string>();
    item["end_date"] = row["end_date"].isNull() ? "" : row["end_date"].as<std::string>();
    item["evidence_source"] = row["evidence_source"].isNull() ? "" : row["evidence_source"].as<std::string>();
    item["notes"] = row["notes"].isNull() ? "" : row["notes"].as<std::string>();
    item["created_at"] = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
    item["updated_at"] = row["updated_at"].isNull() ? "" : row["updated_at"].as<std::string>();
    return item;
}

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

double clampValue(double value, double minValue, double maxValue) {
    return std::max(minValue, std::min(maxValue, value));
}

double roundTo3(double value) {
    return std::round(value * 1000.0) / 1000.0;
}

std::string trimCopy(const std::string &text) {
    const auto begin = std::find_if_not(text.begin(), text.end(), [](unsigned char c) { return std::isspace(c); });
    if (begin == text.end()) {
        return "";
    }
    const auto end = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char c) { return std::isspace(c); }).base();
    return std::string(begin, end);
}

std::string toLowerAscii(const std::string &text) {
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        if (c < 128) {
            return static_cast<char>(std::tolower(c));
        }
        return static_cast<char>(c);
    });
    return out;
}

std::string normalizeAlgorithmVersion(const std::string &raw) {
    std::string version = trimCopy(raw);
    if (version.empty()) {
        version = "v0.1-baseline";
    }
    if (version.size() > 64) {
        version.resize(64);
    }
    return version;
}

bool equalsIgnoreCaseTrimmed(const std::string &left, const std::string &right) {
    return toLowerAscii(trimCopy(left)) == toLowerAscii(trimCopy(right));
}

struct SessionUser {
    long long userId{0};
    std::string username;
    std::string role;
    long long expiresAtEpoch{0};
};

std::mutex gSessionMutex;
std::unordered_map<std::string, SessionUser> gSessionStore;
constexpr long long kSessionTtlSeconds = 8 * 60 * 60;

long long nowUnixSeconds() {
    const auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}

void pruneExpiredSessionsLocked(long long now) {
    for (auto it = gSessionStore.begin(); it != gSessionStore.end();) {
        if (it->second.expiresAtEpoch <= now) {
            it = gSessionStore.erase(it);
        } else {
            ++it;
        }
    }
}

bool hasPrefixIgnoreCase(const std::string &text, const std::string &prefix) {
    if (text.size() < prefix.size()) {
        return false;
    }
    for (size_t i = 0; i < prefix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(text[i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i]))) {
            return false;
        }
    }
    return true;
}

std::string sha256Hex(const std::string &text) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(text.data()), text.size(), digest);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char c : digest) {
        oss << std::setw(2) << static_cast<int>(c);
    }
    return oss.str();
}

bool verifyPassword(const std::string &inputPassword, const std::string &storedPasswordHash) {
    const auto stored = trimCopy(storedPasswordHash);
    if (hasPrefixIgnoreCase(stored, "sha256:")) {
        return sha256Hex(inputPassword) == stored.substr(7);
    }
    if (hasPrefixIgnoreCase(stored, "plain:")) {
        return inputPassword == stored.substr(6);
    }
    return inputPassword == stored;
}

std::string extractAuthToken(const HttpRequestPtr &req) {
    std::string auth = req->getHeader("Authorization");
    if (auth.empty()) {
        auth = req->getHeader("authorization");
    }
    if (!auth.empty() && hasPrefixIgnoreCase(auth, "Bearer ")) {
        return trimCopy(auth.substr(7));
    }

    std::string token = req->getHeader("X-Auth-Token");
    if (token.empty()) {
        token = req->getHeader("x-auth-token");
    }
    return trimCopy(token);
}

std::string createSession(long long userId,
                          const std::string &username,
                          const std::string &role,
                          long long &expiresAtEpoch) {
    std::lock_guard<std::mutex> lock(gSessionMutex);
    const auto now = nowUnixSeconds();
    pruneExpiredSessionsLocked(now);

    SessionUser user;
    user.userId = userId;
    user.username = username;
    user.role = role;
    user.expiresAtEpoch = now + kSessionTtlSeconds;

    const std::string token = drogon::utils::getUuid() + drogon::utils::getUuid();
    gSessionStore[token] = user;
    expiresAtEpoch = user.expiresAtEpoch;
    return token;
}

bool getSessionByToken(const std::string &token, SessionUser &outUser, bool slidingTtl) {
    if (token.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(gSessionMutex);
    const auto now = nowUnixSeconds();
    pruneExpiredSessionsLocked(now);

    auto it = gSessionStore.find(token);
    if (it == gSessionStore.end()) {
        return false;
    }
    if (it->second.expiresAtEpoch <= now) {
        gSessionStore.erase(it);
        return false;
    }
    if (slidingTtl) {
        it->second.expiresAtEpoch = now + kSessionTtlSeconds;
    }
    outUser = it->second;
    return true;
}

bool getSessionFromRequest(const HttpRequestPtr &req, SessionUser &outUser, bool slidingTtl = true) {
    const auto token = extractAuthToken(req);
    return getSessionByToken(token, outUser, slidingTtl);
}

bool revokeSessionByToken(const std::string &token) {
    if (token.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(gSessionMutex);
    return gSessionStore.erase(token) > 0;
}

void revokeSessionsByUserId(long long userId) {
    if (userId <= 0) {
        return;
    }
    std::lock_guard<std::mutex> lock(gSessionMutex);
    for (auto it = gSessionStore.begin(); it != gSessionStore.end();) {
        if (it->second.userId == userId) {
            it = gSessionStore.erase(it);
        } else {
            ++it;
        }
    }
}

bool requireAuthUser(const HttpRequestPtr &req,
                     const DbCallback &callback,
                     SessionUser &outUser,
                     bool requireAdmin = false) {
    if (!getSessionFromRequest(req, outUser)) {
        sendError(callback, drogon::k401Unauthorized, "authentication required", "auth_required");
        return false;
    }
    if (requireAdmin && outUser.role != "admin") {
        sendError(callback, drogon::k403Forbidden, "admin privilege required", "forbidden");
        return false;
    }
    return true;
}

bool isPublicApiRoute(const HttpRequestPtr &req) {
    const auto path = req->path();
    const auto method = req->method();
    if (method == drogon::Options) {
        return true;
    }
    if (path == "/api/health" && method == drogon::Get) {
        return true;
    }
    if (path == "/api/auth/login" && method == drogon::Post) {
        return true;
    }
    if (path == "/api/auth/register" && method == drogon::Post) {
        return true;
    }
    if (path == "/api/auth/password/forgot" && method == drogon::Post) {
        return true;
    }
    if (path == "/api/auth/password/reset" && method == drogon::Post) {
        return true;
    }
    return false;
}

bool isSafeIdentifier(const std::string &text, size_t maxLen = 128) {
    if (text.empty() || text.size() > maxLen) {
        return false;
    }
    for (unsigned char c : text) {
        if (!(std::isalnum(c) || c == '_' || c == '-')) {
            return false;
        }
    }
    return true;
}

bool readJsonFromFile(const std::filesystem::path &path, Json::Value &out, std::string &error) {
    out = Json::Value();
    error.clear();
    std::ifstream in(path);
    if (!in.is_open()) {
        error = "failed to open file";
        return false;
    }
    Json::CharReaderBuilder builder;
    std::string errs;
    if (!Json::parseFromStream(builder, in, &out, &errs)) {
        error = errs.empty() ? "invalid json" : errs;
        return false;
    }
    return true;
}

bool writeJsonToFile(const std::filesystem::path &path, const Json::Value &value, std::string &error) {
    error.clear();
    std::ofstream out(path);
    if (!out.is_open()) {
        error = "failed to open file for writing";
        return false;
    }
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "  ";
    out << Json::writeString(writer, value);
    return true;
}

std::string getEnvTrimmed(const char *key) {
    if (key == nullptr) {
        return "";
    }
    const char *value = std::getenv(key);
    if (value == nullptr) {
        return "";
    }
    return trimCopy(value);
}

std::filesystem::path resolveBackendConfigPath() {
    const auto envPath = getEnvTrimmed("BACKEND_CONFIG_PATH");
    if (!envPath.empty()) {
        return std::filesystem::path(envPath);
    }
    return std::filesystem::path("../config/dev.json");
}

void applyDbConfigOverrides(Json::Value &config) {
    if (!config.isObject() || !config.isMember("db_clients") || !config["db_clients"].isArray()) {
        return;
    }

    const auto dbHost = getEnvTrimmed("GM_DB_HOST");
    const auto dbPort = getEnvTrimmed("GM_DB_PORT");
    const auto dbName = getEnvTrimmed("GM_DB_NAME");
    const auto dbUser = getEnvTrimmed("GM_DB_USER");
    const auto dbPass = getEnvTrimmed("GM_DB_PASS");

    for (Json::ArrayIndex i = 0; i < config["db_clients"].size(); ++i) {
        Json::Value &dbClient = config["db_clients"][i];
        if (!dbClient.isObject()) {
            continue;
        }
        if (!dbHost.empty()) {
            dbClient["host"] = dbHost;
        }
        if (!dbPort.empty()) {
            int parsedPort = 0;
            if (parseStrictInt(dbPort, parsedPort) && parsedPort > 0 && parsedPort <= 65535) {
                dbClient["port"] = parsedPort;
            }
        }
        if (!dbName.empty()) {
            dbClient["dbname"] = dbName;
        }
        if (!dbUser.empty()) {
            dbClient["user"] = dbUser;
        }
        if (!dbPass.empty()) {
            dbClient["passwd"] = dbPass;
        }
    }
}

std::string normalizeJobTypeValue(const std::string &raw) {
    const auto value = toLowerAscii(trimCopy(raw));
    if (value == "internship") {
        return "internship";
    }
    if (value == "contract") {
        return "contract";
    }
    return "full_time";
}

std::string normalizeDegreeValue(const std::string &raw) {
    const auto value = toLowerAscii(trimCopy(raw));
    if (value == "none" || value == "diploma" || value == "bachelor" || value == "master" || value == "phd") {
        return value;
    }
    if (value == "doctor") {
        return "phd";
    }
    if (value == "graduate") {
        return "master";
    }
    if (value == "college") {
        return "diploma";
    }
    if (value == "no_limit") {
        return "none";
    }
    return "bachelor";
}

bool parseBoolLike(const std::string &raw, bool &out) {
    const auto value = toLowerAscii(trimCopy(raw));
    if (value == "1" || value == "true" || value == "yes" || value == "y") {
        out = true;
        return true;
    }
    if (value == "0" || value == "false" || value == "no" || value == "n") {
        out = false;
        return true;
    }
    return false;
}

bool isAsciiDigits(const std::string &text, size_t offset, size_t len) {
    if (offset + len > text.size()) {
        return false;
    }
    for (size_t i = offset; i < offset + len; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) {
            return false;
        }
    }
    return true;
}

std::string normalizeDateTimeValue(const std::string &raw) {
    const auto value = trimCopy(raw);
    if (value.empty()) {
        return "";
    }
    if (value.size() < 10 ||
        !isAsciiDigits(value, 0, 4) || value[4] != '-' ||
        !isAsciiDigits(value, 5, 2) || value[7] != '-' ||
        !isAsciiDigits(value, 8, 2)) {
        return "";
    }

    std::string normalized = value.substr(0, 10) + " 00:00:00";
    if (value.size() == 10) {
        return normalized;
    }

    if (value.size() < 19) {
        return "";
    }
    if (!(value[10] == ' ' || value[10] == 'T')) {
        return "";
    }
    if (!isAsciiDigits(value, 11, 2) || value[13] != ':' ||
        !isAsciiDigits(value, 14, 2) || value[16] != ':' ||
        !isAsciiDigits(value, 17, 2)) {
        return "";
    }
    normalized = value.substr(0, 10);
    normalized.push_back(' ');
    normalized += value.substr(11, 8);
    return normalized;
}

std::string toUpperAscii(const std::string &text) {
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        if (c < 128) {
            return static_cast<char>(std::toupper(c));
        }
        return static_cast<char>(c);
    });
    return out;
}

std::string normalizeImportCode(const std::string &raw, size_t maxLen = 64) {
    auto value = toUpperAscii(trimCopy(raw));
    if (value.size() > maxLen) {
        value.resize(maxLen);
    }
    return value;
}

std::string normalizeImportText(const std::string &raw, size_t maxLen = 1024) {
    auto value = trimCopy(raw);
    if (value.size() > maxLen) {
        value.resize(maxLen);
    }
    return value;
}

std::vector<std::string> parseCsvLineValues(const std::string &line) {
    std::vector<std::string> values;
    std::string current;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (inQuotes) {
            if (ch == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current.push_back('"');
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                current.push_back(ch);
            }
            continue;
        }
        if (ch == '"') {
            inQuotes = true;
            continue;
        }
        if (ch == ',') {
            values.push_back(current);
            current.clear();
            continue;
        }
        current.push_back(ch);
    }
    values.push_back(current);
    return values;
}

std::string normalizeImportHeader(const std::string &raw) {
    std::string key = toLowerAscii(trimCopy(raw));
    for (char &c : key) {
        if (c == ' ' || c == '-' || c == '.') {
            c = '_';
        }
    }

    static const std::unordered_map<std::string, std::string> aliases = {
        {"university", "university_name"},
        {"universitycode", "university_code"},
        {"universityname", "university_name"},
        {"college", "college_name"},
        {"collegecode", "college_code"},
        {"collegename", "college_name"},
        {"major", "major_name"},
        {"majorcode", "major_code"},
        {"majorname", "major_name"},
        {"description", "major_description"},
        {"majordescription", "major_description"},
        {"core_courses", "core_courses_text"},
        {"knowledge_skills", "knowledge_skills_text"},
        {"technical_platform", "technical_platform_text"},
        {"capability", "capability_text"}};
    auto it = aliases.find(key);
    if (it != aliases.end()) {
        return it->second;
    }
    return key;
}

bool parseCsvRowsFromText(const std::string &csvText,
                          std::vector<Json::Value> &rows,
                          std::string &error) {
    rows.clear();
    error.clear();
    std::istringstream in(csvText);
    std::string line;
    std::vector<std::string> headers;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (lineNo == 1 && line.size() >= 3 &&
            static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF) {
            line = line.substr(3);
        }
        if (trimCopy(line).empty()) {
            continue;
        }

        if (headers.empty()) {
            headers = parseCsvLineValues(line);
            if (headers.empty()) {
                error = "empty CSV header";
                return false;
            }
            for (auto &h : headers) {
                h = normalizeImportHeader(h);
            }
            continue;
        }

        auto values = parseCsvLineValues(line);
        if (values.size() > headers.size()) {
            error = "CSV field count mismatch at line " + std::to_string(lineNo);
            return false;
        }
        while (values.size() < headers.size()) {
            values.push_back("");
        }

        Json::Value row(Json::objectValue);
        row["row_no"] = lineNo;
        for (size_t i = 0; i < headers.size(); ++i) {
            row[headers[i]] = trimCopy(values[i]);
        }
        rows.push_back(row);
    }

    if (headers.empty()) {
        error = "CSV header is required";
        return false;
    }
    return true;
}

std::string jsonValueAsString(const Json::Value &obj,
                              const std::string &key,
                              const std::string &fallbackKey = "") {
    if (obj.isMember(key) && !obj[key].isNull()) {
        if (obj[key].isString()) {
            return trimCopy(obj[key].asString());
        }
        if (obj[key].isIntegral()) {
            return std::to_string(obj[key].asInt64());
        }
        if (obj[key].isDouble()) {
            std::ostringstream oss;
            oss << obj[key].asDouble();
            return trimCopy(oss.str());
        }
    }
    if (!fallbackKey.empty() && obj.isMember(fallbackKey) && !obj[fallbackKey].isNull()) {
        if (obj[fallbackKey].isString()) {
            return trimCopy(obj[fallbackKey].asString());
        }
        if (obj[fallbackKey].isIntegral()) {
            return std::to_string(obj[fallbackKey].asInt64());
        }
    }
    return "";
}

bool normalizeMajorImportRow(const Json::Value &rawRow,
                             Json::Value &normalizedRow,
                             std::string &error) {
    normalizedRow = Json::Value(Json::objectValue);
    error.clear();

    const auto universityCode = normalizeImportCode(jsonValueAsString(rawRow, "university_code"), 64);
    const auto universityName = normalizeImportText(jsonValueAsString(rawRow, "university_name"), 255);
    const auto universityProvince = normalizeImportText(jsonValueAsString(rawRow, "university_province"), 64);
    const auto universityCity = normalizeImportText(jsonValueAsString(rawRow, "university_city"), 64);
    const auto collegeCode = normalizeImportCode(jsonValueAsString(rawRow, "college_code"), 64);
    const auto collegeName = normalizeImportText(jsonValueAsString(rawRow, "college_name"), 128);
    const auto majorCode = normalizeImportCode(jsonValueAsString(rawRow, "major_code"), 64);
    const auto majorName = normalizeImportText(jsonValueAsString(rawRow, "major_name"), 128);

    const auto majorDescription = normalizeImportText(
        jsonValueAsString(rawRow, "major_description", "description"), 2048);
    const auto coreCourses = normalizeImportText(jsonValueAsString(rawRow, "core_courses_text"), 65535);
    const auto knowledgeSkills = normalizeImportText(jsonValueAsString(rawRow, "knowledge_skills_text"), 65535);
    const auto technicalPlatform = normalizeImportText(jsonValueAsString(rawRow, "technical_platform_text"), 65535);
    const auto capability = normalizeImportText(jsonValueAsString(rawRow, "capability_text"), 65535);

    if (universityCode.empty() || universityName.empty() ||
        collegeCode.empty() || collegeName.empty() ||
        majorCode.empty() || majorName.empty()) {
        error = "required fields missing (university/college/major code or name)";
        return false;
    }
    if (!isSafeIdentifier(universityCode, 64) ||
        !isSafeIdentifier(collegeCode, 64) ||
        !isSafeIdentifier(majorCode, 64)) {
        error = "invalid code format (only letters, numbers, _ and - are allowed)";
        return false;
    }
    normalizedRow["university_code"] = universityCode;
    normalizedRow["university_name"] = universityName;
    normalizedRow["university_province"] = universityProvince;
    normalizedRow["university_city"] = universityCity;
    normalizedRow["college_code"] = collegeCode;
    normalizedRow["college_name"] = collegeName;
    normalizedRow["major_code"] = majorCode;
    normalizedRow["major_name"] = majorName;
    normalizedRow["major_description"] = majorDescription;
    normalizedRow["core_courses_text"] = coreCourses;
    normalizedRow["knowledge_skills_text"] = knowledgeSkills;
    normalizedRow["technical_platform_text"] = technicalPlatform;
    normalizedRow["capability_text"] = capability;
    return true;
}

std::vector<std::string> splitPathSegments(const std::string &path) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : path) {
        if (c == '/') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
            continue;
        }
        current.push_back(c);
    }
    if (!current.empty()) {
        parts.push_back(current);
    }
    return parts;
}

void parseTargetByPath(const std::string &path,
                       std::string &targetType,
                       long long &targetIdOrZero) {
    targetType = "unknown";
    targetIdOrZero = 0;
    const auto parts = splitPathSegments(path);
    if (parts.size() >= 2 && parts[0] == "api") {
        targetType = parts[1];
    }
    if (!parts.empty()) {
        long long parsed = 0;
        if (parseStrictInt64(parts.back(), parsed)) {
            targetIdOrZero = parsed;
        }
    }
}

void writeSystemLogAsync(const HttpRequestPtr &req, const HttpResponsePtr &resp) {
    const auto path = req->path();
    if (path.rfind("/api/", 0) != 0) {
        return;
    }
    if (path == "/api/health") {
        return;
    }

    const auto method = req->method();
    if (!(method == drogon::Post || method == drogon::Put || method == drogon::Delete)) {
        return;
    }

    std::string targetType;
    long long targetIdOrZero = 0;
    parseTargetByPath(path, targetType, targetIdOrZero);

    Json::Value details;
    details["path"] = path;
    details["query"] = req->query();
    details["status_code"] = static_cast<int>(resp->statusCode());
    if (method == drogon::Post) {
        details["method"] = "POST";
    } else if (method == drogon::Put) {
        details["method"] = "PUT";
    } else if (method == drogon::Delete) {
        details["method"] = "DELETE";
    } else {
        details["method"] = "OTHER";
    }
    const auto requestBody = req->getJsonObject();
    if (requestBody) {
        Json::Value sanitized = *requestBody;
        if (sanitized.isObject()) {
            if (sanitized.isMember("password")) {
                sanitized["password"] = "***";
            }
            if (sanitized.isMember("new_password")) {
                sanitized["new_password"] = "***";
            }
            if (sanitized.isMember("password_hash")) {
                sanitized["password_hash"] = "***";
            }
        }
        details["request_body"] = sanitized;
    }

    SessionUser actor;
    long long actorIdOrZero = 0;
    if (getSessionFromRequest(req, actor, false)) {
        actorIdOrZero = actor.userId;
    }
    const auto ipAddress = req->peerAddr().toIp();
    const std::string action = details["method"].asString() + " " + path;
    const std::string detailsJson = jsonToCompactString(details);

    auto db = drogon::app().getDbClient();
    db->execSqlAsync(
        "INSERT INTO system_logs(actor_user_id, action, target_type, target_id, details_json, ip_address) "
        "VALUES((SELECT u.id FROM users u WHERE u.id = NULLIF(?, 0) LIMIT 1), ?, ?, NULLIF(?, 0), ?, NULLIF(?, ''))",
        [](const drogon::orm::Result &) {},
        [](const DrogonDbException &e) {
            LOG_ERROR << "Failed to write system log: " << e.base().what();
        },
        actorIdOrZero,
        action,
        targetType,
        targetIdOrZero,
        detailsJson,
        ipAddress);
}

size_t utf8CharLen(unsigned char c) {
    if ((c & 0x80U) == 0) {
        return 1;
    }
    if ((c & 0xE0U) == 0xC0U) {
        return 2;
    }
    if ((c & 0xF0U) == 0xE0U) {
        return 3;
    }
    if ((c & 0xF8U) == 0xF0U) {
        return 4;
    }
    return 1;
}

bool isAsciiTokenChar(unsigned char c) {
    return std::isalnum(c) || c == '+' || c == '#' || c == '_' || c == '.' || c == '-';
}

const std::unordered_set<std::string> &stopwords() {
    static const std::unordered_set<std::string> kStopwords = {
        "a", "an", "the", "and", "or", "to", "of", "in", "for", "on", "with", "by", "at", "from",
        "is", "are", "be", "as", "this", "that", "it", "its", "into", "through", "about",
        "及", "和", "与", "在", "对", "等", "中", "的", "了", "并", "及其", "以及", "为"};
    return kStopwords;
}

std::vector<std::string> tokenizeText(const std::string &text) {
    std::vector<std::string> tokens;
    std::string asciiToken;

    const auto flushAscii = [&tokens, &asciiToken]() {
        if (!asciiToken.empty()) {
            tokens.push_back(asciiToken);
            asciiToken.clear();
        }
    };

    for (size_t i = 0; i < text.size();) {
        const auto byte = static_cast<unsigned char>(text[i]);
        if (byte < 128) {
            if (isAsciiTokenChar(byte)) {
                asciiToken.push_back(static_cast<char>(std::tolower(byte)));
            } else {
                flushAscii();
            }
            ++i;
            continue;
        }

        flushAscii();
        size_t len = utf8CharLen(byte);
        if (i + len > text.size()) {
            len = 1;
        }
        tokens.push_back(text.substr(i, len));
        i += len;
    }
    flushAscii();

    std::vector<std::string> filtered;
    filtered.reserve(tokens.size());
    for (const auto &tokenRaw : tokens) {
        const auto token = trimCopy(tokenRaw);
        if (token.empty()) {
            continue;
        }
        if (token.size() == 1 && static_cast<unsigned char>(token[0]) < 128 &&
            !std::isdigit(static_cast<unsigned char>(token[0]))) {
            continue;
        }
        if (stopwords().find(token) != stopwords().end()) {
            continue;
        }
        filtered.push_back(token);
    }
    return filtered;
}

std::unordered_set<std::string> buildTokenSet(const std::string &text) {
    std::unordered_set<std::string> out;
    for (const auto &token : tokenizeText(text)) {
        out.insert(token);
    }
    return out;
}

double jaccardScore(const std::unordered_set<std::string> &left,
                    const std::unordered_set<std::string> &right) {
    if (left.empty() || right.empty()) {
        return 0.0;
    }
    size_t inter = 0;
    for (const auto &token : left) {
        if (right.find(token) != right.end()) {
            ++inter;
        }
    }
    const size_t uni = left.size() + right.size() - inter;
    if (uni == 0) {
        return 0.0;
    }
    return static_cast<double>(inter) / static_cast<double>(uni);
}

std::string jsonToCompactString(const Json::Value &value) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, value);
}

Json::Value parseJsonTextOrDefault(const std::string &text, Json::ValueType expectedType) {
    Json::Value parsed;
    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream in(text);
    if (Json::parseFromStream(builder, in, &parsed, &errs) && parsed.type() == expectedType) {
        return parsed;
    }
    return Json::Value(expectedType);
}

std::vector<std::string> splitAliases(const std::string &text) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : text) {
        if (c == ',' || c == ';' || c == '|' || c == '/' || c == '\n' || c == '\t') {
            const auto part = trimCopy(current);
            if (!part.empty()) {
                parts.push_back(part);
            }
            current.clear();
            continue;
        }
        current.push_back(c);
    }
    const auto last = trimCopy(current);
    if (!last.empty()) {
        parts.push_back(last);
    }
    return parts;
}

bool containsAlias(const std::unordered_set<std::string> &tokenSet,
                   const std::string &textLower,
                   const std::string &aliasRaw) {
    const auto alias = toLowerAscii(trimCopy(aliasRaw));
    if (alias.empty()) {
        return false;
    }
    if (tokenSet.find(alias) != tokenSet.end()) {
        return true;
    }
    return textLower.find(alias) != std::string::npos;
}

std::vector<SkillDictionaryEntry> loadSkillDictionary(const drogon::orm::DbClientPtr &db) {
    std::vector<SkillDictionaryEntry> out;
    const auto result = db->execSqlSync(
        "SELECT id, name, COALESCE(synonyms_text, '') AS synonyms_text FROM skills");
    for (const auto &row : result) {
        SkillDictionaryEntry entry;
        entry.skillId = row["id"].as<long long>();
        entry.name = row["name"].as<std::string>();
        entry.aliases.push_back(entry.name);
        for (const auto &part : splitAliases(row["synonyms_text"].as<std::string>())) {
            entry.aliases.push_back(part);
        }
        out.push_back(std::move(entry));
    }
    return out;
}

std::vector<WeightedSkill> loadMajorSkills(const drogon::orm::DbClientPtr &db, long long majorId) {
    std::vector<WeightedSkill> out;
    const auto result = db->execSqlSync(
        "SELECT ms.skill_id, s.name, ms.weight "
        "FROM major_skills ms INNER JOIN skills s ON s.id = ms.skill_id "
        "WHERE ms.major_id = ?",
        majorId);
    for (const auto &row : result) {
        WeightedSkill skill;
        skill.skillId = row["skill_id"].as<long long>();
        skill.name = row["name"].as<std::string>();
        skill.weight = row["weight"].as<double>();
        skill.mandatory = false;
        out.push_back(std::move(skill));
    }
    return out;
}

std::vector<WeightedSkill> loadJobSkills(const drogon::orm::DbClientPtr &db, long long jobId) {
    std::vector<WeightedSkill> out;
    const auto result = db->execSqlSync(
        "SELECT js.skill_id, s.name, js.weight, js.is_mandatory "
        "FROM job_skills js INNER JOIN skills s ON s.id = js.skill_id "
        "WHERE js.job_id = ?",
        jobId);
    for (const auto &row : result) {
        WeightedSkill skill;
        skill.skillId = row["skill_id"].as<long long>();
        skill.name = row["name"].as<std::string>();
        skill.weight = row["weight"].as<double>();
        skill.mandatory = row["is_mandatory"].as<int>() != 0;
        out.push_back(std::move(skill));
    }
    return out;
}

std::unordered_map<std::string, WeightedSkill> toSkillMap(const std::vector<WeightedSkill> &skills) {
    std::unordered_map<std::string, WeightedSkill> out;
    for (const auto &skill : skills) {
        const auto key = toLowerAscii(trimCopy(skill.name));
        if (key.empty()) {
            continue;
        }
        auto it = out.find(key);
        if (it == out.end()) {
            out.emplace(key, skill);
            continue;
        }
        it->second.weight = std::max(it->second.weight, skill.weight);
        it->second.mandatory = it->second.mandatory || skill.mandatory;
    }
    return out;
}

std::unordered_map<std::string, WeightedSkill> inferSkillMapFromText(
    const std::vector<SkillDictionaryEntry> &dictionary,
    const std::string &text) {
    std::unordered_map<std::string, WeightedSkill> out;
    const auto textLower = toLowerAscii(text);
    const auto tokenSet = buildTokenSet(text);
    for (const auto &entry : dictionary) {
        bool hit = false;
        for (const auto &alias : entry.aliases) {
            if (containsAlias(tokenSet, textLower, alias)) {
                hit = true;
                break;
            }
        }
        if (!hit) {
            continue;
        }
        WeightedSkill skill;
        skill.skillId = entry.skillId;
        skill.name = entry.name;
        skill.weight = 1.0;
        skill.mandatory = false;
        const auto key = toLowerAscii(trimCopy(entry.name));
        out[key] = std::move(skill);
    }
    return out;
}

double computeSkillScore(
    const std::unordered_map<std::string, WeightedSkill> &majorSkills,
    const std::unordered_map<std::string, WeightedSkill> &jobSkills,
    Json::Value &matchedSkills,
    Json::Value &missingSkills,
    int &missingMandatoryCount) {
    matchedSkills = Json::Value(Json::arrayValue);
    missingSkills = Json::Value(Json::arrayValue);
    missingMandatoryCount = 0;

    if (jobSkills.empty()) {
        return 0.0;
    }

    double totalWeight = 0.0;
    double matchedWeight = 0.0;

    for (const auto &[key, jobSkill] : jobSkills) {
        const double jobWeight = std::max(0.001, jobSkill.weight);
        totalWeight += jobWeight;

        Json::Value item;
        item["skill_id"] = static_cast<Json::Int64>(jobSkill.skillId);
        item["skill_name"] = jobSkill.name;
        item["job_weight"] = roundTo3(jobWeight);
        item["mandatory"] = jobSkill.mandatory;

        const auto it = majorSkills.find(key);
        if (it == majorSkills.end()) {
            if (jobSkill.mandatory) {
                ++missingMandatoryCount;
            }
            missingSkills.append(item);
            continue;
        }

        const auto majorWeight = std::max(0.001, it->second.weight);
        const double coveredWeight = std::min(jobWeight, majorWeight);
        matchedWeight += coveredWeight;

        item["major_weight"] = roundTo3(majorWeight);
        item["covered_weight"] = roundTo3(coveredWeight);
        matchedSkills.append(item);
    }

    if (totalWeight <= 0.0) {
        return 0.0;
    }
    return clampValue((matchedWeight / totalWeight) * 100.0, 0.0, 100.0);
}

MatchComputationResult computeBaselineMatch(const drogon::orm::DbClientPtr &db,
                                            long long studentId,
                                            long long jobId,
                                            const std::string &algorithmVersionRaw,
                                            const std::string &preferredProvince,
                                            const std::string &preferredCity) {
    MatchComputationResult result;
    result.studentId = studentId;
    result.jobId = jobId;
    result.algorithmVersion = normalizeAlgorithmVersion(algorithmVersionRaw);

    try {
        const auto studentResult = db->execSqlSync(
            "SELECT s.id, s.major_id, "
            "COALESCE(m.name, '') AS major_name, COALESCE(m.description, '') AS major_description, "
            "COALESCE(s.resume_text, '') AS resume_text, "
            "COALESCE(s.technical_projects_text, '') AS student_projects_text, "
            "COALESCE(s.capability_summary_text, '') AS capability_summary_text, "
            "COALESCE(mp.core_courses_text, '') AS core_courses_text, "
            "COALESCE(mp.knowledge_skills_text, '') AS major_knowledge_skills_text, "
            "COALESCE(mp.technical_platform_text, '') AS major_platform_text, "
            "COALESCE(mp.capability_text, '') AS major_capability_text "
            "FROM students s "
            "INNER JOIN majors m ON m.id = s.major_id "
            "LEFT JOIN major_profiles mp ON mp.major_id = m.id "
            "WHERE s.id = ?",
            studentId);
        if (studentResult.empty()) {
            result.status = drogon::k404NotFound;
            result.message = "student not found: " + std::to_string(studentId);
            return result;
        }

        const auto jobResult = db->execSqlSync(
            "SELECT j.id, COALESCE(j.title, '') AS title, "
            "COALESCE(j.responsibilities_text, '') AS responsibilities_text, "
            "COALESCE(j.technical_projects_text, '') AS job_projects_text, "
            "COALESCE(j.required_knowledge_skills_text, '') AS required_knowledge_skills_text, "
            "COALESCE(j.required_tools_platform_text, '') AS required_tools_platform_text, "
            "COALESCE(j.location_province, '') AS location_province, "
            "COALESCE(j.location_city, '') AS location_city, "
            "COALESCE(c.main_business, '') AS main_business, "
            "COALESCE(c.flagship_products, '') AS flagship_products, "
            "COALESCE(c.domain_area, '') AS domain_area, "
            "COALESCE(c.application_industries, '') AS application_industries "
            "FROM jobs j INNER JOIN companies c ON c.id = j.company_id "
            "WHERE j.id = ?",
            jobId);
        if (jobResult.empty()) {
            result.status = drogon::k404NotFound;
            result.message = "job not found: " + std::to_string(jobId);
            return result;
        }

        const auto &studentRow = studentResult[0];
        const auto &jobRow = jobResult[0];
        result.majorId = studentRow["major_id"].as<long long>();
        const auto jobProvince = jobRow["location_province"].as<std::string>();
        const auto jobCity = jobRow["location_city"].as<std::string>();

        const std::string majorText =
            studentRow["major_name"].as<std::string>() + " " +
            studentRow["major_description"].as<std::string>() + " " +
            studentRow["core_courses_text"].as<std::string>() + " " +
            studentRow["major_knowledge_skills_text"].as<std::string>() + " " +
            studentRow["major_platform_text"].as<std::string>() + " " +
            studentRow["major_capability_text"].as<std::string>() + " " +
            studentRow["resume_text"].as<std::string>() + " " +
            studentRow["student_projects_text"].as<std::string>() + " " +
            studentRow["capability_summary_text"].as<std::string>();

        const std::string jobText =
            jobRow["title"].as<std::string>() + " " +
            jobRow["responsibilities_text"].as<std::string>() + " " +
            jobRow["job_projects_text"].as<std::string>() + " " +
            jobRow["required_knowledge_skills_text"].as<std::string>() + " " +
            jobRow["required_tools_platform_text"].as<std::string>() + " " +
            jobRow["main_business"].as<std::string>() + " " +
            jobRow["flagship_products"].as<std::string>() + " " +
            jobRow["domain_area"].as<std::string>() + " " +
            jobRow["application_industries"].as<std::string>();

        const auto majorTokens = buildTokenSet(majorText);
        const auto jobTokens = buildTokenSet(jobText);
        result.semanticScore = roundTo3(jaccardScore(majorTokens, jobTokens) * 100.0);

        const auto dictionary = loadSkillDictionary(db);
        const auto majorSkillsRaw = loadMajorSkills(db, result.majorId);
        const auto jobSkillsRaw = loadJobSkills(db, jobId);
        auto majorSkills = toSkillMap(majorSkillsRaw);
        auto jobSkills = toSkillMap(jobSkillsRaw);

        const auto inferredMajorSkills = inferSkillMapFromText(dictionary, majorText);
        const auto inferredJobSkills = inferSkillMapFromText(dictionary, jobText);
        if (majorSkills.empty()) {
            majorSkills = inferredMajorSkills;
        } else {
            for (const auto &[key, skill] : inferredMajorSkills) {
                if (majorSkills.find(key) == majorSkills.end()) {
                    majorSkills.emplace(key, skill);
                }
            }
        }
        if (jobSkills.empty()) {
            jobSkills = inferredJobSkills;
        } else {
            for (const auto &[key, skill] : inferredJobSkills) {
                if (jobSkills.find(key) == jobSkills.end()) {
                    jobSkills.emplace(key, skill);
                }
            }
        }

        int missingMandatoryCount = 0;
        result.skillScore = roundTo3(computeSkillScore(
            majorSkills,
            jobSkills,
            result.matchedSkills,
            result.missingSkills,
            missingMandatoryCount));
        if (jobSkills.empty()) {
            result.skillScore = result.semanticScore;
        }

        result.constraintScore = 1.0;

        if (missingMandatoryCount > 0) {
            const double factor = std::max(0.2, 1.0 - 0.2 * static_cast<double>(missingMandatoryCount));
            result.constraintScore *= factor;
            for (Json::ArrayIndex i = 0; i < result.missingSkills.size(); ++i) {
                if (!result.missingSkills[i]["mandatory"].asBool()) {
                    continue;
                }
                Json::Value item;
                item["type"] = "mandatory_skill";
                item["message"] = "missing mandatory skill";
                item["skill_name"] = result.missingSkills[i]["skill_name"].asString();
                result.constraintFailures.append(item);
            }
        }

        const auto expectedProvince = trimCopy(preferredProvince);
        const auto expectedCity = trimCopy(preferredCity);
        if (!expectedProvince.empty() && !jobProvince.empty() &&
            !equalsIgnoreCaseTrimmed(expectedProvince, jobProvince)) {
            result.constraintScore *= 0.9;
            Json::Value item;
            item["type"] = "location_province";
            item["message"] = "preferred province does not match job province";
            item["preferred_province"] = expectedProvince;
            item["job_province"] = jobProvince;
            result.constraintFailures.append(item);
        }
        if (!expectedCity.empty() && !jobCity.empty() &&
            !equalsIgnoreCaseTrimmed(expectedCity, jobCity)) {
            result.constraintScore *= 0.9;
            Json::Value item;
            item["type"] = "location_city";
            item["message"] = "preferred city does not match job city";
            item["preferred_city"] = expectedCity;
            item["job_city"] = jobCity;
            result.constraintFailures.append(item);
        }
        result.constraintScore = roundTo3(clampValue(result.constraintScore, 0.0, 1.0));

        const double fused = 0.55 * result.semanticScore + 0.45 * result.skillScore;
        result.finalScore = roundTo3(clampValue(fused * result.constraintScore, 0.0, 100.0));
        result.status = drogon::k200OK;
        result.ok = true;
        return result;
    } catch (const DrogonDbException &e) {
        LOG_ERROR << "Database error: " << e.base().what();
        result.status = drogon::k500InternalServerError;
        result.message = "database operation failed";
        return result;
    }
}

MatchComputationResult computeMajorJobMatch(const drogon::orm::DbClientPtr &db,
                                            long long majorId,
                                            long long jobId,
                                            const std::string &algorithmVersionRaw) {
    MatchComputationResult result;
    result.studentId = 0;
    result.majorId = majorId;
    result.jobId = jobId;
    result.algorithmVersion = normalizeAlgorithmVersion(algorithmVersionRaw);

    try {
        const auto majorResult = db->execSqlSync(
            "SELECT m.id, "
            "COALESCE(m.name, '') AS major_name, COALESCE(m.description, '') AS major_description, "
            "COALESCE(mp.core_courses_text, '') AS core_courses_text, "
            "COALESCE(mp.knowledge_skills_text, '') AS major_knowledge_skills_text, "
            "COALESCE(mp.technical_platform_text, '') AS major_platform_text, "
            "COALESCE(mp.capability_text, '') AS major_capability_text "
            "FROM majors m "
            "LEFT JOIN major_profiles mp ON mp.major_id = m.id "
            "WHERE m.id = ?",
            majorId);
        if (majorResult.empty()) {
            result.status = drogon::k404NotFound;
            result.message = "major not found: " + std::to_string(majorId);
            return result;
        }

        const auto jobResult = db->execSqlSync(
            "SELECT j.id, COALESCE(j.title, '') AS title, "
            "COALESCE(j.responsibilities_text, '') AS responsibilities_text, "
            "COALESCE(j.technical_projects_text, '') AS job_projects_text, "
            "COALESCE(j.required_knowledge_skills_text, '') AS required_knowledge_skills_text, "
            "COALESCE(j.required_tools_platform_text, '') AS required_tools_platform_text, "
            "COALESCE(c.main_business, '') AS main_business, "
            "COALESCE(c.flagship_products, '') AS flagship_products, "
            "COALESCE(c.domain_area, '') AS domain_area, "
            "COALESCE(c.application_industries, '') AS application_industries "
            "FROM jobs j INNER JOIN companies c ON c.id = j.company_id "
            "WHERE j.id = ?",
            jobId);
        if (jobResult.empty()) {
            result.status = drogon::k404NotFound;
            result.message = "job not found: " + std::to_string(jobId);
            return result;
        }

        const auto &majorRow = majorResult[0];
        const auto &jobRow = jobResult[0];
        const std::string majorText =
            majorRow["major_name"].as<std::string>() + " " +
            majorRow["major_description"].as<std::string>() + " " +
            majorRow["core_courses_text"].as<std::string>() + " " +
            majorRow["major_knowledge_skills_text"].as<std::string>() + " " +
            majorRow["major_platform_text"].as<std::string>() + " " +
            majorRow["major_capability_text"].as<std::string>();

        const std::string jobText =
            jobRow["title"].as<std::string>() + " " +
            jobRow["responsibilities_text"].as<std::string>() + " " +
            jobRow["job_projects_text"].as<std::string>() + " " +
            jobRow["required_knowledge_skills_text"].as<std::string>() + " " +
            jobRow["required_tools_platform_text"].as<std::string>() + " " +
            jobRow["main_business"].as<std::string>() + " " +
            jobRow["flagship_products"].as<std::string>() + " " +
            jobRow["domain_area"].as<std::string>() + " " +
            jobRow["application_industries"].as<std::string>();

        const auto majorTokens = buildTokenSet(majorText);
        const auto jobTokens = buildTokenSet(jobText);
        result.semanticScore = roundTo3(jaccardScore(majorTokens, jobTokens) * 100.0);

        const auto dictionary = loadSkillDictionary(db);
        const auto majorSkillsRaw = loadMajorSkills(db, majorId);
        const auto jobSkillsRaw = loadJobSkills(db, jobId);
        auto majorSkills = toSkillMap(majorSkillsRaw);
        auto jobSkills = toSkillMap(jobSkillsRaw);

        const auto inferredMajorSkills = inferSkillMapFromText(dictionary, majorText);
        const auto inferredJobSkills = inferSkillMapFromText(dictionary, jobText);
        if (majorSkills.empty()) {
            majorSkills = inferredMajorSkills;
        } else {
            for (const auto &[key, skill] : inferredMajorSkills) {
                if (majorSkills.find(key) == majorSkills.end()) {
                    majorSkills.emplace(key, skill);
                }
            }
        }
        if (jobSkills.empty()) {
            jobSkills = inferredJobSkills;
        } else {
            for (const auto &[key, skill] : inferredJobSkills) {
                if (jobSkills.find(key) == jobSkills.end()) {
                    jobSkills.emplace(key, skill);
                }
            }
        }

        int missingMandatoryCount = 0;
        result.skillScore = roundTo3(computeSkillScore(
            majorSkills,
            jobSkills,
            result.matchedSkills,
            result.missingSkills,
            missingMandatoryCount));
        if (jobSkills.empty()) {
            result.skillScore = result.semanticScore;
        }

        result.constraintScore = 1.0;

        if (missingMandatoryCount > 0) {
            const double factor = std::max(0.2, 1.0 - 0.2 * static_cast<double>(missingMandatoryCount));
            result.constraintScore *= factor;
            for (Json::ArrayIndex i = 0; i < result.missingSkills.size(); ++i) {
                if (!result.missingSkills[i]["mandatory"].asBool()) {
                    continue;
                }
                Json::Value item;
                item["type"] = "mandatory_skill";
                item["message"] = "missing mandatory skill";
                item["skill_name"] = result.missingSkills[i]["skill_name"].asString();
                result.constraintFailures.append(item);
            }
        }
        result.constraintScore = roundTo3(clampValue(result.constraintScore, 0.0, 1.0));

        const double fused = 0.55 * result.semanticScore + 0.45 * result.skillScore;
        result.finalScore = roundTo3(clampValue(fused * result.constraintScore, 0.0, 100.0));
        result.status = drogon::k200OK;
        result.ok = true;
        return result;
    } catch (const DrogonDbException &e) {
        LOG_ERROR << "Database error: " << e.base().what();
        result.status = drogon::k500InternalServerError;
        result.message = "database operation failed";
        return result;
    }
}

bool persistMatchResult(const drogon::orm::DbClientPtr &db,
                        MatchComputationResult &result,
                        std::string &errorMessage) {
    try {
        const auto insertResult = db->execSqlSync(
            "INSERT INTO match_results(student_id, major_id, job_id, algorithm_version, semantic_score, skill_score, "
            "constraint_score, final_score, matched_skills_json, missing_skills_json, constraint_failures_json) "
            "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            result.studentId,
            result.majorId,
            result.jobId,
            result.algorithmVersion,
            result.semanticScore,
            result.skillScore,
            result.constraintScore,
            result.finalScore,
            jsonToCompactString(result.matchedSkills),
            jsonToCompactString(result.missingSkills),
            jsonToCompactString(result.constraintFailures));
        result.matchResultId = static_cast<long long>(insertResult.insertId());
        return true;
    } catch (const DrogonDbException &e) {
        LOG_ERROR << "Database error: " << e.base().what();
        errorMessage = "failed to persist match result";
        return false;
    }
}

bool persistMajorJobMatchResult(const drogon::orm::DbClientPtr &db,
                                const MatchComputationResult &result,
                                long long createdByUserId,
                                const std::string &runMode,
                                long long &resultId,
                                std::string &errorMessage) {
    resultId = 0;
    errorMessage.clear();
    try {
        const auto mode = toLowerAscii(trimCopy(runMode));
        const std::string safeMode = (mode == "batch") ? "batch" : "single";
        const auto insertResult = db->execSqlSync(
            "INSERT INTO major_job_match_results("
            "major_id, job_id, algorithm_version, semantic_score, skill_score, constraint_score, final_score, "
            "matched_skills_json, missing_skills_json, constraint_failures_json, run_mode, created_by_user_id) "
            "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, NULLIF(?, 0))",
            result.majorId,
            result.jobId,
            result.algorithmVersion,
            result.semanticScore,
            result.skillScore,
            result.constraintScore,
            result.finalScore,
            jsonToCompactString(result.matchedSkills),
            jsonToCompactString(result.missingSkills),
            jsonToCompactString(result.constraintFailures),
            safeMode,
            createdByUserId);
        resultId = static_cast<long long>(insertResult.insertId());
        return true;
    } catch (const DrogonDbException &e) {
        LOG_ERROR << "Database error: " << e.base().what();
        errorMessage = "failed to persist major-job match result";
        return false;
    }
}

Json::Value matchResultRowToJson(const drogon::orm::Row &row) {
    Json::Value item;
    item["id"] = static_cast<Json::Int64>(row["id"].as<long long>());
    item["student_id"] = static_cast<Json::Int64>(row["student_id"].as<long long>());
    item["student_name"] = row["student_name"].isNull() ? "" : row["student_name"].as<std::string>();
    item["major_id"] = static_cast<Json::Int64>(row["major_id"].as<long long>());
    item["major_name"] = row["major_name"].isNull() ? "" : row["major_name"].as<std::string>();
    item["job_id"] = static_cast<Json::Int64>(row["job_id"].as<long long>());
    item["job_title"] = row["job_title"].isNull() ? "" : row["job_title"].as<std::string>();
    item["algorithm_version"] = row["algorithm_version"].isNull() ? "" : row["algorithm_version"].as<std::string>();
    item["semantic_score"] = row["semantic_score"].isNull() ? 0.0 : row["semantic_score"].as<double>();
    item["skill_score"] = row["skill_score"].isNull() ? 0.0 : row["skill_score"].as<double>();
    item["constraint_score"] = row["constraint_score"].isNull() ? 0.0 : row["constraint_score"].as<double>();
    item["final_score"] = row["final_score"].isNull() ? 0.0 : row["final_score"].as<double>();
    item["matched_skills"] = row["matched_skills_json"].isNull()
                                 ? Json::Value(Json::arrayValue)
                                 : parseJsonTextOrDefault(row["matched_skills_json"].as<std::string>(), Json::arrayValue);
    item["missing_skills"] = row["missing_skills_json"].isNull()
                                 ? Json::Value(Json::arrayValue)
                                 : parseJsonTextOrDefault(row["missing_skills_json"].as<std::string>(), Json::arrayValue);
    item["constraint_failures"] = row["constraint_failures_json"].isNull()
                                      ? Json::Value(Json::arrayValue)
                                      : parseJsonTextOrDefault(
                                            row["constraint_failures_json"].as<std::string>(),
                                            Json::arrayValue);
    item["generated_at"] = row["generated_at"].isNull() ? "" : row["generated_at"].as<std::string>();
    return item;
}

Json::Value majorJobMatchResultRowToJson(const drogon::orm::Row &row) {
    Json::Value item;
    item["id"] = static_cast<Json::Int64>(row["id"].as<long long>());
    item["major_id"] = static_cast<Json::Int64>(row["major_id"].as<long long>());
    item["major_name"] = row["major_name"].isNull() ? "" : row["major_name"].as<std::string>();
    item["job_id"] = static_cast<Json::Int64>(row["job_id"].as<long long>());
    item["job_title"] = row["job_title"].isNull() ? "" : row["job_title"].as<std::string>();
    item["company_name"] = row["company_name"].isNull() ? "" : row["company_name"].as<std::string>();
    item["algorithm_version"] = row["algorithm_version"].isNull() ? "" : row["algorithm_version"].as<std::string>();
    item["semantic_score"] = row["semantic_score"].isNull() ? 0.0 : row["semantic_score"].as<double>();
    item["skill_score"] = row["skill_score"].isNull() ? 0.0 : row["skill_score"].as<double>();
    item["constraint_score"] = row["constraint_score"].isNull() ? 0.0 : row["constraint_score"].as<double>();
    item["final_score"] = row["final_score"].isNull() ? 0.0 : row["final_score"].as<double>();
    item["matched_skills"] = row["matched_skills_json"].isNull()
                                 ? Json::Value(Json::arrayValue)
                                 : parseJsonTextOrDefault(row["matched_skills_json"].as<std::string>(), Json::arrayValue);
    item["missing_skills"] = row["missing_skills_json"].isNull()
                                 ? Json::Value(Json::arrayValue)
                                 : parseJsonTextOrDefault(row["missing_skills_json"].as<std::string>(), Json::arrayValue);
    item["constraint_failures"] = row["constraint_failures_json"].isNull()
                                      ? Json::Value(Json::arrayValue)
                                      : parseJsonTextOrDefault(
                                            row["constraint_failures_json"].as<std::string>(),
                                            Json::arrayValue);
    item["run_mode"] = row["run_mode"].isNull() ? "" : row["run_mode"].as<std::string>();
    item["created_by_user_id"] = row["created_by_user_id"].isNull()
                                     ? static_cast<Json::Int64>(0)
                                     : static_cast<Json::Int64>(row["created_by_user_id"].as<long long>());
    item["created_by_username"] = row["created_by_username"].isNull() ? "" : row["created_by_username"].as<std::string>();
    item["generated_at"] = row["generated_at"].isNull() ? "" : row["generated_at"].as<std::string>();
    return item;
}

Json::Value systemLogRowToJson(const drogon::orm::Row &row) {
    Json::Value item;
    item["id"] = static_cast<Json::Int64>(row["id"].as<long long>());
    item["actor_user_id"] = row["actor_user_id"].isNull()
                                ? static_cast<Json::Int64>(0)
                                : static_cast<Json::Int64>(row["actor_user_id"].as<long long>());
    item["actor_username"] = row["actor_username"].isNull() ? "" : row["actor_username"].as<std::string>();
    item["action"] = row["action"].isNull() ? "" : row["action"].as<std::string>();
    item["target_type"] = row["target_type"].isNull() ? "" : row["target_type"].as<std::string>();
    item["target_id"] = row["target_id"].isNull()
                            ? static_cast<Json::Int64>(0)
                            : static_cast<Json::Int64>(row["target_id"].as<long long>());
    item["details"] = row["details_json"].isNull()
                          ? Json::Value(Json::objectValue)
                          : parseJsonTextOrDefault(row["details_json"].as<std::string>(), Json::objectValue);
    item["ip_address"] = row["ip_address"].isNull() ? "" : row["ip_address"].as<std::string>();
    item["created_at"] = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
    return item;
}

}  // namespace gm
