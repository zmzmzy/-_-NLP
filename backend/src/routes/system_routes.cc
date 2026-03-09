#include "../app_routes.h"

#include "../app_support.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <vector>

namespace gm {

namespace {

long long nowEpochSeconds() {
    const auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}

struct SmtpConfig {
    std::string host;
    int port{0};
    std::string username;
    std::string password;
    std::string fromEmail;
    std::string resetPageUrl;
    std::string authMode{"login"};
    bool useSsl{false};
    bool startTls{true};
    bool skipTlsVerify{false};
    bool debugReturnToken{false};
    long timeoutMs{10000};
};

std::string sanitizeHeaderValue(const std::string &raw) {
    std::string out;
    out.reserve(raw.size());
    for (char c : raw) {
        if (c == '\r' || c == '\n') {
            continue;
        }
        out.push_back(c);
    }
    return out;
}

bool parseEnvBoolOrDefault(const char *key, bool defaultValue) {
    const auto raw = getEnvTrimmed(key);
    if (raw.empty()) {
        return defaultValue;
    }
    bool parsed = defaultValue;
    if (!parseBoolLike(raw, parsed)) {
        return defaultValue;
    }
    return parsed;
}

int parseEnvIntOrDefault(const char *key, int defaultValue) {
    const auto raw = getEnvTrimmed(key);
    if (raw.empty()) {
        return defaultValue;
    }
    int parsed = defaultValue;
    if (!parseStrictInt(raw, parsed)) {
        return defaultValue;
    }
    return parsed;
}

std::string normalizeSmtpAuthMode(const std::string &raw) {
    auto value = toLowerAscii(trimCopy(raw));
    if (value.empty()) {
        return "login";
    }
    if (value == "login" || value == "plain" || value == "none") {
        return value;
    }
    return "login";
}

bool loadSmtpConfig(SmtpConfig &cfg, std::string &error) {
    cfg = SmtpConfig();
    error.clear();

    cfg.host = trimCopy(getEnvTrimmed("GM_SMTP_HOST"));
    cfg.username = trimCopy(getEnvTrimmed("GM_SMTP_USERNAME"));
    cfg.password = getEnvTrimmed("GM_SMTP_PASSWORD");
    cfg.fromEmail = trimCopy(getEnvTrimmed("GM_SMTP_FROM"));
    cfg.resetPageUrl = trimCopy(getEnvTrimmed("GM_PASSWORD_RESET_PAGE_URL"));
    cfg.authMode = normalizeSmtpAuthMode(getEnvTrimmed("GM_SMTP_AUTH"));
    cfg.useSsl = parseEnvBoolOrDefault("GM_SMTP_USE_SSL", false);
    cfg.startTls = parseEnvBoolOrDefault("GM_SMTP_STARTTLS", true);
    cfg.skipTlsVerify = parseEnvBoolOrDefault("GM_SMTP_SKIP_TLS_VERIFY", false);
    cfg.debugReturnToken = parseEnvBoolOrDefault("GM_PASSWORD_RESET_DEBUG_RETURN_TOKEN", false);
    cfg.timeoutMs = std::max(1000, parseEnvIntOrDefault("GM_SMTP_TIMEOUT_MS", 10000));
    cfg.port = parseEnvIntOrDefault("GM_SMTP_PORT", cfg.useSsl ? 465 : 587);

    if (cfg.host.empty() || cfg.username.empty() || cfg.password.empty() || cfg.fromEmail.empty()) {
        error = "SMTP is not configured (required env: GM_SMTP_HOST/GM_SMTP_USERNAME/GM_SMTP_PASSWORD/GM_SMTP_FROM)";
        return false;
    }
    if (cfg.port <= 0 || cfg.port > 65535) {
        error = "invalid GM_SMTP_PORT";
        return false;
    }
    if (cfg.useSsl) {
        cfg.startTls = false;
    }
    return true;
}

std::string shellEscape(const std::string &text) {
    std::string out;
    out.reserve(text.size() + 8);
    out.push_back('\'');
    for (char c : text) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out.push_back(c);
        }
    }
    out.push_back('\'');
    return out;
}

