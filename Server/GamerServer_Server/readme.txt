
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
