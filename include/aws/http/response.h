#pragma once
/*
 * Copyright 2018-present Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include "aws/lambda-runtime/outcome.h"

#include <string>
#include <vector>
#include <algorithm>
#include <cctype> // tolower
#include <cassert>

namespace aws {
namespace http {
enum class response_code;
class response {
public:
    /**
     * lower-case the name but store the value as is
     */
    inline void add_header(std::string name, std::string const& value);
    inline void append_body(const char* p, size_t sz);
    inline bool has_header(char const* header) const;
    inline lambda_runtime::outcome<std::string, bool> get_header(char const* header) const;
    inline response_code get_response_code() const { return m_response_code; }
    inline void set_response_code(aws::http::response_code c);
    inline void set_content_type(char const* ct);
    inline std::string const& get_body() const;
    inline std::string const& get_content_type() const;

private:
    response_code m_response_code;
    using key_value_collection = std::vector<std::pair<std::string, std::string>>;
    key_value_collection m_headers;
    std::string m_body;
    std::string m_content_type;
};

enum class response_code {
    REQUEST_NOT_MADE = -1,
    CONTINUE = 100,
    SWITCHING_PROTOCOLS = 101,
    PROCESSING = 102,
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NON_AUTHORITATIVE_INFORMATION = 203,
    NO_CONTENT = 204,
    RESET_CONTENT = 205,
    PARTIAL_CONTENT = 206,
    MULTI_STATUS = 207,
    ALREADY_REPORTED = 208,
    IM_USED = 226,
    MULTIPLE_CHOICES = 300,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    SEE_OTHER = 303,
    NOT_MODIFIED = 304,
    USE_PROXY = 305,
    SWITCH_PROXY = 306,
    TEMPORARY_REDIRECT = 307,
    PERMANENT_REDIRECT = 308,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    PAYMENT_REQUIRED = 402,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    NOT_ACCEPTABLE = 406,
    PROXY_AUTHENTICATION_REQUIRED = 407,
    REQUEST_TIMEOUT = 408,
    CONFLICT = 409,
    GONE = 410,
    LENGTH_REQUIRED = 411,
    PRECONDITION_FAILED = 412,
    REQUEST_ENTITY_TOO_LARGE = 413,
    REQUEST_URI_TOO_LONG = 414,
    UNSUPPORTED_MEDIA_TYPE = 415,
    REQUESTED_RANGE_NOT_SATISFIABLE = 416,
    EXPECTATION_FAILED = 417,
    IM_A_TEAPOT = 418,
    AUTHENTICATION_TIMEOUT = 419,
    METHOD_FAILURE = 420,
    UNPROC_ENTITY = 422,
    LOCKED = 423,
    FAILED_DEPENDENCY = 424,
    UPGRADE_REQUIRED = 426,
    PRECONDITION_REQUIRED = 427,
    TOO_MANY_REQUESTS = 429,
    REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
    LOGIN_TIMEOUT = 440,
    NO_RESPONSE = 444,
    RETRY_WITH = 449,
    BLOCKED = 450,
    REDIRECT = 451,
    REQUEST_HEADER_TOO_LARGE = 494,
    CERT_ERROR = 495,
    NO_CERT = 496,
    HTTP_TO_HTTPS = 497,
    CLIENT_CLOSED_TO_REQUEST = 499,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503,
    GATEWAY_TIMEOUT = 504,
    HTTP_VERSION_NOT_SUPPORTED = 505,
    VARIANT_ALSO_NEGOTIATES = 506,
    INSUFFICIENT_STORAGE = 507,
    LOOP_DETECTED = 508,
    BANDWIDTH_LIMIT_EXCEEDED = 509,
    NOT_EXTENDED = 510,
    NETWORK_AUTHENTICATION_REQUIRED = 511,
    NETWORK_READ_TIMEOUT = 598,
    NETWORK_CONNECT_TIMEOUT = 599
};

inline void response::set_response_code(http::response_code c)
{
    m_response_code = c;
}

inline void response::set_content_type(char const* ct)
{
    m_content_type = ct;
}

inline std::string const& response::get_body() const
{
    return m_body;
}

inline std::string const& response::get_content_type() const
{
    return m_content_type;
}

inline void response::add_header(std::string name, std::string const& value)
{
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    m_headers.emplace_back(std::move(name), value);
}

inline void response::append_body(const char* p, size_t sz)
{
    // simple and generates significantly less code than std::stringstream
    constexpr size_t min_capacity = 512;
    if (m_body.capacity() < min_capacity) {
        m_body.reserve(min_capacity);
    }

    m_body.append(p, sz);
}

inline bool response::has_header(char const* header) const
{
    return std::any_of(m_headers.begin(), m_headers.end(), [header](std::pair<std::string, std::string> const& p) {
        return p.first == header;
    });
}

inline lambda_runtime::outcome<std::string, bool> response::get_header(char const* header) const
{
    auto it = std::find_if(m_headers.begin(), m_headers.end(), [header](std::pair<std::string, std::string> const& p) {
        return p.first == header;
    });

    if (it == m_headers.end()) {
        return false;
    }
    return it->second;
}

} // namespace http
} // namespace aws
