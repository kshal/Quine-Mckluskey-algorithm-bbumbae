/*
  ���α׷� ���� �帧 ����

  1. main()���� �����ؼ�
  2. parse_input()���� input_minterm.txt �о����
     - minterm�̶� don't care ���� �Ľ�
     - n_lit, n_minterm, n_dontCare, maxDigit ���� ����

  3. QM() ����
    - �ϴ� minterm�̶� don't care �׵��� lst[0]�� �ְ�
     - QM_one_column() �ݺ��ؼ� Prime Implicant ã��
     - QM_one_column() �ȿ�����:
       * QM_compare()�� �׵鳢�� ���� �������� Ȯ��
       * ���յ� �Ŵ� ���� ��(lst[c+1])�� �ְ�
       * ���� �ȵ� �Ŵ� prime_implicant �迭�� ����

  4. ��� ���
     ���� ������ �ʿ䰡 �����ѵ� �������ϱ�


     - Prime Implicant�鿡 ���ؼ�:
       * term_to_binstr()�� 2���� ���ڿ��� �ٲٰ�
       * term_cost()�� Ʈ�������� ��� ���
         sop�� ���ϴ� �ǵ�
           ����Ʈ ���� 2�� �̰�,

          
         AND ����Ʈ Ʈ�������� ���ͷ��� 1��
         OR ����Ʈ Ʈ��������, NOT ����Ʈ <-- �ʿ����



     - �� result.txt�� ����

  �� �ܿ� �ʿ��� �Լ���:
 * - bit_count(): ��Ʈ����ũ���� 1 ���� ����
 * - minterm�� Ŀ���ϴ��� Ȯ�ο�
 *   ++
 *
 *  �̰� �����ؼ� 10-- �� ������ �� ���� 1011�� Ŀ���ϴ��� 1010�� Ŀ���ϴ��� ���� ������ ������
    dontcare��� �����ϱ� �ߴµ�, ����� ����Ϸ��� ���� ������ ���� �ʿ䰡 ����.

    �ƴϸ� 10--�� ����� ���� ã�Ƽ� �Է� minterm�� �ߺ��Ǵ� ���� ã�� ����� �ִµ�, ��ȿ���� ������


 */

#define _CRT_SECURE_NO_WARNINGS  // Windows���� ���� ���� ��� ����
#include <stdio.h>  // ǥ�� ����� �Լ��� ���� ���
#include <stdlib.h>  // �޸� �Ҵ� �� ��ȯ �Լ��� ���� ���
#include <string.h>  // ���ڿ� ó�� �Լ��� ���� ���
#include <stdbool.h>  // boolean Ÿ���� ���� ���

#define MAX_LIT 4    //  �ִ� ���ͷ�(����) ����
#define MAX_TERM 16  // ������ �ִ� �� ����
#define MAX_PI 16   // ������ �ִ� Prime Implicant ����
#define MAX_COVERED_TERMS 16  // �ϳ��� ���� Ŀ���� �� �ִ� �ִ� minterm ����

typedef struct {
    int care;      // ���� ��(���� ��Ʈ ��)
    int dontCare;  // don't care ��Ʈ����ũ(don't care�� ��Ʈ ��ġ�� 1�� ǥ��)

    //�ֳĸ� minterm�� ( a , b ) �̷��� ǥ���ϴϱ� 
    // 10-- ���� ��� �� Ŀ���ϴ��� ���� ���������
    // �ȱ׷��� ����� �� ���ؼ� �ߺ��Ǵ� minterm ���ϰ� �ϴ� ����� �ִµ�, ��ͷ� �ؾ��ؼ� �� ����� ������ ����
    int covered_terms[MAX_COVERED_TERMS];  // �� ���� Ŀ���ϴ� minterm��
    int n_covered;  // Ŀ���ϴ� minterm ����
} Term;  // QM �˰��򿡼� ����� ��(term) ����ü

/*   //���� �ڵ� Ȱ��
*
     struct fileParseNode {
        char type;
        char* boolString;
        fileParseNode* pnext;
};



*/
//���� ������

char literal[MAX_LIT][8];  // ���� �̸� �迭 (��: A, B, C, D)
int minterm[MAX_TERM], dontCare[MAX_TERM];  // minterm�� don't care ���� ���� �����ϴ� �迭
int n_lit, n_minterm, n_dontCare, maxDigit;  // ���ͷ� ����, minterm ����, don't care ����, �ִ� ��Ʈ ��

Term lst[32][MAX_TERM];  // QM �˰����� �� �ܰ躰 �� ����Ʈ
int lst_cnt[32];  // �� �ܰ躰 ���� ����

