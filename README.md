# Frame Window Analyzer

Geometry Dash 입력의 **Local Frame Window**를 240 TPS 기준으로 측정하는 Windows용 Geode 모드입니다. 선택한 PRESS/RELEASE 이벤트 하나의 tick만 이동하고 나머지 macro 입력은 원래 tick에 유지합니다.

GD 2.2081의 `m_tickIndex`는 240Hz 물리 프레임마다 2씩 증가합니다. 따라서 분석은 녹화 tick과 같은 parity를 2 raw tick 간격으로 검사하며, 2 raw tick을 240Hz 1프레임(4.1667ms)으로 계산합니다.

## Requirements

- Windows x86_64
- Geometry Dash 2.2081
- Geode 5.9.0
- 빌드 시 Geode CLI 3.8.0, Geode SDK 5.9.0, CMake 3.29+, Visual Studio 2022+ (MSVC 19.44+)

런타임 hard dependency는 없습니다. CBF, TPS bypass, speedhack 또는 입력/물리를 바꾸는 다른 모드가 켜져 있으면 결과 의미가 달라질 수 있습니다.

## Build

공식 Geode 개발 환경에서:

```powershell
$env:GEODE_SDK = "C:\path\to\geode-sdk"
geode build --config RelWithDebInfo
```

CLI profile이 설정되어 있으면 빌드 후 자동 설치됩니다. 이 저장소에서 검증한 환경은 Geode SDK 5.9.0 / bindings 2.2081 / MSVC 19.44 / CMake 3.31.6입니다.

## Install

생성된 `build/local.frame-window-analyzer.geode`를 다음 폴더에 복사합니다.

```text
<Geometry Dash>/geode/mods/
```

또는 Geometry Dash의 Geode 모드 화면에서 `.geode` 파일을 설치합니다.

## Usage

1. 분석할 레벨을 실행하고 Pause 메뉴를 엽니다.
2. **Frame Window Analyzer**를 누릅니다.
3. **Record**를 누릅니다. 레벨이 처음부터 재시작됩니다.
4. 정상 플레이를 수행합니다. PRESS/RELEASE와 P1/P2, Jump/Left/Right가 개별 이벤트로 기록됩니다.
5. 레벨을 완료하거나 Pause → Analyzer → **Stop**을 누릅니다. 사망 후 시도가 재시작되면 이전 시도의 입력은 자동 폐기되고 새 시도를 계속 녹화합니다.
6. **Playback**으로 macro를 확인합니다.
7. input 번호와 좌/우 search range를 240Hz 프레임 단위로 설정합니다(기본 앞뒤 5프레임). 기본 부분 검증은 선택한 입력 하나만 이동하고, 이후 `Next Inputs` 3개(PRESS/RELEASE 각각 1개)를 녹화 tick 그대로 재생한 뒤 판정합니다. `Next Inputs`가 0일 때만 Validation Frames를 사용하며, 전체 레벨 검증이 필요할 때만 `Until End`를 켭니다.
8. 단일 이벤트는 **Analyze Selected**를 누릅니다. 전체 분석은 **All Clicks**(PRESS만), **All Releases**(RELEASE만), **All Inputs**(둘 다) 중 하나를 선택합니다.
9. 화면 왼쪽 위 HUD에서 현재 input/test/tick 진행상태를 확인합니다. Pause 메뉴에서 Analyzer를 다시 열어 중간 결과를 볼 수 있습니다.
10. 오래 걸리는 분석은 **Cancel**로 안전하게 중단합니다.
11. 완료 후 **Show Results**를 누르면 레벨을 다시 재생하며 각 입력 순간에 frame window가 색상 배지로 표시됩니다.
12. **Export + Open**을 누릅니다. CSV/JSON 저장이 성공하면 Windows 탐색기로 mod 전용 결과 폴더가 자동으로 열립니다.

결과 재생 색상은 `1-2F` 빨강, `3-5F` 주황, `6-10F` 초록, `11F+` 청록입니다. Primary Window가 없는 입력은 회색 `NO WINDOW`로 표시됩니다. 우측 상단에는 최근 입력 6개의 결과가 남습니다.

결과 파일은 Geode가 제공하는 mod 전용 save directory 아래 `results/<level-id> - <level-name>/`에 레벨별로 저장됩니다. 각 폴더에는 `frame-windows.csv`와 `frame-windows.json`이 생성됩니다. JSON은 level name/ID, 비연속 valid tick과 모든 연속 window를 보존합니다. Reference tick이 속한 연속 구간이 Primary Window입니다.

## Validation semantics

- 물리/input 기준: `GJBaseGameLayer::m_tickIndex`; 2 raw tick-index units = 240Hz 1프레임
- 입력 주입: 각 physics command tick 직전 `handleButton`
- 분석 후보 입력 주입: 240Hz 프레임 경계에서 deterministic direct playback
- 후보 격리: 후보마다 `PlayLayer::resetLevelFromStart()` 후 macro 전체를 처음부터 재생
- `Until End` 성공: 나머지 macro를 원래 tick 그대로 재생하여 실제 level complete
- `Until End` 실패: level complete 전 player death
- 부분 검증 성공: 설정한 tick horizon과 `Next Inputs` 입력 이벤트를 모두 통과
- 알고리즘: SAFE exhaustive (`-Left ... +Right`)만 사용

## Current limitations

- v0.1은 정확성 기준 Stage A 구현입니다. 후보마다 처음부터 실시간 재생하므로 Analyze All은 오래 걸릴 수 있습니다.
- FAST boundary search, custom savestate, rendering/audio suppression, GDR import, global window 재최적화는 구현하지 않았습니다.
- practice checkpoint에서 녹화/분석하지 마십시오. Record와 후보 replay는 레벨 시작 상태를 기준으로 합니다.
- 다른 모드가 TPS, scheduler, input queue 또는 physics를 변경하면 240 TPS local window로 해석할 수 없습니다. 해당 모드를 끄고 재측정하는 것이 권장됩니다.
- 빌드는 검증되었지만 각 레벨의 determinism은 레벨과 활성 모드 구성에 좌우됩니다. 먼저 Playback이 원본 플레이를 재현하는지 확인하십시오.

## Output example

```text
Reference: 12671
Valid ticks: 12667 12668 12669 12670 12671
Primary Window: 12667 ~ 12671
Frame Window: 5F
Time Window: 20.833 ms
```