bool sendMailBySmtp(const SmtpConfig &cfg,
                    const std::string &toEmail,
                    const std::string &subjectRaw,
                    const std::string &bodyRaw,
                    std::string &error) {
    error.clear();
    if (toEmail.empty()) {
        error = "recipient is empty";
        return false;
    }

    const std::string subject = sanitizeHeaderValue(subjectRaw);
    const std::string smtpUrl =
        std::string(cfg.useSsl ? "smtps://" : "smtp://") + cfg.host + ":" + std::to_string(cfg.port);
    std::string payload;
    payload.reserve(bodyRaw.size() + 256);
    payload += "To: <" + toEmail + ">\r\n";
    payload += "From: <" + cfg.fromEmail + ">\r\n";
    payload += "Subject: " + subject + "\r\n";
    payload += "MIME-Version: 1.0\r\n";
    payload += "Content-Type: text/plain; charset=UTF-8\r\n";
    payload += "Content-Transfer-Encoding: 8bit\r\n";
    payload += "\r\n";
    payload += bodyRaw;
    payload += "\r\n";

    const auto payloadPath = std::filesystem::temp_directory_path() /
                             ("gm_reset_mail_" + drogon::utils::getUuid() + ".eml");
    const auto netrcPath = std::filesystem::temp_directory_path() /
                           ("gm_reset_smtp_" + drogon::utils::getUuid() + ".netrc");
    {
        std::ofstream out(payloadPath);
        if (!out.is_open()) {
            error = "failed to create smtp payload file";
            return false;
        }
        out << payload;
    }
    {
        std::ofstream out(netrcPath);
        if (!out.is_open()) {
            std::error_code rmEc;
            std::filesystem::remove(payloadPath, rmEc);
            error = "failed to create smtp credential file";
            return false;
        }
        out << "machine " << cfg.host << "\n";
        out << "login " << cfg.username << "\n";
        out << "password " << cfg.password << "\n";
    }
    std::error_code permEc;
    std::filesystem::permissions(
        netrcPath,
        std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
        std::filesystem::perm_options::replace,
        permEc);

    std::string command = "curl --silent --show-error --url " + shellEscape(smtpUrl) +
                          " --mail-from " + shellEscape("<" + cfg.fromEmail + ">") +
                          " --mail-rcpt " + shellEscape("<" + toEmail + ">") +
                          " --upload-file " + shellEscape(payloadPath.string()) +
                          " --netrc-file " + shellEscape(netrcPath.string()) +
                          " --connect-timeout " + std::to_string(std::max(1L, cfg.timeoutMs / 1000)) +
                          " --max-time " + std::to_string(std::max(1L, cfg.timeoutMs / 1000));
    if (cfg.authMode == "login") {
        command += " --login-options " + shellEscape("AUTH=LOGIN");
    } else if (cfg.authMode == "plain") {
        command += " --login-options " + shellEscape("AUTH=PLAIN");
    }
    if (cfg.useSsl || cfg.startTls) {
        command += " --ssl-reqd";
    }
    if (cfg.skipTlsVerify) {
        command += " --insecure";
    }

    const int rc = std::system(command.c_str());
    std::error_code rmPayloadEc;
    std::filesystem::remove(payloadPath, rmPayloadEc);
    std::error_code rmNetrcEc;
    std::filesystem::remove(netrcPath, rmNetrcEc);
    if (rc != 0) {
        error = "smtp send command failed with exit code " + std::to_string(rc);
        return false;
    }
    return true;
}

std::string buildPasswordResetEmailBody(const SmtpConfig &cfg,
                                        const std::string &email,
                                        const std::string &token,
                                        long long expiresAtEpoch) {
    std::ostringstream oss;
    oss << "您好，\n\n";
    oss << "我们收到了您的密码重置请求，请在有效期内完成重置。\n\n";
    if (!cfg.resetPageUrl.empty()) {
        oss << "重置页面: " << cfg.resetPageUrl << "\n";
    }
    oss << "邮箱: " << email << "\n";
    oss << "6位验证码: " << token << "\n";
    oss << "过期时间(Unix时间戳): " << expiresAtEpoch << "\n\n";
    oss << "如果这不是您的操作，请忽略此邮件。";
    return oss.str();
}

std::string generateSixDigitCode() {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 999999);
    const int codeValue = dist(rng);
    std::ostringstream oss;
    oss << std::setw(6) << std::setfill('0') << codeValue;
    return oss.str();
}