bool used[32][MAX_TERM];  // �� ���� ���Ǿ����� ǥ���ϴ� �迭

Term prime_implicant[MAX_PI];  // Prime Implicant ���� �迭
int n_prime = 0;  // Prime Implicant ����


// ���ڿ� ���� 1�� ���� ���� �Լ�
int bit_count(int x) {
    int cnt = 0;  // 1�� ������ ������ ����
    while (x) {  // x�� 0�� �� ������ �ݺ�
        cnt += x & 1;  // ������ ��Ʈ�� 1�̸� ī��Ʈ ����
        x >>= 1;  // �������� 1��Ʈ ����Ʈ
    }
    return cnt;  // 1�� �� ���� ��ȯ
}

// QM ��: �� ���� ���Ͽ� �عְŸ��� 1��Ʈ �������� Ȯ���ϰ�, ���ο� dontCare ����ũ ����
bool QM_compare(Term a, Term b, int* newDontCare) {
    int calculed = a.care ^ b.care;  // �� ���� care ��Ʈ�� XOR�Ͽ� ���� ��Ʈ ���
    if (bit_count(calculed) == 1 && a.dontCare == b.dontCare) {  // ���̰� ��Ȯ�� 1��Ʈ�̰� '-'�� ��ġ�� ������ ��
        *newDontCare = calculed | a.dontCare;  // ���ο� dontCare�� ���� dontCare�� ���� ��Ʈ�� OR
        return true;  // ���� ����
    }
    return false;  // ���� �Ұ���
}

/*
 * QM_combine �����ϴ� �Լ�
 *
 * 1. ������ �ʿ��� �� ����
 *    - '-' ������ �����ؼ� ���� ��ġ�� '-'���� ���ϱ� ( ó���� 1�� ������ �׷�ȭ �Ѱ�ó��) ��¼�� '-'������ �׷�ȭ �ϰ� ������ �׷쳢�� ���ϴϱ�
 *
 *    - ��: 10--, 1-0-, 1--0 ������ ����
 *
 * 2. ���� ����
 *    - '-' ��ġ�� �ٸ��� ���� �Ұ���
 *    - ��: 10--�� 1-0-�� ���� ����
 *    - ���յ� �׵��� �� �̻� ��� ����
 *
 * 3. minterm Ŀ�� ����
 *    - ������ ������ Ŀ���ϴ� minterm�鵵 ���ľ� ��
 *    - ��: 101-�� 100-�� ���յǸ� 10--�� �ǰ�
 *      �̰� 1010, 1011, 1000, 1001�� Ŀ���ϰ� ��
 *
 *
 *    - �ߺ��� minterm�� �����ؾ� ��
 *
 * 4. Prime Implicant ����
 *    - ���� �ȵ� �׵鸸 ���� ( EPI )
 *    - �ߺ� üũ �ʿ�
 *
 *
 */

 // �� ��(column)���� �׵��� �����ϴ� �Լ�
