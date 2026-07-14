#pragma once
#include "RecordStore.h"

// Person 스키마(name/age/isStudent/skills) 관리 메뉴 기반 CLI.
// 저장소 자체(RecordStore)는 이 스키마를 알지 못하는 범용 구현이다.
class Cli {
public:
    explicit Cli(RecordStore& store);

    void run();

private:
    void doCreate();
    void doRead();
    void doUpdate();
    void doDelete();

    void printSummary(const json::Value& person) const;

    RecordStore& store_;
};
