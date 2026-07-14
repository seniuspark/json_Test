#include "Cli.h"
#include <iostream>
#include <sstream>

namespace {

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

// 콤마로 구분된 입력을 앞뒤 공백 제거 후 빈 항목을 걸러낸 문자열 목록으로 변환한다.
std::vector<std::string> splitSkills(const std::string& line) {
    std::vector<std::string> result;
    std::stringstream ss(line);
    std::string token;
    while (std::getline(ss, token, ',')) {
        std::string t = trim(token);
        if (!t.empty()) result.push_back(t);
    }
    return result;
}

// 문자열 전체가 정수로 소비되어야 유효한 것으로 간주한다 (예: "12abc"는 거부).
bool tryParseInt(const std::string& s, int& out) {
    try {
        size_t pos;
        int v = std::stoi(s, &pos);
        if (pos != s.size()) return false;
        out = v;
        return true;
    } catch (...) {
        return false;
    }
}

int readNonNegativeInt(const std::string& prompt) {
    while (true) {
        std::string s = trim(readLine(prompt));
        int v;
        if (tryParseInt(s, v) && v >= 0) return v;
        std::cout << "0 이상의 정수를 입력하세요\n";
    }
}

bool readYesNo(const std::string& prompt) {
    while (true) {
        std::string s = trim(readLine(prompt));
        if (s == "y" || s == "Y") return true;
        if (s == "n" || s == "N") return false;
        std::cout << "y 또는 n을 입력하세요\n";
    }
}

std::string readNonEmptyLine(const std::string& prompt, const std::string& emptyMessage) {
    while (true) {
        std::string s = trim(readLine(prompt));
        if (!s.empty()) return s;
        std::cout << emptyMessage << "\n";
    }
}

} // namespace

Cli::Cli(RecordStore& store) : store_(store) {}

void Cli::printSummary(const json::Value& p) const {
    std::cout << "[id=" << p["id"].asInt() << "] "
               << p["name"].asString() << " / "
               << p["age"].asInt() << "세 / "
               << (p["isStudent"].asBool() ? "학생" : "학생 아님") << " / 기술: ";
    const auto& skills = p["skills"].asArray();
    if (skills.empty()) {
        std::cout << "(없음)";
    } else {
        for (size_t i = 0; i < skills.size(); ++i) {
            std::cout << skills[i].asString();
            if (i + 1 < skills.size()) std::cout << ", ";
        }
    }
    std::cout << "\n";
}

void Cli::doCreate() {
    std::cout << "\n-- Create --\n";
    std::string name = readNonEmptyLine("이름 입력: ", "이름은 비워둘 수 없습니다");
    int age = readNonNegativeInt("나이 입력: ");
    bool isStudent = readYesNo("학생 여부 (y/n): ");
    std::string skillsLine = readLine("보유 기술 입력 (콤마로 구분, 없으면 Enter): ");
    std::vector<std::string> skills = splitSkills(skillsLine);

    json::Value fields = json::Value::MakeObject();
    fields["name"] = json::Value(name);
    fields["age"] = json::Value(age);
    fields["isStudent"] = json::Value(isStudent);
    json::Value skillsArr = json::Value::MakeArray();
    for (auto& s : skills) skillsArr.push_back(json::Value(std::move(s)));
    fields["skills"] = skillsArr;

    int id = store_.addRecord(fields);
    const json::Value* created = store_.findById(id);

    std::cout << "\n생성 완료 (id=" << id << ")\n";
    if (created) std::cout << created->dump(2) << "\n";
}

