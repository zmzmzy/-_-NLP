#include "../app_routes.h"

#include "../app_support.h"

namespace gm {

namespace {

Json::Value jobImportBatchRowToJson(const drogon::orm::Row &row) {
    Json::Value item(Json::objectValue);
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

std::string companyTitleKey(const std::string &companyName, const std::string &title) {
    return toLowerAscii(trimCopy(companyName)) + "|" + toLowerAscii(trimCopy(title));
}

std::string jobImportDedupKey(const Json::Value &normalizedRow) {
    return "title:" + companyTitleKey(
                          jsonValueAsString(normalizedRow, "company_name"),
                          jsonValueAsString(normalizedRow, "title"));
}

bool normalizeJobImportRow(const Json::Value &rawRow,
                           Json::Value &normalizedRow,
                           std::string &error) {
    normalizedRow = Json::Value(Json::objectValue);
    error.clear();

    std::string companyName = normalizeImportText(jsonValueAsString(rawRow, "company_name", "enterprise_name"), 255);
    if (companyName.empty()) {
        companyName = normalizeImportText(jsonValueAsString(rawRow, "company"), 255);
    }
    const auto title = normalizeImportText(jsonValueAsString(rawRow, "title", "job_title"), 255);
    if (companyName.empty()) {
        error = "company_name is required";
        return false;
    }
    if (title.empty()) {
        error = "title is required";
        return false;
    }

    auto jobType = normalizeJobTypeValue(jsonValueAsString(rawRow, "job_type", "employment_type"));
    if (jobType.empty()) {
        jobType = "full_time";
    }
    auto minDegree = normalizeDegreeValue(jsonValueAsString(rawRow, "min_degree"));
    if (minDegree.empty()) {
        minDegree = "bachelor";
    }
    std::string publishedAt;
    const auto publishedAtRaw = trimCopy(jsonValueAsString(rawRow, "published_at"));
    if (!publishedAtRaw.empty()) {
        publishedAt = normalizeDateTimeValue(publishedAtRaw);
        if (publishedAt.empty()) {
            error = "invalid published_at datetime";
            return false;
        }
    }

    auto responsibilities = normalizeImportText(jsonValueAsString(rawRow, "responsibilities_text"), 65535);
    auto technicalProjects = normalizeImportText(jsonValueAsString(rawRow, "technical_projects_text"), 65535);
    auto requiredKnowledge = normalizeImportText(jsonValueAsString(rawRow, "required_knowledge_skills_text"), 65535);
    auto requiredTools = normalizeImportText(jsonValueAsString(rawRow, "required_tools_platform_text"), 65535);
    const auto sharedDescription = normalizeImportText(jsonValueAsString(rawRow, "description"), 2048);
    if (responsibilities.empty()) {
        responsibilities = sharedDescription.empty() ? "待补充" : sharedDescription;
    }
    if (technicalProjects.empty()) {
        technicalProjects = sharedDescription.empty() ? "待补充" : sharedDescription;
    }
    if (requiredKnowledge.empty()) {
        requiredKnowledge = sharedDescription.empty() ? "待补充" : sharedDescription;
    }
    if (requiredTools.empty()) {
        requiredTools = sharedDescription.empty() ? "待补充" : sharedDescription;
    }

    std::string domainArea = normalizeImportText(jsonValueAsString(rawRow, "domain_area"), 512);
    std::string mainBusiness = normalizeImportText(jsonValueAsString(rawRow, "main_business"), 65535);
    std::string flagshipProducts = normalizeImportText(jsonValueAsString(rawRow, "flagship_products"), 65535);
    std::string applicationIndustries = normalizeImportText(jsonValueAsString(rawRow, "application_industries"), 65535);
    if (domainArea.empty()) {
        domainArea = "导入待补充";
    }
    if (mainBusiness.empty()) {
        mainBusiness = "导入待补充";
    }
    if (flagshipProducts.empty()) {
        flagshipProducts = "导入待补充";
    }
    if (applicationIndustries.empty()) {
        applicationIndustries = "导入待补充";
    }

    normalizedRow["company_name"] = companyName;
    normalizedRow["title"] = title;
    normalizedRow["job_type"] = jobType;
    normalizedRow["min_degree"] = minDegree;
    normalizedRow["location_province"] = normalizeImportText(
        jsonValueAsString(rawRow, "location_province", "province"),
        64);
    normalizedRow["location_city"] = normalizeImportText(
        jsonValueAsString(rawRow, "location_city", "city"),
        64);
    normalizedRow["salary_range"] = normalizeImportText(jsonValueAsString(rawRow, "salary_range"), 64);
    normalizedRow["responsibilities_text"] = responsibilities;
    normalizedRow["technical_projects_text"] = technicalProjects;
    normalizedRow["required_knowledge_skills_text"] = requiredKnowledge;
    normalizedRow["required_tools_platform_text"] = requiredTools;
    normalizedRow["published_at"] = publishedAt;
    normalizedRow["company_domain_area"] = domainArea;
    normalizedRow["company_main_business"] = mainBusiness;
    normalizedRow["company_flagship_products"] = flagshipProducts;
    normalizedRow["company_application_industries"] = applicationIndustries;
    normalizedRow["company_province"] = normalizeImportText(
        jsonValueAsString(rawRow, "company_province", "province"),
        64);
    normalizedRow["company_city"] = normalizeImportText(
        jsonValueAsString(rawRow, "company_city", "city"),
        64);
    normalizedRow["company_website"] = normalizeImportText(
        jsonValueAsString(rawRow, "company_website", "website"),
        255);
    normalizedRow["company_description"] = normalizeImportText(
        jsonValueAsString(rawRow, "company_description"),
        2048);
    return true;
}

}  // namespace

void registerJobsRoutes() {
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
            const auto keywordRaw = trimCopy(req->getParameter("keyword"));
            const auto like = "%" + keywordRaw + "%";
            const auto minDegree = trimCopy(req->getParameter("min_degree"));
            const auto jobType = trimCopy(req->getParameter("job_type"));
            static const std::unordered_set<std::string> kJobTypes = {"full_time", "internship", "contract"};
            static const std::unordered_set<std::string> kMinDegrees = {"none", "diploma", "bachelor", "master", "phd"};
            if ((!minDegree.empty() && !validateEnumValue("min_degree", minDegree, kMinDegrees, callback)) ||
                (!jobType.empty() && !validateEnumValue("job_type", jobType, kJobTypes, callback))) {
                return;
            }
            const std::string sortOrder = normalizeSortOrder(req->getParameter("sort_order"));
            static const std::unordered_map<std::string, std::string> kSortMap = {
                {"id", "j.id"},
                {"title", "j.title"},
                {"company_name", "c.name"},
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
                "AND (? = '' OR j.min_degree = ?) "
                "AND (? = '' OR j.job_type = ?) ";
            const std::string baseFromSql =
                "FROM jobs j INNER JOIN companies c ON c.id = j.company_id ";
            const std::string selectSql =
                "SELECT j.id, j.company_id, c.name AS company_name, j.title, j.job_type, "
                "j.location_province, j.location_city, j.min_degree, j.salary_range, "
                "j.responsibilities_text, j.technical_projects_text, j.required_knowledge_skills_text, "
                "j.required_tools_platform_text, j.published_at, j.created_at, j.updated_at ";

            db->execSqlAsync(
                "SELECT COUNT(*) AS total " + baseFromSql + whereSql,
                [callback, db, selectSql, baseFromSql, whereSql, sortField, sortOrder,
                 page, pageSize, offset, keywordRaw, like, companyId, minDegree, jobType]
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
                "j.required_tools_platform_text, j.published_at, j.created_at, j.updated_at "
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
            static const std::unordered_set<std::string> kJobTypes = {"full_time", "internship", "contract"};
            static const std::unordered_set<std::string> kMinDegrees = {"none", "diploma", "bachelor", "master", "phd"};
            if (!validateEnumValue("job_type", jobType, kJobTypes, callback) ||
                !validateEnumValue("min_degree", minDegree, kMinDegrees, callback)) {
                return;
            }
            const auto publishedAtRaw = json->isMember("published_at") ? (*json)["published_at"].asString() : "";
            const auto publishedAt = normalizeDateTimeValue(publishedAtRaw);
            if (!publishedAtRaw.empty() && publishedAt.empty()) {
                sendError(callback, drogon::k400BadRequest, "invalid datetime field in payload");
                return;
            }

            db->execSqlAsync(
                "INSERT INTO jobs(company_id, title, job_type, location_province, location_city, min_degree, "
                "salary_range, responsibilities_text, technical_projects_text, required_knowledge_skills_text, "
                "required_tools_platform_text, published_at) "
                "VALUES(?, ?, ?, NULLIF(?, ''), NULLIF(?, ''), ?, NULLIF(?, ''), ?, ?, ?, ?, NULLIF(?, ''))",
                [callback, db](const drogon::orm::Result &result) {
                    const auto insertedId = static_cast<long long>(result.insertId());
                    db->execSqlAsync(
                        "SELECT j.id, j.company_id, c.name AS company_name, j.title, j.job_type, "
                        "j.location_province, j.location_city, j.min_degree, j.salary_range, "
                        "j.responsibilities_text, j.technical_projects_text, j.required_knowledge_skills_text, "
                        "j.required_tools_platform_text, j.published_at, j.created_at, j.updated_at "
                        "FROM jobs j INNER JOIN companies c ON c.id = j.company_id WHERE j.id = ?",
                        [callback](const drogon::orm::Result &rowResult) {
                            Json::Value body;
                            body["data"] = jobRowToJson(rowResult[0]);
                            callback(makeJson(body, drogon::k201Created));
                        },
                        [callback](const DrogonDbException &e) {
                            sendDbError(e, callback);
                        },
                        insertedId);
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
            static const std::unordered_set<std::string> kJobTypes = {"full_time", "internship", "contract"};
            static const std::unordered_set<std::string> kMinDegrees = {"none", "diploma", "bachelor", "master", "phd"};
            if (!validateEnumValue("job_type", jobType, kJobTypes, callback) ||
                !validateEnumValue("min_degree", minDegree, kMinDegrees, callback)) {
                return;
            }
            const auto publishedAtRaw = json->isMember("published_at") ? (*json)["published_at"].asString() : "";
            const auto publishedAt = normalizeDateTimeValue(publishedAtRaw);
            if (!publishedAtRaw.empty() && publishedAt.empty()) {
                sendError(callback, drogon::k400BadRequest, "invalid datetime field in payload");
                return;
            }

            db->execSqlAsync(
                "UPDATE jobs SET company_id = ?, title = ?, job_type = ?, location_province = NULLIF(?, ''), "
                "location_city = NULLIF(?, ''), min_degree = ?, salary_range = NULLIF(?, ''), "
                "responsibilities_text = ?, technical_projects_text = ?, required_knowledge_skills_text = ?, "
                "required_tools_platform_text = ?, published_at = NULLIF(?, '') WHERE id = ?",
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
                        "j.required_tools_platform_text, j.published_at, j.created_at, j.updated_at "
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
    // Job Import (v0.3)
    // -------------------------
    drogon::app().registerHandler(
        "/api/jobs/import/preview",
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
                std::unordered_set<std::string> existingTitleKeys;
                const auto titleRows = db->execSqlSync(
                    "SELECT c.name AS company_name, j.title "
                    "FROM jobs j INNER JOIN companies c ON c.id = j.company_id");
                for (const auto &row : titleRows) {
                    const auto companyName = row["company_name"].isNull() ? "" : row["company_name"].as<std::string>();
                    const auto title = row["title"].isNull() ? "" : row["title"].as<std::string>();
                    if (!companyName.empty() && !title.empty()) {
                        existingTitleKeys.insert("title:" + companyTitleKey(companyName, title));
                    }
                }

                struct PreviewRowRecord {
                    int rowNo{0};
                    std::string rowStatus;
                    std::string planAction;
                    std::string error;
                    std::string companyName;
                    std::string title;
                    Json::Value rawRow{Json::objectValue};
                    Json::Value normalizedRow{Json::objectValue};
                };

                std::vector<PreviewRowRecord> records;
                records.reserve(rawRows.size());
                std::unordered_set<std::string> seenDedupKeys;

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
                    if (!normalizeJobImportRow(rawRow, normalizedRow, normalizeError)) {
                        record.rowStatus = "invalid";
                        record.error = normalizeError;
                        ++invalidRows;
                    } else {
                        const auto dedupKey = jobImportDedupKey(normalizedRow);
                        if (seenDedupKeys.find(dedupKey) != seenDedupKeys.end()) {
                            record.rowStatus = "invalid";
                            record.error = "duplicate row in current import payload";
                            ++invalidRows;
                        } else {
                            seenDedupKeys.insert(dedupKey);
                            record.normalizedRow = normalizedRow;
                            record.companyName = jsonValueAsString(normalizedRow, "company_name");
                            record.title = jsonValueAsString(normalizedRow, "title");

                            const auto titleKey = "title:" + companyTitleKey(record.companyName, record.title);
                            if (existingTitleKeys.find(titleKey) != existingTitleKeys.end()) {
                                record.planAction = "update";
                                ++estimatedUpdatedRows;
                            } else {
                                record.planAction = "insert";
                                ++estimatedInsertedRows;
                            }
                            record.rowStatus = "valid";
                            ++validRows;
                        }
                    }

                    Json::Value item(Json::objectValue);
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
                    "INSERT INTO job_import_batches("
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
                        "INSERT INTO job_import_rows("
                        "batch_id, row_no, row_status, company_name, title, "
                        "error_message, raw_row_json, normalized_row_json) "
                        "VALUES(?, ?, ?, NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''), ?, ?)",
                        batchId,
                        record.rowNo,
                        record.rowStatus,
                        record.companyName,
                        record.title,
                        record.error,
                        jsonToCompactString(record.rawRow),
                        jsonToCompactString(record.normalizedRow));
                }

                Json::Value body(Json::objectValue);
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
        "/api/jobs/import/apply",
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
                    "FROM job_import_batches WHERE id = ? LIMIT 1 FOR UPDATE",
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
                    "FROM job_import_rows WHERE batch_id = ? ORDER BY row_no ASC, id ASC",
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

                    const auto companyName = normalizeImportText(jsonValueAsString(normalized, "company_name"), 255);
                    const auto title = normalizeImportText(jsonValueAsString(normalized, "title"), 255);
                    const auto jobType = normalizeJobTypeValue(jsonValueAsString(normalized, "job_type"));
                    const auto minDegree = normalizeDegreeValue(jsonValueAsString(normalized, "min_degree"));
                    const auto locationProvince = normalizeImportText(jsonValueAsString(normalized, "location_province"), 64);
                    const auto locationCity = normalizeImportText(jsonValueAsString(normalized, "location_city"), 64);
                    const auto salaryRange = normalizeImportText(jsonValueAsString(normalized, "salary_range"), 64);
                    const auto responsibilities = normalizeImportText(jsonValueAsString(normalized, "responsibilities_text"), 65535);
                    const auto technicalProjects = normalizeImportText(jsonValueAsString(normalized, "technical_projects_text"), 65535);
                    const auto requiredKnowledge = normalizeImportText(jsonValueAsString(normalized, "required_knowledge_skills_text"), 65535);
                    const auto requiredTools = normalizeImportText(jsonValueAsString(normalized, "required_tools_platform_text"), 65535);
                    const auto publishedAt = normalizeDateTimeValue(jsonValueAsString(normalized, "published_at"));

                    const auto domainArea = normalizeImportText(jsonValueAsString(normalized, "company_domain_area"), 512);
                    const auto mainBusiness = normalizeImportText(jsonValueAsString(normalized, "company_main_business"), 65535);
                    const auto flagshipProducts = normalizeImportText(jsonValueAsString(normalized, "company_flagship_products"), 65535);
                    const auto applicationIndustries = normalizeImportText(jsonValueAsString(normalized, "company_application_industries"), 65535);
                    const auto companyProvince = normalizeImportText(jsonValueAsString(normalized, "company_province"), 64);
                    const auto companyCity = normalizeImportText(jsonValueAsString(normalized, "company_city"), 64);
                    const auto companyWebsite = normalizeImportText(jsonValueAsString(normalized, "company_website"), 255);
                    const auto companyDescription = normalizeImportText(jsonValueAsString(normalized, "company_description"), 2048);

                    if (companyName.empty() || title.empty()) {
                        throw std::runtime_error("invalid normalized row content");
                    }

                    long long companyId = 0;
                    const auto companyRows = tx->execSqlSync(
                        "SELECT id FROM companies WHERE name = ? LIMIT 1",
                        companyName);
                    if (companyRows.empty()) {
                        const std::string safeMainBusiness =
                            mainBusiness.empty() ? std::string("导入待补充") : mainBusiness;
                        const std::string safeFlagshipProducts =
                            flagshipProducts.empty() ? std::string("导入待补充") : flagshipProducts;
                        const std::string safeDomainArea =
                            domainArea.empty() ? std::string("导入待补充") : domainArea;
                        const std::string safeApplicationIndustries =
                            applicationIndustries.empty() ? std::string("导入待补充") : applicationIndustries;
                        const auto companyInsert = tx->execSqlSync(
                            "INSERT INTO companies(name, main_business, flagship_products, domain_area, "
                            "application_industries, province, city, website, description) "
                            "VALUES(?, ?, ?, ?, ?, NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''))",
                            companyName,
                            safeMainBusiness,
                            safeFlagshipProducts,
                            safeDomainArea,
                            safeApplicationIndustries,
                            companyProvince,
                            companyCity,
                            companyWebsite,
                            companyDescription);
                        companyId = static_cast<long long>(companyInsert.insertId());
                    } else {
                        companyId = companyRows[0]["id"].as<long long>();
                    }

                    long long jobId = 0;
                    std::string action = "updated";
                    const auto existingByTitle = tx->execSqlSync(
                        "SELECT id FROM jobs WHERE company_id = ? AND title = ? ORDER BY id DESC LIMIT 1",
                        companyId,
                        title);
                    if (!existingByTitle.empty()) {
                        jobId = existingByTitle[0]["id"].as<long long>();
                    }

                    const std::string safeJobType =
                        jobType.empty() ? std::string("full_time") : jobType;
                    const std::string safeMinDegree =
                        minDegree.empty() ? std::string("bachelor") : minDegree;
                    const std::string safeResponsibilities =
                        responsibilities.empty() ? std::string("待补充") : responsibilities;
                    const std::string safeTechnicalProjects =
                        technicalProjects.empty() ? std::string("待补充") : technicalProjects;
                    const std::string safeRequiredKnowledge =
                        requiredKnowledge.empty() ? std::string("待补充") : requiredKnowledge;
                    const std::string safeRequiredTools =
                        requiredTools.empty() ? std::string("待补充") : requiredTools;

                    if (jobId <= 0) {
                        const auto jobInsert = tx->execSqlSync(
                            "INSERT INTO jobs("
                            "company_id, title, job_type, location_province, location_city, min_degree, salary_range, "
                            "responsibilities_text, technical_projects_text, required_knowledge_skills_text, "
                            "required_tools_platform_text, published_at) "
                            "VALUES(?, ?, ?, NULLIF(?, ''), NULLIF(?, ''), ?, NULLIF(?, ''), ?, ?, ?, ?, NULLIF(?, ''))",
                            companyId,
                            title,
                            safeJobType,
                            locationProvince,
                            locationCity,
                            safeMinDegree,
                            salaryRange,
                            safeResponsibilities,
                            safeTechnicalProjects,
                            safeRequiredKnowledge,
                            safeRequiredTools,
                            publishedAt);
                        jobId = static_cast<long long>(jobInsert.insertId());
                        action = "inserted";
                        ++insertedRows;
                    } else {
                        tx->execSqlSync(
                            "UPDATE jobs SET "
                            "company_id = ?, "
                            "title = ?, "
                            "job_type = ?, "
                            "location_province = NULLIF(?, ''), "
                            "location_city = NULLIF(?, ''), "
                            "min_degree = ?, "
                            "salary_range = NULLIF(?, ''), "
                            "responsibilities_text = ?, "
                            "technical_projects_text = ?, "
                            "required_knowledge_skills_text = ?, "
                            "required_tools_platform_text = ?, "
                            "published_at = CASE WHEN NULLIF(?, '') IS NULL THEN published_at ELSE ? END, "
                            "updated_at = CURRENT_TIMESTAMP "
                            "WHERE id = ?",
                            companyId,
                            title,
                            safeJobType,
                            locationProvince,
                            locationCity,
                            safeMinDegree,
                            salaryRange,
                            safeResponsibilities,
                            safeTechnicalProjects,
                            safeRequiredKnowledge,
                            safeRequiredTools,
                            publishedAt,
                            publishedAt,
                            jobId);
                        ++updatedRows;
                    }

                    rowActions.emplace_back(rowId, action);
                }
                for (const auto &it : rowActions) {
                    tx->execSqlSync(
                        "UPDATE job_import_rows "
                        "SET row_status = ?, error_message = NULL "
                        "WHERE id = ?",
                        it.second,
                        it.first);
                }

                tx->execSqlSync(
                    "UPDATE job_import_batches "
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
                    "FROM job_import_batches b "
                    "LEFT JOIN users u ON u.id = b.triggered_by_user_id "
                    "WHERE b.id = ?",
                    batchId);
                tx.reset();
                Json::Value body(Json::objectValue);
                body["data"] = resultRows.empty() ? Json::Value(Json::objectValue)
                                                  : jobImportBatchRowToJson(resultRows[0]);
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
                        "UPDATE job_import_batches "
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
                        "UPDATE job_import_batches "
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
        "/api/jobs/import/batches",
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
            const std::string fromSql =
                "FROM job_import_batches b LEFT JOIN users u ON u.id = b.triggered_by_user_id ";
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
                                list.append(jobImportBatchRowToJson(row));
                            }
                            Json::Value body(Json::objectValue);
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
