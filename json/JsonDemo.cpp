#include "JsonDemo.h"
#include <iostream>
#include "Json.h"

void runJsonDemo() {
    // 1) JSON 객체를 코드로 직접 구성
    json::Value person = json::Value::MakeObject();
    person["name"] = json::Value("Hong Gildong");
    person["age"] = json::Value(30);
    person["isStudent"] = json::Value(false);

    json::Value skills = json::Value::MakeArray();
    skills.push_back(json::Value("C++"));
    skills.push_back(json::Value("Python"));
    person["skills"] = skills;

    // 2) 파일로 저장 (들여쓰기 2칸)
    const std::string path = "person.json";
    person.SaveFile(path, 2);
    std::cout << "저장 완료: " << path << "\n";

    // 3) 파일에서 다시 읽어와 파싱
    json::Value loaded = json::Value::LoadFile(path);
    std::cout << "읽어온 이름: " << loaded["name"].asString() << "\n";
    std::cout << "읽어온 나이: " << loaded["age"].asInt() << "\n";

    // 4) 문자열로부터 직접 파싱
    std::string jsonText = R"({"a": 1, "b": [1, 2, 3], "c": {"nested": true}})";
    json::Value parsed = json::Value::Parse(jsonText);
    std::cout << "파싱 결과 (한 줄): " << parsed.dump() << "\n";
    std::cout << "파싱 결과 (들여쓰기): \n" << parsed.dump(2) << "\n";
}
