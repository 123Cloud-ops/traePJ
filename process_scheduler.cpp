/*
 * 操作系统实验：进程调度模拟
 * 优先数调度算法 + 时间片轮转调度算法（时间片单位=2）
 */

#include <cstdio>
#include <cstring>
#include <cstdlib> /* calloc */

#ifdef _WIN32
#include <windows.h>
#endif

/* 读一行（去掉末尾换行），失败返回 0 */
static int readLine(char *buf, int size) {
    if (!fgets(buf, size, stdin)) return 0;
    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        buf[--len] = '\0';
    }
    return (len > 0 || feof(stdin) == 0) ? 1 : 0;
}

static void waitBeforeExit(void) {
    printf("\n按回车键退出...");
    fflush(stdout);
    (void)getchar();
}

enum ProcState { ST_READY, ST_RUN, ST_FINISH };

struct PCB {
    char name[32];
    int prio;       /* 优先数算法：动态优先数；轮转法下可存常数2用于显示 */
    int cputime;
    int needtime;
    ProcState state;
    PCB *next;
};

static PCB *runPtr = nullptr;
static PCB *readyHead = nullptr;
static PCB *readyTail = nullptr;
static PCB *finishHead = nullptr;
static PCB *finishTail = nullptr;

static PCB *allProcesses[8];
static int processCount = 0;

static void appendFinish(PCB *p) {
    p->state = ST_FINISH;
    p->next = nullptr;
    if (!finishHead) {
        finishHead = finishTail = p;
    } else {
        finishTail->next = p;
        finishTail = p;
    }
}

/* (1) 优先数算法：按优先数从高到低插入就绪队列（队首优先数最大） */
static void INSERT1(PCB *p) {
    p->state = ST_READY;
    p->next = nullptr;

    if (!readyHead) {
        readyHead = readyTail = p;
        return;
    }

    if (p->prio > readyHead->prio) {
        p->next = readyHead;
        readyHead = p;
        return;
    }

    PCB *cur = readyHead;
    while (cur->next && cur->next->prio >= p->prio)
        cur = cur->next;

    p->next = cur->next;
    cur->next = p;
    if (!p->next) readyTail = p;
}

/* (2) 轮转法：未完成进程插到就绪队尾 */
static void INSERT2(PCB *p) {
    p->state = ST_READY;
    p->next = nullptr;
    if (!readyHead) {
        readyHead = readyTail = p;
        return;
    }
    readyTail->next = p;
    readyTail = p;
}

/* (3) 就绪队列第一个进程投入运行 */
static void FIRSTIN() {
    if (runPtr || !readyHead) return;

    PCB *p = readyHead;
    readyHead = readyHead->next;
    if (!readyHead) readyTail = nullptr;

    p->next = nullptr;
    p->state = ST_RUN;
    runPtr = p;
}

static char stateChar(ProcState s) {
    if (s == ST_RUN) return 'R';
    if (s == ST_FINISH) return 'F';
    return 'W';
}

/* (4) 显示每执行一次后所有进程状态：先 R，再 W（就绪队列链序），再 F（完成队列链序） */
static void PRINT(void) {
    int i;
    PCB *rlist[8];
    PCB *wlist[8];
    PCB *flist[8];
    int nr = 0, nw = 0, nf = 0;

    for (i = 0; i < processCount; i++) {
        PCB *p = allProcesses[i];
        if (p->state == ST_RUN) rlist[nr++] = p;
    }

    /* 就绪态按就绪队列链表顺序：优先数算法中为优先数序，轮转法中为轮转序 */
    for (PCB *q = readyHead; q; q = q->next)
        wlist[nw++] = q;

    for (PCB *f = finishHead; f; f = f->next)
        flist[nf++] = f;

    printf("name\tcputime\tneedtime\tpriority\tstate\n");
    for (i = 0; i < nr; i++) {
        PCB *p = rlist[i];
        printf("%s\t%d\t%d\t%d\t\t%c\n", p->name, p->cputime, p->needtime, p->prio, stateChar(p->state));
    }
    for (i = 0; i < nw; i++) {
        PCB *p = wlist[i];
        printf("%s\t%d\t%d\t%d\t\t%c\n", p->name, p->cputime, p->needtime, p->prio, stateChar(p->state));
    }
    for (i = 0; i < nf; i++) {
        PCB *p = flist[i];
        printf("%s\t%d\t%d\t%d\t\t%c\n", p->name, p->cputime, p->needtime, p->prio, stateChar(p->state));
    }
    printf("\n");
}