bool isSixDigitCode(const std::string &value) {
    if (value.size() != 6) {
        return false;
    }
    for (char c : value) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

std::string generateUniqueResetCode(const drogon::orm::DbClientPtr &db) {
    for (int attempt = 0; attempt < 64; ++attempt) {
        const auto code = generateSixDigitCode();
        const auto rows = db->execSqlSync(
            "SELECT id FROM password_reset_tokens WHERE token = ? LIMIT 1",
            code);
        if (rows.empty()) {
            return code;
        }
    }
    throw std::runtime_error("failed to generate unique reset code");
}

std::string normalizeEmailValue(const std::string &raw) {
    return toLowerAscii(trimCopy(raw));
}

bool isValidEmailFormat(const std::string &email) {
    const auto value = normalizeEmailValue(email);
    const auto atPos = value.find('@');
    if (atPos == std::string::npos || atPos == 0 || atPos + 1 >= value.size()) {
        return false;
    }
    if (value.find('@', atPos + 1) != std::string::npos) {
        return false;
    }
    const auto dotPos = value.find('.', atPos + 1);
    if (dotPos == std::string::npos || dotPos + 1 >= value.size()) {
        return false;
    }
    if (value.size() > 128) {
        return false;
    }
    return true;
}

std::string sanitizeUsername(const std::string &raw) {
    std::string out;
    out.reserve(raw.size());
    for (unsigned char c : raw) {
        if (std::isalnum(c) || c == '_' || c == '-') {
            out.push_back(static_cast<char>(c));
        }
    }
    if (out.size() > 64) {
        out.resize(64);
    }
    return out;
}

std::string generateUsernameFromEmail(const std::string &email) {
    const auto normalized = normalizeEmailValue(email);
    const auto atPos = normalized.find('@');
    const auto base = atPos == std::string::npos ? normalized : normalized.substr(0, atPos);
    auto username = sanitizeUsername(base);
    if (username.empty()) {
        username = "user";
    }
    return username;
}

std::string buildUniqueGeneratedUsername(const drogon::orm::DbClientPtr &db,
                                         const std::string &baseRaw) {
    const auto baseSeed = sanitizeUsername(baseRaw);
    const std::string base = baseSeed.empty() ? std::string("user") : baseSeed;
    for (int attempt = 0; attempt < 100; ++attempt) {
        std::string candidate = base;
        if (attempt > 0) {
            const auto suffix = drogon::utils::getUuid().substr(0, 6);
            candidate = base + "_" + suffix;
        }
        if (candidate.size() > 64) {
            candidate.resize(64);
        }
        const auto rows = db->execSqlSync(
            "SELECT id FROM users WHERE username = ? LIMIT 1",
            candidate);
        if (rows.empty()) {
            return candidate;
        }
    }
    return base + "_" + std::to_string(nowEpochSeconds() % 1000000);
}

bool isNonAdminAllowedApiRoute(const HttpRequestPtr &req) {
    const auto path = req->path();
    const auto method = req->method();
    if (path == "/api/auth/logout" && method == drogon::Post) {
        return true;
    }
    if (path == "/api/auth/me" && method == drogon::Get) {
        return true;
    }
    if (path == "/api/me/student-profile" &&
        (method == drogon::Get || method == drogon::Put)) {
        return true;
    }
    if (path == "/api/me/student-profile/bind" && method == drogon::Post) {
        return true;
    }
    if (path == "/api/me/employment-submissions" &&
        (method == drogon::Get || method == drogon::Post)) {
        return true;
    }
    if (path == "/api/colleges" && method == drogon::Get) {
        return true;
    }
    if (path == "/api/majors" && method == drogon::Get) {
        return true;
    }
    if (path.rfind("/api/majors/", 0) == 0 && method == drogon::Get) {
        return true;
    }
    if (path == "/api/companies" && method == drogon::Get) {
        return true;
    }
    if (path == "/api/jobs" && method == drogon::Get) {
        return true;
    }
    if (path.rfind("/api/jobs/", 0) == 0 && method == drogon::Get) {
        return true;
    }
    return false;
}

}  // namespace