void QM_combine(int c) {
    int lenc = lst_cnt[c];  // ���� ���� �� ����

    // dontCare ��Ʈ ���� �������� ����
    for (int i = 0; i < lenc - 1; i++) {
        for (int j = i + 1; j < lenc; j++) {
            if (bit_count(lst[c][i].dontCare) > bit_count(lst[c][j].dontCare)) {  // dontCare ��Ʈ ���� ���� ���� �ڷ� �̵�
                Term tmp = lst[c][i];  // �ӽ� ������ ����
                lst[c][i] = lst[c][j];  // ��ȯ
                lst[c][j] = tmp;  // ��ȯ
            }
        }
    }

    // �� ���� ���յǾ����� üũ�ϴ� �迭
    bool* TF = (bool*)calloc(MAX_TERM, sizeof(bool));  // ���� �Ҵ����� ����
    if (TF == NULL) {
        printf("�޸� �Ҵ� ����\n");
        return;
    }

    // ��� ���� ���� �ȵ� ���·� �ʱ�ȭ
    for (int i = 0; i < lenc && i < MAX_TERM; i++) {
        TF[i] = true;
    }

    // ��� �� ���� ���ϸ鼭 ���� �������� Ȯ��
    for (int i = 0; i < lenc && i < MAX_TERM; i++) {
        for (int j = i + 1; j < lenc && j < MAX_TERM; j++) {
            // '-' ��ġ�� �ٸ��� ���� �Ұ���
            if (lst[c][i].dontCare != lst[c][j].dontCare) continue;

            int newDontCare;  // ���ο� dontCare ����ũ�� ������ ����
            if (QM_compare(lst[c][i], lst[c][j], &newDontCare)) {  // �� ���� ���� �������� Ȯ��
                // ���յ� �׵��� �� �̻� ��� ����
                TF[i] = false;  // i��° ���� ���յ�
                TF[j] = false;  // j��° ���� ���յ�

                // ���ο� �� ����
                Term add_;
                add_.care = lst[c][i].care & (~newDontCare);  // care ��Ʈ ��� (dontCare ��Ʈ�� 0����)
                add_.dontCare = newDontCare;  // ���ο� dontCare ����ũ ����

                // Ŀ���ϴ� minterm���� ��ħ
                add_.n_covered = 0;
                // i��° ���� minterm�� �߰�
                for (int k = 0; k < lst[c][i].n_covered && k < MAX_COVERED_TERMS; k++) {
                    add_.covered_terms[add_.n_covered++] = lst[c][i].covered_terms[k];
                }
                // j��° ���� minterm�鵵 �߰� (�ߺ��� ����)
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
                    lst[c + 1][lst_cnt[c + 1]++] = add_;  // ���� ���� ���ο� �� �߰�
                }
            }
        }
    }

    // ���� �ȵ� �׵��� essential Prime Implicant�� ����
    for (int i = 0; i < lenc && i < MAX_TERM; i++) {
        if (TF[i]) {  // ���� �ȵ� ���̸�
            bool dup = false;  // �ߺ� üũ
            // �̹� ����� Prime Implicant�� ������ Ȯ��
            for (int j = 0; j < n_prime && j < MAX_PI; j++) {
                if (prime_implicant[j].care == lst[c][i].care && prime_implicant[j].dontCare == lst[c][i].dontCare) {
                    dup = true; break;  // �̹� ������ �ߺ����� ǥ��
                }
            }
            if (!dup && n_prime < MAX_PI) {
                prime_implicant[n_prime++] = lst[c][i];  // �ߺ� �ƴϸ� essential Prime Implicant�� �߰�
            }
        }
    }

    free(TF);  // ���� �Ҵ�� �޸� ����
}

// ���� 2���� ���ڿ��� ��ȯ�ϴ� �Լ�
void term_to_binstr(Term t, int n_bit, char* out) {
    if (out == NULL || n_bit <= 0 || n_bit > 16) return;  // �߸��� �Է� üũ

    for (int i = n_bit - 1; i >= 0; i--) {  // �ֻ��� ��Ʈ���� ó��
        if ((t.dontCare >> i) & 1)  // �ش� ��Ʈ�� don't care��
            *out++ = '-';  // '-' ���� ���
        else
            *out++ = ((t.care >> i) & 1) ? '1' : '0';  // care ��Ʈ ���� ���� '1' �Ǵ� '0' ���
    }
    *out = '\0';  // ���ڿ� ����
}

// Ʈ�������� ��� ��� �Լ� (AND ����Ʈ ���� + OR ����Ʈ ����)
int term_cost(Term t, int n_bit) {
    int cost = 0;  // ��� �ʱ�ȭ
    // AND ����Ʈ ����: don't care�� �ƴ� ��Ʈ���� 2���� Ʈ��������
    for (int i = 0; i < n_bit; i++) {
        if (!((t.dontCare >> i) & 1)) cost += 1;  // don't care�� �ƴ� ��Ʈ���� Ʈ�������� 1����
    }
    // OR ����Ʈ ����: ���� �ʿ����,

    return cost;  // �� ��� ��ȯ
}

