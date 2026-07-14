# Feature: Update

> 상위 문서: [[../Plan]]

## 목적

id로 기존 레코드를 선택한 뒤, 사용자가 지정한 필드 하나를 새 값으로 수정하고 `data.json`에 반영한다.

## 관련 API

- `PersonStore::findById(int id) -> const json::Value*` — 수정 대상 조회 및 존재 확인
- `PersonStore::updateField(int id, field, const json::Value& newValue) -> bool` — 단일 필드 수정, 성공 여부 반환

## 사용자 흐름

```
-- Update --
수정할 id 입력: 1

현재 데이터:
{
  "id": 1,
  "name": "홍길동",
  "age": 30,
  "isStudent": false,
  "skills": ["C++", "Python"]
}

수정할 필드 (name/age/isStudent/skills) 입력: age
새 값 입력: 31

수정 완료:
{
  "id": 1,
  "name": "홍길동",
  "age": 31,
  "isStudent": false,
  "skills": ["C++", "Python"]
}
```

## 처리 절차

1. id 입력받아 `findById`로 존재 확인. 없으면 "id=.. 에 해당하는 데이터가 없습니다." 안내 후 메뉴로 복귀 (id 재입력은 강제하지 않음)
2. 대상 레코드 전체를 `dump(2)`로 보여준다 (수정 전 현재 상태 확인용)
3. 수정할 필드명 입력 — `id`는 수정 대상에서 제외 (허용 필드: name/age/isStudent/skills)
4. 필드별 새 값 입력 형식은 Create와 동일한 검증 규칙을 재사용
   - name: 빈 문자열 불가
   - age: 0 이상 정수
   - isStudent: y/n
   - skills: 콤마 구분 문자열 → 전체 배열을 통째로 교체 (부분 추가/삭제 아님)
5. 검증 통과 시 `updateField(id, field, newValue)` 호출 → 내부에서 해당 인덱스의 필드만 교체
6. `save()` 호출로 즉시 파일 반영
7. 수정된 레코드 전체를 다시 `dump(2)`로 출력해 결과 확인

## 필드명 검증

- `id` 입력 시: "id는 수정할 수 없는 필드입니다." 안내 후 필드 재입력 요청
- 허용되지 않는 필드명 입력 시: "수정 가능한 필드: name/age/isStudent/skills" 안내 후 재입력 요청

## 에지 케이스

- 존재하지 않는 id: 에러 없이 안내 메시지 출력 후 메뉴로 복귀 (updateField가 호출되지 않으므로 파일 변경 없음)
- 필드 값 검증 실패(예: age에 문자 입력): Create와 동일하게 재입력 요청, 무효한 값은 저장되지 않음
- skills를 빈 입력으로 수정: 빈 배열 `[]`로 교체됨 (의도적 동작, 기존 값 유지가 아님을 사용자에게 안내 문구로 명시)
- 동일한 값으로 "수정"하는 경우도 정상 처리 (변경 없음이라는 이유로 거부하지 않음)

## 테스트 체크리스트

- [ ] 존재하는 id의 name 필드 수정이 정상 반영되고 파일에 저장되는지
- [ ] 존재하는 id의 age 필드 수정이 정상 반영되는지
- [ ] 존재하는 id의 isStudent 필드 수정이 정상 반영되는지
- [ ] 존재하는 id의 skills 필드 수정(전체 교체)이 정상 반영되는지
- [ ] 존재하지 않는 id 입력 시 파일이 변경되지 않는지
- [ ] id 필드를 수정하려고 시도하면 거부되는지
- [ ] 잘못된 필드명 입력 시 재입력을 요청하는지
- [ ] 잘못된 값(예: age에 문자) 입력 시 재입력을 요청하고 기존 값이 유지되는지
