/*
  프로그램 실행 흐름 정리

  1. main()에서 시작해서
  2. parse_input()으로 input_minterm.txt 읽어오기
     - minterm이랑 don't care 값들 파싱
     - n_lit, n_minterm, n_dontCare, maxDigit 값들 설정

  3. QM() 실행
    - 일단 minterm이랑 don't care 항들을 lst[0]에 넣고
     - QM_one_column() 반복해서 Prime Implicant 찾기
     - QM_one_column() 안에서는:
       * QM_compare()로 항들끼리 결합 가능한지 확인
       * 결합된 거는 다음 열(lst[c+1])에 넣고
       * 결합 안된 거는 prime_implicant 배열에 저장

  4. 결과 출력
     딱히 구현할 필요가 없긴한데 멋있으니까


     - Prime Implicant들에 대해서:
       * term_to_binstr()로 2진수 문자열로 바꾸고
       * term_cost()로 트랜지스터 비용 계산
         sop로 구하는 건데
           게이트 레벨 2개 이고,

          
         AND 게이트 트랜지스터 리터럴당 1개
         OR 게이트 트랜지스터, NOT 게이트 <-- 필요없음



     - 다 result.txt에 저장

  그 외에 필요한 함수들:
 * - bit_count(): 비트마스크에서 1 개수 세기
 * - minterm을 커버하는지 확인용
 *   ++
 *
 *  이게 결합해서 10-- 이 나오면 이 항이 1011을 커버하는지 1010을 커버하는지 몰라도 괜찮지 않을까
    dontcare라고 생각하긴 했는데, 결과로 출력하려면 따로 저장해 놓을 필요가 있음.

    아니면 10--의 경우의 수를 찾아서 입력 minterm과 중복되는 항을 찾는 방법도 있는데, 비효율적 같은데


 */

#define _CRT_SECURE_NO_WARNINGS  // Windows에서 보안 관련 경고 무시
#include <stdio.h>  // 표준 입출력 함수를 위한 헤더
#include <stdlib.h>  // 메모리 할당 및 변환 함수를 위한 헤더
#include <string.h>  // 문자열 처리 함수를 위한 헤더
#include <stdbool.h>  // boolean 타입을 위한 헤더

#define MAX_LIT 4    //  최대 리터럴(변수) 개수
#define MAX_TERM 16  // 가능한 최대 항 개수
#define MAX_PI 16   // 가능한 최대 Prime Implicant 개수
#define MAX_COVERED_TERMS 16  // 하나의 항이 커버할 수 있는 최대 minterm 개수

typedef struct {
    int care;      // 실제 값(항의 비트 값)
    int dontCare;  // don't care 비트마스크(don't care인 비트 위치는 1로 표시)

    //왜냐면 minterm을 ( a , b ) 이렇게 표현하니까 
    // 10-- 같은 경우 뭔 커버하는지 따로 적어줘야함
    // 안그러면 경우의 수 구해서 중복되는 minterm 구하게 하는 방법이 있는데, 재귀로 해야해서 이 방식이 유용해 보임
    int covered_terms[MAX_COVERED_TERMS];  // 이 항이 커버하는 minterm들
    int n_covered;  // 커버하는 minterm 개수
} Term;  // QM 알고리즘에서 사용할 항(term) 구조체

/*   //예제 코드 활용
*
     struct fileParseNode {
        char type;
        char* boolString;
        fileParseNode* pnext;
};



*/
//전역 변수들

char literal[MAX_LIT][8];  // 변수 이름 배열 (예: A, B, C, D)
int minterm[MAX_TERM], dontCare[MAX_TERM];  // minterm과 don't care 항의 값을 저장하는 배열
int n_lit, n_minterm, n_dontCare, maxDigit;  // 리터럴 개수, minterm 개수, don't care 개수, 최대 비트 수

Term lst[32][MAX_TERM];  // QM 알고리즘의 각 단계별 항 리스트
int lst_cnt[32];  // 각 단계별 항의 개수

bool used[32][MAX_TERM];  // 각 항이 사용되었는지 표시하는 배열

Term prime_implicant[MAX_PI];  // Prime Implicant 저장 배열
int n_prime = 0;  // Prime Implicant 개수