/* (5) 创建进程并插入就绪队列 */
static void CREATE(const char *name, int need, bool priorityAlgo) {
    PCB *p = (PCB *)calloc(1, sizeof(PCB));
    strncpy(p->name, name, sizeof(p->name) - 1);
    p->needtime = need;
    p->cputime = 0;
    p->state = ST_READY;
    p->next = nullptr;

    if (priorityAlgo) {
        p->prio = 50 - need;
        INSERT1(p);
    } else {
        p->prio = 2; /* ROUND 常数，用于显示 priority 列 */
        INSERT2(p);
    }
    allProcesses[processCount++] = p;
}

static bool allFinished() {
    for (int i = 0; i < processCount; i++)
        if (allProcesses[i]->state != ST_FINISH) return false;
    return true;
}

/* (6) 按优先数算法调度 */
static void PRISCH() {
    printf("========== 优先数调度算法 ==========\n\n");
    while (!allFinished()) {
        FIRSTIN();
        if (!runPtr) break;

        PCB *p = runPtr;
        p->prio -= 1;
        p->cputime += 1;
        p->needtime -= 1;

        PRINT();

        runPtr = nullptr;
        if (p->needtime <= 0) {
            p->needtime = 0;
            appendFinish(p);
        } else {
            INSERT1(p);
        }
    }
}

/* (7) 按时间片轮转法调度 */
static void ROUNDSCH() {
    const int quantum = 2;
    printf("========== 时间片轮转调度（每单位=%d 个时间片）==========\n\n", quantum);

    while (!allFinished()) {
        FIRSTIN();
        if (!runPtr) break;

        PCB *p = runPtr;
        int slice = quantum;
        if (p->needtime < slice) slice = p->needtime;

        p->cputime += slice;
        p->needtime -= slice;

        PRINT();

        runPtr = nullptr;
        if (p->needtime <= 0) {
            p->needtime = 0;
            appendFinish(p);
        } else {
            INSERT2(p);
        }
    }
}

static void resetSimulation() {
    runPtr = nullptr;
    readyHead = readyTail = nullptr;
    finishHead = finishTail = nullptr;
    for (int i = 0; i < processCount; i++) {
        free(allProcesses[i]);
        allProcesses[i] = nullptr;
    }
    processCount = 0;
}

int main() {
    int algo;
    char line[128];
    char name[32];
    int need;
    const int N = 5;

#ifdef _WIN32
    /* 避免中文提示乱码；若终端仍乱码可改为 936 */
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    printf("进程调度模拟实验\n");
    printf("请选择算法: 1=优先数调度  2=时间片轮转调度\n");
    printf("请输入数字后回车: ");
    fflush(stdout);
    if (!readLine(line, sizeof(line)) || sscanf(line, "%d", &algo) != 1) {
        printf("算法输入错误，请输入 1 或 2（半角数字）。\n");
        waitBeforeExit();
        return 1;
    }

    bool priorityAlgo = (algo == 1);
    if (algo != 1 && algo != 2) {
        printf("无效选择，默认使用优先数调度。\n");
        priorityAlgo = true;
    }

    printf("\n请依次输入 %d 个进程，每行格式: 进程名 空格 NEEDTIME\n", N);
    printf("示例一行: P1 3  （不要只输入进程名，数字必须写在同一行）\n\n");
    for (int i = 0; i < N; i++) {
        printf("进程 %d: ", i + 1);
        fflush(stdout);
        if (!readLine(line, sizeof(line))) {
            printf("读取失败，已输入 %d 个进程。\n", i);
            waitBeforeExit();
            return 1;
        }
        if (sscanf(line, "%31s %d", name, &need) != 2) {
            printf("第 %d 个进程格式错误，收到: [%s]\n", i + 1, line);
            printf("正确格式: 进程名 还需时间片数，例如 P1 3\n");
            waitBeforeExit();
            return 1;
        }
        if (need < 0) need = 0;
        CREATE(name, need, priorityAlgo);
    }

    printf("\n初始就绪队列已建立，开始调度...\n\n");
    fflush(stdout);

    if (priorityAlgo)
        PRISCH();
    else
        ROUNDSCH();

    printf("全部进程已完成。\n");
    resetSimulation();
    waitBeforeExit();
    return 0;
}
