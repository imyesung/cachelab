#!/bin/bash
# cachelab 채점 헬퍼 (Apple Silicon mac + colima x86-64 VM)
#
# 왜 이렇게 도는가:
#   - 호스트(mac)의 csim.c / trans.c 를 평소 에디터로 편집한다.
#   - 그 둘만 컨테이너 내부 빠른 디스크(/root/cachelab)로 복사해서 빌드+채점한다.
#   - test-trans 가 만드는 대용량 valgrind trace 를 virtiofs 마운트(/work)에 쓰면
#     에뮬 위에서 ~6배 느려져 alarm(120) timeout 이 난다. 내부 디스크는 빠르다.
#
# 사용법:  ./grade.sh
set -euo pipefail

docker exec -e USER=root cachelab bash -c '
  cp /work/csim.c /work/trans.c /root/cachelab/
  cd /root/cachelab
  make clean >/dev/null 2>&1 || true
  if ! make; then
    echo ">>> 빌드 실패 — 위 컴파일 에러부터 고쳐라"; exit 1
  fi
  echo "----- driver.py 채점 -----"
  python2 driver.py
'
