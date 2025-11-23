# Arena Allocator with Snapshot

메모리 할당을 효율적으로 관리하는 Arena 할당자 구현체입니다. 스냅샷/롤백 기능과 메모리 오버플로우 탐지 기능을 제공합니다.

## 특징

- **빠른 할당**: 단순한 포인터 증가 방식으로 O(1) 할당
- **일괄 해제**: `arena_destroy()` 한 번으로 모든 메모리 해제
- **스냅샷/롤백**: 특정 시점으로 메모리 상태 복원 가능
- **메모리 정렬**: 8바이트 정렬 지원으로 성능 최적화
- **Guard Pattern**: 버퍼 오버플로우 탐지 기능
- **Overflow 체크**: size_t 오버플로우 방지

## 프로젝트 구조

```
arena-snapshot/
├── include/
│   └── arena.h          # 헤더 파일
├── src/
│   ├── arena.c          # Arena 구현
│   └── test.c           # 테스트 코드
├── build/               # 빌드 출력 (자동 생성)
├── Makefile
└── README.md
```

## 빌드 및 실행

### 요구사항
- GCC 컴파일러
- Make
- C11 표준 지원

### Linux / WSL
```bash
make          # 컴파일
make run      # 실행
make clean    # 정리
```

### Windows (WSL 권장)
```bash
wsl
cd /mnt/c/path/to/arena-snapshot
make run
```

## API

### 기본 함수

```c
// Arena 생성 및 해제
Arena* arena_create(size_t capacity);
void arena_destroy(Arena *arena);

// 메모리 할당
void* arena_alloc(Arena *arena, size_t size, size_t align);

// 스냅샷 및 롤백
Snapshot arena_snapshot(Arena *arena);
void arena_rollback(Arena *arena, Snapshot snapshot);
void arena_reset(Arena* arena);

// Guard 기능 (버퍼 오버플로우 탐지)
void* arena_alloc_guarded(Arena *arena, size_t size, size_t align);
int arena_check_guard(void* data_ptr);
```

## 사용 예제

### 기본 사용법

```c
#include "arena.h"

// Arena 생성 (1MB)
Arena* arena = arena_create(1024 * 1024);

// 메모리 할당
int* numbers = arena_alloc(arena, sizeof(int) * 100, 8);
char* string = arena_alloc(arena, 256, 8);

// 사용...

// 한 번에 모든 메모리 해제
arena_destroy(arena);
```

### 스냅샷 & 롤백

```c
Arena* arena = arena_create(4096);

// 첫 번째 할당
char* data1 = arena_alloc(arena, 100, 8);

// 스냅샷 저장
Snapshot checkpoint = arena_snapshot(arena);

// 임시 작업
char* temp = arena_alloc(arena, 200, 8);
// ... 작업 실패 ...

// 롤백 (temp 할당 취소)
arena_rollback(arena, checkpoint);

// 메모리 재사용
char* data2 = arena_alloc(arena, 150, 8);

arena_destroy(arena);
```

### JSON 파싱 예제

```c
Arena* arena = arena_create(1024 * 1024);

for (int i = 0; i < file_count; i++) {
    Snapshot snap = arena_snapshot(arena);
    
    JsonValue* json = parse_json(arena, files[i]);
    
    if (json) {
        process(json);
    } else {
        // 파싱 실패 시 롤백
        arena_rollback(arena, snap);
    }
    
    // 다음 파일을 위해 리셋
    arena_reset(arena);
}

arena_destroy(arena);
```

### Guard 패턴 (버퍼 오버플로우 탐지)

```c
Arena* arena = arena_create(4096);

// Guard 포함 할당
char* buffer = arena_alloc_guarded(arena, 100, 8);

// 정상 사용
strcpy(buffer, "Hello");

// Guard 체크 (정상)
if (arena_check_guard(buffer)) {
    printf("OK\n");
}

// 버퍼 오버플로우 발생
buffer[105] = 'X';  // 경계 밖 쓰기

// Guard 체크 (실패)
if (!arena_check_guard(buffer)) {
    printf("Buffer overflow detected!\n");
}

arena_destroy(arena);
```

## 사용 사례

Arena 할당자는 다음과 같은 경우에 유용합니다:

1. **컴파일러/인터프리터**: 파싱 → AST 생성 → 코드 생성 단계별 할당
2. **게임 개발**: 프레임마다 임시 데이터 할당 후 리셋
3. **웹 서버**: 요청 처리 후 메모리 일괄 해제
4. **문서 파싱**: HTML/JSON/XML 파싱 시 임시 노드 할당
5. **데이터 처리**: 배치 작업 후 일괄 해제

## 성능 이점

- **할당 속도**: `malloc()` 대비 10-100배 빠름 (단순 포인터 증가)
- **해제 속도**: 개별 `free()` 수천 번 → `arena_destroy()` 한 번
- **메모리 단편화 없음**: 연속된 메모리 블록 사용
- **캐시 효율성**: 메모리 지역성(locality) 향상

## 제한사항

- 개별 객체 해제 불가 (일괄 해제만 가능)
- 장시간 유지되는 다양한 크기의 객체에는 부적합
- Arena 크기를 초과하면 할당 실패


## 라이선스

MIT License