void registerSystemRoutes() {
    // Central auth guard: all /api/* routes require login except public endpoints.
    drogon::app().registerPreRoutingAdvice(
        [](const HttpRequestPtr &req,
           drogon::AdviceCallback &&accept,
           drogon::AdviceChainCallback &&next) {
            const auto path = req->path();
            if (path.rfind("/api/", 0) != 0 || isPublicApiRoute(req)) {
                next();
                return;
            }

            SessionUser sessionUser;
            if (getSessionFromRequest(req, sessionUser)) {
                if (sessionUser.role != "admin" && !isNonAdminAllowedApiRoute(req)) {
                    Json::Value body;
                    body["success"] = false;
                    body["error"] = "forbidden";
                    body["error_code"] = "forbidden";
                    Json::Value detail;
                    detail["code"] = "forbidden";
                    detail["message"] = "this account can only access personal center";
                    body["error_detail"] = detail;
                    accept(makeJson(body, drogon::k403Forbidden));
                    return;
                }
                next();
                return;
            }

            Json::Value body;
            body["success"] = false;
            body["error"] = "authentication required";
            body["error_code"] = "auth_required";
            Json::Value detail;
            detail["code"] = "auth_required";
            detail["message"] = "authentication required";
            body["error_detail"] = detail;
            accept(makeJson(body, drogon::k401Unauthorized));
        });

    drogon::app().registerHandler(
        "/api/health",
        [](const HttpRequestPtr &,
           DbCallback &&callback) {
            Json::Value body;
            body["status"] = "ok";
            body["service"] = "graduate_match_backend";
            body["version"] = "0.3.0";
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
        "/api/auth/register",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!requireFields(*json, {"email", "password"}, callback)) {
                return;
            }

            const auto email = normalizeEmailValue((*json)["email"].asString());
            const auto password = (*json)["password"].asString();
            const auto usernameInput = json->isMember("username")
                                           ? sanitizeUsername((*json)["username"].asString())
                                           : "";
            if (!isValidEmailFormat(email)) {
                sendError(callback, drogon::k400BadRequest, "invalid email format");
                return;
            }
            if (password.size() < 6) {
                sendError(callback, drogon::k400BadRequest, "password must be at least 6 characters");
                return;
            }
            if (!usernameInput.empty() && usernameInput.size() < 3) {
                sendError(callback, drogon::k400BadRequest, "username must contain at least 3 characters");
                return;
            }

            try {
                const auto emailRows = db->execSqlSync(
                    "SELECT id FROM users WHERE email = ? LIMIT 1",
                    email);
                if (!emailRows.empty()) {
                    sendError(callback, drogon::k409Conflict, "email already registered");
                    return;
                }

                std::string username = usernameInput;
                if (!username.empty()) {
                    const auto nameRows = db->execSqlSync(
                        "SELECT id FROM users WHERE username = ? LIMIT 1",
                        username);
                    if (!nameRows.empty()) {
                        sendError(callback, drogon::k409Conflict, "username already exists");
                        return;
                    }
                } else {
                    username = buildUniqueGeneratedUsername(db, generateUsernameFromEmail(email));
                }

                const auto passwordHash = "sha256:" + sha256Hex(password);
                const auto insertResult = db->execSqlSync(
                    "INSERT INTO users(username, email, password_hash, role, is_active) "
                    "VALUES(?, ?, ?, 'viewer', 1)",
                    username,
                    email,
                    passwordHash);
                const auto userId = static_cast<long long>(insertResult.insertId());

                long long expiresAt = 0;
                const auto role = std::string("viewer");
                const auto token = createSession(userId, username, role, expiresAt);

                Json::Value user;
                user["id"] = static_cast<Json::Int64>(userId);
                user["username"] = username;
                user["email"] = email;
                user["role"] = role;
                user["is_active"] = true;
                user["linked_student_id"] = Json::nullValue;

                Json::Value body;
                body["data"]["token"] = token;
                body["data"]["token_type"] = "Bearer";
                body["data"]["expires_at_epoch"] = static_cast<Json::Int64>(expiresAt);
                body["data"]["user"] = user;
                callback(makeJson(body, drogon::k201Created));
            } catch (const DrogonDbException &e) {
                const std::string msg = e.base().what();
                if (msg.find("Duplicate") != std::string::npos) {
                    sendError(callback, drogon::k409Conflict, "username or email already exists");
                    return;
                }
                sendDbError(e, callback);
            }
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/auth/login",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!requireFields(*json, {"password"}, callback)) {
                return;
            }
            const auto username = json->isMember("username") ? trimCopy((*json)["username"].asString()) : "";
            const auto email = json->isMember("email") ? normalizeEmailValue((*json)["email"].asString()) : "";
            const auto account = json->isMember("account") ? trimCopy((*json)["account"].asString()) : "";
            const auto password = (*json)["password"].asString();
            std::string loginKey = trimCopy(account);
            if (loginKey.empty()) {
                loginKey = !email.empty() ? email : username;
            }
            const auto loginKeyEmail = normalizeEmailValue(loginKey);
            if (loginKey.empty() || password.empty()) {
                sendError(callback, drogon::k400BadRequest, "account/email/username and password cannot be empty");
                return;
            }

            db->execSqlAsync(
                "SELECT id, username, email, password_hash, role, is_active, linked_student_id "
                "FROM users WHERE username = ? OR email = ? LIMIT 1",
                [callback, password](const drogon::orm::Result &result) {
                    if (result.empty()) {
                        sendError(callback, drogon::k401Unauthorized, "invalid account/email or password", "auth_failed");
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
                        sendError(callback, drogon::k401Unauthorized, "invalid account/email or password", "auth_failed");
                        return;
                    }

                    const auto userId = row["id"].as<long long>();
                    const auto username = row["username"].as<std::string>();
                    const auto email = row["email"].isNull() ? std::string() : row["email"].as<std::string>();
                    const auto role = row["role"].as<std::string>();
                    long long expiresAt = 0;
                    const auto token = createSession(userId, username, role, expiresAt);

                    Json::Value user;
                    user["id"] = static_cast<Json::Int64>(userId);
                    user["username"] = username;
                    user["email"] = email;
                    user["role"] = role;
                    user["is_active"] = true;
                    user["linked_student_id"] = row["linked_student_id"].isNull()
                                                    ? Json::Value(Json::nullValue)
                                                    : Json::Value(static_cast<Json::Int64>(
                                                          row["linked_student_id"].as<long long>()));

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
                loginKey,
                loginKeyEmail);
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
            auto db = drogon::app().getDbClient();
            db->execSqlAsync(
                "SELECT id, username, email, role, is_active, linked_student_id "
                "FROM users WHERE id = ? LIMIT 1",
                [callback, user](const drogon::orm::Result &result) {
                    if (result.empty()) {
                        sendError(callback, drogon::k401Unauthorized, "session user not found", "auth_required");
                        return;
                    }
                    const auto &row = result[0];
                    const bool isActive = row["is_active"].as<int>() != 0;
                    if (!isActive) {
                        sendError(callback, drogon::k403Forbidden, "user is disabled", "forbidden");
                        return;
                    }
                    Json::Value body;
                    body["data"]["id"] = static_cast<Json::Int64>(row["id"].as<long long>());
                    body["data"]["username"] = row["username"].as<std::string>();
                    body["data"]["email"] = row["email"].isNull()
                                                ? Json::Value(Json::nullValue)
                                                : Json::Value(row["email"].as<std::string>());
                    body["data"]["role"] = row["role"].as<std::string>();
                    body["data"]["linked_student_id"] = row["linked_student_id"].isNull()
                                                             ? Json::Value(Json::nullValue)
                                                             : Json::Value(static_cast<Json::Int64>(
                                                                   row["linked_student_id"].as<long long>()));
                    body["data"]["expires_at_epoch"] = static_cast<Json::Int64>(user.expiresAtEpoch);
                    callback(makeJson(body));
                },
                [callback](const DrogonDbException &e) {
                    sendDbError(e, callback);
                },
                user.userId);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/auth/password/forgot",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!requireFields(*json, {"email"}, callback)) {
                return;
            }
            const auto email = normalizeEmailValue((*json)["email"].asString());
            if (!isValidEmailFormat(email)) {
                sendError(callback, drogon::k400BadRequest, "invalid email format");
                return;
            }
            SmtpConfig smtpConfig;
            std::string smtpConfigError;
            if (!loadSmtpConfig(smtpConfig, smtpConfigError)) {
                LOG_ERROR << "Password reset SMTP config error: " << smtpConfigError;
                sendError(callback,
                          drogon::k503ServiceUnavailable,
                          "password reset email service is unavailable",
                          "service_unavailable");
                return;
            }

            try {
                const auto userRows = db->execSqlSync(
                    "SELECT id, is_active FROM users WHERE email = ? LIMIT 1",
                    email);
                Json::Value body;
                body["data"]["message"] =
                    "if this email is registered, a password reset email has been sent";

                if (userRows.empty() || userRows[0]["is_active"].as<int>() == 0) {
                    callback(makeJson(body));
                    return;
                }

                const auto userId = userRows[0]["id"].as<long long>();
                const auto token = generateUniqueResetCode(db);
                const auto expiresAtEpoch = nowEpochSeconds() + 30 * 60;

                db->execSqlSync(
                    "UPDATE password_reset_tokens SET used_at = NOW() "
                    "WHERE user_id = ? AND used_at IS NULL",
                    userId);
                db->execSqlSync(
                    "INSERT INTO password_reset_tokens(user_id, email, token, expires_at) "
                    "VALUES(?, ?, ?, FROM_UNIXTIME(?))",
                    userId,
                    email,
                    token,
                    expiresAtEpoch);

                const std::string mailSubject = "Password Reset Verification Code";
                const auto mailBody = buildPasswordResetEmailBody(smtpConfig, email, token, expiresAtEpoch);
                std::string sendErrorMessage;
                if (!sendMailBySmtp(smtpConfig, email, mailSubject, mailBody, sendErrorMessage)) {
                    LOG_ERROR << "Password reset email send failed: " << sendErrorMessage;
                    try {
                        db->execSqlSync(
                            "UPDATE password_reset_tokens SET used_at = NOW() WHERE token = ?",
                            token);
                    } catch (const DrogonDbException &e) {
                        LOG_ERROR << "failed to invalidate reset token after smtp failure: " << e.base().what();
                    }
                    sendError(callback,
                              drogon::k502BadGateway,
                              "failed to send password reset email",
                              "smtp_send_failed");
                    return;
                }
                if (smtpConfig.debugReturnToken) {
                    body["data"]["debug_reset_token"] = token;
                    body["data"]["debug_expires_at_epoch"] = static_cast<Json::Int64>(expiresAtEpoch);
                }
                callback(makeJson(body));
            } catch (const DrogonDbException &e) {
                sendDbError(e, callback);
            }
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/auth/password/reset",
        [](const HttpRequestPtr &req, DbCallback &&callback) {
            auto db = drogon::app().getDbClient();
            const Json::Value *json = nullptr;
            if (!requireJsonBody(req, callback, json)) {
                return;
            }
            if (!requireFields(*json, {"email", "token", "new_password"}, callback)) {
                return;
            }
            const auto email = normalizeEmailValue((*json)["email"].asString());
            const auto token = trimCopy((*json)["token"].asString());
            const auto newPassword = (*json)["new_password"].asString();
            if (!isValidEmailFormat(email)) {
                sendError(callback, drogon::k400BadRequest, "invalid email format");
                return;
            }
            if (!isSixDigitCode(token)) {
                sendError(callback, drogon::k400BadRequest, "invalid verification code");
                return;
            }
            if (newPassword.size() < 6) {
                sendError(callback, drogon::k400BadRequest, "new password must be at least 6 characters");
                return;
            }

            std::shared_ptr<drogon::orm::Transaction> tx;
            try {
                tx = db->newTransaction();
                const auto tokenRows = tx->execSqlSync(
                    "SELECT id, user_id FROM password_reset_tokens "
                    "WHERE token = ? AND email = ? AND used_at IS NULL AND expires_at > NOW() "
                    "ORDER BY id DESC LIMIT 1 FOR UPDATE",
                    token,
                    email);
                if (tokenRows.empty()) {
                    tx->rollback();
                    tx.reset();
                    sendError(callback, drogon::k400BadRequest, "invalid or expired reset token");
                    return;
                }

                const auto tokenId = tokenRows[0]["id"].as<long long>();
                const auto userId = tokenRows[0]["user_id"].as<long long>();
                const auto newHash = "sha256:" + sha256Hex(newPassword);

                tx->execSqlSync(
                    "UPDATE users SET password_hash = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?",
                    newHash,
                    userId);
                tx->execSqlSync(
                    "UPDATE password_reset_tokens SET used_at = NOW() "
                    "WHERE user_id = ? AND used_at IS NULL",
                    userId);
                tx->execSqlSync(
                    "UPDATE password_reset_tokens SET used_at = NOW() WHERE id = ?",
                    tokenId);
                tx.reset();

                revokeSessionsByUserId(userId);
                Json::Value body;
                body["message"] = "password has been reset";
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
            } catch (const std::exception &e) {
                if (tx) {
                    try {
                        tx->rollback();
                    } catch (...) {
                    }
                    tx.reset();
                }
                sendError(callback, drogon::k500InternalServerError, e.what(), "reset_password_error");
            }
        },
        {drogon::Post});

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
}

}  // namespace gm
