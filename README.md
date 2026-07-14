# json — JSON 라이브러리 & CRUD 콘솔 애플리케이션

외부 라이브러리 없이 C++20만으로 구현한 JSON 파싱/직렬화 라이브러리와,
그 위에 올린 `data.json` 기반 CRUD(Create/Read/Update/Delete) 콘솔 애플리케이션입니다.

## 목차

- [빌드 & 실행](#빌드--실행)
- [소프트웨어 구조](#소프트웨어-구조)
- [파일 구성](#파일-구성)
- [모듈별 API](#모듈별-api)
  - [json::Value (Json.h)](#jsonvalue-jsonh)
  - [RecordStore](#recordstore)
  - [Cli](#cli)
  - [JsonDemo](#jsondemo)
- [데이터 모델](#데이터-모델)
- [사용법](#사용법)
- [안전장치](#안전장치)
- [관련 문서](#관련-문서)

## 빌드 & 실행

Visual Studio 2022(v145 툴셋, C++20)로 구성된 프로젝트입니다.

```powershell
# MSBuild로 빌드 (x64, Debug)
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" json.slnx /p:Configuration=Debug /p:Platform=x64

# 실행 (실행 파일과 같은 작업 디렉터리에 person.json / data.json 이 생성됩니다)
.\x64\Debug\json.exe
```

Visual Studio에서 `json.slnx`를 열어 F5로 실행해도 동일합니다.

> 소스에 한글 문자열/주석이 포함되어 있어 반드시 **UTF-8 BOM**으로 저장되어야 합니다.
> BOM이 없으면 MSVC가 코드페이지를 잘못 추정해 문자열 리터럴이 깨집니다.

## 소프트웨어 구조

세 계층으로 분리되어 있으며, 위 계층은 아래 계층의 API만 사용하고 내부 구현을 알지 못합니다.

```
┌──────────────────────────────────────────────┐
│  main.cpp                                     │
│   - runJsonDemo() 실행 (JSON 라이브러리 데모)   │
│   - RecordStore + Cli 로 CRUD 앱 실행           │
└───────────────────┬────────────────────────────┘
                     │
   ┌─────────────────┴─────────────────┐
   │                                    │
┌──▼─────────────────────┐   ┌──────────▼─────────────┐
│  Cli (Cli.h/.cpp)      │   │  JsonDemo (JsonDemo.h)  │
│  Person 스키마 CLI 메뉴 │   │  파싱/저장 사용법 데모   │
│  - 입력 검증           │   └──────────┬─────────────┘
│  - 화면 출력           │              │
└──────────┬─────────────┘              │
           │                            │
┌──────────▼─────────────────┐          │
│ RecordStore                │          │
│ (RecordStore.h/.cpp)       │          │
│  - 범용 JSON 레코드 CRUD    │          │
│  - id 자동 채번             │          │
│  - 삭제 전 자동 백업         │          │
└──────────┬──────────────────┘          │
           │                            │
┌──────────▼────────────────────────────▼──┐
│  json::Value  (Json.h/.cpp)              │
│   - JSON 파싱 (Parse)                     │
│   - JSON 직렬화 (dump)                    │
│   - 파일 입출력 (LoadFile / SaveFile)      │
└────────────────────────────────────────────┘
```

- **json::Value**: JSON 값 자체를 표현하고 파싱/직렬화/파일 I/O를 제공하는 최하위 계층. 도메인 지식이 전혀 없습니다.
- **RecordStore**: `json::Value` 위에서 "id를 가진 레코드들의 배열"이라는 범용 CRUD 개념만 알고 있습니다. `Person`이라는 개념 자체를 모릅니다.
- **Cli**: `RecordStore` 위에서 Person 스키마(name/age/isStudent/skills)에 대한 입력 검증과 메뉴 UI를 담당합니다. 스키마가 바뀌면 이 계층만 수정하면 됩니다.
- **JsonDemo**: `json::Value`의 기본 사용법(객체 생성 → 저장 → 로드 → 파싱)을 보여주는 독립 데모로, CRUD 앱과 무관하게 실행됩니다.

### Delete 안전 삭제 흐름

```
사용자                Cli                RecordStore              data.json / .bak
  │                    │                      │                        │
  │ "삭제할 id 입력"    │                      │                        │
  ├───────────────────►│                      │                        │
  │                    │  findById(id)        │                        │
  │                    ├─────────────────────►│                        │
  │                    │◄─────────────────────┤ (레코드 or nullptr)      │
  │  대상 데이터 출력    │                      │                        │
  │◄───────────────────┤                      │                        │
  │ "정말 삭제? (y/n)"  │                      │                        │
  ├───────────────────►│                      │                        │
  │                    │ (n) 취소, 종료        │                        │
  │                    │ (y) removeById(id)   │                        │
  │                    ├─────────────────────►│  backup()              │
  │                    │                      ├───────────────────────►│ data.json → data.json.bak 복사
  │                    │                      │  벡터에서 erase         │
  │                    │                      │  save()                │
  │                    │                      ├───────────────────────►│ data.json 갱신
  │                    │◄─────────────────────┤ true                   │
  │  "삭제 완료"        │                      │                        │
  │◄───────────────────┤                      │                        │
```

## 파일 구성

```
json/
├── json.slnx                    # Visual Studio 솔루션
├── json/
│   ├── Json.h / Json.cpp        # JSON 파싱/직렬화 라이브러리
│   ├── JsonDemo.h / JsonDemo.cpp# json::Value 사용법 데모 (person.json)
│   ├── RecordStore.h / .cpp     # 범용 JSON 레코드 CRUD 저장소
│   ├── Cli.h / Cli.cpp          # Person 스키마 메뉴 CLI
│   ├── main.cpp                 # 진입점
│   └── json.vcxproj[.filters]   # 프로젝트 파일
├── docs/
│   ├── Plan.md                  # CRUD 앱 구현 계획
│   └── features/
│       ├── 01-Create.md
│       ├── 02-Read.md
│       ├── 03-Update.md
│       └── 04-Delete.md
├── person.json                  # JsonDemo 실행 결과물
└── data.json                    # CRUD 앱 데이터 파일 (실행 시 생성, .gitignore 권장)
```

## 모듈별 API

### json::Value (Json.h)

JSON의 6가지 타입(null, bool, number, string, array, object)을 표현하는 값 타입입니다.
object는 삽입 순서를 보존하는 `vector<pair<string, Value>>`로 구현되어 있습니다.

| API | 설명 |
|---|---|
| `Value()` / `Value(nullptr)` | null 값 생성 |
| `Value(bool)` / `Value(int)` / `Value(double)` / `Value(string)` | 각 타입 값 생성 |
| `Value::MakeArray()` / `Value::MakeObject()` | 빈 배열/객체 생성 |
| `isNull()/isBool()/isNumber()/isString()/isArray()/isObject()` | 타입 판별 |
| `asBool()/asDouble()/asInt()/asString()/asArray()/asObject()` | 타입에 맞는 값 꺼내기 (타입 불일치 시 예외) |
| `operator[](string key)` | 객체 필드 접근. non-const 버전은 없는 키를 자동 생성 |
| `operator[](size_t index)` | 배열 원소 접근 |
| `contains(key)` | 객체에 해당 키가 있는지 확인 |
| `push_back(Value)` | 배열 끝에 원소 추가 (null 값에 처음 호출 시 자동으로 배열이 됨) |
| `dump(int indent = -1)` | JSON 문자열로 직렬화. `indent < 0`이면 한 줄 압축, 아니면 들여쓰기 |
| `Value::Parse(string text)` | 문자열을 파싱해 `Value` 생성 (정적 함수) |
| `Value::LoadFile(string path)` | 파일을 읽어 파싱 (정적 함수) |
| `SaveFile(string path, int indent = 2)` | 현재 값을 파일에 직렬화하여 저장 |

```cpp
json::Value person = json::Value::MakeObject();
person["name"] = json::Value("Hong Gildong");
person["skills"] = json::Value::MakeArray();
person["skills"].push_back(json::Value("C++"));

person.SaveFile("person.json", 2);              // 파일 저장
json::Value loaded = json::Value::LoadFile("person.json"); // 파일 로드
std::string oneLine = loaded.dump();             // 압축 직렬화
```

### RecordStore

특정 도메인(Person 등)을 모르는 범용 JSON 레코드 CRUD 저장소입니다.
레코드는 `"id"` 필드를 가진 JSON 객체이고, 전체 데이터는 이 객체들의 배열로 파일에 저장됩니다.

| API | 설명 |
|---|---|
| `RecordStore(string filePath)` | 데이터 파일 경로 지정 |
| `load()` | 파일에서 로드 (파일이 없으면 빈 배열로 시작) |
| `save() const` | 현재 메모리 상태를 파일에 기록 |
| `all() const -> const json::Array&` | 전체 레코드 목록 |
| `findById(int id) const -> const json::Value*` | id로 조회, 없으면 `nullptr` |
| `findByField(field, value) const -> vector<const json::Value*>` | 필드 값으로 검색. 필드의 실제 타입(string/number/bool)에 따라 자동으로 비교 방식이 결정됨 |
| `addRecord(json::Value fields) -> int` | 새 레코드 추가, id 자동 부여 후 반환 |
| `updateField(id, field, newValue) -> bool` | 단일 필드 수정. `"id"` 필드 수정은 항상 거부 |
| `removeById(int id) -> bool` | 백업(`<파일>.bak`) 후 삭제 |

`findByField`의 비교 규칙:

| 필드 타입 | 비교 방식 |
|---|---|
| string | 대소문자 무관 부분 문자열 포함 여부 |
| number | `value`를 숫자로 변환해 정확히 일치 (변환 실패 시 매칭 제외) |
| bool | `value`가 `y`/`n`(대소문자 무관)일 때만 정확히 일치 비교 |
| array / object / null | 검색 대상에서 제외 |

```cpp
RecordStore store("data.json");
store.load();

json::Value fields = json::Value::MakeObject();
fields["name"] = json::Value("Kim Minsu");
fields["age"] = json::Value(25);
int id = store.addRecord(fields);        // id 자동 부여, 파일에 즉시 저장

auto results = store.findByField("name", "min"); // 부분 일치 검색
store.updateField(id, "age", json::Value(26));   // 필드 수정
store.removeById(id);                            // 백업 후 삭제
```

### Cli

`RecordStore` 위에서 Person 스키마(`name`/`age`/`isStudent`/`skills`)에 대한 입력 검증과
메뉴 기반 콘솔 UI를 담당합니다. `RecordStore`는 이 스키마를 알지 못하므로,
필드 구성이 바뀌면 이 계층만 수정하면 됩니다.

| API | 설명 |
|---|---|
| `Cli(RecordStore& store)` | 저장소 참조를 받아 생성 |
| `run()` | 메뉴 루프 시작 (Create/Read/Update/Delete/Exit) |

```cpp
RecordStore store("data.json");
store.load();
Cli cli(store);
cli.run();
```

### JsonDemo

`json::Value`의 기본 사용법(객체 구성 → 파일 저장 → 파일 로드 → 문자열 파싱)을 보여주는
독립 데모입니다. CRUD 앱과는 무관하며 `person.json` 파일을 생성합니다.

| API | 설명 |
|---|---|
| `runJsonDemo()` | 데모 실행 |

## 데이터 모델

`data.json`은 아래 구조를 가진 레코드들의 JSON 배열입니다.

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

| 필드 | 타입 | 설명 |
|---|---|---|
| id | number | `RecordStore`가 자동 부여하는 대리 키. 삭제되어도 재사용하지 않음 |
| name | string | 빈 문자열 불가 |
| age | number | 0 이상 정수 |
| isStudent | bool | true/false |
| skills | array\<string\> | 없으면 빈 배열 |

## 사용법

실행하면 먼저 JSON 데모(`person.json` 생성)가 실행된 뒤, CRUD 메뉴가 나타납니다.

```
=== 인물 관리 시스템 ===
1. Create - 새 레코드 추가
2. Read   - 전체 목록 / 검색
3. Update - 레코드 수정
4. Delete - 레코드 삭제
5. Exit
번호 선택 >
```

**Create**

```
번호 선택 > 1
이름 입력: Kim Minsu
나이 입력: 25
학생 여부 (y/n): y
보유 기술 입력 (콤마로 구분, 없으면 Enter): Java,Spring

생성 완료 (id=1)
{
  "name": "Kim Minsu",
  "age": 25,
  "isStudent": true,
  "skills": ["Java", "Spring"],
  "id": 1
}
```

**Read** — 전체 목록 / id 검색 / 필드 검색(name 부분일치, age 정확일치, isStudent y/n) 3가지 지원

```
번호 선택 > 2
1. 전체 목록 보기
2. id로 검색
3. 필드 값으로 검색
번호 선택 > 1
[id=1] Kim Minsu / 25세 / 학생 / 기술: Java, Spring
총 1건
```

**Update** — id 지정 후 name/age/isStudent/skills 중 하나만 수정 (id 자체는 수정 불가)

```
번호 선택 > 3
수정할 id 입력: 1
수정할 필드 (name/age/isStudent/skills) 입력: age
새 값 입력: 26
```

**Delete** — 대상 확인 후 y/n 승인을 받아야만 삭제 실행

```
번호 선택 > 4
삭제할 id 입력: 1
삭제 대상 데이터: { ... }
정말 삭제하시겠습니까? (y/n): y
삭제 완료 (id=1)
```

## 안전장치

- **입력 검증**: 빈 이름, 숫자가 아닌 나이, y/n 이외 값은 프로그램을 종료시키지 않고 재입력을 요청합니다.
- **id 불변성**: id는 `RecordStore`가 관리하며 사용자가 직접 수정할 수 없고, 삭제된 id는 재사용되지 않습니다.
- **삭제 확인**: Delete는 대상 레코드를 먼저 보여준 뒤 명시적 `y` 승인이 있어야만 실행됩니다.
- **삭제 백업**: 삭제 직전 `data.json`을 `data.json.bak`으로 복사한 뒤 삭제를 진행하며, 백업에 실패하면 삭제 자체를 중단합니다.

## 관련 문서

- [docs/Plan.md](docs/Plan.md) — 전체 구현 계획 및 아키텍처 설계
- [docs/features/01-Create.md](docs/features/01-Create.md)
- [docs/features/02-Read.md](docs/features/02-Read.md)
- [docs/features/03-Update.md](docs/features/03-Update.md)
- [docs/features/04-Delete.md](docs/features/04-Delete.md)