// 문자열 에서 1의 개수 세는 함수
int bit_count(int x) {
    int cnt = 0;  // 1의 개수를 저장할 변수
    while (x) {  // x가 0이 될 때까지 반복
        cnt += x & 1;  // 최하위 비트가 1이면 카운트 증가
        x >>= 1;  // 우측으로 1비트 시프트
    }
    return cnt;  // 1의 총 개수 반환
}

// QM 비교: 두 항을 비교하여 해밍거리가 1비트 차이인지 확인하고, 새로운 dontCare 마스크 생성
bool QM_compare(Term a, Term b, int* newDontCare) {
    int calculed = a.care ^ b.care;  // 두 항의 care 비트를 XOR하여 차이 비트 계산
    if (bit_count(calculed) == 1 && a.dontCare == b.dontCare) {  // 차이가 정확히 1비트이고 '-'의 위치가 동일할 때
        *newDontCare = calculed | a.dontCare;  // 새로운 dontCare는 기존 dontCare와 차이 비트의 OR
        return true;  // 결합 가능
    }
    return false;  // 결합 불가능
}

/*
 * QM_combine 결합하는 함수
 *
 * 1. 정렬이 필요할 것 같음
 *    - '-' 개수로 정렬해서 같은 위치의 '-'끼리 비교하기 ( 처음에 1의 개수로 그룹화 한것처럼) 어쩌피 '-'개수로 그룹화 하고 인접한 그룹끼리 비교하니까
 *
 *    - 예: 10--, 1-0-, 1--0 순서로 정렬
 *
 * 2. 결합 과정
 *    - '-' 위치가 다르면 결합 불가능
 *    - 예: 10--와 1-0-는 결합 못함
 *    - 결합된 항들은 더 이상 사용 안함
 *
 * 3. minterm 커버 정보
 *    - 결합할 때마다 커버하는 minterm들도 합쳐야 함
 *    - 예: 101-와 100-이 결합되면 10--가 되고
 *      이건 1010, 1011, 1000, 1001을 커버하게 됨
 *
 *
 *    - 중복된 minterm은 제외해야 함
 *
 * 4. Prime Implicant 저장
 *    - 결합 안된 항들만 저장 ( EPI )
 *    - 중복 체크 필요
 *
 *
 */

 // 한 열(column)에서 항들을 결합하는 함수
