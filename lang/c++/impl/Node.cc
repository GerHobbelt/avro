/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cmath>

#include "Node.hh"

namespace avro {

using std::string;

Node::~Node() = default;

Name::Name() = default;

Name::Name(const std::string &name) {
    fullname(name);
}

Name::Name(std::string simpleName, std::string ns) : ns_(std::move(ns)), simpleName_(std::move(simpleName)) {
    check();
}

Name::~Name() = default;

string Name::fullname() const {
    return ns_.empty() ? simpleName_ : ns_ + "." + simpleName_;
}

void Name::fullname(const string &name) {
    string::size_type n = name.find_last_of('.');
    if (n == string::npos) {
        simpleName_ = name;
        ns_.clear();
    } else {
        ns_ = name.substr(0, n);
        simpleName_ = name.substr(n + 1);
    }
    check();
}

void Name::addAlias(const std::string &alias) {
    aliases_.push_back(alias);
    if (!ns_.empty() && alias.find_last_of('.') == string::npos) {
        fqAliases_.emplace(ns_ + "." + alias);
    } else {
        fqAliases_.insert(alias);
    }
}

bool Name::operator<(const Name &n) const {
    return (ns_ < n.ns_) || (!(n.ns_ < ns_) && (simpleName_ < n.simpleName_));
}

static bool invalidChar1(char c) {
    return !isalnum(c) && c != '_' && c != '.' && c != '$';
}

static bool invalidChar2(char c) {
    return !isalnum(c) && c != '_';
}

void Name::check() const {
    if (!ns_.empty() && (ns_[0] == '.' || ns_[ns_.size() - 1] == '.' || std::find_if(ns_.begin(), ns_.end(), invalidChar1) != ns_.end())) {
        throw Exception("Invalid namespace: " + ns_);
    }
    if (simpleName_.empty()
        || std::find_if(simpleName_.begin(), simpleName_.end(), invalidChar2) != simpleName_.end()) {
        throw Exception("Invalid name: " + simpleName_);
    }
}

bool Name::operator==(const Name &n) const {
    return ns_ == n.ns_ && simpleName_ == n.simpleName_;
}

bool Name::equalOrAliasedBy(const Name &n) const {
    return *this == n || n.fqAliases_.find(fullname()) != n.fqAliases_.end();
}

void Node::setLogicalType(LogicalType logicalType) {
    checkLock();

    // Check that the logical type is applicable to the node type.
    switch (logicalType.type()) {
        case LogicalType::NONE: break;
        case LogicalType::DECIMAL: {
            if (type_ != AVRO_BYTES && type_ != AVRO_FIXED) {
                throw Exception("DECIMAL logical type can annotate "
                                "only BYTES or FIXED type");
            }
            if (type_ == AVRO_FIXED) {
                // Max precision that can be supported by the current size of
                // the FIXED type.
                long maxPrecision = floor(log10(2.0) * (8.0 * fixedSize() - 1));
                if (logicalType.precision() > maxPrecision) {
                    throw Exception(
                        boost::format(
                            "DECIMAL precision %1% is too large for the "
                            "FIXED type of size %2%, precision cannot be "
                            "larger than %3%")
                        % logicalType.precision() % fixedSize() % maxPrecision);
                }
            }
            if (logicalType.scale() > logicalType.precision()) {
                throw Exception("DECIMAL scale cannot exceed precision");
            }
            break;
        }
        case LogicalType::DATE:
            if (type_ != AVRO_INT) {
                throw Exception("DATE logical type can only annotate INT type");
            }
            break;
        case LogicalType::TIME_MILLIS:
            if (type_ != AVRO_INT) {
                throw Exception("TIME-MILLIS logical type can only annotate "
                                "INT type");
            }
            break;
        case LogicalType::TIME_MICROS:
            if (type_ != AVRO_LONG) {
                throw Exception("TIME-MICROS logical type can only annotate "
                                "LONG type");
            }
            break;
        case LogicalType::TIMESTAMP_MILLIS:
            if (type_ != AVRO_LONG) {
                throw Exception("TIMESTAMP-MILLIS logical type can only annotate "
                                "LONG type");
            }
            break;
        case LogicalType::TIMESTAMP_MICROS:
            if (type_ != AVRO_LONG) {
                throw Exception("TIMESTAMP-MICROS logical type can only annotate "
                                "LONG type");
            }
            break;
        case LogicalType::DURATION:
            if (type_ != AVRO_FIXED || fixedSize() != 12) {
                throw Exception("DURATION logical type can only annotate "
                                "FIXED type of size 12");
            }
            break;
        case LogicalType::UUID:
            if (type_ != AVRO_STRING) {
                throw Exception("UUID logical type can only annotate "
                                "STRING type");
            }
            break;
    }

    logicalType_ = logicalType;
}

} // namespace avro
