# 13. Development Workflow & Verification

Sonheim의 최근 개발에서는 런타임 기능뿐 아니라 **UE Editor 작업과 반복 검증을 자동화하는 흐름**도 함께 개선했습니다.

---

## Agent MCP Integration

Repository에 Claude Code / Codex가 Unreal Editor의 Agent MCP에 연결되는 설정을 두고, Editor 작업을 코드 변경과 같은 workflow에서 실행할 수 있도록 했습니다.

활용 예:

- Animation Blueprint / Montage 편집
- BlendSpace / Inertialization 설정
- Blueprint Class Default 적용 / 확인
- Viewport render capture
- Dungeon content authoring
- Editor 결과 검증

Agent MCP 자체 구현은 별도 프로젝트이며, Sonheim에서는 **개발 workflow의 소비 사례**로만 다룹니다.

---

## Verification Strategy

최근 Dungeon/Boss 개발은 기능 하나를 추가하고 눈으로 한 번 확인하는 방식보다, scenario 단위 반복 검증을 사용했습니다.

대표 범주:

- Definition structural validation
- Shortcut / ExtraWave success path
- Timeout / death / leave failure path
- Listen Server / Client UI consistency
- HUD state
- Boss status / behavior
- Minimap / marker
- Result / record / grade

---

## 검증 수치의 취급

개발 과정에서 여러 verification script를 사용했지만, 일부 script는 현재 `main` repository에 포함되어 있지 않습니다.

따라서 Wiki에서는 다음을 구분합니다.

1. **Repository에서 현재 바로 확인 가능한 code-level validation**
2. **개발 checkout에서 반복 수행한 scenario verification**

재현 가능한 근거보다 숫자를 크게 보이는 것을 우선하지 않습니다.

---

## Regression-oriented Development

최근 수정 예:

- Guardian capture 시 client crash 재현 후 null-state 방어
- Boss wake multicast가 relevancy 이전에 유실되는 문제 분석
- Barrier rule을 actor-local 상태에서 Definition/Snapshot으로 이동
- Dungeon 전용 Toast를 공용 NoticeSubsystem으로 일반화

핵심은 한 번 발견한 문제를 개별 예외로 끝내기보다 **공통 규칙 / 데이터 / 검증 경로로 이동시키는 것**입니다.

---

## Source-only Mirror

전체 UE Project는 asset 때문에 크기가 큽니다. 코드 검토를 위해 GitHub Actions가 다음 항목을 별도 source-only repository로 동기화합니다.

- Source
- Config
- Plugin Source
- UProject
- Doc
- README

[Sonheim.Source](https://github.com/chungheonLee0325/Sonheim.Source)

---

## 관련 저장소

- [Sonheim](https://github.com/chungheonLee0325/Sonheim)
- [Sonheim.Source](https://github.com/chungheonLee0325/Sonheim.Source)
- [AgentMcp](https://github.com/chungheonLee0325/AgentMcp)
