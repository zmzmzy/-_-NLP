#include "../app_routes.h"

#include "../app_support.h"

#include <chrono>
#include <sstream>
#include <unordered_set>

namespace gm {

namespace {

Json::Value employmentSubmissionRowToJson(const drogon::orm::Row &row) {
    Json::Value item(Json::objectValue);
    const bool hasCompany = !row["company_id"].isNull();
    const bool hasJob = !row["job_id"].isNull();
    const std::string proposedCompanyName = row["proposed_company_name"].isNull()
                                                ? std::string()
                                                : row["proposed_company_name"].as<std::string>();
    const std::string proposedJobTitle = row["proposed_job_title"].isNull()
                                             ? std::string()
                                             : row["proposed_job_title"].as<std::string>();
    const std::string companyName = !row["company_name"].isNull()
                                        ? row["company_name"].as<std::string>()
                                        : proposedCompanyName;
    const std::string jobTitle = !row["job_title"].isNull()
                                     ? row["job_title"].as<std::string>()
                                     : proposedJobTitle;

    item["id"] = static_cast<Json::Int64>(row["id"].as<long long>());
    item["submitter_user_id"] = static_cast<Json::Int64>(row["submitter_user_id"].as<long long>());
    item["submitter_username"] = row["submitter_username"].isNull()
                                     ? Json::Value(Json::nullValue)
                                     : Json::Value(row["submitter_username"].as<std::string>());
    item["student_id"] = static_cast<Json::Int64>(row["student_id"].as<long long>());
    item["student_name"] = row["student_name"].isNull()
                               ? Json::Value(Json::nullValue)
                               : Json::Value(row["student_name"].as<std::string>());
    item["company_id"] = hasCompany ? Json::Value(static_cast<Json::Int64>(row["company_id"].as<long long>()))
                                    : Json::Value(Json::nullValue);
    item["company_name"] = companyName.empty() ? Json::Value(Json::nullValue) : Json::Value(companyName);
    item["job_id"] = hasJob ? Json::Value(static_cast<Json::Int64>(row["job_id"].as<long long>()))
                            : Json::Value(Json::nullValue);
    item["job_title"] = jobTitle.empty() ? Json::Value(Json::nullValue) : Json::Value(jobTitle);
    item["company_source"] = hasCompany ? "existing" : "proposed";
    item["job_source"] = hasJob ? "existing" : (proposedJobTitle.empty() ? "none" : "proposed");
    item["proposed_company_name"] = proposedCompanyName.empty()
                                        ? Json::Value(Json::nullValue)
                                        : Json::Value(proposedCompanyName);
    item["proposed_company_main_business"] = row["proposed_company_main_business"].isNull()
                                                 ? Json::Value(Json::nullValue)
                                                 : Json::Value(row["proposed_company_main_business"].as<std::string>());
    item["proposed_company_flagship_products"] = row["proposed_company_flagship_products"].isNull()
                                                     ? Json::Value(Json::nullValue)
                                                     : Json::Value(row["proposed_company_flagship_products"].as<std::string>());
    item["proposed_company_domain_area"] = row["proposed_company_domain_area"].isNull()
                                               ? Json::Value(Json::nullValue)
                                               : Json::Value(row["proposed_company_domain_area"].as<std::string>());
    item["proposed_company_application_industries"] = row["proposed_company_application_industries"].isNull()
                                                           ? Json::Value(Json::nullValue)
                                                           : Json::Value(row["proposed_company_application_industries"].as<std::string>());
    item["proposed_company_province"] = row["proposed_company_province"].isNull()
                                            ? Json::Value(Json::nullValue)
                                            : Json::Value(row["proposed_company_province"].as<std::string>());
    item["proposed_company_city"] = row["proposed_company_city"].isNull()
                                        ? Json::Value(Json::nullValue)
                                        : Json::Value(row["proposed_company_city"].as<std::string>());
    item["proposed_job_title"] = proposedJobTitle.empty()
                                     ? Json::Value(Json::nullValue)
                                     : Json::Value(proposedJobTitle);
    item["proposed_job_type"] = row["proposed_job_type"].isNull()
                                    ? Json::Value(Json::nullValue)
                                    : Json::Value(row["proposed_job_type"].as<std::string>());
    item["proposed_job_min_degree"] = row["proposed_job_min_degree"].isNull()
                                          ? Json::Value(Json::nullValue)
                                          : Json::Value(row["proposed_job_min_degree"].as<std::string>());
    item["proposed_job_salary_range"] = row["proposed_job_salary_range"].isNull()
                                            ? Json::Value(Json::nullValue)
                                            : Json::Value(row["proposed_job_salary_range"].as<std::string>());
    item["proposed_job_location_province"] = row["proposed_job_location_province"].isNull()
                                                 ? Json::Value(Json::nullValue)
                                                 : Json::Value(row["proposed_job_location_province"].as<std::string>());
    item["proposed_job_location_city"] = row["proposed_job_location_city"].isNull()
                                             ? Json::Value(Json::nullValue)
                                             : Json::Value(row["proposed_job_location_city"].as<std::string>());
    item["proposed_job_responsibilities_text"] = row["proposed_job_responsibilities_text"].isNull()
                                                     ? Json::Value(Json::nullValue)
                                                     : Json::Value(row["proposed_job_responsibilities_text"].as<std::string>());
    item["proposed_job_technical_projects_text"] = row["proposed_job_technical_projects_text"].isNull()
                                                       ? Json::Value(Json::nullValue)
                                                       : Json::Value(row["proposed_job_technical_projects_text"].as<std::string>());
    item["proposed_job_required_knowledge_skills_text"] = row["proposed_job_required_knowledge_skills_text"].isNull()
                                                               ? Json::Value(Json::nullValue)
                                                               : Json::Value(row["proposed_job_required_knowledge_skills_text"].as<std::string>());
    item["proposed_job_required_tools_platform_text"] = row["proposed_job_required_tools_platform_text"].isNull()
                                                            ? Json::Value(Json::nullValue)
                                                            : Json::Value(row["proposed_job_required_tools_platform_text"].as<std::string>());
    item["employment_type"] = row["employment_type"].as<std::string>();
    item["employment_status"] = row["employment_status"].as<std::string>();
    item["start_date"] = row["start_date"].isNull()
                             ? Json::Value(Json::nullValue)
                             : Json::Value(row["start_date"].as<std::string>());
    item["end_date"] = row["end_date"].isNull()
                           ? Json::Value(Json::nullValue)
                           : Json::Value(row["end_date"].as<std::string>());
    item["evidence_source"] = row["evidence_source"].isNull()
                                  ? Json::Value(Json::nullValue)
                                  : Json::Value(row["evidence_source"].as<std::string>());
    item["notes"] = row["notes"].isNull()
                        ? Json::Value(Json::nullValue)
                        : Json::Value(row["notes"].as<std::string>());
    item["status"] = row["status"].as<std::string>();
    item["review_comment"] = row["review_comment"].isNull()
                                 ? Json::Value(Json::nullValue)
                                 : Json::Value(row["review_comment"].as<std::string>());
    item["reviewed_by_user_id"] = row["reviewed_by_user_id"].isNull()
                                      ? Json::Value(Json::nullValue)
                                      : Json::Value(static_cast<Json::Int64>(
                                            row["reviewed_by_user_id"].as<long long>()));
    item["reviewed_by_username"] = row["reviewed_by_username"].isNull()
                                       ? Json::Value(Json::nullValue)
                                       : Json::Value(row["reviewed_by_username"].as<std::string>());
    item["reviewed_at"] = row["reviewed_at"].isNull()
                              ? Json::Value(Json::nullValue)
                              : Json::Value(row["reviewed_at"].as<std::string>());
    item["approved_employment_record_id"] = row["approved_employment_record_id"].isNull()
                                                ? Json::Value(Json::nullValue)
                                                : Json::Value(static_cast<Json::Int64>(
                                                      row["approved_employment_record_id"].as<long long>()));
    item["created_at"] = row["created_at"].as<std::string>();
    item["updated_at"] = row["updated_at"].as<std::string>();
    return item;
}

drogon::orm::Result queryEmploymentSubmissionById(const drogon::orm::DbClientPtr &db,
                                                  long long id) {
    return db->execSqlSync(
        "SELECT es.id, es.submitter_user_id, su.username AS submitter_username, "
        "es.student_id, s.full_name AS student_name, "
        "es.company_id, c.name AS company_name, es.proposed_company_name, "
        "es.proposed_company_main_business, es.proposed_company_flagship_products, "
        "es.proposed_company_domain_area, es.proposed_company_application_industries, "
        "es.proposed_company_province, es.proposed_company_city, "
        "es.job_id, j.title AS job_title, es.proposed_job_title, es.proposed_job_type, "
        "es.proposed_job_min_degree, es.proposed_job_salary_range, "
        "es.proposed_job_location_province, es.proposed_job_location_city, "
        "es.proposed_job_responsibilities_text, es.proposed_job_technical_projects_text, "
        "es.proposed_job_required_knowledge_skills_text, es.proposed_job_required_tools_platform_text, "
        "es.employment_type, es.employment_status, es.start_date, es.end_date, "
        "es.evidence_source, es.notes, es.status, es.review_comment, "
        "es.reviewed_by_user_id, ru.username AS reviewed_by_username, es.reviewed_at, "
        "es.approved_employment_record_id, es.created_at, es.updated_at "
        "FROM employment_submissions es "
        "INNER JOIN users su ON su.id = es.submitter_user_id "
        "INNER JOIN students s ON s.id = es.student_id "
        "LEFT JOIN companies c ON c.id = es.company_id "
        "LEFT JOIN jobs j ON j.id = es.job_id "
        "LEFT JOIN users ru ON ru.id = es.reviewed_by_user_id "
        "WHERE es.id = ? LIMIT 1",
        id);
}

bool parseSubmissionStatus(const std::string &raw, std::string &status) {
    status = toLowerAscii(trimCopy(raw));
    if (status.empty()) {
        return true;
    }
    static const std::unordered_set<std::string> kStatuses = {"pending", "approved", "rejected"};
    return kStatuses.find(status) != kStatuses.end();
}

bool parseSubmissionIds(const Json::Value &idsNode,
                        std::vector<long long> &ids,
                        std::string &error) {
    ids.clear();
    error.clear();
    if (!idsNode.isArray()) {
        error = "ids array is required";
        return false;
    }
    std::unordered_set<long long> seen;
    for (Json::ArrayIndex i = 0; i < idsNode.size(); ++i) {
        if (!idsNode[i].isIntegral()) {
            continue;
        }
        const auto id = idsNode[i].asInt64();
        if (id <= 0 || seen.find(id) != seen.end()) {
            continue;
        }
        seen.insert(id);
        ids.push_back(id);
    }
    if (ids.empty()) {
        error = "no valid ids";
        return false;
    }
    if (ids.size() > 200) {
        error = "batch size cannot exceed 200";
        return false;
    }
    return true;
}

std::string csvEscape(const std::string &value) {
    bool needsQuote = false;
    std::string out;
    out.reserve(value.size() + 8);
    for (char c : value) {
        if (c == '"' || c == ',' || c == '\n' || c == '\r') {
            needsQuote = true;
        }
        if (c == '"') {
            out.push_back('"');
            out.push_back('"');
        } else {
            out.push_back(c);
        }
    }
    if (!needsQuote) {
        return out;
    }
    return "\"" + out + "\"";
}

std::string fieldToCsv(const drogon::orm::Row &row, const std::string &field) {
    if (row[field].isNull()) {
        return "";
    }
    return csvEscape(row[field].as<std::string>());
}

std::string rowTextOrEmpty(const drogon::orm::Row &row, const std::string &field) {
    return row[field].isNull() ? std::string() : trimCopy(row[field].as<std::string>());
}

std::string nonEmptyOrDefault(const std::string &value, const std::string &fallback) {
    const auto t = trimCopy(value);
    return t.empty() ? fallback : t;
}

std::string normalizeOptionalJobType(const std::string &raw) {
    const auto t = toLowerAscii(trimCopy(raw));
    if (t == "full_time" || t == "internship" || t == "contract") {
        return t;
    }
    return "";
}

std::string normalizeOptionalDegree(const std::string &raw) {
    const auto t = toLowerAscii(trimCopy(raw));
    if (t.empty()) {
        return "";
    }
    if (t == "none" || t == "diploma" || t == "bachelor" || t == "master" || t == "phd") {
        return t;
    }
    if (t == "doctor") {
        return "phd";
    }
    if (t == "graduate") {
        return "master";
    }
    if (t == "college") {
        return "diploma";
    }
    if (t == "no_limit") {
        return "none";
    }
    return "";
}

drogon::orm::Result queryEmploymentSubmissionForApproval(const drogon::orm::DbClientPtr &db,
                                                         long long id) {
    return db->execSqlSync(
        "SELECT id, status, student_id, company_id, job_id, "
        "proposed_company_name, proposed_company_main_business, proposed_company_flagship_products, "
        "proposed_company_domain_area, proposed_company_application_industries, "
        "proposed_company_province, proposed_company_city, "
        "proposed_job_title, proposed_job_type, proposed_job_min_degree, proposed_job_salary_range, "
        "proposed_job_location_province, proposed_job_location_city, "
        "proposed_job_responsibilities_text, proposed_job_technical_projects_text, "
        "proposed_job_required_knowledge_skills_text, proposed_job_required_tools_platform_text, "
        "employment_type, employment_status, start_date, end_date, evidence_source, notes "
        "FROM employment_submissions WHERE id = ? LIMIT 1 FOR UPDATE",
        id);
}

long long ensureCompanyForSubmission(const drogon::orm::DbClientPtr &db,
                                     const drogon::orm::Row &row,
                                     std::string &error) {
    error.clear();
    if (!row["company_id"].isNull()) {
        return row["company_id"].as<long long>();
    }
    const auto companyName = rowTextOrEmpty(row, "proposed_company_name");
    if (companyName.empty()) {
        error = "submission missing company information";
        return 0;
    }
    const auto existing = db->execSqlSync(
        "SELECT id FROM companies WHERE name = ? LIMIT 1",
        companyName);
    if (!existing.empty()) {
        return existing[0]["id"].as<long long>();
    }
    const auto mainBusiness = nonEmptyOrDefault(rowTextOrEmpty(row, "proposed_company_main_business"), "待补充");
    const auto flagshipProducts =
        nonEmptyOrDefault(rowTextOrEmpty(row, "proposed_company_flagship_products"), "待补充");
    const auto domainArea = nonEmptyOrDefault(rowTextOrEmpty(row, "proposed_company_domain_area"), "待补充");
    const auto applicationIndustries =
        nonEmptyOrDefault(rowTextOrEmpty(row, "proposed_company_application_industries"), "待补充");
    const auto province = rowTextOrEmpty(row, "proposed_company_province");
    const auto city = rowTextOrEmpty(row, "proposed_company_city");
    const auto insertRes = db->execSqlSync(
        "INSERT INTO companies("
        "name, main_business, flagship_products, domain_area, application_industries, "
        "province, city, website, description"
        ") VALUES(?, ?, ?, ?, ?, NULLIF(?, ''), NULLIF(?, ''), NULL, ?)",
        companyName,
        mainBusiness,
        flagshipProducts,
        domainArea,
        applicationIndustries,
        province,
        city,
        "submitted by student and approved by admin");
    return static_cast<long long>(insertRes.insertId());
}

long long ensureJobForSubmission(const drogon::orm::DbClientPtr &db,
                                 const drogon::orm::Row &row,
                                 long long companyId,
                                 std::string &error) {
    error.clear();
    if (companyId <= 0) {
        error = "company_id is required to resolve job";
        return 0;
    }
    if (!row["job_id"].isNull()) {
        const auto jobId = row["job_id"].as<long long>();
        const auto jobRows = db->execSqlSync(
            "SELECT id, company_id FROM jobs WHERE id = ? LIMIT 1",
            jobId);
        if (jobRows.empty()) {
            error = "job in submission no longer exists";
            return 0;
        }
        const auto jobCompanyId = jobRows[0]["company_id"].as<long long>();
        if (jobCompanyId != companyId) {
            error = "job company does not match submission company";
            return 0;
        }
        return jobId;
    }

    const auto title = rowTextOrEmpty(row, "proposed_job_title");
    if (title.empty()) {
        return 0;
    }
    const auto existing = db->execSqlSync(
        "SELECT id FROM jobs WHERE company_id = ? AND title = ? LIMIT 1",
        companyId,
        title);
    if (!existing.empty()) {
        return existing[0]["id"].as<long long>();
    }

    auto jobType = normalizeOptionalJobType(rowTextOrEmpty(row, "proposed_job_type"));
    if (jobType.empty()) {
        jobType = "full_time";
    }
    auto minDegree = normalizeOptionalDegree(rowTextOrEmpty(row, "proposed_job_min_degree"));
    if (minDegree.empty()) {
        minDegree = "bachelor";
    }
    const auto salaryRange = rowTextOrEmpty(row, "proposed_job_salary_range");
    const auto locationProvince = rowTextOrEmpty(row, "proposed_job_location_province");
    const auto locationCity = rowTextOrEmpty(row, "proposed_job_location_city");
    const auto responsibilities =
        nonEmptyOrDefault(rowTextOrEmpty(row, "proposed_job_responsibilities_text"), "待补充");
    const auto technicalProjects =
        nonEmptyOrDefault(rowTextOrEmpty(row, "proposed_job_technical_projects_text"), "待补充");
    const auto requiredKnowledgeSkills =
        nonEmptyOrDefault(rowTextOrEmpty(row, "proposed_job_required_knowledge_skills_text"), "待补充");
    const auto requiredToolsPlatform =
        nonEmptyOrDefault(rowTextOrEmpty(row, "proposed_job_required_tools_platform_text"), "待补充");

    const auto insertRes = db->execSqlSync(
        "INSERT INTO jobs("
        "company_id, title, job_type, location_province, location_city, min_degree, salary_range, "
        "responsibilities_text, technical_projects_text, required_knowledge_skills_text, "
        "required_tools_platform_text, published_at"
        ") VALUES(?, ?, ?, NULLIF(?, ''), NULLIF(?, ''), ?, NULLIF(?, ''), ?, ?, ?, ?, NOW())",
        companyId,
        title,
        jobType,
        locationProvince,
        locationCity,
        minDegree,
        salaryRange,
        responsibilities,
        technicalProjects,
        requiredKnowledgeSkills,
        requiredToolsPlatform);
    return static_cast<long long>(insertRes.insertId());
}

std::string buildStudentNoSeed(long long userId) {
    const auto now = std::chrono::system_clock::now();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now.time_since_epoch())
                            .count();
    return "U" + std::to_string(userId) + "_" + std::to_string(millis % 1000000000);
}

std::string ensureUniqueStudentNo(const drogon::orm::DbClientPtr &db,
                                  const std::string &preferred,
                                  long long userId) {
    std::string candidate = trimCopy(preferred);
    if (candidate.empty()) {
        candidate = buildStudentNoSeed(userId);
    }
    if (candidate.size() > 64) {
        candidate.resize(64);
    }

    for (int i = 0; i < 50; ++i) {
        const auto rows = db->execSqlSync(
            "SELECT id FROM students WHERE student_no = ? LIMIT 1",
            candidate);
        if (rows.empty()) {
            return candidate;
        }
        candidate = buildStudentNoSeed(userId) + "_" + std::to_string(i + 1);
        if (candidate.size() > 64) {
            candidate.resize(64);
        }
    }
    return "U" + std::to_string(userId) + "_" + drogon::utils::getUuid().substr(0, 8);
}

drogon::orm::Result queryStudentProfileById(const drogon::orm::DbClientPtr &db, long long studentId) {
    return db->execSqlSync(
        "SELECT s.id, s.student_no, s.full_name, s.gender, s.college_id, c.name AS college_name, "
        "s.major_id, m.name AS major_name, s.graduation_year, s.email, s.phone, s.resume_text, "
        "s.technical_projects_text, s.capability_summary_text, s.created_at, s.updated_at "
        "FROM students s "
        "INNER JOIN colleges c ON c.id = s.college_id "
        "INNER JOIN majors m ON m.id = s.major_id "
        "WHERE s.id = ? LIMIT 1",
        studentId);
}

}  // namespace

