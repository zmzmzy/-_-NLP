#include "../app_routes.h"

#include "../app_support.h"

namespace gm {

void registerEducationRoutes() {
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
        [](const HttpRequestPtr &req, DbCallback &&callback, long long id) {
            auto db = drogon::app().getDbClient();
            bool forceDelete = false;
            const auto forceParam = trimCopy(req->getParameter("force"));
            if (!forceParam.empty() && !parseBoolLike(forceParam, forceDelete)) {
                sendError(callback, drogon::k400BadRequest, "invalid query param: force");
                return;
            }

            try {
                const auto collegeResult = db->execSqlSync(
                    "SELECT id FROM colleges WHERE id = ? LIMIT 1",
                    id);
                if (collegeResult.empty()) {
                    sendError(callback, drogon::k404NotFound,
                              "college not found: " + std::to_string(id));
                    return;
                }

                const auto majorCountResult = db->execSqlSync(
                    "SELECT COUNT(*) AS total FROM majors WHERE college_id = ?",
                    id);
                const auto studentCountResult = db->execSqlSync(
                    "SELECT COUNT(*) AS total FROM students WHERE college_id = ?",
                    id);
                const auto majorCount = majorCountResult[0]["total"].as<long long>();
                const auto studentCount = studentCountResult[0]["total"].as<long long>();

                if (!forceDelete) {
                    if (majorCount > 0 || studentCount > 0) {
                        sendError(
                            callback,
                            drogon::k409Conflict,
                            "college has related data (majors=" + std::to_string(majorCount) +
                                ", students=" + std::to_string(studentCount) +
                                "), delete related records first or use force=true as admin");
                        return;
                    }
                    db->execSqlSync("DELETE FROM colleges WHERE id = ?", id);
                    Json::Value body;
                    body["message"] = "deleted";
                    body["id"] = static_cast<Json::Int64>(id);
                    callback(makeJson(body));
                    return;
                }

                SessionUser user;
                if (!requireAuthUser(req, callback, user, true)) {
                    return;
                }

                auto tx = db->newTransaction();
                try {
                    tx->execSqlSync(
                        "DELETE FROM match_results "
                        "WHERE student_id IN (SELECT id FROM students WHERE college_id = ?) "
                        "OR major_id IN (SELECT id FROM majors WHERE college_id = ?)",
                        id,
                        id);
                    tx->execSqlSync(
                        "DELETE FROM employment_records "
                        "WHERE student_id IN (SELECT id FROM students WHERE college_id = ?)",
                        id);
                    tx->execSqlSync("DELETE FROM students WHERE college_id = ?", id);
                    tx->execSqlSync("DELETE FROM majors WHERE college_id = ?", id);
                    tx->execSqlSync("DELETE FROM colleges WHERE id = ?", id);
                    tx.reset();  // commit
                } catch (...) {
                    try {
                        tx->rollback();
                    } catch (...) {
                    }
                    throw;
                }

                Json::Value body;
                body["message"] = "deleted";
                body["id"] = static_cast<Json::Int64>(id);
                body["force"] = true;
                body["deleted_majors"] = static_cast<Json::Int64>(majorCount);
                body["deleted_students"] = static_cast<Json::Int64>(studentCount);
                callback(makeJson(body));
            } catch (const DrogonDbException &e) {
                sendDbError(e, callback);
            }
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
            const std::string sortOrder = normalizeSortOrder(req->getParameter("sort_order"));
            static const std::unordered_map<std::string, std::string> kSortMap = {
                {"id", "m.id"},
                {"code", "m.code"},
                {"name", "m.name"},
                {"college_name", "c.name"},
                {"created_at", "m.created_at"},
                {"updated_at", "m.updated_at"}};
            const std::string sortField =
                resolveSortField(req->getParameter("sort_by"), kSortMap, "m.id");
            const std::string whereSql =
                "WHERE (? = '' OR m.code LIKE ? OR m.name LIKE ? OR c.name LIKE ? OR "
                "m.description LIKE ?) "
                "AND (? = 0 OR m.college_id = ?) ";
            const std::string baseFromSql =
                "FROM majors m INNER JOIN colleges c ON c.id = m.college_id ";
            const std::string selectSql =
                "SELECT m.id, m.college_id, c.name AS college_name, m.code, m.name, "
                "m.description, "
                "m.created_at, m.updated_at ";

            db->execSqlAsync(
                "SELECT COUNT(*) AS total " + baseFromSql + whereSql,
                [callback, db, selectSql, baseFromSql, whereSql, sortField, sortOrder,
                 page, pageSize, offset, keywordRaw, like, collegeId]
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
                        collegeId,
                        collegeId,
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
                collegeId);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/majors/{1}",
        [](const HttpRequestPtr &, DbCallback &&callback, long long id) {
            auto db = drogon::app().getDbClient();
            db->execSqlAsync(
                "SELECT m.id, m.college_id, c.name AS college_name, m.code, m.name, "
                "m.description, "
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
            const auto description =
                json->isMember("description") ? (*json)["description"].asString() : "";

            db->execSqlAsync(
                "INSERT INTO majors(college_id, code, name, description) VALUES(?, ?, ?, ?)",
                [callback, collegeId, code, name, description]
                (const drogon::orm::Result &result) {
                    Json::Value item;
                    item["id"] = static_cast<Json::UInt64>(result.insertId());
                    item["college_id"] = static_cast<Json::Int64>(collegeId);
                    item["code"] = code;
                    item["name"] = name;
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
                        sendError(callback,
                                  drogon::k409Conflict,
                                  "duplicate major name in the same college");
                        return;
                    }
                    sendDbError(e, callback);
                },
                collegeId,
                code,
                name,
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
            const auto description =
                json->isMember("description") ? (*json)["description"].asString() : "";

            db->execSqlAsync(
                "UPDATE majors SET college_id = ?, code = ?, name = ?, description = ? WHERE id = ?",
                [callback, db, id](const drogon::orm::Result &result) {
                    if (result.affectedRows() == 0) {
                        sendError(callback, drogon::k404NotFound,
                                  "major not found: " + std::to_string(id));
                        return;
                    }
                    db->execSqlAsync(
                        "SELECT m.id, m.college_id, c.name AS college_name, m.code, m.name, "
                        "m.description, "
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
                        sendError(callback,
                                  drogon::k409Conflict,
                                  "duplicate major name in the same college");
                        return;
                    }
                    sendDbError(e, callback);
                },
                collegeId,
                code,
                name,
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
    // Major Import (v0.2)
    // -------------------------
    drogon::app().registerHandler(
        "/api/majors/import/preview",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            SessionUser user;
            if (!requireAuthUser(req, callback, user, true)) {
                return;
            }

            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }

            std::vector<Json::Value> rawRows;
            std::string parseError;
            std::string sourceType = "manual";
            const auto sourceFilename = normalizeImportText(
                json->isMember("source_filename") ? (*json)["source_filename"].asString() : "",
                512);

            if (json->isMember("csv_text") && !(*json)["csv_text"].isNull()) {
                if (!(*json)["csv_text"].isString()) {
                    sendError(callback, drogon::k400BadRequest, "invalid field: csv_text");
                    return;
                }
                if (!parseCsvRowsFromText((*json)["csv_text"].asString(), rawRows, parseError)) {
                    sendError(callback, drogon::k400BadRequest, parseError);
                    return;
                }
                sourceType = "csv";
            } else if (json->isMember("rows") && (*json)["rows"].isArray()) {
                for (Json::ArrayIndex i = 0; i < (*json)["rows"].size(); ++i) {
                    if (!(*json)["rows"][i].isObject()) {
                        sendError(callback, drogon::k400BadRequest, "invalid field: rows must be object array");
                        return;
                    }
                    Json::Value row = (*json)["rows"][i];
                    row["row_no"] = static_cast<int>(i + 1);
                    rawRows.push_back(row);
                }
                sourceType = "manual";
            } else {
                sendError(callback, drogon::k400BadRequest, "require either csv_text or rows");
                return;
            }

            if (rawRows.empty()) {
                sendError(callback, drogon::k400BadRequest, "no import rows found");
                return;
            }

            const auto requestedSourceType = toLowerAscii(trimCopy(
                json->isMember("source_type") ? (*json)["source_type"].asString() : sourceType));
            if (requestedSourceType == "csv" || requestedSourceType == "xlsx" || requestedSourceType == "manual") {
                sourceType = requestedSourceType;
            }

            auto db = drogon::app().getDbClient();
            try {
                auto makeScopedMajorKey = [](const std::string &universityCode,
                                             const std::string &collegeCode,
                                             const std::string &majorName) {
                    return universityCode + "::" + collegeCode + "::" + majorName;
                };
                std::unordered_set<std::string> existingMajorScopedCodes;
                const auto majorRows = db->execSqlSync(
                    "SELECT COALESCE(u.code, '') AS university_code, c.code AS college_code, m.name AS major_name "
                    "FROM majors m "
                    "INNER JOIN colleges c ON c.id = m.college_id "
                    "LEFT JOIN universities u ON u.id = c.university_id");
                for (const auto &row : majorRows) {
                    const auto universityCode = normalizeImportCode(row["university_code"].as<std::string>(), 64);
                    const auto collegeCode = normalizeImportCode(row["college_code"].as<std::string>(), 64);
                    const auto majorName = normalizeImportText(row["major_name"].as<std::string>(), 128);
                    existingMajorScopedCodes.insert(
                        makeScopedMajorKey(universityCode, collegeCode, majorName));
                }

                struct PreviewRowRecord {
                    int rowNo{0};
                    std::string rowStatus;
                    std::string planAction;
                    std::string error;
                    std::string universityCode;
                    std::string collegeCode;
                    std::string majorCode;
                    std::string majorName;
                    Json::Value rawRow{Json::objectValue};
                    Json::Value normalizedRow{Json::objectValue};
                };

                std::vector<PreviewRowRecord> records;
                records.reserve(rawRows.size());
                std::unordered_set<std::string> seenMajorScopedCodes;

                int validRows = 0;
                int invalidRows = 0;
                int estimatedInsertedRows = 0;
                int estimatedUpdatedRows = 0;

                Json::Value previewRows(Json::arrayValue);
                for (size_t i = 0; i < rawRows.size(); ++i) {
                    const auto &rawRow = rawRows[i];
                    PreviewRowRecord record;
                    record.rowNo = rawRow.isMember("row_no") && rawRow["row_no"].isIntegral()
                                       ? rawRow["row_no"].asInt()
                                       : static_cast<int>(i + 1);
                    record.rawRow = rawRow;

                    Json::Value normalizedRow(Json::objectValue);
                    std::string normalizeError;
                    if (!normalizeMajorImportRow(rawRow, normalizedRow, normalizeError)) {
                        record.rowStatus = "invalid";
                        record.error = normalizeError;
                        ++invalidRows;
                    } else {
                        record.normalizedRow = normalizedRow;
                        record.universityCode = normalizedRow["university_code"].asString();
                        record.collegeCode = normalizedRow["college_code"].asString();
                        record.majorCode = normalizedRow["major_code"].asString();
                        record.majorName = normalizedRow["major_name"].asString();

                        const auto scopedMajorCode = makeScopedMajorKey(
                            record.universityCode,
                            record.collegeCode,
                            record.majorName);
                        if (seenMajorScopedCodes.find(scopedMajorCode) != seenMajorScopedCodes.end()) {
                            record.rowStatus = "invalid";
                            record.error =
                                "duplicate major_name in current import payload for same university_code+college_code";
                            ++invalidRows;
                        } else {
                            seenMajorScopedCodes.insert(scopedMajorCode);
                            record.rowStatus = "valid";
                            if (existingMajorScopedCodes.find(scopedMajorCode) !=
                                existingMajorScopedCodes.end()) {
                                record.planAction = "update";
                                ++estimatedUpdatedRows;
                            } else {
                                record.planAction = "insert";
                                ++estimatedInsertedRows;
                            }
                            ++validRows;
                        }
                    }

                    Json::Value item;
                    item["row_no"] = record.rowNo;
                    item["row_status"] = record.rowStatus;
                    item["plan_action"] = record.planAction;
                    item["error_message"] = record.error;
                    item["normalized_row"] = record.normalizedRow;
                    previewRows.append(item);
                    records.push_back(std::move(record));
                }

                Json::Value previewPayload(Json::objectValue);
                previewPayload["summary"]["total_rows"] = static_cast<Json::UInt64>(rawRows.size());
                previewPayload["summary"]["valid_rows"] = validRows;
                previewPayload["summary"]["invalid_rows"] = invalidRows;
                previewPayload["summary"]["estimated_inserted_rows"] = estimatedInsertedRows;
                previewPayload["summary"]["estimated_updated_rows"] = estimatedUpdatedRows;
                previewPayload["rows"] = previewRows;

                const auto batchInsert = db->execSqlSync(
                    "INSERT INTO major_import_batches("
                    "source_type, source_filename, status, triggered_by_user_id, "
                    "total_rows, valid_rows, invalid_rows, inserted_rows, updated_rows, skipped_rows, error_rows, "
                    "preview_payload_json, started_at, finished_at, message) "
                    "VALUES(?, NULLIF(?, ''), 'preview', NULLIF(?, 0), ?, ?, ?, ?, ?, 0, ?, ?, NOW(), NOW(), ?)",
                    sourceType,
                    sourceFilename,
                    user.userId,
                    static_cast<int>(rawRows.size()),
                    validRows,
                    invalidRows,
                    estimatedInsertedRows,
                    estimatedUpdatedRows,
                    invalidRows,
                    jsonToCompactString(previewPayload),
                    "preview generated");
                const auto batchId = static_cast<long long>(batchInsert.insertId());

                for (const auto &record : records) {
                    db->execSqlSync(
                        "INSERT INTO major_import_rows("
                        "batch_id, row_no, row_status, university_code, college_code, major_code, "
                        "error_message, raw_row_json, normalized_row_json) "
                        "VALUES(?, ?, ?, NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''), ?, ?)",
                        batchId,
                        record.rowNo,
                        record.rowStatus,
                        record.universityCode,
                        record.collegeCode,
                        record.majorCode,
                        record.error,
                        jsonToCompactString(record.rawRow),
                        jsonToCompactString(record.normalizedRow));
                }

                Json::Value body;
                body["data"]["batch_id"] = static_cast<Json::Int64>(batchId);
                body["data"]["source_type"] = sourceType;
                body["data"]["source_filename"] = sourceFilename;
                body["data"]["summary"] = previewPayload["summary"];
                body["data"]["rows"] = previewRows;
                callback(makeJson(body, drogon::k201Created));
            } catch (const DrogonDbException &e) {
                sendDbError(e, callback);
            } catch (const std::exception &e) {
                sendError(callback, drogon::k500InternalServerError, e.what(), "import_error");
            }
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/majors/import/apply",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            SessionUser user;
            if (!requireAuthUser(req, callback, user, true)) {
                return;
            }

            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!requireFields(*json, {"batch_id"}, callback)) {
                return;
            }
            if (!(*json)["batch_id"].isIntegral()) {
                sendError(callback, drogon::k400BadRequest, "invalid field: batch_id");
                return;
            }
            const auto batchId = (*json)["batch_id"].asInt64();
            if (batchId <= 0) {
                sendError(callback, drogon::k400BadRequest, "invalid field: batch_id");
                return;
            }

            auto db = drogon::app().getDbClient();
            std::shared_ptr<drogon::orm::Transaction> tx;
            try {
                tx = db->newTransaction();
                const auto batchRows = tx->execSqlSync(
                    "SELECT id, status, valid_rows, invalid_rows "
                    "FROM major_import_batches WHERE id = ? LIMIT 1 FOR UPDATE",
                    batchId);
                if (batchRows.empty()) {
                    tx->rollback();
                    tx.reset();
                    sendError(callback, drogon::k404NotFound, "import batch not found: " + std::to_string(batchId));
                    return;
                }
                const auto currentStatus = batchRows[0]["status"].as<std::string>();
                const auto validRows = batchRows[0]["valid_rows"].isNull() ? 0 : batchRows[0]["valid_rows"].as<int>();
                const auto invalidRows = batchRows[0]["invalid_rows"].isNull() ? 0 : batchRows[0]["invalid_rows"].as<int>();
                if (currentStatus != "preview") {
                    tx->rollback();
                    tx.reset();
                    sendError(callback,
                              drogon::k409Conflict,
                              "import batch status must be preview before apply",
                              "import_state_conflict");
                    return;
                }
                if (validRows <= 0) {
                    tx->rollback();
                    tx.reset();
                    sendError(callback,
                              drogon::k409Conflict,
                              "no valid rows to apply; please fix CSV errors and run preview again",
                              "import_no_valid_rows");
                    return;
                }

                const auto importRows = tx->execSqlSync(
                    "SELECT id, row_no, row_status, normalized_row_json "
                    "FROM major_import_rows WHERE batch_id = ? ORDER BY row_no ASC, id ASC",
                    batchId);

                int insertedRows = 0;
                int updatedRows = 0;
                int skippedRows = 0;
                std::vector<std::pair<long long, std::string>> rowActions;
                rowActions.reserve(importRows.size());

                for (const auto &row : importRows) {
                    const auto rowId = row["id"].as<long long>();
                    const auto rowStatus = row["row_status"].as<std::string>();
                    if (rowStatus != "valid") {
                        continue;
                    }

                    const auto normalized = parseJsonTextOrDefault(
                        row["normalized_row_json"].as<std::string>(),
                        Json::objectValue);
                    if (!normalized.isObject()) {
                        throw std::runtime_error("invalid normalized_row_json in import row");
                    }

                    const auto universityCode = normalizeImportCode(jsonValueAsString(normalized, "university_code"), 64);
                    const auto universityName = normalizeImportText(jsonValueAsString(normalized, "university_name"), 255);
                    const auto universityProvince = normalizeImportText(jsonValueAsString(normalized, "university_province"), 64);
                    const auto universityCity = normalizeImportText(jsonValueAsString(normalized, "university_city"), 64);
                    const auto collegeCode = normalizeImportCode(jsonValueAsString(normalized, "college_code"), 64);
                    const auto collegeName = normalizeImportText(jsonValueAsString(normalized, "college_name"), 128);
                    const auto majorCode = normalizeImportCode(jsonValueAsString(normalized, "major_code"), 64);
                    const auto majorName = normalizeImportText(jsonValueAsString(normalized, "major_name"), 128);
                    const auto majorDescription = normalizeImportText(
                        jsonValueAsString(normalized, "major_description"), 2048);
                    const auto coreCourses = normalizeImportText(
                        jsonValueAsString(normalized, "core_courses_text"), 65535);
                    const auto knowledgeSkills = normalizeImportText(
                        jsonValueAsString(normalized, "knowledge_skills_text"), 65535);
                    const auto technicalPlatform = normalizeImportText(
                        jsonValueAsString(normalized, "technical_platform_text"), 65535);
                    const auto capability = normalizeImportText(
                        jsonValueAsString(normalized, "capability_text"), 65535);

                    if (universityCode.empty() || collegeCode.empty() || majorCode.empty() ||
                        universityName.empty() || collegeName.empty() || majorName.empty()) {
                        throw std::runtime_error("invalid normalized row content");
                    }

                    auto uniUpsert = tx->execSqlSync(
                        "INSERT INTO universities(code, name, province, city, description) "
                        "VALUES(?, ?, NULLIF(?, ''), NULLIF(?, ''), NULL) "
                        "ON DUPLICATE KEY UPDATE "
                        "id = LAST_INSERT_ID(id), "
                        "name = VALUES(name), "
                        "province = COALESCE(NULLIF(VALUES(province), ''), province), "
                        "city = COALESCE(NULLIF(VALUES(city), ''), city), "
                        "updated_at = CURRENT_TIMESTAMP",
                        universityCode,
                        universityName,
                        universityProvince,
                        universityCity);
                    long long universityId = static_cast<long long>(uniUpsert.insertId());
                    if (universityId <= 0) {
                        const auto uniRows = tx->execSqlSync(
                            "SELECT id FROM universities WHERE code = ? LIMIT 1",
                            universityCode);
                        if (uniRows.empty()) {
                            throw std::runtime_error("failed to resolve university id");
                        }
                        universityId = uniRows[0]["id"].as<long long>();
                    }

                    auto collegeUpsert = tx->execSqlSync(
                        "INSERT INTO colleges(university_id, code, name, description) "
                        "VALUES(NULLIF(?, 0), ?, ?, NULL) "
                        "ON DUPLICATE KEY UPDATE "
                        "id = LAST_INSERT_ID(id), "
                        "university_id = COALESCE(NULLIF(VALUES(university_id), 0), university_id), "
                        "name = VALUES(name), "
                        "updated_at = CURRENT_TIMESTAMP",
                        universityId,
                        collegeCode,
                        collegeName);
                    long long collegeId = static_cast<long long>(collegeUpsert.insertId());
                    if (collegeId <= 0) {
                        const auto collegeRows = tx->execSqlSync(
                            "SELECT id FROM colleges WHERE code = ? LIMIT 1",
                            collegeCode);
                        if (collegeRows.empty()) {
                            throw std::runtime_error("failed to resolve college id");
                        }
                        collegeId = collegeRows[0]["id"].as<long long>();
                    }

                    long long majorId = 0;
                    std::string action = "updated";
                    const auto majorRows = tx->execSqlSync(
                        "SELECT id FROM majors WHERE college_id = ? AND name = ? LIMIT 1",
                        collegeId,
                        majorName);
                    if (majorRows.empty()) {
                        const auto majorInsert = tx->execSqlSync(
                            "INSERT INTO majors(college_id, code, name, description) "
                            "VALUES(?, ?, ?, NULLIF(?, ''))",
                            collegeId,
                            majorCode,
                            majorName,
                            majorDescription);
                        majorId = static_cast<long long>(majorInsert.insertId());
                        action = "inserted";
                        ++insertedRows;
                    } else {
                        majorId = majorRows[0]["id"].as<long long>();
                        tx->execSqlSync(
                            "UPDATE majors SET code = ?, name = ?, "
                            "description = NULLIF(?, ''), updated_at = CURRENT_TIMESTAMP "
                            "WHERE id = ?",
                            majorCode,
                            majorName,
                            majorDescription,
                            majorId);
                        ++updatedRows;
                    }

                    if (!coreCourses.empty() || !knowledgeSkills.empty() ||
                        !technicalPlatform.empty() || !capability.empty()) {
                        tx->execSqlSync(
                            "INSERT INTO major_profiles(major_id, core_courses_text, knowledge_skills_text, "
                            "technical_platform_text, capability_text, updated_by_user_id) "
                            "VALUES(?, ?, ?, ?, ?, NULLIF(?, 0)) "
                            "ON DUPLICATE KEY UPDATE "
                            "core_courses_text = VALUES(core_courses_text), "
                            "knowledge_skills_text = VALUES(knowledge_skills_text), "
                            "technical_platform_text = VALUES(technical_platform_text), "
                            "capability_text = VALUES(capability_text), "
                            "updated_by_user_id = VALUES(updated_by_user_id), "
                            "updated_at = CURRENT_TIMESTAMP",
                            majorId,
                            coreCourses,
                            knowledgeSkills,
                            technicalPlatform,
                            capability,
                            user.userId);
                    }

                    rowActions.emplace_back(rowId, action);
                }
                for (const auto &it : rowActions) {
                    tx->execSqlSync(
                        "UPDATE major_import_rows "
                        "SET row_status = ?, error_message = NULL "
                        "WHERE id = ?",
                        it.second,
                        it.first);
                }

                tx->execSqlSync(
                    "UPDATE major_import_batches "
                    "SET status = 'applied', finished_at = NOW(), inserted_rows = ?, updated_rows = ?, "
                    "skipped_rows = ?, error_rows = ?, message = ? "
                    "WHERE id = ?",
                    insertedRows,
                    updatedRows,
                    skippedRows,
                    invalidRows,
                    "import applied",
                    batchId);
                const auto resultRows = tx->execSqlSync(
                    "SELECT b.id, b.source_type, b.source_filename, b.status, b.triggered_by_user_id, "
                    "u.username AS triggered_by_username, b.total_rows, b.valid_rows, b.invalid_rows, "
                    "b.inserted_rows, b.updated_rows, b.skipped_rows, b.error_rows, "
                    "b.started_at, b.finished_at, b.message, b.created_at "
                    "FROM major_import_batches b "
                    "LEFT JOIN users u ON u.id = b.triggered_by_user_id "
                    "WHERE b.id = ?",
                    batchId);
                tx.reset();
                Json::Value body;
                body["data"] = resultRows.empty() ? Json::Value(Json::objectValue)
                                                  : majorImportBatchRowToJson(resultRows[0]);
                callback(makeJson(body));
            } catch (const DrogonDbException &e) {
                if (tx) {
                    try {
                        tx->rollback();
                    } catch (...) {
                    }
                    tx.reset();
                }
                try {
                    db->execSqlSync(
                        "UPDATE major_import_batches "
                        "SET status = 'failed', finished_at = NOW(), message = ?, error_rows = error_rows + 1 "
                        "WHERE id = ?",
                        "import apply failed",
                        batchId);
                } catch (...) {
                }
                sendDbError(e, callback);
            } catch (const std::exception &e) {
                if (tx) {
                    try {
                        tx->rollback();
                    } catch (...) {
                    }
                    tx.reset();
                }
                try {
                    db->execSqlSync(
                        "UPDATE major_import_batches "
                        "SET status = 'failed', finished_at = NOW(), message = ? "
                        "WHERE id = ?",
                        std::string("import apply failed: ") + e.what(),
                        batchId);
                } catch (...) {
                }
                sendError(callback, drogon::k500InternalServerError, e.what(), "import_error");
            }
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/majors/import/batches",
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
            const auto status = trimCopy(req->getParameter("status"));
            static const std::unordered_set<std::string> kBatchStatus = {"preview", "applied", "failed"};
            if (!status.empty() && !validateEnumValue("status", status, kBatchStatus, callback)) {
                return;
            }

            const std::string sortOrder = normalizeSortOrder(req->getParameter("sort_order"));
            static const std::unordered_map<std::string, std::string> kSortMap = {
                {"id", "b.id"},
                {"status", "b.status"},
                {"source_type", "b.source_type"},
                {"started_at", "b.started_at"},
                {"finished_at", "b.finished_at"},
                {"created_at", "b.created_at"}};
            const std::string sortField = resolveSortField(req->getParameter("sort_by"), kSortMap, "b.id");
            const std::string whereSql = "WHERE (? = '' OR b.status = ?) ";
            const std::string fromSql = "FROM major_import_batches b LEFT JOIN users u ON u.id = b.triggered_by_user_id ";
            const std::string selectSql =
                "SELECT b.id, b.source_type, b.source_filename, b.status, b.triggered_by_user_id, "
                "u.username AS triggered_by_username, b.total_rows, b.valid_rows, b.invalid_rows, "
                "b.inserted_rows, b.updated_rows, b.skipped_rows, b.error_rows, "
                "b.started_at, b.finished_at, b.message, b.created_at ";

            db->execSqlAsync(
                "SELECT COUNT(*) AS total " + fromSql + whereSql,
                [callback, db, selectSql, fromSql, whereSql, sortField, sortOrder,
                 page, pageSize, offset, status]
                (const drogon::orm::Result &countResult) {
                    const auto total = static_cast<Json::UInt64>(countResult[0]["total"].as<long long>());
                    db->execSqlAsync(
                        selectSql + fromSql + whereSql + "ORDER BY " + sortField + " " + sortOrder +
                            " LIMIT ? OFFSET ?",
                        [callback, total, page, pageSize](const drogon::orm::Result &result) {
                            Json::Value list(Json::arrayValue);
                            for (const auto &row : result) {
                                list.append(majorImportBatchRowToJson(row));
                            }
                            Json::Value body;
                            body["data"] = list;
                            appendPagedMeta(body, total, page, pageSize);
                            callback(makeJson(body));
                        },
                        [callback](const DrogonDbException &e) {
                            sendDbError(e, callback);
                        },
                        status,
                        status,
                        pageSize,
                        offset);
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                status,
                status);
        },
        {drogon::Get});
}

}  // namespace gm
