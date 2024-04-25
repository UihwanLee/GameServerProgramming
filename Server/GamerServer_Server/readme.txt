
<게임서버프로그래밍>
2018156032 이의환

Client - OpenGL / Server - SFML socket 통신

[서버 실습]
수업 실습 시간에 구현한 서버 코드를
클라이언트 OpenGL에 맞게 코드 수정

주의 사항)
용량 때문에 SFML package를 뺐으니 이를 설치해주세요
Release 모드에서 실행 주세요!

5개의 서버를 구현한다.
 - non_blocking 서버
 - overlapped callback 서버
 - multi_thread_I/O 서버
 - single_iocp 서버
 - multi_iocp 서버

<동작 처리>
- 왼쪽 이동
- 오른쪽 이동
- 위로 이동
- 아래로 이동

packet 프로토콜 정리

"protocol.h"로 통해 패킷 프로토콜 정의