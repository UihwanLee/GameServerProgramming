
<게임서버프로그래밍>
2018156032 이의환

서버에서 관리하는 데이터 및 목록

[클라이언트 player pos]

Figure::Board[] : 8x8 Board에서 말의 위치를 정보를 glm::vec3 형태로 갖고 있는 배열

move_packet : 말의 위치 정보와 현재 인덱스 정보를 갖고 있는 구조체
 -	short size		: 패킷 사이즈
 -	char  type		: 패킷 용도
 -	int   idx		: Borad에서 현재 player idx
 -	glm::vec3 pos	: 변경될 player pos 데이터

<동작 처리>
- 왼쪽 이동
- 오른쪽 이동
- 위로 이동
- 아래로 이동


packet 프로토콜 정리

[packet의 type 값으로 행동 처리]
type 0:			자신의 플레이어 생성 및 기존에 존재하는 플레이어 생성
type 1~4:		자신의 플레이어 말 움직이기
type 5:			다른 플레이어 말 생성