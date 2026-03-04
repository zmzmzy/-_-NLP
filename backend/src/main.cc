#include <drogon/drogon.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <limits>
#include <mutex>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <openssl/sha.h>

using drogon::HttpRequestPtr;
using drogon::HttpResponse;
using drogon::HttpResponsePtr;
using drogon::orm::DrogonDbException;
using DbCallback = std::function<void(const HttpResponsePtr &)>;

namespace {

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
    item["education_level"] = row["education_level"].isNull() ? "" : row["education_level"].as<std::string>();
    item["discipline_category"] = row["discipline_category"].isNull() ? "" : row["discipline_category"].as<std::string>();
    item["enrollment_year"] = row["enrollment_year"].isNull() ? 0 : row["enrollment_year"].as<int>();
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
    item["status"] = row["status"].isNull() ? "" : row["status"].as<std::string>();
    item["published_at"] = row["published_at"].isNull() ? "" : row["published_at"].as<std::string>();
    item["created_at"] = row["created_at"].isNull() ? "" : row["created_at"].as<std::string>();
    item["updated_at"] = row["updated_at"].isNull() ? "" : row["updated_at"].as<std::string>();
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
        "VALUES(NULLIF(?, 0), ?, ?, NULLIF(?, 0), ?, NULLIF(?, ''))",
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

int degreeRank(const std::string &degreeRaw) {
    const auto degree = toLowerAscii(trimCopy(degreeRaw));
    if (degree == "none") {
        return 0;
    }
    if (degree == "diploma") {
        return 1;
    }
    if (degree == "bachelor") {
        return 2;
    }
    if (degree == "master") {
        return 3;
    }
    if (degree == "phd") {
        return 4;
    }
    return 0;
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
            "SELECT s.id, s.major_id, m.education_level AS major_education_level, "
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
            "SELECT j.id, j.min_degree, COALESCE(j.title, '') AS title, "
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
        const auto studentDegree = studentRow["major_education_level"].as<std::string>();
        const auto jobMinDegree = jobRow["min_degree"].as<std::string>();
        if (degreeRank(studentDegree) < degreeRank(jobMinDegree)) {
            result.constraintScore *= 0.4;
            Json::Value item;
            item["type"] = "degree";
            item["message"] = "student degree does not meet job minimum degree";
            item["student_degree"] = studentDegree;
            item["job_min_degree"] = jobMinDegree;
            result.constraintFailures.append(item);
        }

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

}  // namespace

int main() {
    drogon::app().loadConfigFile("../config/dev.json");

    drogon::app().registerHandler(
        "/api/health",
        [](const HttpRequestPtr &,
           DbCallback &&callback) {
            Json::Value body;
            body["status"] = "ok";
            body["service"] = "graduate_match_backend";
            body["version"] = "0.1.0";
            callback(makeJson(body));
        },
        {drogon::Get});

    drogon::app().registerPostHandlingAdvice(
        [](const HttpRequestPtr &req, const HttpResponsePtr &resp) {
            writeSystemLogAsync(req, resp);
        });

    // -------------------------
    // Auth (Session)
    // -------------------------
    drogon::app().registerHandler(
        "/api/auth/login",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!requireFields(*json, {"username", "password"}, callback)) {
                return;
            }
            const auto username = trimCopy((*json)["username"].asString());
            const auto password = (*json)["password"].asString();
            if (username.empty() || password.empty()) {
                sendError(callback, drogon::k400BadRequest, "username and password cannot be empty");
                return;
            }

            db->execSqlAsync(
                "SELECT id, username, password_hash, role, is_active FROM users WHERE username = ? LIMIT 1",
                [callback, username, password](const drogon::orm::Result &result) {
                    if (result.empty()) {
                        sendError(callback, drogon::k401Unauthorized, "invalid username or password", "auth_failed");
                        return;
                    }
                    const auto &row = result[0];
                    const bool isActive = row["is_active"].as<int>() != 0;
                    if (!isActive) {
                        sendError(callback, drogon::k403Forbidden, "user is disabled", "forbidden");
                        return;
                    }
                    const auto storedHash = row["password_hash"].as<std::string>();
                    if (!verifyPassword(password, storedHash)) {
                        sendError(callback, drogon::k401Unauthorized, "invalid username or password", "auth_failed");
                        return;
                    }

                    const auto userId = row["id"].as<long long>();
                    const auto role = row["role"].as<std::string>();
                    long long expiresAt = 0;
                    const auto token = createSession(userId, username, role, expiresAt);

                    Json::Value user;
                    user["id"] = static_cast<Json::Int64>(userId);
                    user["username"] = username;
                    user["role"] = role;
                    user["is_active"] = true;

                    Json::Value body;
                    body["data"]["token"] = token;
                    body["data"]["token_type"] = "Bearer";
                    body["data"]["expires_at_epoch"] = static_cast<Json::Int64>(expiresAt);
                    body["data"]["user"] = user;
                    callback(makeJson(body));
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                username);
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/auth/logout",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            SessionUser user;
            if (!requireAuthUser(req, callback, user)) {
                return;
            }
            const auto token = extractAuthToken(req);
            revokeSessionByToken(token);

            Json::Value body;
            body["message"] = "logged out";
            callback(makeJson(body));
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/auth/me",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            SessionUser user;
            if (!requireAuthUser(req, callback, user)) {
                return;
            }
            Json::Value body;
            body["data"]["id"] = static_cast<Json::Int64>(user.userId);
            body["data"]["username"] = user.username;
            body["data"]["role"] = user.role;
            body["data"]["expires_at_epoch"] = static_cast<Json::Int64>(user.expiresAtEpoch);
            callback(makeJson(body));
        },
        {drogon::Get});

    // -------------------------
    // System Logs
    // -------------------------
    drogon::app().registerHandler(
        "/api/system-logs",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            SessionUser user;
            if (!requireAuthUser(req, callback, user, true)) {
                return;
            }

            auto db = drogon::app().getDbClient();
            int page = 1;
            int pageSize = 20;
            if (!parsePagingParams(req, callback, page, pageSize, 1, 20, 100)) {
                return;
            }
            const int offset = (page - 1) * pageSize;
            long long actorUserId = 0;
            long long targetId = 0;
            if (!parseOptionalInt64Param(req, "actor_user_id", callback, actorUserId)) {
                return;
            }
            if (!parseOptionalInt64Param(req, "target_id", callback, targetId)) {
                return;
            }
            const auto targetType = trimCopy(req->getParameter("target_type"));
            const auto action = trimCopy(req->getParameter("action"));
            const auto keywordRaw = trimCopy(req->getParameter("keyword"));
            const auto keywordLike = "%" + keywordRaw + "%";

            const std::string sortOrder = normalizeSortOrder(req->getParameter("sort_order"));
            static const std::unordered_map<std::string, std::string> kSortMap = {
                {"id", "sl.id"},
                {"action", "sl.action"},
                {"target_type", "sl.target_type"},
                {"created_at", "sl.created_at"}};
            const std::string sortField = resolveSortField(req->getParameter("sort_by"), kSortMap, "sl.id");
            const std::string whereSql =
                "WHERE (? = 0 OR sl.actor_user_id = ?) "
                "AND (? = 0 OR sl.target_id = ?) "
                "AND (? = '' OR sl.target_type = ?) "
                "AND (? = '' OR sl.action = ?) "
                "AND (? = '' OR sl.action LIKE ? OR sl.target_type LIKE ?) ";
            const std::string baseSql =
                "FROM system_logs sl "
                "LEFT JOIN users u ON u.id = sl.actor_user_id ";
            const std::string selectSql =
                "SELECT sl.id, sl.actor_user_id, u.username AS actor_username, sl.action, sl.target_type, "
                "sl.target_id, sl.details_json, sl.ip_address, sl.created_at ";

            db->execSqlAsync(
                "SELECT COUNT(*) AS total " + baseSql + whereSql,
                [callback, db, selectSql, baseSql, whereSql, sortField, sortOrder,
                 page, pageSize, offset, actorUserId, targetId, targetType, action, keywordRaw, keywordLike]
                (const drogon::orm::Result &countResult) {
                    const auto total = static_cast<Json::UInt64>(countResult[0]["total"].as<long long>());
                    db->execSqlAsync(
                        selectSql + baseSql + whereSql + "ORDER BY " + sortField + " " + sortOrder +
                            " LIMIT ? OFFSET ?",
                        [callback, total, page, pageSize](const drogon::orm::Result &result) {
                            Json::Value list(Json::arrayValue);
                            for (const auto &row : result) {
                                list.append(systemLogRowToJson(row));
                            }
                            Json::Value body;
                            body["data"] = list;
                            appendPagedMeta(body, total, page, pageSize);
                            callback(makeJson(body));
                        },
                        [callback](const DrogonDbException &e) {
                            sendDbError(e, callback);
                        },
                        actorUserId,
                        actorUserId,
                        targetId,
                        targetId,
                        targetType,
                        targetType,
                        action,
                        action,
                        keywordRaw,
                        keywordLike,
                        keywordLike,
                        pageSize,
                        offset);
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                actorUserId,
                actorUserId,
                targetId,
                targetId,
                targetType,
                targetType,
                action,
                action,
                keywordRaw,
                keywordLike,
                keywordLike);
        },
        {drogon::Get});

