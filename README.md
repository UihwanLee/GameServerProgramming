# GameServerProgramming
 TUK 게임서버프로그래밍
 
 ## 게임 이름 
 > MMO RPG 나만의 레벨업!

![썸네일](https://github.com/UihwanLee/GameServerProgramming/assets/36596037/02fa8333-345a-4987-9d58-5a390c2680df)

* [유튜브영상](https://youtu.be/RZBfah17T7Y?si=I8cxQ43HX7PpZC8S)

## 개발 컨셉

 * high_concept: 몬스터를 처치하여 성장하는 MMORPG 게임
 * 핵심 기술스택: iocp를 이용한 서버. SFML을 이용한 클라이언트 개발

  
## 개발 주요 요소 

* SQL DB 연동
  - SQL DB를 이용하여 게임 데이터 연동. Stored Procedure을 이용한 id 체크, 플레이어 위치 업데이트, 플레이어 상태 업데이트 구현
* 충돌처리
  - 서버 map 정보를 이용하여 플레이 이동 시 벽과의 충돌체크를 통한 충돌처리 구현. 클라이언트에서도 사전 충돌처리를 통한 빠른 통신 구현
* 전투시스템
  - 몬스터 공격 시 피격 효과음, 데미지 이펙트, 데미지 로그 구현. 몬스터 처치 시 경험치 흭득 및 레벨 시스템 구현
* 파티 시스템
  - 주변 플레이어에게 파티 요청을 통해 상대방 플레이어가 수락 시 파티가 되는 시스템 구현. 파티가 된 플레이어들끼리 경험치 공유.


