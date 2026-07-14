# CRUD 콘솔 애플리케이션 구현 계획

## 1. 목표

기존에 구현한 `json::Value` 파싱/직렬화 라이브러리(`json/Json.h`, `json/Json.cpp`) 위에,
`data.json` 파일을 데이터 저장소로 사용하는 CRUD(Create/Read/Update/Delete) 콘솔 애플리케이션을 추가한다.

- Create: 새 레코드를 입력받아 `data.json`에 저장
- Read: 전체 목록 조회 + id/필드 값으로 검색
- Update: 기존 레코드를 선택해 특정 필드만 수정
- Delete: 특정 레코드를 확인 절차를 거쳐 안전하게 삭제

## 2. 데이터 모델

기존 `main.cpp`의 person 데모를 확장한 고정 스키마를 사용한다. 각 레코드는 JSON 객체이며,
전체 데이터는 이 객체들의 JSON 배열(`data.json`)로 저장한다.

```json
[
  {
    "id": 1,
    "name": "Hong Gildong",
    "age": 30,
    "isStudent": false,
    "skills": ["C++", "Python"]
  }
]
```

| 필드 | 타입 | 필수 | 설명 |
|---|---|---|---|
| id | number(int) | O | 자동 증가, 사용자 입력 불가, 삭제된 id는 재사용하지 않음 |
| name | string | O | 빈 문자열 불가 |
| age | number(int) | O | 0 이상 정수 |
| isStudent | bool | O | true/false |
| skills | array\<string\> | X (기본값 빈 배열) | 콤마로 구분된 입력을 파싱 |

`id`는 저장소가 관리하는 대리 키(surrogate key)로, Read/Update/Delete에서 레코드를 지정하는
기본 식별자로 사용한다.

## 3. 파일 구조

```
json/
  Json.h / Json.cpp        // 기존 JSON 파싱/직렬화 라이브러리 (변경 없음)
  PersonStore.h            // 데이터 접근 계층 (신규)
  PersonStore.cpp
  Cli.h / Cli.cpp          // 메뉴 기반 CLI 계층 (신규)
  main.cpp                 // 진입점: PersonStore 로드 후 Cli 실행 (수정)
data.json                  // CRUD 전용 데이터 파일 (신규, 실행 파일과 같은 작업 디렉터리에 생성)
docs/
  Plan.md                  // 본 문서
  features/
    01-Create.md
    02-Read.md
    03-Update.md
    04-Delete.md
```

기존 `person.json` 데모 파일 및 `main.cpp`의 파싱 데모 코드는 그대로 두되, CRUD 기능은
별도 파일(`data.json`)과 별도 모듈(`PersonStore`)로 분리해 서로 간섭하지 않게 한다.

## 4. 아키텍처

```
[CLI 계층: Cli.h/.cpp]
   메뉴 출력, 사용자 입력 처리, 결과 출력
        |
        v
[데이터 접근 계층: PersonStore.h/.cpp]
   레코드 목록 보관(json::Array), id 채번, 검색/추가/수정/삭제, 파일 저장 트리거
        |
        v
[JSON 라이브러리: Json.h/.cpp]
   json::Value 파싱/직렬화, 파일 I/O (LoadFile/SaveFile)
```

- CLI 계층은 `json::Value`를 직접 다루지 않고 `PersonStore`가 제공하는 API만 사용한다.
  (필드 구조가 바뀌어도 CLI 로직 영향 최소화)
- `PersonStore`는 메모리에 `json::Value`(Array) 형태로 전체 레코드를 들고 있다가,
  변경이 발생한 시점(Create/Update/Delete 성공 시)마다 즉시 `data.json`에 다시 저장한다
  (트랜잭션 단위 = 한 번의 CLI 조작).

## 5. PersonStore 공통 API 설계

```cpp
class PersonStore {
public:
    explicit PersonStore(std::string filePath);

    void load();                 // data.json이 없으면 빈 배열로 시작
    void save() const;           // 현재 메모리 상태를 data.json에 기록

    const json::Array& all() const;                       // Read: 전체 목록
    const json::Value* findById(int id) const;             // Read/Update/Delete: id로 조회
    std::vector<const json::Value*> findByField(
        const std::string& field, const std::string& value) const; // Read: 필드 검색

    int addPerson(const std::string& name, int age,
                   bool isStudent, std::vector<std::string> skills); // Create, 새 id 반환

    bool updateField(int id, const std::string& field,
                      const json::Value& newValue);        // Update, 성공 여부 반환

    bool removeById(int id);                                // Delete, 성공 여부 반환

private:
    int nextId() const;          // 현재 레코드 중 최대 id + 1 (빈 경우 1)
    std::string filePath_;
    json::Value data_;           // json::Value(Type::Array)
};
```

- 존재하지 않는 `id`에 대한 조회/수정/삭제는 `nullptr`/`false`를 반환하고 예외를 던지지 않는다.
  (CLI가 사용자에게 "해당 id 없음" 메시지를 출력하도록 위임)
- 파일 입출력 실패(권한 없음 등)만 예외(`std::runtime_error`)로 전파한다.

## 6. CLI 메뉴 흐름 (main.cpp 진입 후)

```
=== 인물 관리 시스템 ===
1. Create - 새 레코드 추가
2. Read   - 전체 목록 / 검색
3. Update - 레코드 수정
4. Delete - 레코드 삭제
5. Exit
번호 선택 >
```

각 기능의 세부 입력/출력 흐름은 `docs/features/` 하위 문서를 따른다.

## 7. 공통 원칙

- **입력 검증**: 잘못된 입력(빈 값, 숫자 아님, 범위 초과)은 저장하지 않고 재입력을 요청하거나
  작업을 취소한다. 프로그램을 종료시키는 예외를 던지지 않는다.
- **안전한 삭제**: Delete는 삭제 전 대상 레코드 내용을 보여주고 `y/n` 확인을 받은 뒤에만 실행한다.
- **일관된 저장 시점**: Create/Update/Delete는 메모리 변경 직후 항상 `save()`를 호출해
  `data.json`과 메모리 상태가 어긋나지 않도록 한다.
- **id 불변성**: id는 한 번 부여되면 수정/재사용하지 않는다. 삭제된 id는 비워둔 채로 남는다.

## 8. 구현 순서 (마일스톤)

1. `PersonStore` 데이터 접근 계층 구현 (Create/Read/Update/Delete API 전부 포함, CLI 없이 단위 테스트로 검증)
2. Create 기능 CLI 연결 및 수동 테스트 → [[features/01-Create]]
3. Read 기능 CLI 연결 (목록 + 검색) → [[features/02-Read]]
4. Update 기능 CLI 연결 → [[features/03-Update]]
5. Delete 기능 CLI 연결 (안전 삭제 확인 포함) → [[features/04-Delete]]
6. 전체 통합 후 `.vcxproj`/`.vcxproj.filters`에 신규 파일 등록, 빌드 및 시나리오 수동 검증

## 9. 세부 기능 문서

- [[features/01-Create]] — 새 데이터 입력 및 저장
- [[features/02-Read]] — 전체 목록 및 id/필드 검색
- [[features/03-Update]] — 특정 필드 수정
- [[features/04-Delete]] — 안전한 삭제