    // -------------------------
    // Colleges CRUD
    // -------------------------
    drogon::app().registerHandler(
        "/api/colleges",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            int page = 1;
            int pageSize = 20;
            if (!parsePagingParams(req, callback, page, pageSize)) {
                return;
            }
            const int offset = (page - 1) * pageSize;
            const std::string keyword = req->getParameter("keyword");
            const std::string sortOrder = normalizeSortOrder(req->getParameter("sort_order"));
            static const std::unordered_map<std::string, std::string> kSortMap = {
                {"id", "id"},
                {"code", "code"},
                {"name", "name"},
                {"created_at", "created_at"},
                {"updated_at", "updated_at"}};
            const std::string sortField =
                resolveSortField(req->getParameter("sort_by"), kSortMap, "id");
            const std::string listSql =
                "SELECT id, code, name, description, created_at, updated_at "
                "FROM colleges ";
            const std::string whereSql =
                "WHERE code LIKE ? OR name LIKE ? OR description LIKE ? ";
            const std::string orderSql = "ORDER BY " + sortField + " " + sortOrder + " ";

            if (keyword.empty()) {
                db->execSqlAsync(
                    "SELECT COUNT(*) AS total FROM colleges",
                    [callback, db, listSql, orderSql, page, pageSize, offset]
                    (const drogon::orm::Result &countResult) {
                        const auto total = static_cast<Json::UInt64>(countResult[0]["total"].as<long long>());
                        db->execSqlAsync(
                            listSql + orderSql + "LIMIT ? OFFSET ?",
                            [callback, total, page, pageSize](const drogon::orm::Result &result) {
                                Json::Value list(Json::arrayValue);
                                for (const auto &row : result) {
                                    list.append(collegeRowToJson(row));
                                }
                                Json::Value body;
                                body["data"] = list;
                                appendPagedMeta(body, total, page, pageSize);
                                callback(makeJson(body));
                            },
                            [callback](const DrogonDbException &e) {
                                sendDbError(e, callback);
                            },
                            pageSize,
                            offset);
                    },
                    [callback](const DrogonDbException &e) {
                        sendDbError(e, callback);
                    });
                return;
            }

            const std::string like = "%" + keyword + "%";
            db->execSqlAsync(
                "SELECT COUNT(*) AS total FROM colleges " + whereSql,
                [callback, db, listSql, whereSql, orderSql, page, pageSize, offset, like]
                (const drogon::orm::Result &countResult) {
                    const auto total = static_cast<Json::UInt64>(countResult[0]["total"].as<long long>());
                    db->execSqlAsync(
                        listSql + whereSql + orderSql + "LIMIT ? OFFSET ?",
                        [callback, total, page, pageSize](const drogon::orm::Result &result) {
                            Json::Value list(Json::arrayValue);
                            for (const auto &row : result) {
                                list.append(collegeRowToJson(row));
                            }
                            Json::Value body;
                            body["data"] = list;
                            appendPagedMeta(body, total, page, pageSize);
                            callback(makeJson(body));
                        },
                        [callback](const DrogonDbException &e) {
                            sendDbError(e, callback);
                        },
                        like,
                        like,
                        like,
                        pageSize,
                        offset);
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                like,
                like,
                like);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/colleges/{1}",
        [](const HttpRequestPtr &, DbCallback &&callback, long long id) {
            auto db = drogon::app().getDbClient();
            db->execSqlAsync(
                "SELECT id, code, name, description, created_at, updated_at "
                "FROM colleges WHERE id = ?",
                [callback, id](const drogon::orm::Result &result) {
                    if (result.empty()) {
                        sendError(callback, drogon::k404NotFound,
                                  "college not found: " + std::to_string(id));
                        return;
                    }
                    Json::Value body;
                    body["data"] = collegeRowToJson(result[0]);
                    callback(makeJson(body));
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                id);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/colleges",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!requireFields(*json, {"code", "name"}, callback)) {
                return;
            }
            const auto code = (*json)["code"].asString();
            const auto name = (*json)["name"].asString();
            const auto description =
                json->isMember("description") ? (*json)["description"].asString() : "";

            db->execSqlAsync(
                "INSERT INTO colleges(code, name, description) VALUES(?, ?, ?)",
                [callback, code, name, description](const drogon::orm::Result &result) {
                    Json::Value item;
                    item["id"] = static_cast<Json::UInt64>(result.insertId());
                    item["code"] = code;
                    item["name"] = name;
                    item["description"] = description;
                    Json::Value body;
                    body["data"] = item;
                    callback(makeJson(body, drogon::k201Created));
                },
                [callback](const DrogonDbException &e) {
                    std::string msg = e.base().what();
                    if (msg.find("Duplicate") != std::string::npos) {
                        sendError(callback, drogon::k409Conflict, "duplicate college code or name");
                        return;
                    }
                    sendDbError(e, callback);
                },
                code,
                name,
                description);
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/colleges/{1}",
        [](const HttpRequestPtr &req, DbCallback &&callback, long long id) {
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!requireFields(*json, {"code", "name"}, callback)) {
                return;
            }
            const auto code = (*json)["code"].asString();
            const auto name = (*json)["name"].asString();
            const auto description =
                json->isMember("description") ? (*json)["description"].asString() : "";

            db->execSqlAsync(
                "UPDATE colleges SET code = ?, name = ?, description = ? WHERE id = ?",
                [callback, db, id](const drogon::orm::Result &result) {
                    if (result.affectedRows() == 0) {
                        sendError(callback, drogon::k404NotFound,
                                  "college not found: " + std::to_string(id));
                        return;
                    }
                    db->execSqlAsync(
                        "SELECT id, code, name, description, created_at, updated_at "
                        "FROM colleges WHERE id = ?",
                        [callback](const drogon::orm::Result &rowResult) {
                            Json::Value body;
                            body["data"] = collegeRowToJson(rowResult[0]);
                            callback(makeJson(body));
                        },
                        [callback](const DrogonDbException &e) {
                            sendDbError(e, callback);
                        },
                        id);
                },
                [callback](const DrogonDbException &e) {
                    std::string msg = e.base().what();
                    if (msg.find("Duplicate") != std::string::npos) {
                        sendError(callback, drogon::k409Conflict, "duplicate college code or name");
                        return;
                    }
                    sendDbError(e, callback);
                },
                code,
                name,
                description,
                id);
        },
        {drogon::Put});

    drogon::app().registerHandler(
        "/api/colleges/{1}",
        [](const HttpRequestPtr &, DbCallback &&callback, long long id) {
            auto db = drogon::app().getDbClient();
            db->execSqlAsync(
                "DELETE FROM colleges WHERE id = ?",
                [callback, id](const drogon::orm::Result &result) {
                    if (result.affectedRows() == 0) {
                        sendError(callback, drogon::k404NotFound,
                                  "college not found: " + std::to_string(id));
                        return;
                    }
                    Json::Value body;
                    body["message"] = "deleted";
                    body["id"] = static_cast<Json::Int64>(id);
                    callback(makeJson(body));
                },
                [callback](const DrogonDbException &e) {
                    std::string msg = e.base().what();
                    if (msg.find("foreign key constraint fails") != std::string::npos) {
                        sendError(callback, drogon::k409Conflict, "college is referenced by other data");
                        return;
                    }
                    sendDbError(e, callback);
                },
                id);
        },
        {drogon::Delete});

    // -------------------------
    // Majors CRUD
    // -------------------------
    drogon::app().registerHandler(
        "/api/majors",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            int page = 1;
            int pageSize = 20;
            if (!parsePagingParams(req, callback, page, pageSize)) {
                return;
            }
            const int offset = (page - 1) * pageSize;
            long long collegeId = 0;
            if (!parseOptionalInt64Param(req, "college_id", callback, collegeId)) {
                return;
            }
            const auto keywordRaw = req->getParameter("keyword");
            const auto like = "%" + keywordRaw + "%";
            const auto educationLevel = req->getParameter("education_level");
            static const std::unordered_set<std::string> kEduLevels = {"bachelor", "master", "phd"};
            if (!educationLevel.empty() &&
                !validateEnumValue("education_level", educationLevel, kEduLevels, callback)) {
                return;
            }
            const std::string sortOrder = normalizeSortOrder(req->getParameter("sort_order"));
            static const std::unordered_map<std::string, std::string> kSortMap = {
                {"id", "m.id"},
                {"code", "m.code"},
                {"name", "m.name"},
                {"college_name", "c.name"},
                {"education_level", "m.education_level"},
                {"enrollment_year", "m.enrollment_year"},
                {"created_at", "m.created_at"},
                {"updated_at", "m.updated_at"}};
            const std::string sortField =
                resolveSortField(req->getParameter("sort_by"), kSortMap, "m.id");
            const std::string whereSql =
                "WHERE (? = '' OR m.code LIKE ? OR m.name LIKE ? OR c.name LIKE ? OR "
                "m.discipline_category LIKE ? OR m.description LIKE ?) "
                "AND (? = 0 OR m.college_id = ?) "
                "AND (? = '' OR m.education_level = ?) ";
            const std::string baseFromSql =
                "FROM majors m INNER JOIN colleges c ON c.id = m.college_id ";
            const std::string selectSql =
                "SELECT m.id, m.college_id, c.name AS college_name, m.code, m.name, "
                "m.education_level, m.discipline_category, m.enrollment_year, m.description, "
                "m.created_at, m.updated_at ";

            db->execSqlAsync(
                "SELECT COUNT(*) AS total " + baseFromSql + whereSql,
                [callback, db, selectSql, baseFromSql, whereSql, sortField, sortOrder,
                 page, pageSize, offset, keywordRaw, like, collegeId, educationLevel]
                (const drogon::orm::Result &countResult) {
                    const auto total = static_cast<Json::UInt64>(countResult[0]["total"].as<long long>());
                    db->execSqlAsync(
                        selectSql + baseFromSql + whereSql + "ORDER BY " + sortField + " " + sortOrder +
                            " LIMIT ? OFFSET ?",
                        [callback, total, page, pageSize](const drogon::orm::Result &result) {
                            Json::Value list(Json::arrayValue);
                            for (const auto &row : result) {
                                list.append(majorRowToJson(row));
                            }
                            Json::Value body;
                            body["data"] = list;
                            appendPagedMeta(body, total, page, pageSize);
                            callback(makeJson(body));
                        },
                        [callback](const DrogonDbException &e) {
                            sendDbError(e, callback);
                        },
                        keywordRaw,
                        like,
                        like,
                        like,
                        like,
                        like,
                        collegeId,
                        collegeId,
                        educationLevel,
                        educationLevel,
                        pageSize,
                        offset);
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                keywordRaw,
                like,
                like,
                like,
                like,
                like,
                collegeId,
                collegeId,
                educationLevel,
                educationLevel);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/majors/{1}",
        [](const HttpRequestPtr &, DbCallback &&callback, long long id) {
            auto db = drogon::app().getDbClient();
            db->execSqlAsync(
                "SELECT m.id, m.college_id, c.name AS college_name, m.code, m.name, "
                "m.education_level, m.discipline_category, m.enrollment_year, m.description, "
                "m.created_at, m.updated_at "
                "FROM majors m INNER JOIN colleges c ON c.id = m.college_id "
                "WHERE m.id = ?",
                [callback, id](const drogon::orm::Result &result) {
                    if (result.empty()) {
                        sendError(callback, drogon::k404NotFound,
                                  "major not found: " + std::to_string(id));
                        return;
                    }
                    Json::Value body;
                    body["data"] = majorRowToJson(result[0]);
                    callback(makeJson(body));
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                id);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/majors",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!requireFields(*json, {"college_id", "code", "name"}, callback)) {
                return;
            }
            const auto collegeId = (*json)["college_id"].asInt64();
            const auto code = (*json)["code"].asString();
            const auto name = (*json)["name"].asString();
            const auto educationLevel =
                json->isMember("education_level") ? (*json)["education_level"].asString() : "bachelor";
            static const std::unordered_set<std::string> kEduLevels = {"bachelor", "master", "phd"};
            if (!validateEnumValue("education_level", educationLevel, kEduLevels, callback)) {
                return;
            }
            const auto disciplineCategory =
                json->isMember("discipline_category") ? (*json)["discipline_category"].asString() : "";
            const auto enrollmentYear =
                json->isMember("enrollment_year") ? (*json)["enrollment_year"].asInt() : 0;
            const auto description =
                json->isMember("description") ? (*json)["description"].asString() : "";

            db->execSqlAsync(
                "INSERT INTO majors(college_id, code, name, education_level, discipline_category, "
                "enrollment_year, description) VALUES(?, ?, ?, ?, ?, ?, ?)",
                [callback, collegeId, code, name, educationLevel, disciplineCategory, enrollmentYear, description]
                (const drogon::orm::Result &result) {
                    Json::Value item;
                    item["id"] = static_cast<Json::UInt64>(result.insertId());
                    item["college_id"] = static_cast<Json::Int64>(collegeId);
                    item["code"] = code;
                    item["name"] = name;
                    item["education_level"] = educationLevel;
                    item["discipline_category"] = disciplineCategory;
                    item["enrollment_year"] = enrollmentYear;
                    item["description"] = description;
                    Json::Value body;
                    body["data"] = item;
                    callback(makeJson(body, drogon::k201Created));
                },
                [callback](const DrogonDbException &e) {
                    const std::string msg = e.base().what();
                    if (msg.find("foreign key constraint fails") != std::string::npos) {
                        sendError(callback, drogon::k400BadRequest, "college_id does not exist");
                        return;
                    }
                    if (msg.find("Duplicate") != std::string::npos) {
                        sendError(callback, drogon::k409Conflict, "duplicate major code or college-major name");
                        return;
                    }
                    sendDbError(e, callback);
                },
                collegeId,
                code,
                name,
                educationLevel,
                disciplineCategory,
                enrollmentYear,
                description);
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/majors/{1}",
        [](const HttpRequestPtr &req, DbCallback &&callback, long long id) {
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!requireFields(*json, {"college_id", "code", "name"}, callback)) {
                return;
            }
            const auto collegeId = (*json)["college_id"].asInt64();
            const auto code = (*json)["code"].asString();
            const auto name = (*json)["name"].asString();
            const auto educationLevel =
                json->isMember("education_level") ? (*json)["education_level"].asString() : "bachelor";
            static const std::unordered_set<std::string> kEduLevels = {"bachelor", "master", "phd"};
            if (!validateEnumValue("education_level", educationLevel, kEduLevels, callback)) {
                return;
            }
            const auto disciplineCategory =
                json->isMember("discipline_category") ? (*json)["discipline_category"].asString() : "";
            const auto enrollmentYear =
                json->isMember("enrollment_year") ? (*json)["enrollment_year"].asInt() : 0;
            const auto description =
                json->isMember("description") ? (*json)["description"].asString() : "";

            db->execSqlAsync(
                "UPDATE majors SET college_id = ?, code = ?, name = ?, education_level = ?, "
                "discipline_category = ?, enrollment_year = ?, description = ? WHERE id = ?",
                [callback, db, id](const drogon::orm::Result &result) {
                    if (result.affectedRows() == 0) {
                        sendError(callback, drogon::k404NotFound,
                                  "major not found: " + std::to_string(id));
                        return;
                    }
                    db->execSqlAsync(
                        "SELECT m.id, m.college_id, c.name AS college_name, m.code, m.name, "
                        "m.education_level, m.discipline_category, m.enrollment_year, m.description, "
                        "m.created_at, m.updated_at "
                        "FROM majors m INNER JOIN colleges c ON c.id = m.college_id WHERE m.id = ?",
                        [callback](const drogon::orm::Result &rowResult) {
                            Json::Value body;
                            body["data"] = majorRowToJson(rowResult[0]);
                            callback(makeJson(body));
                        },
                        [callback](const DrogonDbException &e) {
                            sendDbError(e, callback);
                        },
                        id);
                },
                [callback](const DrogonDbException &e) {
                    const std::string msg = e.base().what();
                    if (msg.find("foreign key constraint fails") != std::string::npos) {
                        sendError(callback, drogon::k400BadRequest, "college_id does not exist");
                        return;
                    }
                    if (msg.find("Duplicate") != std::string::npos) {
                        sendError(callback, drogon::k409Conflict, "duplicate major code or college-major name");
                        return;
                    }
                    sendDbError(e, callback);
                },
                collegeId,
                code,
                name,
                educationLevel,
                disciplineCategory,
                enrollmentYear,
                description,
                id);
        },
        {drogon::Put});

    drogon::app().registerHandler(
        "/api/majors/{1}",
        [](const HttpRequestPtr &, DbCallback &&callback, long long id) {
            auto db = drogon::app().getDbClient();
            db->execSqlAsync(
                "DELETE FROM majors WHERE id = ?",
                [callback, id](const drogon::orm::Result &result) {
                    if (result.affectedRows() == 0) {
                        sendError(callback, drogon::k404NotFound,
                                  "major not found: " + std::to_string(id));
                        return;
                    }
                    Json::Value body;
                    body["message"] = "deleted";
                    body["id"] = static_cast<Json::Int64>(id);
                    callback(makeJson(body));
                },
                [callback](const DrogonDbException &e) {
                    const std::string msg = e.base().what();
                    if (msg.find("foreign key constraint fails") != std::string::npos) {
                        sendError(callback, drogon::k409Conflict, "major is referenced by other data");
                        return;
                    }
                    sendDbError(e, callback);
                },
                id);
        },
        {drogon::Delete});

    // -------------------------
    // Major Profiles CRUD
    // -------------------------
    drogon::app().registerHandler(
        "/api/major-profiles/{1}",
        [](const HttpRequestPtr &, DbCallback &&callback, long long majorId) {
            auto db = drogon::app().getDbClient();
            db->execSqlAsync(
                "SELECT mp.id, mp.major_id, m.name AS major_name, mp.core_courses_text, "
                "mp.knowledge_skills_text, mp.technical_platform_text, mp.capability_text, "
                "mp.updated_by_user_id, mp.updated_at "
                "FROM major_profiles mp INNER JOIN majors m ON m.id = mp.major_id "
                "WHERE mp.major_id = ?",
                [callback, majorId](const drogon::orm::Result &result) {
                    if (result.empty()) {
                        sendError(callback, drogon::k404NotFound,
                                  "major profile not found for major_id: " + std::to_string(majorId));
                        return;
                    }
                    Json::Value body;
                    body["data"] = majorProfileRowToJson(result[0]);
                    callback(makeJson(body));
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                majorId);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/major-profiles/{1}",
        [](const HttpRequestPtr &req, DbCallback &&callback, long long majorId) {
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!requireFields(*json,
                               {"core_courses_text",
                                "knowledge_skills_text",
                                "technical_platform_text",
                                "capability_text"},
                               callback)) {
                return;
            }
            const auto coreCourses = (*json)["core_courses_text"].asString();
            const auto knowledgeSkills = (*json)["knowledge_skills_text"].asString();
            const auto technicalPlatform = (*json)["technical_platform_text"].asString();
            const auto capability = (*json)["capability_text"].asString();
            const bool hasUpdatedBy =
                json->isMember("updated_by_user_id") && !(*json)["updated_by_user_id"].isNull();
            const auto updatedBy =
                hasUpdatedBy ? (*json)["updated_by_user_id"].asInt64() : 0;

            auto onUpsertSuccess = [callback, db, majorId](const drogon::orm::Result &) {
                db->execSqlAsync(
                    "SELECT mp.id, mp.major_id, m.name AS major_name, mp.core_courses_text, "
                    "mp.knowledge_skills_text, mp.technical_platform_text, mp.capability_text, "
                    "mp.updated_by_user_id, mp.updated_at "
                    "FROM major_profiles mp INNER JOIN majors m ON m.id = mp.major_id "
                    "WHERE mp.major_id = ?",
                    [callback](const drogon::orm::Result &rowResult) {
                        Json::Value body;
                        body["data"] = majorProfileRowToJson(rowResult[0]);
                        callback(makeJson(body));
                    },
                    [callback](const DrogonDbException &e) {
                        sendDbError(e, callback);
                    },
                    majorId);
            };

            auto onUpsertError = [callback](const DrogonDbException &e) {
                const std::string msg = e.base().what();
                if (msg.find("foreign key constraint fails") != std::string::npos) {
                    sendError(callback, drogon::k400BadRequest, "major_id or updated_by_user_id does not exist");
                    return;
                }
                sendDbError(e, callback);
            };

            if (hasUpdatedBy) {
                db->execSqlAsync(
                    "INSERT INTO major_profiles "
                    "(major_id, core_courses_text, knowledge_skills_text, technical_platform_text, capability_text, updated_by_user_id) "
                    "VALUES(?, ?, ?, ?, ?, ?) "
                    "ON DUPLICATE KEY UPDATE "
                    "core_courses_text = VALUES(core_courses_text), "
                    "knowledge_skills_text = VALUES(knowledge_skills_text), "
                    "technical_platform_text = VALUES(technical_platform_text), "
                    "capability_text = VALUES(capability_text), "
                    "updated_by_user_id = VALUES(updated_by_user_id), "
                    "updated_at = CURRENT_TIMESTAMP",
                    onUpsertSuccess,
                    onUpsertError,
                    majorId,
                    coreCourses,
                    knowledgeSkills,
                    technicalPlatform,
                    capability,
                    updatedBy);
            } else {
                db->execSqlAsync(
                    "INSERT INTO major_profiles "
                    "(major_id, core_courses_text, knowledge_skills_text, technical_platform_text, capability_text, updated_by_user_id) "
                    "VALUES(?, ?, ?, ?, ?, NULL) "
                    "ON DUPLICATE KEY UPDATE "
                    "core_courses_text = VALUES(core_courses_text), "
                    "knowledge_skills_text = VALUES(knowledge_skills_text), "
                    "technical_platform_text = VALUES(technical_platform_text), "
                    "capability_text = VALUES(capability_text), "
                    "updated_by_user_id = NULL, "
                    "updated_at = CURRENT_TIMESTAMP",
                    onUpsertSuccess,
                    onUpsertError,
                    majorId,
                    coreCourses,
                    knowledgeSkills,
                    technicalPlatform,
                    capability);
            }
        },
        {drogon::Put});

    // -------------------------
    // Companies CRUD
    // -------------------------
    drogon::app().registerHandler(
        "/api/companies",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            int page = 1;
            int pageSize = 20;
            if (!parsePagingParams(req, callback, page, pageSize)) {
                return;
            }
            const int offset = (page - 1) * pageSize;
            const auto keywordRaw = req->getParameter("keyword");
            const auto like = "%" + keywordRaw + "%";
            const auto domainArea = req->getParameter("domain_area");
            const auto province = req->getParameter("province");
            const auto city = req->getParameter("city");
            const std::string sortOrder = normalizeSortOrder(req->getParameter("sort_order"));
            static const std::unordered_map<std::string, std::string> kSortMap = {
                {"id", "id"},
                {"name", "name"},
                {"domain_area", "domain_area"},
                {"province", "province"},
                {"city", "city"},
                {"created_at", "created_at"},
                {"updated_at", "updated_at"}};
            const std::string sortField =
                resolveSortField(req->getParameter("sort_by"), kSortMap, "id");
            const std::string whereSql =
                "WHERE (? = '' OR name LIKE ? OR main_business LIKE ? OR flagship_products LIKE ? OR "
                "domain_area LIKE ? OR application_industries LIKE ? OR description LIKE ?) "
                "AND (? = '' OR domain_area = ?) "
                "AND (? = '' OR province = ?) "
                "AND (? = '' OR city = ?) ";
            const std::string selectSql =
                "SELECT id, name, main_business, flagship_products, domain_area, "
                "application_industries, province, city, website, description, created_at, updated_at "
                "FROM companies ";

            db->execSqlAsync(
                "SELECT COUNT(*) AS total FROM companies " + whereSql,
                [callback, db, selectSql, whereSql, sortField, sortOrder,
                 page, pageSize, offset, keywordRaw, like, domainArea, province, city]
                (const drogon::orm::Result &countResult) {
                    const auto total = static_cast<Json::UInt64>(countResult[0]["total"].as<long long>());
                    db->execSqlAsync(
                        selectSql + whereSql + "ORDER BY " + sortField + " " + sortOrder + " LIMIT ? OFFSET ?",
                        [callback, total, page, pageSize](const drogon::orm::Result &result) {
                            Json::Value list(Json::arrayValue);
                            for (const auto &row : result) {
                                list.append(companyRowToJson(row));
                            }
                            Json::Value body;
                            body["data"] = list;
                            appendPagedMeta(body, total, page, pageSize);
                            callback(makeJson(body));
                        },
                        [callback](const DrogonDbException &e) {
                            sendDbError(e, callback);
                        },
                        keywordRaw,
                        like,
                        like,
                        like,
                        like,
                        like,
                        like,
                        domainArea,
                        domainArea,
                        province,
                        province,
                        city,
                        city,
                        pageSize,
                        offset);
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                keywordRaw,
                like,
                like,
                like,
                like,
                like,
                like,
                domainArea,
                domainArea,
                province,
                province,
                city,
                city);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/companies/{1}",
        [](const HttpRequestPtr &, DbCallback &&callback, long long id) {
            auto db = drogon::app().getDbClient();
            db->execSqlAsync(
                "SELECT id, name, main_business, flagship_products, domain_area, "
                "application_industries, province, city, website, description, created_at, updated_at "
                "FROM companies WHERE id = ?",
                [callback, id](const drogon::orm::Result &result) {
                    if (result.empty()) {
                        sendError(callback, drogon::k404NotFound,
                                  "company not found: " + std::to_string(id));
                        return;
                    }
                    Json::Value body;
                    body["data"] = companyRowToJson(result[0]);
                    callback(makeJson(body));
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                id);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/companies",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!requireFields(*json,
                               {"name",
                                "main_business",
                                "flagship_products",
                                "domain_area",
                                "application_industries"},
                               callback)) {
                return;
            }
            const auto name = (*json)["name"].asString();
            const auto mainBusiness = (*json)["main_business"].asString();
            const auto flagshipProducts = (*json)["flagship_products"].asString();
            const auto domainArea = (*json)["domain_area"].asString();
            const auto applicationIndustries = (*json)["application_industries"].asString();
            const auto province = json->isMember("province") ? (*json)["province"].asString() : "";
            const auto city = json->isMember("city") ? (*json)["city"].asString() : "";
            const auto website = json->isMember("website") ? (*json)["website"].asString() : "";
            const auto description = json->isMember("description") ? (*json)["description"].asString() : "";

            db->execSqlAsync(
                "INSERT INTO companies(name, main_business, flagship_products, domain_area, "
                "application_industries, province, city, website, description) "
                "VALUES(?, ?, ?, ?, ?, NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''))",
                [callback, name, mainBusiness, flagshipProducts, domainArea, applicationIndustries,
                 province, city, website, description](const drogon::orm::Result &result) {
                    Json::Value item;
                    item["id"] = static_cast<Json::UInt64>(result.insertId());
                    item["name"] = name;
                    item["main_business"] = mainBusiness;
                    item["flagship_products"] = flagshipProducts;
                    item["domain_area"] = domainArea;
                    item["application_industries"] = applicationIndustries;
                    item["province"] = province;
                    item["city"] = city;
                    item["website"] = website;
                    item["description"] = description;
                    Json::Value body;
                    body["data"] = item;
                    callback(makeJson(body, drogon::k201Created));
                },
                [callback](const DrogonDbException &e) {
                    const std::string msg = e.base().what();
                    if (msg.find("Duplicate") != std::string::npos) {
                        sendError(callback, drogon::k409Conflict, "duplicate company name");
                        return;
                    }
                    sendDbError(e, callback);
                },
                name,
                mainBusiness,
                flagshipProducts,
                domainArea,
                applicationIndustries,
                province,
                city,
                website,
                description);
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/companies/{1}",
        [](const HttpRequestPtr &req, DbCallback &&callback, long long id) {
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!requireFields(*json,
                               {"name",
                                "main_business",
                                "flagship_products",
                                "domain_area",
                                "application_industries"},
                               callback)) {
                return;
            }
            const auto name = (*json)["name"].asString();
            const auto mainBusiness = (*json)["main_business"].asString();
            const auto flagshipProducts = (*json)["flagship_products"].asString();
            const auto domainArea = (*json)["domain_area"].asString();
            const auto applicationIndustries = (*json)["application_industries"].asString();
            const auto province = json->isMember("province") ? (*json)["province"].asString() : "";
            const auto city = json->isMember("city") ? (*json)["city"].asString() : "";
            const auto website = json->isMember("website") ? (*json)["website"].asString() : "";
            const auto description = json->isMember("description") ? (*json)["description"].asString() : "";

            db->execSqlAsync(
                "UPDATE companies SET name = ?, main_business = ?, flagship_products = ?, domain_area = ?, "
                "application_industries = ?, province = NULLIF(?, ''), city = NULLIF(?, ''), "
                "website = NULLIF(?, ''), description = NULLIF(?, '') WHERE id = ?",
                [callback, db, id](const drogon::orm::Result &result) {
                    if (result.affectedRows() == 0) {
                        sendError(callback, drogon::k404NotFound,
                                  "company not found: " + std::to_string(id));
                        return;
                    }
                    db->execSqlAsync(
                        "SELECT id, name, main_business, flagship_products, domain_area, "
                        "application_industries, province, city, website, description, created_at, updated_at "
                        "FROM companies WHERE id = ?",
                        [callback](const drogon::orm::Result &rowResult) {
                            Json::Value body;
                            body["data"] = companyRowToJson(rowResult[0]);
                            callback(makeJson(body));
                        },
                        [callback](const DrogonDbException &e) {
                            sendDbError(e, callback);
                        },
                        id);
                },
                [callback](const DrogonDbException &e) {
                    const std::string msg = e.base().what();
                    if (msg.find("Duplicate") != std::string::npos) {
                        sendError(callback, drogon::k409Conflict, "duplicate company name");
                        return;
                    }
                    sendDbError(e, callback);
                },
                name,
                mainBusiness,
                flagshipProducts,
                domainArea,
                applicationIndustries,
                province,
                city,
                website,
                description,
                id);
        },
        {drogon::Put});

    drogon::app().registerHandler(
        "/api/companies/{1}",
        [](const HttpRequestPtr &, DbCallback &&callback, long long id) {
            auto db = drogon::app().getDbClient();
            db->execSqlAsync(
                "DELETE FROM companies WHERE id = ?",
                [callback, id](const drogon::orm::Result &result) {
                    if (result.affectedRows() == 0) {
                        sendError(callback, drogon::k404NotFound,
                                  "company not found: " + std::to_string(id));
                        return;
                    }
                    Json::Value body;
                    body["message"] = "deleted";
                    body["id"] = static_cast<Json::Int64>(id);
                    callback(makeJson(body));
                },
                [callback](const DrogonDbException &e) {
                    const std::string msg = e.base().what();
                    if (msg.find("foreign key constraint fails") != std::string::npos) {
                        sendError(callback, drogon::k409Conflict, "company is referenced by other data");
                        return;
                    }
                    sendDbError(e, callback);
                },
                id);
        },
        {drogon::Delete});

    // -------------------------
    // Jobs CRUD
    // -------------------------
    drogon::app().registerHandler(
        "/api/jobs",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            int page = 1;
            int pageSize = 20;
            if (!parsePagingParams(req, callback, page, pageSize)) {
                return;
            }
            const int offset = (page - 1) * pageSize;
            long long companyId = 0;
            if (!parseOptionalInt64Param(req, "company_id", callback, companyId)) {
                return;
            }
            const auto keywordRaw = req->getParameter("keyword");
            const auto like = "%" + keywordRaw + "%";
            const auto status = req->getParameter("status");
            const auto minDegree = req->getParameter("min_degree");
            const auto jobType = req->getParameter("job_type");
            static const std::unordered_set<std::string> kJobTypes = {"full_time", "internship", "contract"};
            static const std::unordered_set<std::string> kMinDegrees = {"none", "diploma", "bachelor", "master", "phd"};
            static const std::unordered_set<std::string> kJobStatus = {"open", "closed"};
            if ((!status.empty() && !validateEnumValue("status", status, kJobStatus, callback)) ||
                (!minDegree.empty() && !validateEnumValue("min_degree", minDegree, kMinDegrees, callback)) ||
                (!jobType.empty() && !validateEnumValue("job_type", jobType, kJobTypes, callback))) {
                return;
            }
            const std::string sortOrder = normalizeSortOrder(req->getParameter("sort_order"));
            static const std::unordered_map<std::string, std::string> kSortMap = {
                {"id", "j.id"},
                {"title", "j.title"},
                {"company_name", "c.name"},
                {"status", "j.status"},
                {"min_degree", "j.min_degree"},
                {"job_type", "j.job_type"},
                {"published_at", "j.published_at"},
                {"created_at", "j.created_at"},
                {"updated_at", "j.updated_at"}};
            const std::string sortField =
                resolveSortField(req->getParameter("sort_by"), kSortMap, "j.id");
            const std::string whereSql =
                "WHERE (? = '' OR j.title LIKE ? OR c.name LIKE ? OR j.responsibilities_text LIKE ? OR "
                "j.technical_projects_text LIKE ? OR j.required_knowledge_skills_text LIKE ? OR "
                "j.required_tools_platform_text LIKE ?) "
                "AND (? = 0 OR j.company_id = ?) "
                "AND (? = '' OR j.status = ?) "
                "AND (? = '' OR j.min_degree = ?) "
                "AND (? = '' OR j.job_type = ?) ";
            const std::string baseFromSql =
                "FROM jobs j INNER JOIN companies c ON c.id = j.company_id ";
            const std::string selectSql =
                "SELECT j.id, j.company_id, c.name AS company_name, j.title, j.job_type, "
                "j.location_province, j.location_city, j.min_degree, j.salary_range, "
                "j.responsibilities_text, j.technical_projects_text, j.required_knowledge_skills_text, "
                "j.required_tools_platform_text, j.status, j.published_at, j.created_at, j.updated_at ";

            db->execSqlAsync(
                "SELECT COUNT(*) AS total " + baseFromSql + whereSql,
                [callback, db, selectSql, baseFromSql, whereSql, sortField, sortOrder,
                 page, pageSize, offset, keywordRaw, like, companyId, status, minDegree, jobType]
                (const drogon::orm::Result &countResult) {
                    const auto total = static_cast<Json::UInt64>(countResult[0]["total"].as<long long>());
                    db->execSqlAsync(
                        selectSql + baseFromSql + whereSql + "ORDER BY " + sortField + " " + sortOrder +
                            " LIMIT ? OFFSET ?",
                        [callback, total, page, pageSize](const drogon::orm::Result &result) {
                            Json::Value list(Json::arrayValue);
                            for (const auto &row : result) {
                                list.append(jobRowToJson(row));
                            }
                            Json::Value body;
                            body["data"] = list;
                            appendPagedMeta(body, total, page, pageSize);
                            callback(makeJson(body));
                        },
                        [callback](const DrogonDbException &e) {
                            sendDbError(e, callback);
                        },
                        keywordRaw,
                        like,
                        like,
                        like,
                        like,
                        like,
                        like,
                        companyId,
                        companyId,
                        status,
                        status,
                        minDegree,
                        minDegree,
                        jobType,
                        jobType,
                        pageSize,
                        offset);
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                keywordRaw,
                like,
                like,
                like,
                like,
                like,
                like,
                companyId,
                companyId,
                status,
                status,
                minDegree,
                minDegree,
                jobType,
                jobType);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/jobs/{1}",
        [](const HttpRequestPtr &, DbCallback &&callback, long long id) {
            auto db = drogon::app().getDbClient();
            db->execSqlAsync(
                "SELECT j.id, j.company_id, c.name AS company_name, j.title, j.job_type, "
                "j.location_province, j.location_city, j.min_degree, j.salary_range, "
                "j.responsibilities_text, j.technical_projects_text, j.required_knowledge_skills_text, "
                "j.required_tools_platform_text, j.status, j.published_at, j.created_at, j.updated_at "
                "FROM jobs j INNER JOIN companies c ON c.id = j.company_id WHERE j.id = ?",
                [callback, id](const drogon::orm::Result &result) {
                    if (result.empty()) {
                        sendError(callback, drogon::k404NotFound,
                                  "job not found: " + std::to_string(id));
                        return;
                    }
                    Json::Value body;
                    body["data"] = jobRowToJson(result[0]);
                    callback(makeJson(body));
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                id);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/jobs",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!requireFields(*json,
                               {"company_id",
                                "title",
                                "responsibilities_text",
                                "technical_projects_text",
                                "required_knowledge_skills_text",
                                "required_tools_platform_text"},
                               callback)) {
                return;
            }
            const auto companyId = (*json)["company_id"].asInt64();
            const auto title = (*json)["title"].asString();
            const auto jobType = json->isMember("job_type") ? (*json)["job_type"].asString() : "full_time";
            const auto locationProvince =
                json->isMember("location_province") ? (*json)["location_province"].asString() : "";
            const auto locationCity =
                json->isMember("location_city") ? (*json)["location_city"].asString() : "";
            const auto minDegree = json->isMember("min_degree") ? (*json)["min_degree"].asString() : "bachelor";
            const auto salaryRange = json->isMember("salary_range") ? (*json)["salary_range"].asString() : "";
            const auto responsibilities = (*json)["responsibilities_text"].asString();
            const auto technicalProjects = (*json)["technical_projects_text"].asString();
            const auto requiredKnowledge = (*json)["required_knowledge_skills_text"].asString();
            const auto requiredTools = (*json)["required_tools_platform_text"].asString();
            const auto status = json->isMember("status") ? (*json)["status"].asString() : "open";
            static const std::unordered_set<std::string> kJobTypes = {"full_time", "internship", "contract"};
            static const std::unordered_set<std::string> kMinDegrees = {"none", "diploma", "bachelor", "master", "phd"};
            static const std::unordered_set<std::string> kJobStatus = {"open", "closed"};
            if (!validateEnumValue("job_type", jobType, kJobTypes, callback) ||
                !validateEnumValue("min_degree", minDegree, kMinDegrees, callback) ||
                !validateEnumValue("status", status, kJobStatus, callback)) {
                return;
            }
            const auto publishedAt = json->isMember("published_at") ? (*json)["published_at"].asString() : "";

            db->execSqlAsync(
                "INSERT INTO jobs(company_id, title, job_type, location_province, location_city, min_degree, "
                "salary_range, responsibilities_text, technical_projects_text, required_knowledge_skills_text, "
                "required_tools_platform_text, status, published_at) "
                "VALUES(?, ?, ?, NULLIF(?, ''), NULLIF(?, ''), ?, NULLIF(?, ''), ?, ?, ?, ?, ?, NULLIF(?, ''))",
                [callback, companyId, title, jobType, locationProvince, locationCity, minDegree, salaryRange,
                 responsibilities, technicalProjects, requiredKnowledge, requiredTools, status, publishedAt]
                (const drogon::orm::Result &result) {
                    Json::Value item;
                    item["id"] = static_cast<Json::UInt64>(result.insertId());
                    item["company_id"] = static_cast<Json::Int64>(companyId);
                    item["title"] = title;
                    item["job_type"] = jobType;
                    item["location_province"] = locationProvince;
                    item["location_city"] = locationCity;
                    item["min_degree"] = minDegree;
                    item["salary_range"] = salaryRange;
                    item["responsibilities_text"] = responsibilities;
                    item["technical_projects_text"] = technicalProjects;
                    item["required_knowledge_skills_text"] = requiredKnowledge;
                    item["required_tools_platform_text"] = requiredTools;
                    item["status"] = status;
                    item["published_at"] = publishedAt;
                    Json::Value body;
                    body["data"] = item;
                    callback(makeJson(body, drogon::k201Created));
                },
                [callback](const DrogonDbException &e) {
                    const std::string msg = e.base().what();
                    if (msg.find("foreign key constraint fails") != std::string::npos) {
                        sendError(callback, drogon::k400BadRequest, "company_id does not exist");
                        return;
                    }
                    sendDbError(e, callback);
                },
                companyId,
                title,
                jobType,
                locationProvince,
                locationCity,
                minDegree,
                salaryRange,
                responsibilities,
                technicalProjects,
                requiredKnowledge,
                requiredTools,
                status,
                publishedAt);
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/jobs/{1}",
        [](const HttpRequestPtr &req, DbCallback &&callback, long long id) {
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!requireFields(*json,
                               {"company_id",
                                "title",
                                "responsibilities_text",
                                "technical_projects_text",
                                "required_knowledge_skills_text",
                                "required_tools_platform_text"},
                               callback)) {
                return;
            }
            const auto companyId = (*json)["company_id"].asInt64();
            const auto title = (*json)["title"].asString();
            const auto jobType = json->isMember("job_type") ? (*json)["job_type"].asString() : "full_time";
            const auto locationProvince =
                json->isMember("location_province") ? (*json)["location_province"].asString() : "";
            const auto locationCity =
                json->isMember("location_city") ? (*json)["location_city"].asString() : "";
            const auto minDegree = json->isMember("min_degree") ? (*json)["min_degree"].asString() : "bachelor";
            const auto salaryRange = json->isMember("salary_range") ? (*json)["salary_range"].asString() : "";
            const auto responsibilities = (*json)["responsibilities_text"].asString();
            const auto technicalProjects = (*json)["technical_projects_text"].asString();
            const auto requiredKnowledge = (*json)["required_knowledge_skills_text"].asString();
            const auto requiredTools = (*json)["required_tools_platform_text"].asString();
            const auto status = json->isMember("status") ? (*json)["status"].asString() : "open";
            static const std::unordered_set<std::string> kJobTypes = {"full_time", "internship", "contract"};
            static const std::unordered_set<std::string> kMinDegrees = {"none", "diploma", "bachelor", "master", "phd"};
            static const std::unordered_set<std::string> kJobStatus = {"open", "closed"};
            if (!validateEnumValue("job_type", jobType, kJobTypes, callback) ||
                !validateEnumValue("min_degree", minDegree, kMinDegrees, callback) ||
                !validateEnumValue("status", status, kJobStatus, callback)) {
                return;
            }
            const auto publishedAt = json->isMember("published_at") ? (*json)["published_at"].asString() : "";

            db->execSqlAsync(
                "UPDATE jobs SET company_id = ?, title = ?, job_type = ?, location_province = NULLIF(?, ''), "
                "location_city = NULLIF(?, ''), min_degree = ?, salary_range = NULLIF(?, ''), "
                "responsibilities_text = ?, technical_projects_text = ?, required_knowledge_skills_text = ?, "
                "required_tools_platform_text = ?, status = ?, published_at = NULLIF(?, '') WHERE id = ?",
                [callback, db, id](const drogon::orm::Result &result) {
                    if (result.affectedRows() == 0) {
                        sendError(callback, drogon::k404NotFound,
                                  "job not found: " + std::to_string(id));
                        return;
                    }
                    db->execSqlAsync(
                        "SELECT j.id, j.company_id, c.name AS company_name, j.title, j.job_type, "
                        "j.location_province, j.location_city, j.min_degree, j.salary_range, "
                        "j.responsibilities_text, j.technical_projects_text, j.required_knowledge_skills_text, "
                        "j.required_tools_platform_text, j.status, j.published_at, j.created_at, j.updated_at "
                        "FROM jobs j INNER JOIN companies c ON c.id = j.company_id WHERE j.id = ?",
                        [callback](const drogon::orm::Result &rowResult) {
                            Json::Value body;
                            body["data"] = jobRowToJson(rowResult[0]);
                            callback(makeJson(body));
                        },
                        [callback](const DrogonDbException &e) {
                            sendDbError(e, callback);
                        },
                        id);
                },
                [callback](const DrogonDbException &e) {
                    const std::string msg = e.base().what();
                    if (msg.find("foreign key constraint fails") != std::string::npos) {
                        sendError(callback, drogon::k400BadRequest, "company_id does not exist");
                        return;
                    }
                    sendDbError(e, callback);
                },
                companyId,
                title,
                jobType,
                locationProvince,
                locationCity,
                minDegree,
                salaryRange,
                responsibilities,
                technicalProjects,
                requiredKnowledge,
                requiredTools,
                status,
                publishedAt,
                id);
        },
        {drogon::Put});

    drogon::app().registerHandler(
        "/api/jobs/{1}",
        [](const HttpRequestPtr &, DbCallback &&callback, long long id) {
            auto db = drogon::app().getDbClient();
            db->execSqlAsync(
                "DELETE FROM jobs WHERE id = ?",
                [callback, id](const drogon::orm::Result &result) {
                    if (result.affectedRows() == 0) {
                        sendError(callback, drogon::k404NotFound,
                                  "job not found: " + std::to_string(id));
                        return;
                    }
                    Json::Value body;
                    body["message"] = "deleted";
                    body["id"] = static_cast<Json::Int64>(id);
                    callback(makeJson(body));
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                id);
        },
        {drogon::Delete});

    // -------------------------
    // Students CRUD
    // -------------------------
    drogon::app().registerHandler(
        "/api/students",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            int page = 1;
            int pageSize = 20;
            if (!parsePagingParams(req, callback, page, pageSize)) {
                return;
            }
            const int offset = (page - 1) * pageSize;
            long long collegeId = 0;
            long long majorId = 0;
            long long gradYear = 0;
            if (!parseOptionalInt64Param(req, "college_id", callback, collegeId)) {
                return;
            }
            if (!parseOptionalInt64Param(req, "major_id", callback, majorId)) {
                return;
            }
            if (!parseOptionalInt64Param(req, "graduation_year", callback, gradYear)) {
                return;
            }
            const auto keywordRaw = req->getParameter("keyword");
            const auto like = "%" + keywordRaw + "%";
            const std::string sortOrder = normalizeSortOrder(req->getParameter("sort_order"));
            static const std::unordered_map<std::string, std::string> kSortMap = {
                {"id", "s.id"},
                {"student_no", "s.student_no"},
                {"full_name", "s.full_name"},
                {"college_name", "c.name"},
                {"major_name", "m.name"},
                {"graduation_year", "s.graduation_year"},
                {"created_at", "s.created_at"},
                {"updated_at", "s.updated_at"}};
            const std::string sortField =
                resolveSortField(req->getParameter("sort_by"), kSortMap, "s.id");
            const std::string whereSql =
                "WHERE (? = '' OR s.student_no LIKE ? OR s.full_name LIKE ? OR s.email LIKE ? OR s.phone LIKE ?) "
                "AND (? = 0 OR s.college_id = ?) "
                "AND (? = 0 OR s.major_id = ?) "
                "AND (? = 0 OR s.graduation_year = ?) ";
            const std::string baseFromSql =
                "FROM students s "
                "INNER JOIN colleges c ON c.id = s.college_id "
                "INNER JOIN majors m ON m.id = s.major_id ";
            const std::string selectSql =
                "SELECT s.id, s.student_no, s.full_name, s.gender, s.college_id, c.name AS college_name, "
                "s.major_id, m.name AS major_name, s.graduation_year, s.email, s.phone, s.resume_text, "
                "s.technical_projects_text, s.capability_summary_text, s.created_at, s.updated_at ";

            db->execSqlAsync(
                "SELECT COUNT(*) AS total " + baseFromSql + whereSql,
                [callback, db, selectSql, baseFromSql, whereSql, sortField, sortOrder,
                 page, pageSize, offset, keywordRaw, like, collegeId, majorId, gradYear]
                (const drogon::orm::Result &countResult) {
                    const auto total = static_cast<Json::UInt64>(countResult[0]["total"].as<long long>());
                    db->execSqlAsync(
                        selectSql + baseFromSql + whereSql + "ORDER BY " + sortField + " " + sortOrder +
                            " LIMIT ? OFFSET ?",
                        [callback, total, page, pageSize](const drogon::orm::Result &result) {
                            Json::Value list(Json::arrayValue);
                            for (const auto &row : result) {
                                list.append(studentRowToJson(row));
                            }
                            Json::Value body;
                            body["data"] = list;
                            appendPagedMeta(body, total, page, pageSize);
                            callback(makeJson(body));
                        },
                        [callback](const DrogonDbException &e) {
                            sendDbError(e, callback);
                        },
                        keywordRaw,
                        like,
                        like,
                        like,
                        like,
                        collegeId,
                        collegeId,
                        majorId,
                        majorId,
                        gradYear,
                        gradYear,
                        pageSize,
                        offset);
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                keywordRaw,
                like,
                like,
                like,
                like,
                collegeId,
                collegeId,
                majorId,
                majorId,
                gradYear,
                gradYear);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/students/{1}",
        [](const HttpRequestPtr &, DbCallback &&callback, long long id) {
            auto db = drogon::app().getDbClient();
            db->execSqlAsync(
                "SELECT s.id, s.student_no, s.full_name, s.gender, s.college_id, c.name AS college_name, "
                "s.major_id, m.name AS major_name, s.graduation_year, s.email, s.phone, s.resume_text, "
                "s.technical_projects_text, s.capability_summary_text, s.created_at, s.updated_at "
                "FROM students s "
                "INNER JOIN colleges c ON c.id = s.college_id "
                "INNER JOIN majors m ON m.id = s.major_id "
                "WHERE s.id = ?",
                [callback, id](const drogon::orm::Result &result) {
                    if (result.empty()) {
                        sendError(callback, drogon::k404NotFound,
                                  "student not found: " + std::to_string(id));
                        return;
                    }
                    Json::Value body;
                    body["data"] = studentRowToJson(result[0]);
                    callback(makeJson(body));
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                id);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/students",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!requireFields(*json,
                               {"student_no", "full_name", "college_id", "major_id", "graduation_year"},
                               callback)) {
                return;
            }
            const auto studentNo = (*json)["student_no"].asString();
            const auto fullName = (*json)["full_name"].asString();
            const auto gender = json->isMember("gender") ? (*json)["gender"].asString() : "unknown";
            static const std::unordered_set<std::string> kGenderValues = {"male", "female", "other", "unknown"};
            if (!validateEnumValue("gender", gender, kGenderValues, callback)) {
                return;
            }
            const auto collegeId = (*json)["college_id"].asInt64();
            const auto majorId = (*json)["major_id"].asInt64();
            const auto graduationYear = (*json)["graduation_year"].asInt();
            const auto email = json->isMember("email") ? (*json)["email"].asString() : "";
            const auto phone = json->isMember("phone") ? (*json)["phone"].asString() : "";
            const auto resumeText = json->isMember("resume_text") ? (*json)["resume_text"].asString() : "";
            const auto technicalProjects =
                json->isMember("technical_projects_text") ? (*json)["technical_projects_text"].asString() : "";
            const auto capabilitySummary =
                json->isMember("capability_summary_text") ? (*json)["capability_summary_text"].asString() : "";

            db->execSqlAsync(
                "INSERT INTO students(student_no, full_name, gender, college_id, major_id, graduation_year, "
                "email, phone, resume_text, technical_projects_text, capability_summary_text) "
                "VALUES(?, ?, ?, ?, ?, ?, NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''))",
                [callback, studentNo, fullName, gender, collegeId, majorId, graduationYear, email,
                 phone, resumeText, technicalProjects, capabilitySummary](const drogon::orm::Result &result) {
                    Json::Value item;
                    item["id"] = static_cast<Json::UInt64>(result.insertId());
                    item["student_no"] = studentNo;
                    item["full_name"] = fullName;
                    item["gender"] = gender;
                    item["college_id"] = static_cast<Json::Int64>(collegeId);
                    item["major_id"] = static_cast<Json::Int64>(majorId);
                    item["graduation_year"] = graduationYear;
                    item["email"] = email;
                    item["phone"] = phone;
                    item["resume_text"] = resumeText;
                    item["technical_projects_text"] = technicalProjects;
                    item["capability_summary_text"] = capabilitySummary;
                    Json::Value body;
                    body["data"] = item;
                    callback(makeJson(body, drogon::k201Created));
                },
                [callback](const DrogonDbException &e) {
                    const std::string msg = e.base().what();
                    if (msg.find("Duplicate") != std::string::npos) {
                        sendError(callback, drogon::k409Conflict, "duplicate student_no");
                        return;
                    }
                    if (msg.find("foreign key constraint fails") != std::string::npos) {
                        sendError(callback, drogon::k400BadRequest, "college_id or major_id does not exist");
                        return;
                    }
                    if (msg.find("Data truncated") != std::string::npos) {
                        sendError(callback, drogon::k400BadRequest, "invalid enum value in gender");
                        return;
                    }
                    sendDbError(e, callback);
                },
                studentNo,
                fullName,
                gender,
                collegeId,
                majorId,
                graduationYear,
                email,
                phone,
                resumeText,
                technicalProjects,
                capabilitySummary);
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/students/{1}",
        [](const HttpRequestPtr &req, DbCallback &&callback, long long id) {
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!requireFields(*json,
                               {"student_no", "full_name", "college_id", "major_id", "graduation_year"},
                               callback)) {
                return;
            }
            const auto studentNo = (*json)["student_no"].asString();
            const auto fullName = (*json)["full_name"].asString();
            const auto gender = json->isMember("gender") ? (*json)["gender"].asString() : "unknown";
            static const std::unordered_set<std::string> kGenderValues = {"male", "female", "other", "unknown"};
            if (!validateEnumValue("gender", gender, kGenderValues, callback)) {
                return;
            }
            const auto collegeId = (*json)["college_id"].asInt64();
            const auto majorId = (*json)["major_id"].asInt64();
            const auto graduationYear = (*json)["graduation_year"].asInt();
            const auto email = json->isMember("email") ? (*json)["email"].asString() : "";
            const auto phone = json->isMember("phone") ? (*json)["phone"].asString() : "";
            const auto resumeText = json->isMember("resume_text") ? (*json)["resume_text"].asString() : "";
            const auto technicalProjects =
                json->isMember("technical_projects_text") ? (*json)["technical_projects_text"].asString() : "";
            const auto capabilitySummary =
                json->isMember("capability_summary_text") ? (*json)["capability_summary_text"].asString() : "";

            db->execSqlAsync(
                "UPDATE students SET student_no = ?, full_name = ?, gender = ?, college_id = ?, major_id = ?, "
                "graduation_year = ?, email = NULLIF(?, ''), phone = NULLIF(?, ''), resume_text = NULLIF(?, ''), "
                "technical_projects_text = NULLIF(?, ''), capability_summary_text = NULLIF(?, '') WHERE id = ?",
                [callback, db, id](const drogon::orm::Result &result) {
                    if (result.affectedRows() == 0) {
                        sendError(callback, drogon::k404NotFound,
                                  "student not found: " + std::to_string(id));
                        return;
                    }
                    db->execSqlAsync(
                        "SELECT s.id, s.student_no, s.full_name, s.gender, s.college_id, c.name AS college_name, "
                        "s.major_id, m.name AS major_name, s.graduation_year, s.email, s.phone, s.resume_text, "
                        "s.technical_projects_text, s.capability_summary_text, s.created_at, s.updated_at "
                        "FROM students s "
                        "INNER JOIN colleges c ON c.id = s.college_id "
                        "INNER JOIN majors m ON m.id = s.major_id "
                        "WHERE s.id = ?",
                        [callback](const drogon::orm::Result &rowResult) {
                            Json::Value body;
                            body["data"] = studentRowToJson(rowResult[0]);
                            callback(makeJson(body));
                        },
                        [callback](const DrogonDbException &e) {
                            sendDbError(e, callback);
                        },
                        id);
                },
                [callback](const DrogonDbException &e) {
                    const std::string msg = e.base().what();
                    if (msg.find("Duplicate") != std::string::npos) {
                        sendError(callback, drogon::k409Conflict, "duplicate student_no");
                        return;
                    }
                    if (msg.find("foreign key constraint fails") != std::string::npos) {
                        sendError(callback, drogon::k400BadRequest, "college_id or major_id does not exist");
                        return;
                    }
                    if (msg.find("Data truncated") != std::string::npos) {
                        sendError(callback, drogon::k400BadRequest, "invalid enum value in gender");
                        return;
                    }
                    sendDbError(e, callback);
                },
                studentNo,
                fullName,
                gender,
                collegeId,
                majorId,
                graduationYear,
                email,
                phone,
                resumeText,
                technicalProjects,
                capabilitySummary,
                id);
        },
        {drogon::Put});

    drogon::app().registerHandler(
        "/api/students/{1}",
        [](const HttpRequestPtr &, DbCallback &&callback, long long id) {
            auto db = drogon::app().getDbClient();
            db->execSqlAsync(
                "DELETE FROM students WHERE id = ?",
                [callback, id](const drogon::orm::Result &result) {
                    if (result.affectedRows() == 0) {
                        sendError(callback, drogon::k404NotFound,
                                  "student not found: " + std::to_string(id));
                        return;
                    }
                    Json::Value body;
                    body["message"] = "deleted";
                    body["id"] = static_cast<Json::Int64>(id);
                    callback(makeJson(body));
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                id);
        },
        {drogon::Delete});

    // -------------------------
    // Employment Records CRUD
    // -------------------------
    drogon::app().registerHandler(
        "/api/employment-records",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            int page = 1;
            int pageSize = 20;
            if (!parsePagingParams(req, callback, page, pageSize)) {
                return;
            }
            const int offset = (page - 1) * pageSize;
            long long studentId = 0;
            long long companyId = 0;
            long long jobId = 0;
            if (!parseOptionalInt64Param(req, "student_id", callback, studentId)) {
                return;
            }
            if (!parseOptionalInt64Param(req, "company_id", callback, companyId)) {
                return;
            }
            if (!parseOptionalInt64Param(req, "job_id", callback, jobId)) {
                return;
            }
            const auto keywordRaw = req->getParameter("keyword");
            const auto like = "%" + keywordRaw + "%";
            const auto employmentType = req->getParameter("employment_type");
            const auto employmentStatus = req->getParameter("employment_status");
            static const std::unordered_set<std::string> kEmploymentTypes = {"signed", "offer", "internship", "pending"};
            static const std::unordered_set<std::string> kEmploymentStatus = {"active", "left", "unknown"};
            if ((!employmentType.empty() &&
                 !validateEnumValue("employment_type", employmentType, kEmploymentTypes, callback)) ||
                (!employmentStatus.empty() &&
                 !validateEnumValue("employment_status", employmentStatus, kEmploymentStatus, callback))) {
                return;
            }
            const std::string sortOrder = normalizeSortOrder(req->getParameter("sort_order"));
            static const std::unordered_map<std::string, std::string> kSortMap = {
                {"id", "er.id"},
                {"student_name", "s.full_name"},
                {"company_name", "c.name"},
                {"job_title", "j.title"},
                {"employment_type", "er.employment_type"},
                {"employment_status", "er.employment_status"},
                {"start_date", "er.start_date"},
                {"created_at", "er.created_at"},
                {"updated_at", "er.updated_at"}};
            const std::string sortField =
                resolveSortField(req->getParameter("sort_by"), kSortMap, "er.id");
            const std::string whereSql =
                "WHERE (? = '' OR s.full_name LIKE ? OR c.name LIKE ? OR j.title LIKE ? OR "
                "er.evidence_source LIKE ? OR er.notes LIKE ?) "
                "AND (? = 0 OR er.student_id = ?) "
                "AND (? = 0 OR er.company_id = ?) "
                "AND (? = 0 OR er.job_id = ?) "
                "AND (? = '' OR er.employment_type = ?) "
                "AND (? = '' OR er.employment_status = ?) ";
            const std::string baseFromSql =
                "FROM employment_records er "
                "INNER JOIN students s ON s.id = er.student_id "
                "INNER JOIN companies c ON c.id = er.company_id "
                "LEFT JOIN jobs j ON j.id = er.job_id ";
            const std::string selectSql =
                "SELECT er.id, er.student_id, s.full_name AS student_name, er.company_id, c.name AS company_name, "
                "er.job_id, j.title AS job_title, er.employment_type, er.employment_status, er.start_date, "
                "er.end_date, er.evidence_source, er.notes, er.created_at, er.updated_at ";

            db->execSqlAsync(
                "SELECT COUNT(*) AS total " + baseFromSql + whereSql,
                [callback, db, selectSql, baseFromSql, whereSql, sortField, sortOrder,
                 page, pageSize, offset, keywordRaw, like, studentId, companyId, jobId,
                 employmentType, employmentStatus]
                (const drogon::orm::Result &countResult) {
                    const auto total = static_cast<Json::UInt64>(countResult[0]["total"].as<long long>());
                    db->execSqlAsync(
                        selectSql + baseFromSql + whereSql + "ORDER BY " + sortField + " " + sortOrder +
                            " LIMIT ? OFFSET ?",
                        [callback, total, page, pageSize](const drogon::orm::Result &result) {
                            Json::Value list(Json::arrayValue);
                            for (const auto &row : result) {
                                list.append(employmentRowToJson(row));
                            }
                            Json::Value body;
                            body["data"] = list;
                            appendPagedMeta(body, total, page, pageSize);
                            callback(makeJson(body));
                        },
                        [callback](const DrogonDbException &e) {
                            sendDbError(e, callback);
                        },
                        keywordRaw,
                        like,
                        like,
                        like,
                        like,
                        like,
                        studentId,
                        studentId,
                        companyId,
                        companyId,
                        jobId,
                        jobId,
                        employmentType,
                        employmentType,
                        employmentStatus,
                        employmentStatus,
                        pageSize,
                        offset);
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                keywordRaw,
                like,
                like,
                like,
                like,
                like,
                studentId,
                studentId,
                companyId,
                companyId,
                jobId,
                jobId,
                employmentType,
                employmentType,
                employmentStatus,
                employmentStatus);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/employment-records/{1}",
        [](const HttpRequestPtr &, DbCallback &&callback, long long id) {
            auto db = drogon::app().getDbClient();
            db->execSqlAsync(
                "SELECT er.id, er.student_id, s.full_name AS student_name, er.company_id, c.name AS company_name, "
                "er.job_id, j.title AS job_title, er.employment_type, er.employment_status, er.start_date, "
                "er.end_date, er.evidence_source, er.notes, er.created_at, er.updated_at "
                "FROM employment_records er "
                "INNER JOIN students s ON s.id = er.student_id "
                "INNER JOIN companies c ON c.id = er.company_id "
                "LEFT JOIN jobs j ON j.id = er.job_id "
                "WHERE er.id = ?",
                [callback, id](const drogon::orm::Result &result) {
                    if (result.empty()) {
                        sendError(callback, drogon::k404NotFound,
                                  "employment record not found: " + std::to_string(id));
                        return;
                    }
                    Json::Value body;
                    body["data"] = employmentRowToJson(result[0]);
                    callback(makeJson(body));
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                id);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/employment-records",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!requireFields(*json, {"student_id", "company_id"}, callback)) {
                return;
            }
            const auto studentId = (*json)["student_id"].asInt64();
            const auto companyId = (*json)["company_id"].asInt64();
            const bool hasJobId = json->isMember("job_id") && !(*json)["job_id"].isNull();
            const auto jobId = hasJobId ? (*json)["job_id"].asInt64() : 0;
            const auto employmentType =
                json->isMember("employment_type") ? (*json)["employment_type"].asString() : "signed";
            const auto employmentStatus =
                json->isMember("employment_status") ? (*json)["employment_status"].asString() : "active";
            static const std::unordered_set<std::string> kEmploymentTypes = {"signed", "offer", "internship", "pending"};
            static const std::unordered_set<std::string> kEmploymentStatus = {"active", "left", "unknown"};
            if (!validateEnumValue("employment_type", employmentType, kEmploymentTypes, callback) ||
                !validateEnumValue("employment_status", employmentStatus, kEmploymentStatus, callback)) {
                return;
            }
            const auto startDate = json->isMember("start_date") ? (*json)["start_date"].asString() : "";
            const auto endDate = json->isMember("end_date") ? (*json)["end_date"].asString() : "";
            const auto evidenceSource =
                json->isMember("evidence_source") ? (*json)["evidence_source"].asString() : "";
            const auto notes = json->isMember("notes") ? (*json)["notes"].asString() : "";

            auto onInsertSuccess = [callback, db](const drogon::orm::Result &result) {
                const auto newId = static_cast<long long>(result.insertId());
                db->execSqlAsync(
                    "SELECT er.id, er.student_id, s.full_name AS student_name, er.company_id, c.name AS company_name, "
                    "er.job_id, j.title AS job_title, er.employment_type, er.employment_status, er.start_date, "
                    "er.end_date, er.evidence_source, er.notes, er.created_at, er.updated_at "
                    "FROM employment_records er "
                    "INNER JOIN students s ON s.id = er.student_id "
                    "INNER JOIN companies c ON c.id = er.company_id "
                    "LEFT JOIN jobs j ON j.id = er.job_id "
                    "WHERE er.id = ?",
                    [callback](const drogon::orm::Result &rowResult) {
                        Json::Value body;
                        body["data"] = employmentRowToJson(rowResult[0]);
                        callback(makeJson(body, drogon::k201Created));
                    },
                    [callback](const DrogonDbException &e) {
                        sendDbError(e, callback);
                    },
                    newId);
            };

            auto onInsertError = [callback](const DrogonDbException &e) {
                const std::string msg = e.base().what();
                if (msg.find("foreign key constraint fails") != std::string::npos) {
                    sendError(callback, drogon::k400BadRequest, "student_id/company_id/job_id does not exist");
                    return;
                }
                if (msg.find("Data truncated") != std::string::npos) {
                    sendError(callback, drogon::k400BadRequest, "invalid enum or date format");
                    return;
                }
                sendDbError(e, callback);
            };

            if (hasJobId) {
                db->execSqlAsync(
                    "INSERT INTO employment_records(student_id, company_id, job_id, employment_type, employment_status, "
                    "start_date, end_date, evidence_source, notes) "
                    "VALUES(?, ?, ?, ?, ?, NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''))",
                    onInsertSuccess,
                    onInsertError,
                    studentId,
                    companyId,
                    jobId,
                    employmentType,
                    employmentStatus,
                    startDate,
                    endDate,
                    evidenceSource,
                    notes);
            } else {
                db->execSqlAsync(
                    "INSERT INTO employment_records(student_id, company_id, job_id, employment_type, employment_status, "
                    "start_date, end_date, evidence_source, notes) "
                    "VALUES(?, ?, NULL, ?, ?, NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''))",
                    onInsertSuccess,
                    onInsertError,
                    studentId,
                    companyId,
                    employmentType,
                    employmentStatus,
                    startDate,
                    endDate,
                    evidenceSource,
                    notes);
            }
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/employment-records/{1}",
        [](const HttpRequestPtr &req, DbCallback &&callback, long long id) {
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!requireFields(*json, {"student_id", "company_id"}, callback)) {
                return;
            }
            const auto studentId = (*json)["student_id"].asInt64();
            const auto companyId = (*json)["company_id"].asInt64();
            const bool hasJobId = json->isMember("job_id") && !(*json)["job_id"].isNull();
            const auto jobId = hasJobId ? (*json)["job_id"].asInt64() : 0;
            const auto employmentType =
                json->isMember("employment_type") ? (*json)["employment_type"].asString() : "signed";
            const auto employmentStatus =
                json->isMember("employment_status") ? (*json)["employment_status"].asString() : "active";
            static const std::unordered_set<std::string> kEmploymentTypes = {"signed", "offer", "internship", "pending"};
            static const std::unordered_set<std::string> kEmploymentStatus = {"active", "left", "unknown"};
            if (!validateEnumValue("employment_type", employmentType, kEmploymentTypes, callback) ||
                !validateEnumValue("employment_status", employmentStatus, kEmploymentStatus, callback)) {
                return;
            }
            const auto startDate = json->isMember("start_date") ? (*json)["start_date"].asString() : "";
            const auto endDate = json->isMember("end_date") ? (*json)["end_date"].asString() : "";
            const auto evidenceSource =
                json->isMember("evidence_source") ? (*json)["evidence_source"].asString() : "";
            const auto notes = json->isMember("notes") ? (*json)["notes"].asString() : "";

            auto onUpdateSuccess = [callback, db, id](const drogon::orm::Result &result) {
                if (result.affectedRows() == 0) {
                    sendError(callback, drogon::k404NotFound,
                              "employment record not found: " + std::to_string(id));
                    return;
                }
                db->execSqlAsync(
                    "SELECT er.id, er.student_id, s.full_name AS student_name, er.company_id, c.name AS company_name, "
                    "er.job_id, j.title AS job_title, er.employment_type, er.employment_status, er.start_date, "
                    "er.end_date, er.evidence_source, er.notes, er.created_at, er.updated_at "
                    "FROM employment_records er "
                    "INNER JOIN students s ON s.id = er.student_id "
                    "INNER JOIN companies c ON c.id = er.company_id "
                    "LEFT JOIN jobs j ON j.id = er.job_id "
                    "WHERE er.id = ?",
                    [callback](const drogon::orm::Result &rowResult) {
                        Json::Value body;
                        body["data"] = employmentRowToJson(rowResult[0]);
                        callback(makeJson(body));
                    },
                    [callback](const DrogonDbException &e) {
                        sendDbError(e, callback);
                    },
                    id);
            };

            auto onUpdateError = [callback](const DrogonDbException &e) {
                const std::string msg = e.base().what();
                if (msg.find("foreign key constraint fails") != std::string::npos) {
                    sendError(callback, drogon::k400BadRequest, "student_id/company_id/job_id does not exist");
                    return;
                }
                if (msg.find("Data truncated") != std::string::npos) {
                    sendError(callback, drogon::k400BadRequest, "invalid enum or date format");
                    return;
                }
                sendDbError(e, callback);
            };

            if (hasJobId) {
                db->execSqlAsync(
                    "UPDATE employment_records SET student_id = ?, company_id = ?, job_id = ?, employment_type = ?, "
                    "employment_status = ?, start_date = NULLIF(?, ''), end_date = NULLIF(?, ''), "
                    "evidence_source = NULLIF(?, ''), notes = NULLIF(?, '') WHERE id = ?",
                    onUpdateSuccess,
                    onUpdateError,
                    studentId,
                    companyId,
                    jobId,
                    employmentType,
                    employmentStatus,
                    startDate,
                    endDate,
                    evidenceSource,
                    notes,
                    id);
            } else {
                db->execSqlAsync(
                    "UPDATE employment_records SET student_id = ?, company_id = ?, job_id = NULL, employment_type = ?, "
                    "employment_status = ?, start_date = NULLIF(?, ''), end_date = NULLIF(?, ''), "
                    "evidence_source = NULLIF(?, ''), notes = NULLIF(?, '') WHERE id = ?",
                    onUpdateSuccess,
                    onUpdateError,
                    studentId,
                    companyId,
                    employmentType,
                    employmentStatus,
                    startDate,
                    endDate,
                    evidenceSource,
                    notes,
                    id);
            }
        },
        {drogon::Put});

    drogon::app().registerHandler(
        "/api/employment-records/{1}",
        [](const HttpRequestPtr &, DbCallback &&callback, long long id) {
            auto db = drogon::app().getDbClient();
            db->execSqlAsync(
                "DELETE FROM employment_records WHERE id = ?",
                [callback, id](const drogon::orm::Result &result) {
                    if (result.affectedRows() == 0) {
                        sendError(callback, drogon::k404NotFound,
                                  "employment record not found: " + std::to_string(id));
                        return;
                    }
                    Json::Value body;
                    body["message"] = "deleted";
                    body["id"] = static_cast<Json::Int64>(id);
                    callback(makeJson(body));
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                id);
        },
        {drogon::Delete});

    // -------------------------
    // Matching Engine (Baseline)
    // -------------------------
    drogon::app().registerHandler(
        "/api/match/single",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!requireFields(*json, {"student_id", "job_id"}, callback)) {
                return;
            }

            const auto studentId = (*json)["student_id"].asInt64();
            const auto jobId = (*json)["job_id"].asInt64();
            if (studentId <= 0 || jobId <= 0) {
                sendError(callback, drogon::k400BadRequest, "student_id and job_id must be positive integers");
                return;
            }

            const auto algorithmVersion =
                json->isMember("algorithm_version") ? (*json)["algorithm_version"].asString() : "v0.1-baseline";
            const bool persist = !json->isMember("persist") || (*json)["persist"].asBool();
            const auto preferredProvince =
                json->isMember("preferred_location_province")
                    ? (*json)["preferred_location_province"].asString()
                    : "";
            const auto preferredCity =
                json->isMember("preferred_location_city")
                    ? (*json)["preferred_location_city"].asString()
                    : "";

            auto match = computeBaselineMatch(
                db, studentId, jobId, algorithmVersion, preferredProvince, preferredCity);
            if (!match.ok) {
                sendError(callback, match.status, match.message);
                return;
            }

            if (persist) {
                std::string persistError;
                if (!persistMatchResult(db, match, persistError)) {
                    sendError(callback, drogon::k500InternalServerError, persistError, "database_error");
                    return;
                }
            }

            Json::Value item;
            item["student_id"] = static_cast<Json::Int64>(match.studentId);
            item["major_id"] = static_cast<Json::Int64>(match.majorId);
            item["job_id"] = static_cast<Json::Int64>(match.jobId);
            item["algorithm_version"] = match.algorithmVersion;
            item["semantic_score"] = match.semanticScore;
            item["skill_score"] = match.skillScore;
            item["constraint_score"] = match.constraintScore;
            item["final_score"] = match.finalScore;
            item["matched_skills"] = match.matchedSkills;
            item["missing_skills"] = match.missingSkills;
            item["constraint_failures"] = match.constraintFailures;
            item["persisted"] = persist;
            if (persist) {
                item["match_result_id"] = static_cast<Json::Int64>(match.matchResultId);
            }

            Json::Value body;
            body["data"] = item;
            callback(makeJson(body, persist ? drogon::k201Created : drogon::k200OK));
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/match/batch",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!json->isMember("student_ids") || !(*json)["student_ids"].isArray() ||
                !json->isMember("job_ids") || !(*json)["job_ids"].isArray()) {
                sendError(callback, drogon::k400BadRequest, "student_ids and job_ids must be JSON arrays");
                return;
            }

            std::vector<long long> studentIds;
            for (const auto &v : (*json)["student_ids"]) {
                if (!v.isIntegral()) {
                    sendError(callback, drogon::k400BadRequest, "student_ids must contain integers only");
                    return;
                }
                const auto id = v.asInt64();
                if (id <= 0) {
                    sendError(callback, drogon::k400BadRequest, "student_ids must contain positive integers only");
                    return;
                }
                studentIds.push_back(id);
            }

            std::vector<long long> jobIds;
            for (const auto &v : (*json)["job_ids"]) {
                if (!v.isIntegral()) {
                    sendError(callback, drogon::k400BadRequest, "job_ids must contain integers only");
                    return;
                }
                const auto id = v.asInt64();
                if (id <= 0) {
                    sendError(callback, drogon::k400BadRequest, "job_ids must contain positive integers only");
                    return;
                }
                jobIds.push_back(id);
            }

            if (studentIds.empty() || jobIds.empty()) {
                sendError(callback, drogon::k400BadRequest, "student_ids and job_ids cannot be empty");
                return;
            }

            const size_t totalPairs = studentIds.size() * jobIds.size();
            if (totalPairs > 200) {
                sendError(callback, drogon::k400BadRequest, "too many pairs in one batch, max is 200");
                return;
            }

            const auto algorithmVersion =
                json->isMember("algorithm_version") ? (*json)["algorithm_version"].asString() : "v0.1-baseline";
            const bool persist = !json->isMember("persist") || (*json)["persist"].asBool();
            const auto preferredProvince =
                json->isMember("preferred_location_province")
                    ? (*json)["preferred_location_province"].asString()
                    : "";
            const auto preferredCity =
                json->isMember("preferred_location_city")
                    ? (*json)["preferred_location_city"].asString()
                    : "";

            Json::Value data(Json::arrayValue);
            Json::Value failed(Json::arrayValue);
            size_t successCount = 0;

            for (const auto studentId : studentIds) {
                for (const auto jobId : jobIds) {
                    auto match = computeBaselineMatch(
                        db, studentId, jobId, algorithmVersion, preferredProvince, preferredCity);
                    if (!match.ok) {
                        Json::Value failItem;
                        failItem["student_id"] = static_cast<Json::Int64>(studentId);
                        failItem["job_id"] = static_cast<Json::Int64>(jobId);
                        failItem["error"] = match.message;
                        failItem["status"] = static_cast<int>(match.status);
                        failed.append(failItem);
                        continue;
                    }

                    if (persist) {
                        std::string persistError;
                        if (!persistMatchResult(db, match, persistError)) {
                            Json::Value failItem;
                            failItem["student_id"] = static_cast<Json::Int64>(studentId);
                            failItem["job_id"] = static_cast<Json::Int64>(jobId);
                            failItem["error"] = persistError;
                            failItem["status"] = 500;
                            failed.append(failItem);
                            continue;
                        }
                    }

                    Json::Value item;
                    item["student_id"] = static_cast<Json::Int64>(match.studentId);
                    item["major_id"] = static_cast<Json::Int64>(match.majorId);
                    item["job_id"] = static_cast<Json::Int64>(match.jobId);
                    item["algorithm_version"] = match.algorithmVersion;
                    item["semantic_score"] = match.semanticScore;
                    item["skill_score"] = match.skillScore;
                    item["constraint_score"] = match.constraintScore;
                    item["final_score"] = match.finalScore;
                    item["matched_skills"] = match.matchedSkills;
                    item["missing_skills"] = match.missingSkills;
                    item["constraint_failures"] = match.constraintFailures;
                    item["persisted"] = persist;
                    if (persist) {
                        item["match_result_id"] = static_cast<Json::Int64>(match.matchResultId);
                    }
                    data.append(item);
                    ++successCount;
                }
            }

            Json::Value body;
            body["data"] = data;
            body["failed"] = failed;
            Json::Value summary;
            summary["total_pairs"] = static_cast<Json::UInt64>(totalPairs);
            summary["success_count"] = static_cast<Json::UInt64>(successCount);
            summary["failed_count"] = static_cast<Json::UInt64>(failed.size());
            body["summary"] = summary;
            callback(makeJson(body));
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/match-results",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            int page = 1;
            int pageSize = 20;
            if (!parsePagingParams(req, callback, page, pageSize)) {
                return;
            }
            const int offset = (page - 1) * pageSize;
            long long studentId = 0;
            long long majorId = 0;
            long long jobId = 0;
            if (!parseOptionalInt64Param(req, "student_id", callback, studentId)) {
                return;
            }
            if (!parseOptionalInt64Param(req, "major_id", callback, majorId)) {
                return;
            }
            if (!parseOptionalInt64Param(req, "job_id", callback, jobId)) {
                return;
            }

            const auto algorithmVersion = req->getParameter("algorithm_version");
            const std::string sortOrder = normalizeSortOrder(req->getParameter("sort_order"));
            static const std::unordered_map<std::string, std::string> kSortMap = {
                {"id", "mr.id"},
                {"final_score", "mr.final_score"},
                {"semantic_score", "mr.semantic_score"},
                {"skill_score", "mr.skill_score"},
                {"constraint_score", "mr.constraint_score"},
                {"generated_at", "mr.generated_at"}};
            const std::string sortField =
                resolveSortField(req->getParameter("sort_by"), kSortMap, "mr.id");
            const std::string whereSql =
                "WHERE (? = 0 OR mr.student_id = ?) "
                "AND (? = 0 OR mr.major_id = ?) "
                "AND (? = 0 OR mr.job_id = ?) "
                "AND (? = '' OR mr.algorithm_version = ?) ";
            const std::string baseFromSql =
                "FROM match_results mr "
                "INNER JOIN students s ON s.id = mr.student_id "
                "INNER JOIN majors m ON m.id = mr.major_id "
                "INNER JOIN jobs j ON j.id = mr.job_id ";
            const std::string selectSql =
                "SELECT mr.id, mr.student_id, s.full_name AS student_name, mr.major_id, m.name AS major_name, "
                "mr.job_id, j.title AS job_title, mr.algorithm_version, mr.semantic_score, mr.skill_score, "
                "mr.constraint_score, mr.final_score, mr.matched_skills_json, mr.missing_skills_json, "
                "mr.constraint_failures_json, mr.generated_at ";

            db->execSqlAsync(
                "SELECT COUNT(*) AS total " + baseFromSql + whereSql,
                [callback, db, selectSql, baseFromSql, whereSql, sortField, sortOrder,
                 page, pageSize, offset, studentId, majorId, jobId, algorithmVersion]
                (const drogon::orm::Result &countResult) {
                    const auto total = static_cast<Json::UInt64>(countResult[0]["total"].as<long long>());
                    db->execSqlAsync(
                        selectSql + baseFromSql + whereSql + "ORDER BY " + sortField + " " + sortOrder +
                            " LIMIT ? OFFSET ?",
                        [callback, total, page, pageSize](const drogon::orm::Result &result) {
                            Json::Value list(Json::arrayValue);
                            for (const auto &row : result) {
                                list.append(matchResultRowToJson(row));
                            }
                            Json::Value body;
                            body["data"] = list;
                            appendPagedMeta(body, total, page, pageSize);
                            callback(makeJson(body));
                        },
                        [callback](const DrogonDbException &e) {
                            sendDbError(e, callback);
                        },
                        studentId,
                        studentId,
                        majorId,
                        majorId,
                        jobId,
                        jobId,
                        algorithmVersion,
                        algorithmVersion,
                        pageSize,
                        offset);
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                studentId,
                studentId,
                majorId,
                majorId,
                jobId,
                jobId,
                algorithmVersion,
                algorithmVersion);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/dashboard/major-alignment",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            SessionUser user;
            if (!requireAuthUser(req, callback, user)) {
                return;
            }

            auto db = drogon::app().getDbClient();
            long long graduationYear = 0;
            if (!parseOptionalInt64Param(req, "graduation_year", callback, graduationYear)) {
                return;
            }
            if (graduationYear < 0) {
                sendError(callback, drogon::k400BadRequest, "invalid query param: graduation_year");
                return;
            }

            const auto algorithmVersion = trimCopy(req->getParameter("algorithm_version"));
            double alignmentThreshold = 70.0;
            const auto thresholdParam = trimCopy(req->getParameter("alignment_threshold"));
            if (!thresholdParam.empty()) {
                if (!parseStrictDouble(thresholdParam, alignmentThreshold) ||
                    alignmentThreshold < 0.0 || alignmentThreshold > 100.0) {
                    sendError(callback, drogon::k400BadRequest, "invalid query param: alignment_threshold");
                    return;
                }
            }

            int minStudents = 1;
            const auto minStudentsParam = trimCopy(req->getParameter("min_students"));
            if (!minStudentsParam.empty()) {
                if (!parseStrictInt(minStudentsParam, minStudents) || minStudents < 0 || minStudents > 1000000) {
                    sendError(callback, drogon::k400BadRequest, "invalid query param: min_students");
                    return;
                }
            }

            const std::string sql = R"SQL(
WITH filtered_students AS (
    SELECT s.id AS student_id, s.major_id
    FROM students s
    WHERE (? = 0 OR s.graduation_year = ?)
),
latest_employment AS (
    SELECT er.student_id, er.job_id
    FROM employment_records er
    INNER JOIN (
        SELECT student_id, MAX(id) AS latest_id
        FROM employment_records
        WHERE employment_type <> 'pending'
        GROUP BY student_id
    ) latest ON latest.latest_id = er.id
),
latest_match AS (
    SELECT mr.student_id, mr.job_id, mr.final_score
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
        m.id AS major_id,
        m.code AS major_code,
        m.name AS major_name,
        c.name AS college_name,
        COUNT(DISTINCT fs.student_id) AS total_students,
        COUNT(DISTINCT CASE WHEN le.student_id IS NOT NULL THEN fs.student_id END) AS employed_students,
        COUNT(DISTINCT CASE WHEN le.job_id IS NOT NULL AND lm.final_score IS NOT NULL THEN fs.student_id END)
            AS employed_with_match_students,
        COUNT(DISTINCT CASE WHEN le.job_id IS NOT NULL AND lm.final_score >= ? THEN fs.student_id END)
            AS aligned_students,
        AVG(CASE WHEN le.job_id IS NOT NULL AND lm.final_score IS NOT NULL THEN lm.final_score END)
            AS avg_match_score
    FROM majors m
    INNER JOIN colleges c ON c.id = m.college_id
    LEFT JOIN filtered_students fs ON fs.major_id = m.id
    LEFT JOIN latest_employment le ON le.student_id = fs.student_id
    LEFT JOIN latest_match lm ON lm.student_id = fs.student_id AND lm.job_id = le.job_id
    GROUP BY m.id, m.code, m.name, c.name
)
SELECT
    major_id,
    major_code,
    major_name,
    college_name,
    total_students,
    employed_students,
    employed_with_match_students,
    aligned_students,
    COALESCE(ROUND(avg_match_score, 3), 0.0) AS avg_match_score,
    CASE
        WHEN total_students <= 0 THEN 0.0
        ELSE ROUND(employed_students * 100.0 / total_students, 2)
    END AS employment_rate,
    CASE
        WHEN employed_students <= 0 THEN 0.0
        ELSE ROUND(aligned_students * 100.0 / employed_students, 2)
    END AS alignment_rate
FROM major_stats
WHERE total_students >= ?
ORDER BY alignment_rate DESC, avg_match_score DESC, employed_students DESC, major_id ASC
)SQL";

            db->execSqlAsync(
                sql,
                [callback, graduationYear, algorithmVersion, alignmentThreshold, minStudents]
                (const drogon::orm::Result &result) {
                    Json::Value list(Json::arrayValue);
                    for (const auto &row : result) {
                        Json::Value item;
                        item["major_id"] = static_cast<Json::Int64>(row["major_id"].as<long long>());
                        item["major_code"] = row["major_code"].isNull() ? "" : row["major_code"].as<std::string>();
                        item["major_name"] = row["major_name"].isNull() ? "" : row["major_name"].as<std::string>();
                        item["college_name"] = row["college_name"].isNull() ? "" : row["college_name"].as<std::string>();
                        item["total_students"] = static_cast<Json::UInt64>(row["total_students"].as<unsigned long long>());
                        item["employed_students"] =
                            static_cast<Json::UInt64>(row["employed_students"].as<unsigned long long>());
                        item["employed_with_match_students"] =
                            static_cast<Json::UInt64>(row["employed_with_match_students"].as<unsigned long long>());
                        item["aligned_students"] =
                            static_cast<Json::UInt64>(row["aligned_students"].as<unsigned long long>());
                        item["avg_match_score"] = row["avg_match_score"].isNull() ? 0.0 : row["avg_match_score"].as<double>();
                        item["employment_rate"] = row["employment_rate"].isNull() ? 0.0 : row["employment_rate"].as<double>();
                        item["alignment_rate"] = row["alignment_rate"].isNull() ? 0.0 : row["alignment_rate"].as<double>();
                        list.append(item);
                    }

                    Json::Value body;
                    body["data"] = list;
                    body["meta"]["graduation_year"] = static_cast<Json::Int64>(graduationYear);
                    body["meta"]["algorithm_version"] = algorithmVersion;
                    body["meta"]["alignment_threshold"] = alignmentThreshold;
                    body["meta"]["min_students"] = minStudents;
                    callback(makeJson(body));
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                graduationYear,
                graduationYear,
                algorithmVersion,
                algorithmVersion,
                alignmentThreshold,
                minStudents);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/dashboard/major-skill-gaps",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            SessionUser user;
            if (!requireAuthUser(req, callback, user)) {
                return;
            }

            auto db = drogon::app().getDbClient();
            long long graduationYear = 0;
            if (!parseOptionalInt64Param(req, "graduation_year", callback, graduationYear)) {
                return;
            }
            if (graduationYear < 0) {
                sendError(callback, drogon::k400BadRequest, "invalid query param: graduation_year");
                return;
            }

            const auto algorithmVersion = trimCopy(req->getParameter("algorithm_version"));
            int topN = 5;
            const auto topNParam = trimCopy(req->getParameter("top_n"));
            if (!topNParam.empty()) {
                if (!parseStrictInt(topNParam, topN) || topN <= 0 || topN > 20) {
                    sendError(callback, drogon::k400BadRequest, "invalid query param: top_n");
                    return;
                }
            }
            int minGapCount = 1;
            const auto minGapCountParam = trimCopy(req->getParameter("min_gap_count"));
            if (!minGapCountParam.empty()) {
                if (!parseStrictInt(minGapCountParam, minGapCount) ||
                    minGapCount <= 0 || minGapCount > 1000000) {
                    sendError(callback, drogon::k400BadRequest, "invalid query param: min_gap_count");
                    return;
                }
            }

            const std::string sql = R"SQL(
WITH filtered_students AS (
    SELECT s.id AS student_id, s.major_id
    FROM students s
    WHERE (? = 0 OR s.graduation_year = ?)
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

            db->execSqlAsync(
                sql,
                [callback, graduationYear, algorithmVersion, topN, minGapCount]
                (const drogon::orm::Result &result) {
                    Json::Value grouped(Json::arrayValue);
                    std::unordered_map<long long, Json::ArrayIndex> majorIndex;

                    for (const auto &row : result) {
                        const auto majorId = row["major_id"].as<long long>();
                        auto indexIt = majorIndex.find(majorId);
                        Json::ArrayIndex idx = 0;
                        if (indexIt == majorIndex.end()) {
                            Json::Value majorItem;
                            majorItem["major_id"] = static_cast<Json::Int64>(majorId);
                            majorItem["major_code"] = row["major_code"].isNull() ? "" : row["major_code"].as<std::string>();
                            majorItem["major_name"] = row["major_name"].isNull() ? "" : row["major_name"].as<std::string>();
                            majorItem["college_name"] = row["college_name"].isNull() ? "" : row["college_name"].as<std::string>();
                            majorItem["skills"] = Json::Value(Json::arrayValue);
                            idx = grouped.size();
                            grouped.append(majorItem);
                            majorIndex[majorId] = idx;
                        } else {
                            idx = indexIt->second;
                        }

                        Json::Value skillItem;
                        skillItem["rank"] = row["rank_no"].isNull() ? 0 : row["rank_no"].as<int>();
                        skillItem["skill_name"] = row["skill_name"].isNull() ? "" : row["skill_name"].as<std::string>();
                        skillItem["gap_count"] =
                            static_cast<Json::UInt64>(row["gap_count"].as<unsigned long long>());
                        skillItem["student_count"] =
                            static_cast<Json::UInt64>(row["student_count"].as<unsigned long long>());
                        skillItem["mandatory_count"] =
                            static_cast<Json::UInt64>(row["mandatory_count"].as<unsigned long long>());
                        grouped[idx]["skills"].append(skillItem);
                    }

                    Json::Value body;
                    body["data"] = grouped;
                    body["meta"]["graduation_year"] = static_cast<Json::Int64>(graduationYear);
                    body["meta"]["algorithm_version"] = algorithmVersion;
                    body["meta"]["top_n"] = topN;
                    body["meta"]["min_gap_count"] = minGapCount;
                    callback(makeJson(body));
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                graduationYear,
                graduationYear,
                algorithmVersion,
                algorithmVersion,
                minGapCount,
                topN);
        },
        {drogon::Get});

    drogon::app().run();
    return 0;
}
