#include "../app_routes.h"

#include "../app_support.h"

namespace gm {

void registerStudentEmploymentRoutes() {
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
            long long majorId = 0;
            long long companyId = 0;
            long long jobId = 0;
            if (!parseOptionalInt64Param(req, "student_id", callback, studentId)) {
                return;
            }
            if (!parseOptionalInt64Param(req, "major_id", callback, majorId)) {
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
                "AND (? = 0 OR s.major_id = ?) "
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
                 page, pageSize, offset, keywordRaw, like, studentId, majorId, companyId, jobId,
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
                        majorId,
                        majorId,
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
                majorId,
                majorId,
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
}

}  // namespace gm