void Cli::doRead() {
    std::cout << "\n-- Read --\n"
                 "1. 전체 목록 보기\n"
                 "2. id로 검색\n"
                 "3. 필드 값으로 검색\n";
    std::string choice = trim(readLine("번호 선택 > "));

    if (choice == "1") {
        const auto& all = store_.all();
        if (all.empty()) {
            std::cout << "저장된 데이터가 없습니다.\n";
            return;
        }
        for (const auto& p : all) printSummary(p);
        std::cout << "총 " << all.size() << "건\n";
    } else if (choice == "2") {
        int id = readNonNegativeInt("검색할 id 입력: ");
        const json::Value* found = store_.findById(id);
        if (!found) {
            std::cout << "id=" << id << " 에 해당하는 데이터가 없습니다.\n";
            return;
        }
        std::cout << "\n" << found->dump(2) << "\n";
    } else if (choice == "3") {
        std::string field;
        while (true) {
            field = trim(readLine("검색할 필드 이름 (name/age/isStudent) 입력: "));
            if (field == "name" || field == "age" || field == "isStudent") break;
            std::cout << "지원하지 않는 검색 필드입니다 (name/age/isStudent 중 선택)\n";
        }
        std::string value = trim(readLine("검색어 입력: "));
        auto results = store_.findByField(field, value);
        if (results.empty()) {
            std::cout << "검색 결과가 없습니다.\n";
            return;
        }
        std::cout << "검색 결과 " << results.size() << "건:\n";
        for (const auto* p : results) printSummary(*p);
    } else {
        std::cout << "잘못된 선택입니다.\n";
    }
}

void Cli::doUpdate() {
    std::cout << "\n-- Update --\n";
    int id = readNonNegativeInt("수정할 id 입력: ");
    const json::Value* target = store_.findById(id);
    if (!target) {
        std::cout << "id=" << id << " 에 해당하는 데이터가 없습니다.\n";
        return;
    }
    std::cout << "\n현재 데이터:\n" << target->dump(2) << "\n";

    std::string field;
    while (true) {
        field = trim(readLine("수정할 필드 (name/age/isStudent/skills) 입력: "));
        if (field == "id") {
            std::cout << "id는 수정할 수 없는 필드입니다.\n";
            continue;
        }
        if (field == "name" || field == "age" || field == "isStudent" || field == "skills") break;
        std::cout << "수정 가능한 필드: name/age/isStudent/skills\n";
    }

    json::Value newValue;
    if (field == "name") {
        newValue = json::Value(readNonEmptyLine("새 값 입력: ", "이름은 비워둘 수 없습니다"));
    } else if (field == "age") {
        newValue = json::Value(readNonNegativeInt("새 값 입력: "));
    } else if (field == "isStudent") {
        newValue = json::Value(readYesNo("새 값 (y/n) 입력: "));
    } else { // skills
        std::string s = readLine("새 값 입력 (콤마로 구분, 없으면 Enter): ");
        json::Value arr = json::Value::MakeArray();
        for (auto& sk : splitSkills(s)) arr.push_back(json::Value(std::move(sk)));
        newValue = arr;
    }

    bool ok = store_.updateField(id, field, newValue);
    if (!ok) {
        std::cout << "수정에 실패했습니다.\n";
        return;
    }
    const json::Value* updated = store_.findById(id);
    std::cout << "\n수정 완료:\n";
    if (updated) std::cout << updated->dump(2) << "\n";
}

void Cli::doDelete() {
    std::cout << "\n-- Delete --\n";
    int id = readNonNegativeInt("삭제할 id 입력: ");
    const json::Value* target = store_.findById(id);
    if (!target) {
        std::cout << "id=" << id << " 에 해당하는 데이터가 없습니다.\n";
        return;
    }
    std::cout << "\n삭제 대상 데이터:\n" << target->dump(2) << "\n";

    if (!readYesNo("정말 삭제하시겠습니까? (y/n): ")) {
        std::cout << "삭제를 취소했습니다.\n";
        return;
    }

    try {
        bool ok = store_.removeById(id);
        if (ok) {
            std::cout << "삭제 완료 (id=" << id << ")\n";
        } else {
            std::cout << "삭제에 실패했습니다.\n";
        }
    } catch (const std::exception& ex) {
        std::cout << "삭제 중 오류가 발생했습니다: " << ex.what() << "\n";
    }
}

void Cli::run() {
    while (true) {
        std::cout << "\n=== 인물 관리 시스템 ===\n"
                     "1. Create - 새 레코드 추가\n"
                     "2. Read   - 전체 목록 / 검색\n"
                     "3. Update - 레코드 수정\n"
                     "4. Delete - 레코드 삭제\n"
                     "5. Exit\n";
        std::string choice = trim(readLine("번호 선택 > "));

        if (choice == "1") doCreate();
        else if (choice == "2") doRead();
        else if (choice == "3") doUpdate();
        else if (choice == "4") doDelete();
        else if (choice == "5") {
            std::cout << "종료합니다.\n";
            break;
        } else {
            std::cout << "잘못된 선택입니다.\n";
        }
    }
}