void QM_combine(int c) {
    int lenc = lst_cnt[c];  // 현재 열의 항 개수

    // dontCare 비트 개수 기준으로 정렬
    for (int i = 0; i < lenc - 1; i++) {
        for (int j = i + 1; j < lenc; j++) {
            if (bit_count(lst[c][i].dontCare) > bit_count(lst[c][j].dontCare)) {  // dontCare 비트 수가 많은 항을 뒤로 이동
                Term tmp = lst[c][i];  // 임시 변수에 저장
                lst[c][i] = lst[c][j];  // 교환
                lst[c][j] = tmp;  // 교환
            }
        }
    }

    // 각 항이 결합되었는지 체크하는 배열
    bool* TF = (bool*)calloc(MAX_TERM, sizeof(bool));  // 동적 할당으로 변경
    if (TF == NULL) {
        printf("메모리 할당 실패\n");
        return;
    }

    // 모든 항을 결합 안된 상태로 초기화
    for (int i = 0; i < lenc && i < MAX_TERM; i++) {
        TF[i] = true;
    }

    // 모든 항 쌍을 비교하면서 결합 가능한지 확인
    for (int i = 0; i < lenc && i < MAX_TERM; i++) {
        for (int j = i + 1; j < lenc && j < MAX_TERM; j++) {
            // '-' 위치가 다르면 결합 불가능
            if (lst[c][i].dontCare != lst[c][j].dontCare) continue;

            int newDontCare;  // 새로운 dontCare 마스크를 저장할 변수
            if (QM_compare(lst[c][i], lst[c][j], &newDontCare)) {  // 두 항이 결합 가능한지 확인
                // 결합된 항들은 더 이상 사용 안함
                TF[i] = false;  // i번째 항이 결합됨
                TF[j] = false;  // j번째 항이 결합됨

                // 새로운 항 생성
                Term add_;
                add_.care = lst[c][i].care & (~newDontCare);  // care 비트 계산 (dontCare 비트는 0으로)
                add_.dontCare = newDontCare;  // 새로운 dontCare 마스크 설정

                // 커버하는 minterm들을 합침
                add_.n_covered = 0;
                // i번째 항의 minterm들 추가
                for (int k = 0; k < lst[c][i].n_covered && k < MAX_COVERED_TERMS; k++) {
                    add_.covered_terms[add_.n_covered++] = lst[c][i].covered_terms[k];
                }
                // j번째 항의 minterm들도 추가 (중복은 제외)
                for (int k = 0; k < lst[c][j].n_covered && k < MAX_COVERED_TERMS; k++) {
                    bool is_duplicate = false;
                    for (int m = 0; m < add_.n_covered && m < MAX_COVERED_TERMS; m++) {
                        if (add_.covered_terms[m] == lst[c][j].covered_terms[k]) {
                            is_duplicate = true;
                            break;
                        }
                    }
                    if (!is_duplicate && add_.n_covered < MAX_COVERED_TERMS) {
                        add_.covered_terms[add_.n_covered++] = lst[c][j].covered_terms[k];
                    }
                }

                if (lst_cnt[c + 1] < MAX_TERM) {
                    lst[c + 1][lst_cnt[c + 1]++] = add_;  // 다음 열에 새로운 항 추가
                }
            }
        }
    }

    // 결합 안된 항들은 essential Prime Implicant로 저장
    for (int i = 0; i < lenc && i < MAX_TERM; i++) {
        if (TF[i]) {  // 결합 안된 항이면
            bool dup = false;  // 중복 체크
            // 이미 저장된 Prime Implicant랑 같은지 확인
            for (int j = 0; j < n_prime && j < MAX_PI; j++) {
                if (prime_implicant[j].care == lst[c][i].care && prime_implicant[j].dontCare == lst[c][i].dontCare) {
                    dup = true; break;  // 이미 있으면 중복으로 표시
                }
            }
            if (!dup && n_prime < MAX_PI) {
                prime_implicant[n_prime++] = lst[c][i];  // 중복 아니면 essential Prime Implicant로 추가
            }
        }
    }

    free(TF);  // 동적 할당된 메모리 해제
}

// 항을 2진수 문자열로 변환하는 함수
void term_to_binstr(Term t, int n_bit, char* out) {
    if (out == NULL || n_bit <= 0 || n_bit > 16) return;  // 잘못된 입력 체크

    for (int i = n_bit - 1; i >= 0; i--) {  // 최상위 비트부터 처리
        if ((t.dontCare >> i) & 1)  // 해당 비트가 don't care면
            *out++ = '-';  // '-' 문자 출력
        else
            *out++ = ((t.care >> i) & 1) ? '1' : '0';  // care 비트 값에 따라 '1' 또는 '0' 출력
    }
    *out = '\0';  // 문자열 종료
}

// 트랜지스터 비용 계산 함수 (AND 게이트 레벨 + OR 게이트 레벨)
int term_cost(Term t, int n_bit) {
    int cost = 0;  // 비용 초기화
    // AND 게이트 레벨: don't care가 아닌 비트마다 2개의 트랜지스터
    for (int i = 0; i < n_bit; i++) {
        if (!((t.dontCare >> i) & 1)) cost += 1;  // don't care가 아닌 비트마다 트랜지스터 1개씩
    }
    // OR 게이트 레벨: 구할 필요없음,

    return cost;  // 총 비용 반환
}

