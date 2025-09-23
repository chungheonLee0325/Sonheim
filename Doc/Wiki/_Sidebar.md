*   **[Home](Home)**

*   **주요 기술 시연 (For Busy Interviewer)**
    > 어느 페이지에서든 프로젝트의 핵심 기술 사례로 빠르게 이동할 수 있는 바로가기 링크입니다.
    *   [[사례 연구: 팰 포획 시퀀스|8.1_Case_Study_Pal_Capture_Sequence]]
    *   [[사례 연구: 서버 권위 제작|8.2_Case_Study_Server_Authority_Crafting]]
    *   [[사례 연구: 데이터 기반 근접 공격|8.3_Case_Study_Melee_Attack]]
    *   [[사례 연구: 반응형 인벤토리 UX|8.4_Case_Study_Inventory_Interaction]]

*   **1. 프로젝트 개요**
    > 프로젝트의 비전, 핵심 게임플레이 루프, 그리고 전체 아키텍처를 관통하는 설계 원칙을 소개합니다.
    *   [[1.1 프로젝트 비전 및 목표|1.1_Project_Overview]]
    *   [[1.2 주요 기능 요약|1.2_Key_Features]]

*   **2. 핵심 설계 원칙**
    > Sonheim의 아키텍처를 지탱하는 가장 근본적인 기술 철학을 다룹니다. 서버 권위, 데이터 주도, 컴포넌트 기반 설계, 그리고 언리얼의 표준 플레이어 아키텍처에 대해 설명합니다.
    *   [[2.1 서버 권위 원칙|2.1_Server_Authority_Architecture]]
    *   [[2.2 데이터 주도 설계|2.2_Data_Driven_Design]]
    *   [[2.3 컴포넌트 기반 설계|2.3_Component_Based_Design]]
    *   [[2.4 플레이어 클래스 아키텍처|2.4_Player_Class_Architecture]]

*   **3. 공용 프레임워크 (AAreaObject)**
    > 플레이어와 몬스터를 포함한 모든 살아있는 개체(AAreaObject)가 공유하는 공통 기능의 기반을 설명합니다.
    *   [[3.1 AAreaObject: 모든 개체의 기반|3.1_AreaObject_Framework]]
    *   [[3.2 어트리뷰트 시스템|3.2_Attribute_System]]
    *   [[3.3 스킬 시스템|3.3_Skill_Architecture]]
    *   [[3.4 애니메이션 시스템|3.4_Animation_System]]
    *   [[3.5 전투 및 피드백 시스템|3.5_Combat_and_Feedback_System]]

*   **4. 플레이어 (ASonheimPlayer)**
    > 게임의 주인공인 플레이어 캐릭터의 고유한 기능들을 심층적으로 다룹니다.
    *   [[4.1 플레이어 캐릭터 컨트롤|4.1_Player_Character_Control]]
    *   [[4.2 인벤토리 시스템|4.2_Inventory_System]]
    *   [[4.3 스탯 시스템|4.3_Stat_System]]
    *   [[4.4 팰 관리 시스템|4.4_Pal_Management_System]]

*   **5. AI (ABaseMonster)**
    > 살아있는 생명체처럼 행동하는 몬스터의 인공지능을 다룹니다.
    *   [[5.1 FSM 기반 AI 프레임워크|5.1_FSM_based_AI]]
    *   [[5.2 파트너 AI|5.2_Partner_AI]]

*   **6. 월드와 상호작용**
    > 플레이어가 월드에 존재하는 다양한 오브젝트들과 상호작용하는 방식을 설명합니다.
    *   [[6.1 통합 상호작용 원칙|6.1_Unified_Interaction_System]]
    *   [[6.2 아이템 시스템|6.2_Item_System]]
    *   [[6.3 자원 시스템|6.3_Resource_System]]
    *   [[6.4 보관함 시스템|6.4_Container_System]]
    *   [[6.5 제작 시스템|6.5_Crafting_System]]

*   **7. UI 시스템**
    > UI 시스템의 근간을 이루는 **설계 원칙**부터, 공통 문제를 해결하는 **솔루션**, 그리고 이 모든 것을 종합하여 완성한 **핵심 시스템 심층 분석**까지, 체계적인 접근 방식을 통해 UI 시스템을 구축한 과정을 설명합니다.
    *   **UI Architecture Principles**
        *   [[7.1 이벤트 기반 아키텍처|7.1_Event-Driven_UI_Architecture]]
        *   [[7.2 중앙화된 디자인 시스템|7.2_Centralized_UI_Design_System]]
    *   **Solving Common UI Problems**
        *   [[7.3 UI 최적화: 오브젝트 풀링|7.3_Optimizing_UI_with_Object_Pooling]]
        *   [[7.4 확장 가능한 컨텍스트 UI|7.4_Scalable_Contextual_UI]]
    *   **UI System Deep Dives**
        *   [[7.5 인벤토리 UI 심층 분석|7.5_Building_a_Reusable_Inventory_UI]]
        *   [[7.6 제작 UI 심층 분석|7.6_Designing_a_Collaborative_Crafting_UI]]

*   **8. 사례 연구**
    > 각 시스템들이 어떻게 유기적으로 협력하여 하나의 완전한 기능을 완성하는지 실제 사례를 통해 보여줍니다.
    *   [[8.1 팰 포획 시퀀스|8.1_Case_Study_Pal_Capture_Sequence]]
    *   [[8.2 서버 권위 제작|8.2_Case_Study_Server_Authority_Crafting]]
    *   [[8.3 데이터 기반 근접 공격|8.3_Case_Study_Melee_Attack]]
    *   [[8.4 반응형 인벤토리 UX|8.4_Case_Study_Inventory_Interaction]]

*   **9. 회고 및 향후 계획**
    > 프로젝트를 통해 얻은 기술적 교훈과 미래 발전 가능성을 다룹니다.
    *   [[9.1 프로젝트 회고|9.1_Project_Retrospective]]
    *   [[9.2 향후 작업 계획|9.2_Future_Work]]

*   **10. 부록**
    > 프로젝트의 진행 과정 및 기타 자료를 포함합니다.
    *   [[10.1 전체 개발 일지|10.1_Development_History]]