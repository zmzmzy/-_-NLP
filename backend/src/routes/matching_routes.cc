#include "../app_routes.h"

#include "../app_support.h"

#include <unordered_set>
#include <vector>

namespace gm {

namespace {

struct StudentEmploymentRecomputeOptions {
    std::string algorithmVersion{"v0.4-major-student"};
    bool persist{true};
    size_t maxPairs{20000};
};

struct StudentEmploymentRecomputeResult {
    Json::Value data{Json::arrayValue};
    Json::Value failed{Json::arrayValue};
    size_t successCount{0};
    size_t failedCount{0};
};

bool parseStudentEmploymentRecomputeOptions(const HttpRequestPtr &req,
                                            const DbCallback &callback,
                                            StudentEmploymentRecomputeOptions &options) {
    options = StudentEmploymentRecomputeOptions();
    const auto json = req->getJsonObject();
    if (!json || !json->isObject()) {
        return true;
    }
    if (json->isMember("algorithm_version") && !(*json)["algorithm_version"].isNull()) {
        options.algorithmVersion = (*json)["algorithm_version"].asString();
    }
    options.algorithmVersion = normalizeAlgorithmVersion(options.algorithmVersion);
    if (json->isMember("persist")) {
        options.persist = (*json)["persist"].asBool();
    }
    if (json->isMember("max_pairs")) {
        if (!(*json)["max_pairs"].isIntegral()) {
            sendError(callback, drogon::k400BadRequest, "max_pairs must be an integer");
            return false;
        }
        const auto raw = (*json)["max_pairs"].asInt64();
        if (raw <= 0 || raw > 50000) {
            sendError(callback, drogon::k400BadRequest, "max_pairs must be in range [1, 50000]");
            return false;
        }
        options.maxPairs = static_cast<size_t>(raw);
    }
    return true;
}

void appendRecomputeFailedItem(Json::Value &failed,
                               long long studentId,
                               long long majorId,
                               long long jobId,
                               int status,
                               const std::string &error) {
    Json::Value failItem(Json::objectValue);
    failItem["student_id"] = static_cast<Json::Int64>(studentId);
    failItem["major_id"] = static_cast<Json::Int64>(majorId);
    failItem["job_id"] = static_cast<Json::Int64>(jobId);
    failItem["status"] = status;
    failItem["error"] = error;
    failed.append(failItem);
}

StudentEmploymentRecomputeResult runStudentEmploymentRecompute(const drogon::orm::DbClientPtr &db,
                                                               const drogon::orm::Result &pairs,
                                                               const std::string &algorithmVersion,
                                                               bool persist) {
    StudentEmploymentRecomputeResult result;
    for (const auto &row : pairs) {
        const auto studentId = row["student_id"].as<long long>();
        const auto majorId = row["major_id"].as<long long>();
        const auto jobId = row["job_id"].as<long long>();

        auto match = computeBaselineMatch(db, studentId, jobId, algorithmVersion, "", "");
        if (!match.ok) {
            appendRecomputeFailedItem(
                result.failed,
                studentId,
                majorId,
                jobId,
                static_cast<int>(match.status),
                match.message);
            ++result.failedCount;
            continue;
        }

        if (persist) {
            std::string persistError;
            if (!persistMatchResult(db, match, persistError)) {
                appendRecomputeFailedItem(result.failed, studentId, majorId, jobId, 500, persistError);
                ++result.failedCount;
                continue;
            }
        }

        Json::Value item(Json::objectValue);
        item["student_id"] = static_cast<Json::Int64>(match.studentId);
        item["major_id"] = static_cast<Json::Int64>(match.majorId);
        item["job_id"] = static_cast<Json::Int64>(match.jobId);
        item["algorithm_version"] = match.algorithmVersion;
        item["semantic_score"] = match.semanticScore;
        item["skill_score"] = match.skillScore;
        item["constraint_score"] = match.constraintScore;
        item["final_score"] = match.finalScore;
        item["persisted"] = persist;
        if (persist) {
            item["match_result_id"] = static_cast<Json::Int64>(match.matchResultId);
        }
        result.data.append(item);
        ++result.successCount;
    }
    return result;
}

}  // namespace

void registerMatchingRoutes() {
    // -------------------------
    // Matching Engine (Major-Job Primary)
    // -------------------------
    drogon::app().registerHandler(
        "/api/match/major-job",
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
            if (!requireFields(*json, {"major_id", "job_id"}, callback)) {
                return;
            }
            if (!(*json)["major_id"].isIntegral() || !(*json)["job_id"].isIntegral()) {
                sendError(callback, drogon::k400BadRequest, "major_id and job_id must be integers");
                return;
            }

            const auto majorId = (*json)["major_id"].asInt64();
            const auto jobId = (*json)["job_id"].asInt64();
            if (majorId <= 0 || jobId <= 0) {
                sendError(callback, drogon::k400BadRequest, "major_id and job_id must be positive integers");
                return;
            }

            const bool persist = json->isMember("persist") && (*json)["persist"].asBool();

            const auto algorithmVersion =
                json->isMember("algorithm_version") ? (*json)["algorithm_version"].asString() : "v0.2-major-job";
            auto match = computeMajorJobMatch(db, majorId, jobId, algorithmVersion);
            if (!match.ok) {
                sendError(callback, match.status, match.message);
                return;
            }

            long long majorJobMatchResultId = 0;
            if (persist) {
                std::string persistError;
                if (!persistMajorJobMatchResult(
                        db,
                        match,
                        user.userId,
                        "single",
                        majorJobMatchResultId,
                        persistError)) {
                    sendError(callback, drogon::k500InternalServerError, persistError, "database_error");
                    return;
                }
            }

            Json::Value item;
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
                item["major_job_match_result_id"] = static_cast<Json::Int64>(majorJobMatchResultId);
            }

            Json::Value body;
            body["data"] = item;
            callback(makeJson(body, persist ? drogon::k201Created : drogon::k200OK));
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/match/major-job/batch",
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
            if (!json->isMember("major_ids") || !(*json)["major_ids"].isArray() ||
                !json->isMember("job_ids") || !(*json)["job_ids"].isArray()) {
                sendError(callback, drogon::k400BadRequest, "major_ids and job_ids must be JSON arrays");
                return;
            }

            const bool persist = json->isMember("persist") && (*json)["persist"].asBool();

            std::vector<long long> majorIds;
            for (const auto &v : (*json)["major_ids"]) {
                if (!v.isIntegral()) {
                    sendError(callback, drogon::k400BadRequest, "major_ids must contain integers only");
                    return;
                }
                const auto id = v.asInt64();
                if (id <= 0) {
                    sendError(callback, drogon::k400BadRequest, "major_ids must contain positive integers only");
                    return;
                }
                majorIds.push_back(id);
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

            if (majorIds.empty() || jobIds.empty()) {
                sendError(callback, drogon::k400BadRequest, "major_ids and job_ids cannot be empty");
                return;
            }

            const size_t totalPairs = majorIds.size() * jobIds.size();
            if (totalPairs > 300) {
                sendError(callback, drogon::k400BadRequest, "too many pairs in one batch, max is 300");
                return;
            }

            const auto algorithmVersion =
                json->isMember("algorithm_version") ? (*json)["algorithm_version"].asString() : "v0.2-major-job";

            Json::Value data(Json::arrayValue);
            Json::Value failed(Json::arrayValue);
            size_t successCount = 0;
            size_t persistedCount = 0;

            for (const auto majorId : majorIds) {
                for (const auto jobId : jobIds) {
                    auto match = computeMajorJobMatch(db, majorId, jobId, algorithmVersion);
                    if (!match.ok) {
                        Json::Value failItem;
                        failItem["major_id"] = static_cast<Json::Int64>(majorId);
                        failItem["job_id"] = static_cast<Json::Int64>(jobId);
                        failItem["error"] = match.message;
                        failItem["status"] = static_cast<int>(match.status);
                        failed.append(failItem);
                        continue;
                    }

                    long long majorJobMatchResultId = 0;
                    if (persist) {
                        std::string persistError;
                        if (!persistMajorJobMatchResult(
                                db,
                                match,
                                user.userId,
                                "batch",
                                majorJobMatchResultId,
                                persistError)) {
                            Json::Value failItem;
                            failItem["major_id"] = static_cast<Json::Int64>(majorId);
                            failItem["job_id"] = static_cast<Json::Int64>(jobId);
                            failItem["error"] = persistError;
                            failItem["status"] = 500;
                            failed.append(failItem);
                            continue;
                        }
                        ++persistedCount;
                    }

                    Json::Value item;
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
                        item["major_job_match_result_id"] = static_cast<Json::Int64>(majorJobMatchResultId);
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
            summary["persisted_count"] = static_cast<Json::UInt64>(persistedCount);
            body["summary"] = summary;
            callback(makeJson(body));
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/match/major-job/results",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            int page = 1;
            int pageSize = 20;
            if (!parsePagingParams(req, callback, page, pageSize)) {
                return;
            }
            const int offset = (page - 1) * pageSize;

            long long majorId = 0;
            long long jobId = 0;
            if (!parseOptionalInt64Param(req, "major_id", callback, majorId)) {
                return;
            }
            if (!parseOptionalInt64Param(req, "job_id", callback, jobId)) {
                return;
            }

            const auto algorithmVersion = trimCopy(req->getParameter("algorithm_version"));
            const auto runMode = trimCopy(req->getParameter("run_mode"));
            static const std::unordered_set<std::string> kRunMode = {"single", "batch"};
            if (!runMode.empty() && !validateEnumValue("run_mode", runMode, kRunMode, callback)) {
                return;
            }

            const std::string sortOrder = normalizeSortOrder(req->getParameter("sort_order"));
            static const std::unordered_map<std::string, std::string> kSortMap = {
                {"id", "r.id"},
                {"final_score", "r.final_score"},
                {"semantic_score", "r.semantic_score"},
                {"skill_score", "r.skill_score"},
                {"constraint_score", "r.constraint_score"},
                {"generated_at", "r.generated_at"}};
            const std::string sortField = resolveSortField(req->getParameter("sort_by"), kSortMap, "r.id");
            const std::string whereSql =
                "WHERE (? = 0 OR r.major_id = ?) "
                "AND (? = 0 OR r.job_id = ?) "
                "AND (? = '' OR r.algorithm_version = ?) "
                "AND (? = '' OR r.run_mode = ?) ";
            const std::string baseFromSql =
                "FROM major_job_match_results r "
                "INNER JOIN majors m ON m.id = r.major_id "
                "INNER JOIN jobs j ON j.id = r.job_id "
                "INNER JOIN companies c ON c.id = j.company_id "
                "LEFT JOIN users u ON u.id = r.created_by_user_id ";
            const std::string selectSql =
                "SELECT r.id, r.major_id, m.name AS major_name, r.job_id, j.title AS job_title, "
                "c.name AS company_name, r.algorithm_version, r.semantic_score, r.skill_score, "
                "r.constraint_score, r.final_score, r.matched_skills_json, r.missing_skills_json, "
                "r.constraint_failures_json, r.run_mode, r.created_by_user_id, "
                "u.username AS created_by_username, r.generated_at ";

            db->execSqlAsync(
                "SELECT COUNT(*) AS total " + baseFromSql + whereSql,
                [callback, db, selectSql, baseFromSql, whereSql, sortField, sortOrder,
                 page, pageSize, offset, majorId, jobId, algorithmVersion, runMode]
                (const drogon::orm::Result &countResult) {
                    const auto total = static_cast<Json::UInt64>(countResult[0]["total"].as<long long>());
                    db->execSqlAsync(
                        selectSql + baseFromSql + whereSql + "ORDER BY " + sortField + " " + sortOrder +
                            " LIMIT ? OFFSET ?",
                        [callback, total, page, pageSize](const drogon::orm::Result &result) {
                            Json::Value list(Json::arrayValue);
                            for (const auto &row : result) {
                                list.append(majorJobMatchResultRowToJson(row));
                            }
                            Json::Value body;
                            body["data"] = list;
                            appendPagedMeta(body, total, page, pageSize);
                            callback(makeJson(body));
                        },
                        [callback](const DrogonDbException &e) {
                            sendDbError(e, callback);
                        },
                        majorId,
                        majorId,
                        jobId,
                        jobId,
                        algorithmVersion,
                        algorithmVersion,
                        runMode,
                        runMode,
                        pageSize,
                        offset);
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                majorId,
                majorId,
                jobId,
                jobId,
                algorithmVersion,
                algorithmVersion,
                runMode,
                runMode);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/match/major-job/results/{1}",
        [](const HttpRequestPtr &req, DbCallback &&callback, long long id) {
            SessionUser user;
            if (!requireAuthUser(req, callback, user, true)) {
                return;
            }
            auto db = drogon::app().getDbClient();
            db->execSqlAsync(
                "DELETE FROM major_job_match_results WHERE id = ?",
                [callback, id](const drogon::orm::Result &result) {
                    if (result.affectedRows() == 0) {
                        sendError(callback, drogon::k404NotFound,
                                  "major job match result not found: " + std::to_string(id));
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
    // Matching Engine (Student-Job Extension)
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
        "/api/match/major/{1}/recompute",
        [](const HttpRequestPtr &req, DbCallback &&callback, long long majorId) {
            if (majorId <= 0) {
                sendError(callback, drogon::k400BadRequest, "major_id must be a positive integer");
                return;
            }

            SessionUser user;
            if (!requireAuthUser(req, callback, user)) {
                return;
            }
            StudentEmploymentRecomputeOptions options;
            if (!parseStudentEmploymentRecomputeOptions(req, callback, options)) {
                return;
            }

            auto db = drogon::app().getDbClient();
            try {
                const auto pairs = db->execSqlSync(
                    "SELECT s.id AS student_id, s.major_id, er.job_id "
                    "FROM students s "
                    "INNER JOIN ("
                    "  SELECT student_id, MAX(id) AS latest_id "
                    "  FROM employment_records "
                    "  WHERE employment_type <> 'pending' AND job_id IS NOT NULL "
                    "  GROUP BY student_id"
                    ") latest ON latest.student_id = s.id "
                    "INNER JOIN employment_records er ON er.id = latest.latest_id "
                    "WHERE s.major_id = ? "
                    "ORDER BY s.id ASC",
                    majorId);

                if (pairs.size() > options.maxPairs) {
                    sendError(
                        callback,
                        drogon::k400BadRequest,
                        "too many employment pairs for recompute, please narrow scope or increase max_pairs");
                    return;
                }

                const auto recomputeResult = runStudentEmploymentRecompute(
                    db,
                    pairs,
                    options.algorithmVersion,
                    options.persist);

                Json::Value body;
                body["data"] = recomputeResult.data;
                body["failed"] = recomputeResult.failed;
                body["summary"]["major_id"] = static_cast<Json::Int64>(majorId);
                body["summary"]["scope"] = "major";
                body["summary"]["algorithm_version"] = options.algorithmVersion;
                body["summary"]["total_pairs"] = static_cast<Json::UInt64>(pairs.size());
                body["summary"]["success_count"] = static_cast<Json::UInt64>(recomputeResult.successCount);
                body["summary"]["failed_count"] = static_cast<Json::UInt64>(recomputeResult.failedCount);
                body["summary"]["persisted"] = options.persist;
                callback(makeJson(body));
            } catch (const DrogonDbException &e) {
                sendDbError(e, callback);
            }
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/match/college/{1}/recompute",
        [](const HttpRequestPtr &req, DbCallback &&callback, long long collegeId) {
            if (collegeId <= 0) {
                sendError(callback, drogon::k400BadRequest, "college_id must be a positive integer");
                return;
            }
            SessionUser user;
            if (!requireAuthUser(req, callback, user, true)) {
                return;
            }
            StudentEmploymentRecomputeOptions options;
            if (!parseStudentEmploymentRecomputeOptions(req, callback, options)) {
                return;
            }

            auto db = drogon::app().getDbClient();
            try {
                const auto pairs = db->execSqlSync(
                    "SELECT s.id AS student_id, s.major_id, er.job_id "
                    "FROM students s "
                    "INNER JOIN ("
                    "  SELECT student_id, MAX(id) AS latest_id "
                    "  FROM employment_records "
                    "  WHERE employment_type <> 'pending' AND job_id IS NOT NULL "
                    "  GROUP BY student_id"
                    ") latest ON latest.student_id = s.id "
                    "INNER JOIN employment_records er ON er.id = latest.latest_id "
                    "WHERE s.college_id = ? "
                    "ORDER BY s.id ASC",
                    collegeId);
                if (pairs.size() > options.maxPairs) {
                    sendError(
                        callback,
                        drogon::k400BadRequest,
                        "too many employment pairs for recompute, please narrow scope or increase max_pairs");
                    return;
                }

                std::unordered_set<long long> majorSet;
                for (const auto &row : pairs) {
                    majorSet.insert(row["major_id"].as<long long>());
                }

                const auto recomputeResult = runStudentEmploymentRecompute(
                    db,
                    pairs,
                    options.algorithmVersion,
                    options.persist);
                Json::Value body;
                body["data"] = recomputeResult.data;
                body["failed"] = recomputeResult.failed;
                body["summary"]["scope"] = "college";
                body["summary"]["college_id"] = static_cast<Json::Int64>(collegeId);
                body["summary"]["major_count"] = static_cast<Json::UInt64>(majorSet.size());
                body["summary"]["algorithm_version"] = options.algorithmVersion;
                body["summary"]["total_pairs"] = static_cast<Json::UInt64>(pairs.size());
                body["summary"]["success_count"] = static_cast<Json::UInt64>(recomputeResult.successCount);
                body["summary"]["failed_count"] = static_cast<Json::UInt64>(recomputeResult.failedCount);
                body["summary"]["persisted"] = options.persist;
                callback(makeJson(body));
            } catch (const DrogonDbException &e) {
                sendDbError(e, callback);
            }
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/match/recompute-all",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            SessionUser user;
            if (!requireAuthUser(req, callback, user, true)) {
                return;
            }
            StudentEmploymentRecomputeOptions options;
            if (!parseStudentEmploymentRecomputeOptions(req, callback, options)) {
                return;
            }

            auto db = drogon::app().getDbClient();
            try {
                const auto pairs = db->execSqlSync(
                    "SELECT s.id AS student_id, s.major_id, s.college_id, er.job_id "
                    "FROM students s "
                    "INNER JOIN ("
                    "  SELECT student_id, MAX(id) AS latest_id "
                    "  FROM employment_records "
                    "  WHERE employment_type <> 'pending' AND job_id IS NOT NULL "
                    "  GROUP BY student_id"
                    ") latest ON latest.student_id = s.id "
                    "INNER JOIN employment_records er ON er.id = latest.latest_id "
                    "ORDER BY s.id ASC");
                if (pairs.size() > options.maxPairs) {
                    sendError(
                        callback,
                        drogon::k400BadRequest,
                        "too many employment pairs for recompute, please narrow scope or increase max_pairs");
                    return;
                }

                std::unordered_set<long long> majorSet;
                std::unordered_set<long long> collegeSet;
                for (const auto &row : pairs) {
                    majorSet.insert(row["major_id"].as<long long>());
                    collegeSet.insert(row["college_id"].as<long long>());
                }

                const auto recomputeResult = runStudentEmploymentRecompute(
                    db,
                    pairs,
                    options.algorithmVersion,
                    options.persist);
                Json::Value body;
                body["data"] = recomputeResult.data;
                body["failed"] = recomputeResult.failed;
                body["summary"]["scope"] = "all";
                body["summary"]["college_count"] = static_cast<Json::UInt64>(collegeSet.size());
                body["summary"]["major_count"] = static_cast<Json::UInt64>(majorSet.size());
                body["summary"]["algorithm_version"] = options.algorithmVersion;
                body["summary"]["total_pairs"] = static_cast<Json::UInt64>(pairs.size());
                body["summary"]["success_count"] = static_cast<Json::UInt64>(recomputeResult.successCount);
                body["summary"]["failed_count"] = static_cast<Json::UInt64>(recomputeResult.failedCount);
                body["summary"]["persisted"] = options.persist;
                callback(makeJson(body));
            } catch (const DrogonDbException &e) {
                sendDbError(e, callback);
            }
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
            int smoothingK = 20;
            const auto smoothingKParam = trimCopy(req->getParameter("smoothing_k"));
            if (!smoothingKParam.empty()) {
                if (!parseStrictInt(smoothingKParam, smoothingK) || smoothingK <= 0 || smoothingK > 1000000) {
                    sendError(callback, drogon::k400BadRequest, "invalid query param: smoothing_k");
                    return;
                }
            }

            const std::string sql = R"SQL(
WITH major_students AS (
    SELECT
        m.id AS major_id,
        m.code AS major_code,
        m.name AS major_name,
        c.name AS college_name,
        s.id AS student_id
    FROM majors m
    INNER JOIN colleges c ON c.id = m.college_id
    LEFT JOIN students s ON s.major_id = m.id
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
    GROUP BY ms.major_id, ms.major_code, ms.major_name, ms.college_name
),
metrics AS (
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
        s.college_name,
        s.total_students,
        s.employed_students,
        s.employed_with_match_students,
        s.aligned_students,
        s.avg_match_score,
        s.weighted_avg_match_score,
        s.employment_rate,
        s.alignment_rate,
        s.coverage_rate,
        s.raw_major_score,
        CASE
            WHEN s.employed_students <= 0 THEN 0.0
            ELSE ROUND(
                (s.employed_students * 1.0 / (s.employed_students + ?)) * s.raw_major_score +
                (? * 1.0 / (s.employed_students + ?)) * gm.score_mean,
                3
            )
        END AS excellent_graduate_score,
        CASE
            WHEN s.employed_students >= 20 THEN 'high'
            WHEN s.employed_students >= 10 THEN 'medium'
            WHEN s.employed_students > 0 THEN 'low'
            ELSE 'none'
        END AS confidence_level
    FROM scored s
    CROSS JOIN global_mean gm
),
ranked AS (
    SELECT
        fs.*,
        ROW_NUMBER() OVER (
            ORDER BY fs.excellent_graduate_score DESC,
                     fs.raw_major_score DESC,
                     fs.alignment_rate DESC,
                     fs.weighted_avg_match_score DESC,
                     fs.employed_students DESC,
                     fs.major_id ASC
        ) AS rank_position
    FROM final_stats fs
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
    avg_match_score,
    weighted_avg_match_score,
    raw_major_score,
    excellent_graduate_score,
    employment_rate,
    alignment_rate,
    coverage_rate,
    confidence_level,
    rank_position
FROM ranked
ORDER BY rank_position ASC
)SQL";

            db->execSqlAsync(
                sql,
                [callback, algorithmVersion, alignmentThreshold, minStudents, smoothingK]
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
                        item["rank_position"] =
                            row["rank_position"].isNull() ? 0 : row["rank_position"].as<int>();
                        list.append(item);
                    }

                    Json::Value body;
                    body["data"] = list;
                    body["meta"]["algorithm_version"] = algorithmVersion;
                    body["meta"]["alignment_threshold"] = alignmentThreshold;
                    body["meta"]["min_students"] = minStudents;
                    body["meta"]["smoothing_k"] = smoothingK;
                    callback(makeJson(body));
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                algorithmVersion,
                algorithmVersion,
                alignmentThreshold,
                minStudents,
                smoothingK,
                smoothingK,
                smoothingK);
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
                [callback, algorithmVersion, topN, minGapCount]
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
                    body["meta"]["algorithm_version"] = algorithmVersion;
                    body["meta"]["top_n"] = topN;
                    body["meta"]["min_gap_count"] = minGapCount;
                    callback(makeJson(body));
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                algorithmVersion,
                algorithmVersion,
                minGapCount,
                topN);
        },
        {drogon::Get});
}

}  // namespace gm