// 입력 파일 파싱하는 함수
void parse_input() {
    FILE* fp = fopen("input_minterm.txt", "r");  // 입력 파일 열기
    if (fp == NULL) {
        printf("input_minterm.txt ERROR .\n");
        exit(-1);
    }

    char buf[256] = { 0 };  // 입력 버퍼 초기화
    // 첫 줄: 비트 수
    if (fgets(buf, sizeof(buf), fp) == NULL) {
        printf("파일 읽기 오류\n");
        fclose(fp);
        exit(-1);
    }

    n_lit = atoi(buf);  // 리터럴 개수 설정
    if (n_lit <= 0 || n_lit > MAX_LIT) {
        printf("잘못된 리터럴 개수: %d\n", n_lit);
        fclose(fp);
        exit(-1);
    }

    maxDigit = n_lit;  // 최대 비트 수 설정
    n_minterm = n_dontCare = 0;  // minterm과 don't care 개수 리셋

    while (fgets(buf, sizeof(buf), fp) && n_minterm < MAX_TERM && n_dontCare < MAX_TERM) {
        char type;  // 타입 문자 (m 또는 d)
        char bits[16] = { 0 };  // 비트 문자열 초기화

        if (sscanf(buf, " %c %s", &type, bits) == 2) {  // 타입과 비트 문자열 파싱
            int val = 0;  // 10진수 값 초기화
            for (int i = 0; bits[i] && i < 16; i++) {
                if (bits[i] != '0' && bits[i] != '1') {
                    printf("잘못된 비트 값: %c\n", bits[i]);
                    continue;
                }
                val = (val << 1) | (bits[i] - '0');  // 2진수 문자열을 10진수로 변환
            }
            if (type == 'm' && n_minterm < MAX_TERM) minterm[n_minterm++] = val;
            else if (type == 'd' && n_dontCare < MAX_TERM) dontCare[n_dontCare++] = val;
        }
    }
    fclose(fp);  // 파일 닫기
}

// QM 알고리즘 메인 함수
void QM() {
    memset(lst, 0, sizeof(lst));  // lst 배열 초기화
    memset(lst_cnt, 0, sizeof(lst_cnt));  // lst_cnt 배열 초기화
    n_prime = 0;  // Prime Implicant 개수 초기화

    // minterm을 첫 열에 추가
    for (int i = 0; i < n_minterm; i++) {
        Term t = { minterm[i], 0, {minterm[i]}, 1 };  // 자기 자신만 커버
        lst[0][lst_cnt[0]++] = t;
    }

    // don't care를 첫 열에 추가
    for (int i = 0; i < n_dontCare; i++) {
        Term t = { dontCare[i], 0, {dontCare[i]}, 1 };  // 자기 자신만 커버
        lst[0][lst_cnt[0]++] = t;
    }

    int i = 0;  // 열 인덱스
    while (lst_cnt[i]) {  // 현재 열에 항이 있으면 계속 진행
        QM_combine(i);  // 현재 열의 항들을 결합
        i++;  // 다음 열로 이동
    }
}



int main() {
    parse_input();  // 입력 파일 파싱
    QM();  // QM 알고리즘 실행

    FILE* fout = fopen("result_QM.txt", "w");  // 결과 파일 열기
    if (fout == NULL) {
        printf("result_QM.txt ERROR.\n");
        return -1;
    }

    int total_cost = 0;  // 총 트랜지스터 비용

    // ++++++ 커버하는 minterm만 출력, 안그럼 모든 결합한 minterm이 출력된다.

    bool is_minterm_covered[MAX_TERM] = { false };  // 각 minterm이 커버되었는지 체크

    // 각 Prime Implicant에 대해
    for (int i = 0; i < n_prime && i < MAX_PI; i++) {
        bool covers_minterm = false;  // 이 항이 minterm을 커버하는지 체크용

        // 이 항이 커버하는 모든 term들을 확인
        for (int j = 0; j < prime_implicant[i].n_covered; j++) {
            int covered_term = prime_implicant[i].covered_terms[j];

            // 이 term이 minterm인지 확인
            for (int k = 0; k < n_minterm; k++) {
                if (covered_term == minterm[k]) {
                    covers_minterm = true;
                    is_minterm_covered[k] = true;  // 이 minterm이 커버되었음을 표시용
                    break;
                }
            }
        }

        // minterm을 커버하는 항만 출력
        if (covers_minterm) {
            char binstr[16] = { 0 };  // 문자열 초기화 , 배열이 손상된다는 에러 방지용 

            term_to_binstr(prime_implicant[i], maxDigit, binstr);  // 항을 2진수 문자열로 변환
            fprintf(fout, "%s\n", binstr);  // 결과 파일에 출력
            total_cost += term_cost(prime_implicant[i], maxDigit);  // 트랜지스터 비용 누적
        }
    }



    fprintf(fout, "Cost (# of transistors): %d (SOP)\n", total_cost);  // 총 비용 출력
    fclose(fout);  // 파일 닫기
    return 0;  // 프로그램 종료
}