// �Է� ���� �Ľ��ϴ� �Լ�
void parse_input() {
    FILE* fp = fopen("input_minterm.txt", "r");  // �Է� ���� ����
    if (fp == NULL) {
        printf("input_minterm.txt ERROR .\n");
        exit(-1);
    }

    char buf[256] = { 0 };  // �Է� ���� �ʱ�ȭ
    // ù ��: ��Ʈ ��
    if (fgets(buf, sizeof(buf), fp) == NULL) {
        printf("���� �б� ����\n");
        fclose(fp);
        exit(-1);
    }

    n_lit = atoi(buf);  // ���ͷ� ���� ����
    if (n_lit <= 0 || n_lit > MAX_LIT) {
        printf("�߸��� ���ͷ� ����: %d\n", n_lit);
        fclose(fp);
        exit(-1);
    }

    maxDigit = n_lit;  // �ִ� ��Ʈ �� ����
    n_minterm = n_dontCare = 0;  // minterm�� don't care ���� ����

    while (fgets(buf, sizeof(buf), fp) && n_minterm < MAX_TERM && n_dontCare < MAX_TERM) {
        char type;  // Ÿ�� ���� (m �Ǵ� d)
        char bits[16] = { 0 };  // ��Ʈ ���ڿ� �ʱ�ȭ

        if (sscanf(buf, " %c %s", &type, bits) == 2) {  // Ÿ�԰� ��Ʈ ���ڿ� �Ľ�
            int val = 0;  // 10���� �� �ʱ�ȭ
            for (int i = 0; bits[i] && i < 16; i++) {
                if (bits[i] != '0' && bits[i] != '1') {
                    printf("�߸��� ��Ʈ ��: %c\n", bits[i]);
                    continue;
                }
                val = (val << 1) | (bits[i] - '0');  // 2���� ���ڿ��� 10������ ��ȯ
            }
            if (type == 'm' && n_minterm < MAX_TERM) minterm[n_minterm++] = val;
            else if (type == 'd' && n_dontCare < MAX_TERM) dontCare[n_dontCare++] = val;
        }
    }
    fclose(fp);  // ���� �ݱ�
}

// QM �˰��� ���� �Լ�
void QM() {
    memset(lst, 0, sizeof(lst));  // lst �迭 �ʱ�ȭ
    memset(lst_cnt, 0, sizeof(lst_cnt));  // lst_cnt �迭 �ʱ�ȭ
    n_prime = 0;  // Prime Implicant ���� �ʱ�ȭ

    // minterm�� ù ���� �߰�
    for (int i = 0; i < n_minterm; i++) {
        Term t = { minterm[i], 0, {minterm[i]}, 1 };  // �ڱ� �ڽŸ� Ŀ��
        lst[0][lst_cnt[0]++] = t;
    }

    // don't care�� ù ���� �߰�
    for (int i = 0; i < n_dontCare; i++) {
        Term t = { dontCare[i], 0, {dontCare[i]}, 1 };  // �ڱ� �ڽŸ� Ŀ��
        lst[0][lst_cnt[0]++] = t;
    }

    int i = 0;  // �� �ε���
    while (lst_cnt[i]) {  // ���� ���� ���� ������ ��� ����
        QM_combine(i);  // ���� ���� �׵��� ����
        i++;  // ���� ���� �̵�
    }
}



int main() {
    parse_input();  // �Է� ���� �Ľ�
    QM();  // QM �˰��� ����

    FILE* fout = fopen("result_QM.txt", "w");  // ��� ���� ����
    if (fout == NULL) {
        printf("result_QM.txt ERROR.\n");
        return -1;
    }

    int total_cost = 0;  // �� Ʈ�������� ���

    // ++++++ Ŀ���ϴ� minterm�� ���, �ȱ׷� ��� ������ minterm�� ��µȴ�.

    bool is_minterm_covered[MAX_TERM] = { false };  // �� minterm�� Ŀ���Ǿ����� üũ

    // �� Prime Implicant�� ����
    for (int i = 0; i < n_prime && i < MAX_PI; i++) {
        bool covers_minterm = false;  // �� ���� minterm�� Ŀ���ϴ��� üũ��

        // �� ���� Ŀ���ϴ� ��� term���� Ȯ��
        for (int j = 0; j < prime_implicant[i].n_covered; j++) {
            int covered_term = prime_implicant[i].covered_terms[j];

            // �� term�� minterm���� Ȯ��
            for (int k = 0; k < n_minterm; k++) {
                if (covered_term == minterm[k]) {
                    covers_minterm = true;
                    is_minterm_covered[k] = true;  // �� minterm�� Ŀ���Ǿ����� ǥ�ÿ�
                    break;
                }
            }
        }

        // minterm�� Ŀ���ϴ� �׸� ���
        if (covers_minterm) {
            char binstr[16] = { 0 };  // ���ڿ� �ʱ�ȭ , �迭�� �ջ�ȴٴ� ���� ������ 

            term_to_binstr(prime_implicant[i], maxDigit, binstr);  // ���� 2���� ���ڿ��� ��ȯ
            fprintf(fout, "%s\n", binstr);  // ��� ���Ͽ� ���
            total_cost += term_cost(prime_implicant[i], maxDigit);  // Ʈ�������� ��� ����
        }
    }



    fprintf(fout, "Cost (# of transistors): %d (SOP)\n", total_cost);  // �� ��� ���
    fclose(fout);  // ���� �ݱ�
    return 0;  // ���α׷� ����
}