void registerProfileRoutes() {
    drogon::app().registerHandler(
        "/api/me/student-profile/bind",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            SessionUser user;
            if (!requireAuthUser(req, callback, user)) {
                return;
            }
            if (user.role == "admin") {
                sendError(callback, drogon::k403Forbidden, "admin account should manage data in management pages");
                return;
            }
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!requireFields(*json, {"student_no", "full_name"}, callback)) {
                return;
            }
            const auto studentNo = trimCopy((*json)["student_no"].asString());
            const auto fullName = trimCopy((*json)["full_name"].asString());
            if (studentNo.empty() || fullName.empty()) {
                sendError(callback, drogon::k400BadRequest, "student_no and full_name cannot be empty");
                return;
            }

            auto db = drogon::app().getDbClient();
            std::shared_ptr<drogon::orm::Transaction> tx;
            try {
                tx = db->newTransaction();

                const auto userRows = tx->execSqlSync(
                    "SELECT id, linked_student_id FROM users WHERE id = ? LIMIT 1 FOR UPDATE",
                    user.userId);
                if (userRows.empty()) {
                    tx->rollback();
                    tx.reset();
                    sendError(callback, drogon::k404NotFound, "user not found");
                    return;
                }

                auto linkedStudentId = userRows[0]["linked_student_id"].isNull()
                                           ? 0LL
                                           : userRows[0]["linked_student_id"].as<long long>();
                if (linkedStudentId > 0) {
                    const auto linkedRows = queryStudentProfileById(tx, linkedStudentId);
                    if (!linkedRows.empty()) {
                        tx.reset();
                        Json::Value body;
                        body["data"]["already_bound"] = true;
                        body["data"]["linked_student_id"] = static_cast<Json::Int64>(linkedStudentId);
                        body["data"]["student"] = studentRowToJson(linkedRows[0]);
                        callback(makeJson(body));
                        return;
                    }
                    // Stale link: clear and continue to bind.
                    tx->execSqlSync(
                        "UPDATE users SET linked_student_id = NULL WHERE id = ?",
                        user.userId);
                    linkedStudentId = 0;
                }

                const auto studentRows = tx->execSqlSync(
                    "SELECT id FROM students WHERE student_no = ? AND full_name = ? LIMIT 1 FOR UPDATE",
                    studentNo,
                    fullName);
                if (studentRows.empty()) {
                    tx->rollback();
                    tx.reset();
                    sendError(callback, drogon::k404NotFound, "student not found, please verify student_no and full_name");
                    return;
                }
                const auto studentId = studentRows[0]["id"].as<long long>();

                const auto occupiedRows = tx->execSqlSync(
                    "SELECT id FROM users WHERE linked_student_id = ? AND id <> ? LIMIT 1 FOR UPDATE",
                    studentId,
                    user.userId);
                if (!occupiedRows.empty()) {
                    tx->rollback();
                    tx.reset();
                    sendError(callback, drogon::k409Conflict, "this student is already bound to another account");
                    return;
                }

                tx->execSqlSync(
                    "UPDATE users SET linked_student_id = ? WHERE id = ?",
                    studentId,
                    user.userId);

                const auto boundRows = queryStudentProfileById(tx, studentId);
                tx.reset();

                Json::Value body;
                body["data"]["already_bound"] = false;
                body["data"]["linked_student_id"] = static_cast<Json::Int64>(studentId);
                body["data"]["student"] = boundRows.empty()
                                              ? Json::Value(Json::nullValue)
                                              : studentRowToJson(boundRows[0]);
                callback(makeJson(body));
            } catch (const DrogonDbException &e) {
                if (tx) {
                    try {
                        tx->rollback();
                    } catch (...) {
                    }
                    tx.reset();
                }
                const std::string msg = e.base().what();
                if (msg.find("Duplicate") != std::string::npos) {
                    sendError(callback, drogon::k409Conflict, "this student is already bound to another account");
                    return;
                }
                sendDbError(e, callback);
            }
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/me/student-profile",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            SessionUser user;
            if (!requireAuthUser(req, callback, user)) {
                return;
            }
            auto db = drogon::app().getDbClient();
            try {
                const auto userRows = db->execSqlSync(
                    "SELECT id, username, email, role, linked_student_id "
                    "FROM users WHERE id = ? LIMIT 1",
                    user.userId);
                if (userRows.empty()) {
                    sendError(callback, drogon::k404NotFound, "user not found");
                    return;
                }
                const auto &u = userRows[0];
                long long linkedStudentId = u["linked_student_id"].isNull()
                                                ? 0LL
                                                : u["linked_student_id"].as<long long>();

                Json::Value body;
                body["data"]["user"]["id"] = static_cast<Json::Int64>(u["id"].as<long long>());
                body["data"]["user"]["username"] = u["username"].as<std::string>();
                body["data"]["user"]["email"] = u["email"].isNull()
                                                    ? Json::Value(Json::nullValue)
                                                    : Json::Value(u["email"].as<std::string>());
                body["data"]["user"]["role"] = u["role"].as<std::string>();
                if (linkedStudentId > 0) {
                    const auto studentRows = queryStudentProfileById(db, linkedStudentId);
                    if (studentRows.empty()) {
                        // Auto-heal stale user->student link if referenced student has been deleted.
                        db->execSqlSync(
                            "UPDATE users SET linked_student_id = NULL WHERE id = ?",
                            user.userId);
                        linkedStudentId = 0;
                        body["data"]["student"] = Json::Value(Json::nullValue);
                    } else {
                        body["data"]["student"] = studentRowToJson(studentRows[0]);
                    }
                } else {
                    body["data"]["student"] = Json::Value(Json::nullValue);
                }
                body["data"]["user"]["linked_student_id"] = linkedStudentId <= 0
                                                                 ? Json::Value(Json::nullValue)
                                                                 : Json::Value(static_cast<Json::Int64>(
                                                                       linkedStudentId));
                callback(makeJson(body));
            } catch (const DrogonDbException &e) {
                sendDbError(e, callback);
            }
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/me/student-profile",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            SessionUser user;
            if (!requireAuthUser(req, callback, user)) {
                return;
            }
            if (user.role == "admin") {
                sendError(callback, drogon::k403Forbidden, "admin account should manage data in management pages");
                return;
            }
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (json->isMember("student_no") ||
                json->isMember("full_name") ||
                json->isMember("college_id") ||
                json->isMember("major_id") ||
                json->isMember("graduation_year")) {
                sendError(callback,
                          drogon::k400BadRequest,
                          "student_no/full_name/college_id/major_id/graduation_year are immutable after binding");
                return;
            }

            const auto gender = json->isMember("gender") ? trimCopy((*json)["gender"].asString()) : "unknown";
            const auto email = json->isMember("email") ? trimCopy((*json)["email"].asString()) : "";
            const auto phone = json->isMember("phone") ? trimCopy((*json)["phone"].asString()) : "";
            const auto resumeText = json->isMember("resume_text") ? (*json)["resume_text"].asString() : "";
            const auto technicalProjects =
                json->isMember("technical_projects_text") ? (*json)["technical_projects_text"].asString() : "";
            const auto capabilitySummary =
                json->isMember("capability_summary_text") ? (*json)["capability_summary_text"].asString() : "";
            static const std::unordered_set<std::string> kGenderValues = {"male", "female", "other", "unknown"};
            if (!validateEnumValue("gender", gender, kGenderValues, callback)) {
                return;
            }

            std::shared_ptr<drogon::orm::Transaction> tx;
            try {
                tx = db->newTransaction();

                const auto userRows = tx->execSqlSync(
                    "SELECT id, linked_student_id FROM users WHERE id = ? LIMIT 1 FOR UPDATE",
                    user.userId);
                if (userRows.empty()) {
                    tx->rollback();
                    tx.reset();
                    sendError(callback, drogon::k404NotFound, "user not found");
                    return;
                }
                long long linkedStudentId = userRows[0]["linked_student_id"].isNull()
                                                ? 0LL
                                                : userRows[0]["linked_student_id"].as<long long>();
                if (linkedStudentId <= 0) {
                    tx->rollback();
                    tx.reset();
                    sendError(callback,
                              drogon::k409Conflict,
                              "please bind student identity first with student_no and full_name");
                    return;
                }

                const auto linkedCheck = tx->execSqlSync(
                    "SELECT id FROM students WHERE id = ? LIMIT 1 FOR UPDATE",
                    linkedStudentId);
                if (linkedCheck.empty()) {
                    tx->execSqlSync(
                        "UPDATE users SET linked_student_id = NULL WHERE id = ?",
                        user.userId);
                    tx.reset();
                    sendError(callback,
                              drogon::k409Conflict,
                              "linked student profile not found, please bind student identity again");
                    return;
                }

                tx->execSqlSync(
                    "UPDATE students SET "
                    "gender = ?, "
                    "email = NULLIF(?, ''), phone = NULLIF(?, ''), resume_text = NULLIF(?, ''), "
                    "technical_projects_text = NULLIF(?, ''), capability_summary_text = NULLIF(?, '') "
                    "WHERE id = ?",
                    gender,
                    email,
                    phone,
                    resumeText,
                    technicalProjects,
                    capabilitySummary,
                    linkedStudentId);
                const auto studentRows = queryStudentProfileById(tx, linkedStudentId);
                tx.reset();

                Json::Value body;
                body["data"]["linked_student_id"] = static_cast<Json::Int64>(linkedStudentId);
                body["data"]["student"] = studentRows.empty()
                                              ? Json::Value(Json::nullValue)
                                              : studentRowToJson(studentRows[0]);
                callback(makeJson(body));
            } catch (const DrogonDbException &e) {
                if (tx) {
                    try {
                        tx->rollback();
                    } catch (...) {
                    }
                    tx.reset();
                }
                const std::string msg = e.base().what();
                if (msg.find("Data truncated") != std::string::npos) {
                    sendError(callback, drogon::k400BadRequest, "invalid enum value");
                    return;
                }
                sendDbError(e, callback);
            }
        },
        {drogon::Put});

    drogon::app().registerHandler(
        "/api/me/employment-submissions",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            SessionUser user;
            if (!requireAuthUser(req, callback, user)) {
                return;
            }
            auto db = drogon::app().getDbClient();
            int page = 1;
            int pageSize = 20;
            if (!parsePagingParams(req, callback, page, pageSize, 1, 20, 100)) {
                return;
            }
            const int offset = (page - 1) * pageSize;
            std::string status;
            if (!parseSubmissionStatus(req->getParameter("status"), status)) {
                sendError(callback, drogon::k400BadRequest, "invalid query param: status");
                return;
            }
            const std::string sortOrder = normalizeSortOrder(req->getParameter("sort_order"));
            const std::string whereSql =
                "WHERE es.submitter_user_id = ? AND (? = '' OR es.status = ?) ";
            const std::string baseSql =
                "FROM employment_submissions es "
                "INNER JOIN users su ON su.id = es.submitter_user_id "
                "INNER JOIN students s ON s.id = es.student_id "
                "LEFT JOIN companies c ON c.id = es.company_id "
                "LEFT JOIN jobs j ON j.id = es.job_id "
                "LEFT JOIN users ru ON ru.id = es.reviewed_by_user_id ";
            const std::string selectSql =
                "SELECT es.id, es.submitter_user_id, su.username AS submitter_username, "
                "es.student_id, s.full_name AS student_name, "
                "es.company_id, c.name AS company_name, es.proposed_company_name, "
                "es.proposed_company_main_business, es.proposed_company_flagship_products, "
                "es.proposed_company_domain_area, es.proposed_company_application_industries, "
                "es.proposed_company_province, es.proposed_company_city, "
                "es.job_id, j.title AS job_title, es.proposed_job_title, es.proposed_job_type, "
                "es.proposed_job_min_degree, es.proposed_job_salary_range, "
                "es.proposed_job_location_province, es.proposed_job_location_city, "
                "es.proposed_job_responsibilities_text, es.proposed_job_technical_projects_text, "
                "es.proposed_job_required_knowledge_skills_text, es.proposed_job_required_tools_platform_text, "
                "es.employment_type, es.employment_status, es.start_date, es.end_date, "
                "es.evidence_source, es.notes, es.status, es.review_comment, "
                "es.reviewed_by_user_id, ru.username AS reviewed_by_username, es.reviewed_at, "
                "es.approved_employment_record_id, es.created_at, es.updated_at ";
            db->execSqlAsync(
                "SELECT COUNT(*) AS total " + baseSql + whereSql,
                [callback, db, baseSql, selectSql, whereSql, page, pageSize, offset, sortOrder, user, status]
                (const drogon::orm::Result &countResult) {
                    const auto total = static_cast<Json::UInt64>(countResult[0]["total"].as<long long>());
                    db->execSqlAsync(
                        selectSql + baseSql + whereSql + "ORDER BY es.id " + sortOrder + " LIMIT ? OFFSET ?",
                        [callback, total, page, pageSize](const drogon::orm::Result &result) {
                            Json::Value list(Json::arrayValue);
                            for (const auto &row : result) {
                                list.append(employmentSubmissionRowToJson(row));
                            }
                            Json::Value body;
                            body["data"] = list;
                            appendPagedMeta(body, total, page, pageSize);
                            callback(makeJson(body));
                        },
                        [callback](const DrogonDbException &e) { sendDbError(e, callback); },
                        user.userId,
                        status,
                        status,
                        pageSize,
                        offset);
                },
                [callback](const DrogonDbException &e) { sendDbError(e, callback); },
                user.userId,
                status,
                status);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/me/employment-submissions",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            SessionUser user;
            if (!requireAuthUser(req, callback, user)) {
                return;
            }
            if (user.role == "admin") {
                sendError(callback, drogon::k403Forbidden, "admin account should review via admin panel");
                return;
            }
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            const bool hasCompanyId = json->isMember("company_id") && !(*json)["company_id"].isNull();
            long long companyId = hasCompanyId ? (*json)["company_id"].asInt64() : 0;
            const bool hasJobId = json->isMember("job_id") && !(*json)["job_id"].isNull();
            long long jobId = hasJobId ? (*json)["job_id"].asInt64() : 0;
            if (companyId < 0 || jobId < 0) {
                sendError(callback, drogon::k400BadRequest, "company_id/job_id must be non-negative");
                return;
            }
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
            const auto proposedCompanyName =
                trimCopy(json->isMember("proposed_company_name") ? (*json)["proposed_company_name"].asString() : "");
            const auto proposedCompanyMainBusiness =
                trimCopy(json->isMember("proposed_company_main_business") ? (*json)["proposed_company_main_business"].asString() : "");
            const auto proposedCompanyFlagshipProducts =
                trimCopy(json->isMember("proposed_company_flagship_products") ? (*json)["proposed_company_flagship_products"].asString() : "");
            const auto proposedCompanyDomainArea =
                trimCopy(json->isMember("proposed_company_domain_area") ? (*json)["proposed_company_domain_area"].asString() : "");
            const auto proposedCompanyApplicationIndustries =
                trimCopy(json->isMember("proposed_company_application_industries") ? (*json)["proposed_company_application_industries"].asString() : "");
            const auto proposedCompanyProvince =
                trimCopy(json->isMember("proposed_company_province") ? (*json)["proposed_company_province"].asString() : "");
            const auto proposedCompanyCity =
                trimCopy(json->isMember("proposed_company_city") ? (*json)["proposed_company_city"].asString() : "");
            const auto proposedJobTitle =
                trimCopy(json->isMember("proposed_job_title") ? (*json)["proposed_job_title"].asString() : "");
            const auto proposedJobTypeRaw =
                trimCopy(json->isMember("proposed_job_type") ? (*json)["proposed_job_type"].asString() : "");
            const auto proposedJobMinDegreeRaw =
                trimCopy(json->isMember("proposed_job_min_degree") ? (*json)["proposed_job_min_degree"].asString() : "");
            const auto proposedJobSalaryRange =
                trimCopy(json->isMember("proposed_job_salary_range") ? (*json)["proposed_job_salary_range"].asString() : "");
            const auto proposedJobLocationProvince =
                trimCopy(json->isMember("proposed_job_location_province") ? (*json)["proposed_job_location_province"].asString() : "");
            const auto proposedJobLocationCity =
                trimCopy(json->isMember("proposed_job_location_city") ? (*json)["proposed_job_location_city"].asString() : "");
            const auto proposedJobResponsibilities =
                trimCopy(json->isMember("proposed_job_responsibilities_text")
                             ? (*json)["proposed_job_responsibilities_text"].asString()
                             : "");
            const auto proposedJobTechnicalProjects =
                trimCopy(json->isMember("proposed_job_technical_projects_text")
                             ? (*json)["proposed_job_technical_projects_text"].asString()
                             : "");
            const auto proposedJobRequiredKnowledgeSkills =
                trimCopy(json->isMember("proposed_job_required_knowledge_skills_text")
                             ? (*json)["proposed_job_required_knowledge_skills_text"].asString()
                             : "");
            const auto proposedJobRequiredToolsPlatform =
                trimCopy(json->isMember("proposed_job_required_tools_platform_text")
                             ? (*json)["proposed_job_required_tools_platform_text"].asString()
                             : "");
            const auto proposedJobType = normalizeOptionalJobType(proposedJobTypeRaw);
            const auto proposedJobMinDegree = normalizeOptionalDegree(proposedJobMinDegreeRaw);
            if (!proposedJobTypeRaw.empty() && proposedJobType.empty()) {
                sendError(callback, drogon::k400BadRequest, "invalid proposed_job_type");
                return;
            }
            if (!proposedJobMinDegreeRaw.empty() && proposedJobMinDegree.empty()) {
                sendError(callback, drogon::k400BadRequest, "invalid proposed_job_min_degree");
                return;
            }
            const auto startDate = trimCopy(json->isMember("start_date") ? (*json)["start_date"].asString() : "");
            const auto endDate = trimCopy(json->isMember("end_date") ? (*json)["end_date"].asString() : "");
            const auto evidenceSource = trimCopy(json->isMember("evidence_source") ? (*json)["evidence_source"].asString() : "");
            const auto notes = trimCopy(json->isMember("notes") ? (*json)["notes"].asString() : "");
            if (companyId <= 0 && proposedCompanyName.empty() && jobId <= 0) {
                sendError(callback, drogon::k400BadRequest, "company_id or proposed_company_name is required");
                return;
            }

            try {
                if (jobId > 0) {
                    const auto jobRows = db->execSqlSync(
                        "SELECT id, company_id FROM jobs WHERE id = ? LIMIT 1",
                        jobId);
                    if (jobRows.empty()) {
                        sendError(callback, drogon::k400BadRequest, "job_id is invalid");
                        return;
                    }
                    const auto jobCompanyId = jobRows[0]["company_id"].as<long long>();
                    if (companyId > 0 && companyId != jobCompanyId) {
                        sendError(callback, drogon::k400BadRequest, "company_id does not match job_id");
                        return;
                    }
                    if (companyId <= 0) {
                        companyId = jobCompanyId;
                    }
                }
                const auto userRows = db->execSqlSync(
                    "SELECT linked_student_id FROM users WHERE id = ? LIMIT 1",
                    user.userId);
                if (userRows.empty()) {
                    sendError(callback, drogon::k404NotFound, "user not found");
                    return;
                }
                const auto studentId = userRows[0]["linked_student_id"].isNull()
                                           ? 0LL
                                           : userRows[0]["linked_student_id"].as<long long>();
                if (studentId <= 0) {
                    sendError(callback,
                              drogon::k409Conflict,
                              "please complete your student profile before submitting employment");
                    return;
                }
                const auto studentRows = db->execSqlSync(
                    "SELECT id FROM students WHERE id = ? LIMIT 1",
                    studentId);
                if (studentRows.empty()) {
                    // Auto-heal stale link and ask user to re-save profile.
                    db->execSqlSync(
                        "UPDATE users SET linked_student_id = NULL WHERE id = ?",
                        user.userId);
                    sendError(callback,
                              drogon::k409Conflict,
                              "linked student profile not found, please save your student profile again");
                    return;
                }

                const auto insertRes = db->execSqlSync(
                    "INSERT INTO employment_submissions("
                    "submitter_user_id, student_id, company_id, job_id, "
                    "proposed_company_name, proposed_company_main_business, proposed_company_flagship_products, "
                    "proposed_company_domain_area, proposed_company_application_industries, "
                    "proposed_company_province, proposed_company_city, "
                    "proposed_job_title, proposed_job_type, proposed_job_min_degree, proposed_job_salary_range, "
                    "proposed_job_location_province, proposed_job_location_city, "
                    "proposed_job_responsibilities_text, proposed_job_technical_projects_text, "
                    "proposed_job_required_knowledge_skills_text, proposed_job_required_tools_platform_text, "
                    "employment_type, employment_status, start_date, end_date, evidence_source, notes, status"
                    ") VALUES("
                    "?, ?, NULLIF(?, 0), NULLIF(?, 0), "
                    "NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''), "
                    "NULLIF(?, ''), NULLIF(?, ''), "
                    "NULLIF(?, ''), NULLIF(?, ''), "
                    "NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''), "
                    "NULLIF(?, ''), NULLIF(?, ''), "
                    "NULLIF(?, ''), NULLIF(?, ''), "
                    "NULLIF(?, ''), NULLIF(?, ''), "
                    "?, ?, NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''), 'pending')",
                    user.userId,
                    studentId,
                    companyId,
                    jobId,
                    proposedCompanyName,
                    proposedCompanyMainBusiness,
                    proposedCompanyFlagshipProducts,
                    proposedCompanyDomainArea,
                    proposedCompanyApplicationIndustries,
                    proposedCompanyProvince,
                    proposedCompanyCity,
                    proposedJobTitle,
                    proposedJobType,
                    proposedJobMinDegree,
                    proposedJobSalaryRange,
                    proposedJobLocationProvince,
                    proposedJobLocationCity,
                    proposedJobResponsibilities,
                    proposedJobTechnicalProjects,
                    proposedJobRequiredKnowledgeSkills,
                    proposedJobRequiredToolsPlatform,
                    employmentType,
                    employmentStatus,
                    startDate,
                    endDate,
                    evidenceSource,
                    notes);
                const auto newId = static_cast<long long>(insertRes.insertId());

                const auto row = queryEmploymentSubmissionById(db, newId);
                Json::Value body;
                body["data"] = row.empty() ? Json::Value(Json::objectValue)
                                           : employmentSubmissionRowToJson(row[0]);
                callback(makeJson(body, drogon::k201Created));
            } catch (const DrogonDbException &e) {
                const std::string msg = e.base().what();
                if (msg.find("foreign key constraint fails") != std::string::npos) {
                    sendError(callback, drogon::k400BadRequest, "company_id/job_id is invalid");
                    return;
                }
                if (msg.find("Data truncated") != std::string::npos) {
                    sendError(callback, drogon::k400BadRequest, "invalid enum or date format");
                    return;
                }
                sendDbError(e, callback);
            }
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/admin/employment-submissions",
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
            std::string status;
            if (!parseSubmissionStatus(req->getParameter("status"), status)) {
                sendError(callback, drogon::k400BadRequest, "invalid query param: status");
                return;
            }
            const auto keywordRaw = trimCopy(req->getParameter("keyword"));
            const auto keywordLike = "%" + keywordRaw + "%";
            const std::string sortOrder = normalizeSortOrder(req->getParameter("sort_order"));
            const std::string whereSql =
                "WHERE (? = '' OR es.status = ?) "
                "AND (? = '' OR s.full_name LIKE ? OR c.name LIKE ? OR j.title LIKE ? OR su.username LIKE ? "
                "OR es.proposed_company_name LIKE ? OR es.proposed_job_title LIKE ?) ";
            const std::string baseSql =
                "FROM employment_submissions es "
                "INNER JOIN users su ON su.id = es.submitter_user_id "
                "INNER JOIN students s ON s.id = es.student_id "
                "LEFT JOIN companies c ON c.id = es.company_id "
                "LEFT JOIN jobs j ON j.id = es.job_id "
                "LEFT JOIN users ru ON ru.id = es.reviewed_by_user_id ";
            const std::string selectSql =
                "SELECT es.id, es.submitter_user_id, su.username AS submitter_username, "
                "es.student_id, s.full_name AS student_name, "
                "es.company_id, c.name AS company_name, es.proposed_company_name, "
                "es.proposed_company_main_business, es.proposed_company_flagship_products, "
                "es.proposed_company_domain_area, es.proposed_company_application_industries, "
                "es.proposed_company_province, es.proposed_company_city, "
                "es.job_id, j.title AS job_title, es.proposed_job_title, es.proposed_job_type, "
                "es.proposed_job_min_degree, es.proposed_job_salary_range, "
                "es.proposed_job_location_province, es.proposed_job_location_city, "
                "es.proposed_job_responsibilities_text, es.proposed_job_technical_projects_text, "
                "es.proposed_job_required_knowledge_skills_text, es.proposed_job_required_tools_platform_text, "
                "es.employment_type, es.employment_status, es.start_date, es.end_date, "
                "es.evidence_source, es.notes, es.status, es.review_comment, "
                "es.reviewed_by_user_id, ru.username AS reviewed_by_username, es.reviewed_at, "
                "es.approved_employment_record_id, es.created_at, es.updated_at ";
            db->execSqlAsync(
                "SELECT COUNT(*) AS total " + baseSql + whereSql,
                [callback, db, baseSql, selectSql, whereSql, page, pageSize, offset, status, keywordRaw, keywordLike, sortOrder]
                (const drogon::orm::Result &countResult) {
                    const auto total = static_cast<Json::UInt64>(countResult[0]["total"].as<long long>());
                    db->execSqlAsync(
                        selectSql + baseSql + whereSql + "ORDER BY es.id " + sortOrder + " LIMIT ? OFFSET ?",
                        [callback, total, page, pageSize](const drogon::orm::Result &result) {
                            Json::Value list(Json::arrayValue);
                            for (const auto &row : result) {
                                list.append(employmentSubmissionRowToJson(row));
                            }
                            Json::Value body;
                            body["data"] = list;
                            appendPagedMeta(body, total, page, pageSize);
                            callback(makeJson(body));
                        },
                        [callback](const DrogonDbException &e) { sendDbError(e, callback); },
                        status,
                        status,
                        keywordRaw,
                        keywordLike,
                        keywordLike,
                        keywordLike,
                        keywordLike,
                        keywordLike,
                        keywordLike,
                        pageSize,
                        offset);
                },
                [callback](const DrogonDbException &e) { sendDbError(e, callback); },
                status,
                status,
                keywordRaw,
                keywordLike,
                keywordLike,
                keywordLike,
                keywordLike,
                keywordLike,
                keywordLike);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/admin/employment-submissions/{1}/approve",
        [](const HttpRequestPtr &req, DbCallback &&callback, long long id) {
            SessionUser user;
            if (!requireAuthUser(req, callback, user, true)) {
                return;
            }
            auto db = drogon::app().getDbClient();
            std::string reviewComment;
            if (const auto *json = req->getJsonObject().get(); json != nullptr &&
                json->isObject() && json->isMember("review_comment")) {
                reviewComment = (*json)["review_comment"].asString();
            }

            std::shared_ptr<drogon::orm::Transaction> tx;
            try {
                tx = db->newTransaction();
                const auto rows = queryEmploymentSubmissionForApproval(tx, id);
                if (rows.empty()) {
                    tx->rollback();
                    tx.reset();
                    sendError(callback, drogon::k404NotFound, "submission not found");
                    return;
                }
                const auto &row = rows[0];
                const auto status = row["status"].as<std::string>();
                if (status != "pending") {
                    tx->rollback();
                    tx.reset();
                    sendError(callback, drogon::k409Conflict, "submission is not pending");
                    return;
                }

                std::string resolveError;
                const auto resolvedCompanyId = ensureCompanyForSubmission(tx, row, resolveError);
                if (resolvedCompanyId <= 0) {
                    tx->rollback();
                    tx.reset();
                    sendError(callback, drogon::k400BadRequest, resolveError.empty() ? "failed to resolve company" : resolveError);
                    return;
                }
                const auto resolvedJobId = ensureJobForSubmission(tx, row, resolvedCompanyId, resolveError);
                if (!resolveError.empty()) {
                    tx->rollback();
                    tx.reset();
                    sendError(callback, drogon::k400BadRequest, resolveError);
                    return;
                }
                const auto startDate = rowTextOrEmpty(row, "start_date");
                const auto endDate = rowTextOrEmpty(row, "end_date");
                const auto evidenceSource = rowTextOrEmpty(row, "evidence_source");
                const auto notes = rowTextOrEmpty(row, "notes");
                const auto insertRes = tx->execSqlSync(
                    "INSERT INTO employment_records("
                    "student_id, company_id, job_id, employment_type, employment_status, "
                    "start_date, end_date, evidence_source, notes"
                    ") VALUES(?, ?, NULLIF(?, 0), ?, ?, NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''))",
                    row["student_id"].as<long long>(),
                    resolvedCompanyId,
                    resolvedJobId,
                    row["employment_type"].as<std::string>(),
                    row["employment_status"].as<std::string>(),
                    startDate,
                    endDate,
                    evidenceSource,
                    notes);
                const auto employmentId = static_cast<long long>(insertRes.insertId());

                tx->execSqlSync(
                    "UPDATE employment_submissions "
                    "SET company_id = ?, job_id = NULLIF(?, 0), "
                    "status = 'approved', reviewed_by_user_id = ?, reviewed_at = NOW(), "
                    "review_comment = NULLIF(?, ''), approved_employment_record_id = ? "
                    "WHERE id = ?",
                    resolvedCompanyId,
                    resolvedJobId,
                    user.userId,
                    reviewComment,
                    employmentId,
                    id);
                const auto resultRows = queryEmploymentSubmissionById(tx, id);
                tx.reset();

                Json::Value body;
                body["data"] = resultRows.empty() ? Json::Value(Json::objectValue)
                                                  : employmentSubmissionRowToJson(resultRows[0]);
                callback(makeJson(body));
            } catch (const DrogonDbException &e) {
                if (tx) {
                    try {
                        tx->rollback();
                    } catch (...) {
                    }
                    tx.reset();
                }
                sendDbError(e, callback);
            }
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/admin/employment-submissions/{1}/reject",
        [](const HttpRequestPtr &req, DbCallback &&callback, long long id) {
            SessionUser user;
            if (!requireAuthUser(req, callback, user, true)) {
                return;
            }
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            const auto reviewComment = json->isMember("review_comment")
                                           ? (*json)["review_comment"].asString()
                                           : std::string("rejected by admin");

            std::shared_ptr<drogon::orm::Transaction> tx;
            try {
                tx = db->newTransaction();
                const auto rows = tx->execSqlSync(
                    "SELECT id, status FROM employment_submissions WHERE id = ? LIMIT 1 FOR UPDATE",
                    id);
                if (rows.empty()) {
                    tx->rollback();
                    tx.reset();
                    sendError(callback, drogon::k404NotFound, "submission not found");
                    return;
                }
                if (rows[0]["status"].as<std::string>() != "pending") {
                    tx->rollback();
                    tx.reset();
                    sendError(callback, drogon::k409Conflict, "submission is not pending");
                    return;
                }

                tx->execSqlSync(
                    "UPDATE employment_submissions "
                    "SET status = 'rejected', reviewed_by_user_id = ?, reviewed_at = NOW(), "
                    "review_comment = NULLIF(?, '') "
                    "WHERE id = ?",
                    user.userId,
                    reviewComment,
                    id);
                const auto resultRows = queryEmploymentSubmissionById(tx, id);
                tx.reset();

                Json::Value body;
                body["data"] = resultRows.empty() ? Json::Value(Json::objectValue)
                                                  : employmentSubmissionRowToJson(resultRows[0]);
                callback(makeJson(body));
            } catch (const DrogonDbException &e) {
                if (tx) {
                    try {
                        tx->rollback();
                    } catch (...) {
                    }
                    tx.reset();
                }
                sendDbError(e, callback);
            }
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/admin/employment-submissions/batch-approve",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            SessionUser user;
            if (!requireAuthUser(req, callback, user, true)) {
                return;
            }
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!json->isMember("ids")) {
                sendError(callback, drogon::k400BadRequest, "ids array is required");
                return;
            }
            const auto reviewComment = json->isMember("review_comment")
                                           ? (*json)["review_comment"].asString()
                                           : std::string();
            std::vector<long long> ids;
            std::string idParseError;
            if (!parseSubmissionIds((*json)["ids"], ids, idParseError)) {
                if (idParseError == "no valid ids") {
                    sendError(callback, drogon::k400BadRequest, "no valid ids to approve");
                } else {
                    sendError(callback, drogon::k400BadRequest, idParseError);
                }
                return;
            }

            auto db = drogon::app().getDbClient();
            std::shared_ptr<drogon::orm::Transaction> tx;
            try {
                tx = db->newTransaction();
                Json::Value approvedIds(Json::arrayValue);
                Json::Value skippedIds(Json::arrayValue);
                int approvedCount = 0;
                int skippedCount = 0;

                for (long long id : ids) {
                    const auto rows = queryEmploymentSubmissionForApproval(tx, id);
                    if (rows.empty() || rows[0]["status"].as<std::string>() != "pending") {
                        skippedIds.append(static_cast<Json::Int64>(id));
                        ++skippedCount;
                        continue;
                    }
                    const auto &row = rows[0];
                    std::string resolveError;
                    const auto resolvedCompanyId = ensureCompanyForSubmission(tx, row, resolveError);
                    if (resolvedCompanyId <= 0) {
                        skippedIds.append(static_cast<Json::Int64>(id));
                        ++skippedCount;
                        continue;
                    }
                    const auto resolvedJobId = ensureJobForSubmission(tx, row, resolvedCompanyId, resolveError);
                    if (!resolveError.empty()) {
                        skippedIds.append(static_cast<Json::Int64>(id));
                        ++skippedCount;
                        continue;
                    }
                    const auto startDate = rowTextOrEmpty(row, "start_date");
                    const auto endDate = rowTextOrEmpty(row, "end_date");
                    const auto evidenceSource = rowTextOrEmpty(row, "evidence_source");
                    const auto notes = rowTextOrEmpty(row, "notes");
                    const auto insertRes = tx->execSqlSync(
                        "INSERT INTO employment_records("
                        "student_id, company_id, job_id, employment_type, employment_status, "
                        "start_date, end_date, evidence_source, notes"
                        ") VALUES(?, ?, NULLIF(?, 0), ?, ?, NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''))",
                        row["student_id"].as<long long>(),
                        resolvedCompanyId,
                        resolvedJobId,
                        row["employment_type"].as<std::string>(),
                        row["employment_status"].as<std::string>(),
                        startDate,
                        endDate,
                        evidenceSource,
                        notes);
                    const auto employmentId = static_cast<long long>(insertRes.insertId());
                    tx->execSqlSync(
                        "UPDATE employment_submissions "
                        "SET company_id = ?, job_id = NULLIF(?, 0), "
                        "status = 'approved', reviewed_by_user_id = ?, reviewed_at = NOW(), "
                        "review_comment = NULLIF(?, ''), approved_employment_record_id = ? "
                        "WHERE id = ?",
                        resolvedCompanyId,
                        resolvedJobId,
                        user.userId,
                        reviewComment,
                        employmentId,
                        id);
                    approvedIds.append(static_cast<Json::Int64>(id));
                    ++approvedCount;
                }

                tx.reset();
                Json::Value body;
                body["data"]["total_ids"] = static_cast<Json::Int64>(ids.size());
                body["data"]["approved_count"] = approvedCount;
                body["data"]["skipped_count"] = skippedCount;
                body["data"]["approved_ids"] = approvedIds;
                body["data"]["skipped_ids"] = skippedIds;
                callback(makeJson(body));
            } catch (const DrogonDbException &e) {
                if (tx) {
                    try {
                        tx->rollback();
                    } catch (...) {
                    }
                    tx.reset();
                }
                sendDbError(e, callback);
            }
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/admin/employment-submissions/batch-reject",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            SessionUser user;
            if (!requireAuthUser(req, callback, user, true)) {
                return;
            }
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!json->isMember("ids")) {
                sendError(callback, drogon::k400BadRequest, "ids array is required");
                return;
            }
            const auto reviewComment = json->isMember("review_comment")
                                           ? (*json)["review_comment"].asString()
                                           : std::string("rejected by admin");
            std::vector<long long> ids;
            std::string idParseError;
            if (!parseSubmissionIds((*json)["ids"], ids, idParseError)) {
                if (idParseError == "no valid ids") {
                    sendError(callback, drogon::k400BadRequest, "no valid ids to reject");
                } else {
                    sendError(callback, drogon::k400BadRequest, idParseError);
                }
                return;
            }

            auto db = drogon::app().getDbClient();
            std::shared_ptr<drogon::orm::Transaction> tx;
            try {
                tx = db->newTransaction();
                Json::Value rejectedIds(Json::arrayValue);
                Json::Value skippedIds(Json::arrayValue);
                int rejectedCount = 0;
                int skippedCount = 0;

                for (long long id : ids) {
                    const auto rows = tx->execSqlSync(
                        "SELECT id, status FROM employment_submissions "
                        "WHERE id = ? LIMIT 1 FOR UPDATE",
                        id);
                    if (rows.empty() || rows[0]["status"].as<std::string>() != "pending") {
                        skippedIds.append(static_cast<Json::Int64>(id));
                        ++skippedCount;
                        continue;
                    }
                    tx->execSqlSync(
                        "UPDATE employment_submissions "
                        "SET status = 'rejected', reviewed_by_user_id = ?, reviewed_at = NOW(), "
                        "review_comment = NULLIF(?, '') "
                        "WHERE id = ?",
                        user.userId,
                        reviewComment,
                        id);
                    rejectedIds.append(static_cast<Json::Int64>(id));
                    ++rejectedCount;
                }

                tx.reset();
                Json::Value body;
                body["data"]["total_ids"] = static_cast<Json::Int64>(ids.size());
                body["data"]["rejected_count"] = rejectedCount;
                body["data"]["skipped_count"] = skippedCount;
                body["data"]["rejected_ids"] = rejectedIds;
                body["data"]["skipped_ids"] = skippedIds;
                callback(makeJson(body));
            } catch (const DrogonDbException &e) {
                if (tx) {
                    try {
                        tx->rollback();
                    } catch (...) {
                    }
                    tx.reset();
                }
                sendDbError(e, callback);
            }
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/admin/employment-submissions/export",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            SessionUser user;
            if (!requireAuthUser(req, callback, user, true)) {
                return;
            }
            auto db = drogon::app().getDbClient();
            std::string status;
            if (!parseSubmissionStatus(req->getParameter("status"), status)) {
                sendError(callback, drogon::k400BadRequest, "invalid query param: status");
                return;
            }
            const auto keywordRaw = trimCopy(req->getParameter("keyword"));
            const auto keywordLike = "%" + keywordRaw + "%";
            const std::string sortOrder = normalizeSortOrder(req->getParameter("sort_order"));
            int exportLimit = 2000;
            const auto limitRaw = trimCopy(req->getParameter("limit"));
            if (!limitRaw.empty()) {
                int parsed = 0;
                if (!parseStrictInt(limitRaw, parsed) || parsed < 1 || parsed > 10000) {
                    sendError(callback, drogon::k400BadRequest, "invalid query param: limit");
                    return;
                }
                exportLimit = parsed;
            }

            const std::string whereSql =
                "WHERE (? = '' OR es.status = ?) "
                "AND (? = '' OR s.full_name LIKE ? OR c.name LIKE ? OR j.title LIKE ? OR su.username LIKE ? "
                "OR es.proposed_company_name LIKE ? OR es.proposed_job_title LIKE ?) ";
            const std::string baseSql =
                "FROM employment_submissions es "
                "INNER JOIN users su ON su.id = es.submitter_user_id "
                "INNER JOIN students s ON s.id = es.student_id "
                "LEFT JOIN companies c ON c.id = es.company_id "
                "LEFT JOIN jobs j ON j.id = es.job_id "
                "LEFT JOIN users ru ON ru.id = es.reviewed_by_user_id ";
            const std::string selectSql =
                "SELECT es.id, es.status, s.student_no, s.full_name AS student_name, "
                "COALESCE(c.name, es.proposed_company_name) AS company_name, "
                "COALESCE(j.title, es.proposed_job_title) AS job_title, "
                "es.employment_type, es.employment_status, "
                "es.start_date, es.end_date, es.evidence_source, es.notes, "
                "su.username AS submitter_username, ru.username AS reviewed_by_username, "
                "es.review_comment, es.created_at, es.reviewed_at ";

            try {
                const auto result = db->execSqlSync(
                    selectSql + baseSql + whereSql + "ORDER BY es.id " + sortOrder + " LIMIT ?",
                    status,
                    status,
                    keywordRaw,
                    keywordLike,
                    keywordLike,
                    keywordLike,
                    keywordLike,
                    keywordLike,
                    keywordLike,
                    exportLimit);

                std::ostringstream csv;
                csv << "id,status,student_no,student_name,company_name,job_title,employment_type,employment_status,"
                       "start_date,end_date,evidence_source,notes,submitter_username,reviewed_by_username,review_comment,"
                       "created_at,reviewed_at\n";
                for (const auto &row : result) {
                    csv << row["id"].as<long long>() << ","
                        << csvEscape(row["status"].as<std::string>()) << ","
                        << fieldToCsv(row, "student_no") << ","
                        << fieldToCsv(row, "student_name") << ","
                        << fieldToCsv(row, "company_name") << ","
                        << fieldToCsv(row, "job_title") << ","
                        << fieldToCsv(row, "employment_type") << ","
                        << fieldToCsv(row, "employment_status") << ","
                        << fieldToCsv(row, "start_date") << ","
                        << fieldToCsv(row, "end_date") << ","
                        << fieldToCsv(row, "evidence_source") << ","
                        << fieldToCsv(row, "notes") << ","
                        << fieldToCsv(row, "submitter_username") << ","
                        << fieldToCsv(row, "reviewed_by_username") << ","
                        << fieldToCsv(row, "review_comment") << ","
                        << fieldToCsv(row, "created_at") << ","
                        << fieldToCsv(row, "reviewed_at") << "\n";
                }

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setBody(csv.str());
                resp->setContentTypeString("text/csv; charset=utf-8");
                const auto ts = std::chrono::duration_cast<std::chrono::seconds>(
                                    std::chrono::system_clock::now().time_since_epoch())
                                    .count();
                resp->addHeader("Content-Disposition",
                                "attachment; filename=\"employment_submissions_" +
                                    std::to_string(ts) + ".csv\"");
                callback(resp);
            } catch (const DrogonDbException &e) {
                sendDbError(e, callback);
            }
        },
        {drogon::Get});
}

}  // namespace gm
