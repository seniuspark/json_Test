#pragma once
#include <string>
#include <vector>
#include "Json.h"

// 특정 도메인(Person 등)에 종속되지 않는 범용 JSON 레코드 CRUD 저장소.
// 각 레코드는 "id" 필드를 가진 JSON 객체이며, id는 저장소가 자동으로 부여/관리한다.
// 그 외 필드 구성은 호출자가 자유롭게 정한다.
class RecordStore {
public:
    explicit RecordStore(std::string filePath);

    // 파일이 없으면 빈 배열로 시작한다.
    void load();
    // 현재 메모리 상태를 파일에 기록한다.
    void save() const;

    const json::Array& all() const;

    // 존재하지 않으면 nullptr 반환 (예외를 던지지 않음)
    const json::Value* findById(int id) const;

    // field가 존재하는 레코드만 대상으로, 해당 필드의 실제 타입에 맞춰 value와 비교한다.
    //   string 필드 -> 대소문자 무관 부분 문자열 일치
    //   number 필드 -> value를 숫자로 변환해 정확히 일치 (변환 실패 시 매칭 제외)
    //   bool   필드 -> value가 "y"/"n"(대소문자 무관)일 때만 정확히 일치 비교, 그 외 값은 매칭 제외
    //   array/object/null 필드 -> 검색 대상에서 제외
    std::vector<const json::Value*> findByField(const std::string& field, const std::string& value) const;

    // fields는 "id"를 제외한 필드로 구성된 JSON 객체. 저장소가 새 id를 부여해 추가하고 그 id를 반환한다.
    int addRecord(json::Value fields);

    // "id" 필드 수정은 항상 거부한다. 대상 id가 없으면 false를 반환한다.
    bool updateField(int id, const std::string& field, const json::Value& newValue);

    // 삭제 직전 파일을 "<파일경로>.bak"으로 백업한 뒤 삭제한다.
    // 대상 id가 없으면 false. 백업 실패 시 std::runtime_error를 던진다.
    bool removeById(int id);

private:
    int nextId() const;
    int findIndexById(int id) const;
    void backup() const;

    std::string filePath_;
    json::Value data_;
};
