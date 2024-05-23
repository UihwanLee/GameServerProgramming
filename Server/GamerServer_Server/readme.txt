
<게임서버프로그래밍>
2018156032 이의환

Client - OpenGL / Server - SFML socket 통신

[서버 실습]
수업 실습 시간에 구현한 서버 코드를
클라이언트 OpenGL에 맞게 코드 수정

주의 사항)
용량 때문에 SFML package를 뺐으니 이를 설치해주세요
Release 모드에서 실행 주세요!
클라이언트/STRESS_TEST 접속 시 서버 주소인 127.0.0.1를 입력하고 들어가 주세요!

실습 세팅
 - multi_iocp 서버
 - 서버 보드 크기: 2000x2000
 - 클라이언트 전체 보드 13x13
 - VIEW_RANGE : 5 (11x11)
 - NPC: 200000 마리
 - lua ai 처리
   - chat
   - move

실습 기법
 - multi_iocp:			시야 처리 O / 타이머 multiThread 사용

<동작 처리>
- 왼쪽 이동
- 오른쪽 이동
- 위로 이동
- 아래로 이동

packet 프로토콜 정리

"protocol.h"로 통해 패킷 프로토콜 정의