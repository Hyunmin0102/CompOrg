#!/usr/bin/env bash
# W5 회귀 검증: 리팩터링한 bpred(W5)의 출력이 W4 bpred와 trace별로 완전히 같은지 diff로 비교한다.
#
#   사용법: ./compare_w4.sh      (또는 make compare)
#
#   - W4 실행 파일과 trace는 ../w4_bpred/ 에 있는 것을 그대로 쓴다 (W4 폴더에는 아무것도 쓰지 않는다).
#   - 표에 trace 경로가 찍히므로, 두 실행 파일 모두 W4 폴더 안에서 같은 상대 경로(tests/xxx.txt)로 실행한다.
#   - 설정마다(기본, 작은 테이블, 짧은 히스토리) × trace마다 --top 출력과 종료 코드를 비교한다.
#   - 하나라도 다르면 diff를 보여 주고 종료 코드 1로 끝난다.
set -u

HERE="$(cd "$(dirname "$0")" && pwd)"
W4_DIR="$(cd "$HERE/../w4_bpred" && pwd)"
W4_BIN="$W4_DIR/bpred"
W5_BIN="$HERE/bpred"
OUT_DIR="$HERE/build/compare"

if [ ! -x "$W4_BIN" ]; then
    echo "error: W4 실행 파일이 없다: $W4_BIN  (먼저 W4 폴더에서 make)" >&2
    exit 2
fi
if [ ! -x "$W5_BIN" ]; then
    echo "error: W5 실행 파일이 없다: $W5_BIN  (먼저 make)" >&2
    exit 2
fi
if [ "$W4_DIR/bpred.cpp" -nt "$W4_BIN" ]; then
    echo "warning: W4 bpred.cpp가 W4 실행 파일보다 새롭다. W4 폴더에서 make를 다시 하는 것이 좋다" >&2
fi

# 비교할 설정. 기본값 외에 테이블이 작아 aliasing이 생기는 경우와 history < index인 경우를 넣는다.
CONFIGS=(
    ""
    "--index-bits 4"
    "--index-bits 4 --history-bits 2"
    "--index-bits 12 --history-bits 6"
)

mkdir -p "$OUT_DIR"
cd "$W4_DIR" || exit 2

pass=0
fail=0
for cfg in "${CONFIGS[@]}"; do
    for trace in tests/*.txt; do
        name="$(basename "$trace" .txt)"
        tag="${cfg:-default}"
        tag="${tag// /_}"
        w4_out="$OUT_DIR/${name}${tag}.w4.txt"
        w5_out="$OUT_DIR/${name}${tag}.w5.txt"

        # cfg는 일부러 따옴표 없이 넘긴다 (공백으로 나뉘어 인자 여러 개가 되어야 한다)
        # shellcheck disable=SC2086
        "$W4_BIN" --top $cfg "$trace" > "$w4_out" 2>&1
        w4_code=$?
        # shellcheck disable=SC2086
        "$W5_BIN" --top $cfg "$trace" > "$w5_out" 2>&1
        w5_code=$?

        label="$trace [${cfg:-default}]"
        if [ "$w4_code" -ne "$w5_code" ]; then
            echo "FAIL $label: exit code W4=$w4_code W5=$w5_code"
            fail=$((fail + 1))
        elif ! diff -u --label "W4 $label" --label "W5 $label" "$w4_out" "$w5_out"; then
            echo "FAIL $label"
            fail=$((fail + 1))
        else
            echo "ok   $label"
            pass=$((pass + 1))
        fi
    done
done

echo "---"
echo "compare with W4: $pass passed, $fail failed"
[ "$fail" -eq 0 ]
