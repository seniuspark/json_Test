#include "RecordStore.h"
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace {

std::string toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

bool fileExists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

bool tryParseDouble(const std::string& s, double& out) {
    try {
        size_t pos;
        double v = std::stod(s, &pos);
        if (pos != s.size()) return false;
        out = v;
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace

RecordStore::RecordStore(std::string filePath) : filePath_(std::move(filePath)) {}

void RecordStore::load() {
    if (fileExists(filePath_)) {
        data_ = json::Value::LoadFile(filePath_);
    } else {
        data_ = json::Value::MakeArray();
    }
}

void RecordStore::save() const {
    data_.SaveFile(filePath_, 2);
}

const json::Array& RecordStore::all() const {
    return data_.asArray();
}

int RecordStore::findIndexById(int id) const {
    const auto& arr = data_.asArray();
    for (size_t i = 0; i < arr.size(); ++i) {
        if (arr[i]["id"].asInt() == id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

const json::Value* RecordStore::findById(int id) const {
    int idx = findIndexById(id);
    if (idx < 0) return nullptr;
    return &data_.asArray()[idx];
}

std::vector<const json::Value*> RecordStore::findByField(const std::string& field, const std::string& value) const {
    std::vector<const json::Value*> results;

    for (const auto& item : data_.asArray()) {
        if (!item.isObject() || !item.contains(field)) continue;
        const json::Value& fieldValue = item[field];

        if (fieldValue.isString()) {
            if (toLower(fieldValue.asString()).find(toLower(value)) != std::string::npos) {
                results.push_back(&item);
            }
        } else if (fieldValue.isNumber()) {
            double target;
            if (tryParseDouble(value, target) && fieldValue.asDouble() == target) {
                results.push_back(&item);
            }
        } else if (fieldValue.isBool()) {
            std::string v = toLower(value);
            if (v == "y" && fieldValue.asBool() == true) results.push_back(&item);
            else if (v == "n" && fieldValue.asBool() == false) results.push_back(&item);
        }
        // array/object/null 필드는 검색 대상에서 제외
    }

    return results;
}

int RecordStore::nextId() const {
    int maxId = 0;
    for (const auto& item : data_.asArray()) {
        int id = item["id"].asInt();
        if (id > maxId) maxId = id;
    }
    return maxId + 1;
}

int RecordStore::addRecord(json::Value fields) {
    int id = nextId();
    fields["id"] = json::Value(id);
    data_.push_back(std::move(fields));
    save();
    return id;
}

bool RecordStore::updateField(int id, const std::string& field, const json::Value& newValue) {
    if (field == "id") return false;

    int idx = findIndexById(id);
    if (idx < 0) return false;

    data_.asArray()[idx][field] = newValue;
    save();
    return true;
}

void RecordStore::backup() const {
    std::ifstream src(filePath_, std::ios::binary);
    if (!src) {
        // 삭제 전 원본 파일이 아직 없는 경우 — 백업할 것이 없다.
        return;
    }
    std::ofstream dst(filePath_ + ".bak", std::ios::binary | std::ios::trunc);
    if (!dst) {
        throw std::runtime_error("백업 파일을 생성할 수 없습니다: " + filePath_ + ".bak");
    }
    dst << src.rdbuf();
    if (!dst) {
        throw std::runtime_error("백업 파일 쓰기에 실패했습니다: " + filePath_ + ".bak");
    }
}

bool RecordStore::removeById(int id) {
    int idx = findIndexById(id);
    if (idx < 0) return false;

    backup();
    data_.asArray().erase(data_.asArray().begin() + idx);
    save();
    return true;
}
