#include <iostream>
#include <string>
#include <iomanip>

using namespace std;

// 进程状态常量
const char STATE_RUN = 'R';    // 执行态
const char STATE_READY = 'W';  // 就绪态（等待）
const char STATE_FINISH = 'F'; // 完成态

// 时间片单位（轮转法固定为2）
const int TIME_SLICE = 2;

// 进程控制块 PCB
struct PCB {
    string name;        // 进程名
    int priority;       // 优先级（优先数算法使用）或时间片数（轮转法显示用）
    int cputime;        // 已占用CPU时间片数
    int needtime;       // 完成还需时间片数
    char state;         // 进程状态
    PCB* next;          // 链表指针

    PCB(string n, int need, int prio) {
        name = n;
        priority = prio;
        cputime = 0;
        needtime = need;
        state = STATE_READY;
        next = nullptr;
    }
};

// 全局指针
PCB* RUN = nullptr;           // 当前运行进程
PCB* READY = nullptr;         // 就绪队列头
PCB* TAIL = nullptr;          // 就绪队列尾
PCB* FINISH = nullptr;        // 完成队列头
PCB* FINISH_TAIL = nullptr;   // 完成队列尾（方便插入）

// 函数声明
void INSERT1(PCB* p);                 // 优先数算法：按优先级插入就绪队列
void INSERT2(PCB* p);                 // 轮转算法：插入就绪队列尾部
void FIRSTIN();                       // 从就绪队列取第一个进程投入运行
void PRINT();                         // 显示所有进程状态
void CREATE(string name, int needtime, int algo); // 创建进程并插入就绪队列
void PRISCH();                        // 优先数调度主程序
void ROUNDSCH();                      // 时间片轮转调度主程序

// 将进程插入完成队列尾部
void insertFinish(PCB* p) {
    p->state = STATE_FINISH;
    p->next = nullptr;
    if (FINISH == nullptr) {
        FINISH = FINISH_TAIL = p;
    } else {
        FINISH_TAIL->next = p;
        FINISH_TAIL = p;
    }
}

// 优先数算法：按优先级降序插入就绪队列（优先级高的在前）
void INSERT1(PCB* p) {
    p->state = STATE_READY;
    p->next = nullptr;
    // 如果队列为空
    if (READY == nullptr) {
        READY = TAIL = p;
        return;
    }
    // 如果优先级比队首高，插入队首
    if (p->priority > READY->priority) {
        p->next = READY;
        READY = p;
        return;
    }
    // 否则找到第一个优先级小于等于p的位置，插入前面
    PCB* prev = READY;
    PCB* cur = READY->next;
    while (cur != nullptr && p->priority <= cur->priority) {
        prev = cur;
        cur = cur->next;
    }
    prev->next = p;
    p->next = cur;
    if (cur == nullptr) TAIL = p;
}

// 轮转算法：插入队尾
void INSERT2(PCB* p) {
    p->state = STATE_READY;
    p->next = nullptr;
    if (READY == nullptr) {
        READY = TAIL = p;
    } else {
        TAIL->next = p;
        TAIL = p;
    }
}

// 从就绪队列取出第一个进程投入运行
void FIRSTIN() {
    if (READY == nullptr) {
        RUN = nullptr;
        return;
    }
    RUN = READY;
    RUN->state = STATE_RUN;
    READY = READY->next;
    if (READY == nullptr) TAIL = nullptr;
}

// 显示所有进程的状态（先R，再W，再F）
void PRINT() {
    // 收集三个队列的节点（不破坏原链表，只是遍历）
    // 由于需要按队列顺序显示，直接遍历链表即可
    cout << "name    cpu time    needtime    priority    state" << endl;
    
    // 1. 显示运行态（如果有）
    if (RUN != nullptr) {
        cout << setw(8) << left << RUN->name
             << setw(10) << left << RUN->cputime
             << setw(12) << left << RUN->needtime
             << setw(12) << left << RUN->priority
             << setw(2) << RUN->state << endl;
    }
    
    // 2. 显示就绪态（按链表顺序）
    PCB* p = READY;
    while (p != nullptr) {
        cout << setw(8) << left << p->name
             << setw(10) << left << p->cputime
             << setw(12) << left << p->needtime
             << setw(12) << left << p->priority
             << setw(2) << p->state << endl;
        p = p->next;
    }
    
    // 3. 显示完成态（按链表顺序）
    p = FINISH;
    while (p != nullptr) {
        cout << setw(8) << left << p->name
             << setw(10) << left << p->cputime
             << setw(12) << left << p->needtime
             << setw(12) << left << p->priority
             << setw(2) << p->state << endl;
        p = p->next;
    }
    cout << endl;
}

// 创建新进程，algo=1表示优先数算法，algo=2表示轮转法
void CREATE(string name, int needtime, int algo) {
    int initPrio;
    if (algo == 1) {
        initPrio = 50 - needtime;  // 优先数初值
    } else {
        initPrio = TIME_SLICE;      // 轮转法中priority字段显示时间片数
    }
    PCB* p = new PCB(name, needtime, initPrio);
    if (algo == 1) {
        INSERT1(p);
    } else {
        INSERT2(p);
    }
}

// 优先数调度算法主程序
void PRISCH() {
    cout << "优先数调度算法开始运行..." << endl;
    // 初始调度：从就绪队列取出第一个进程运行
    FIRSTIN();
    PRINT();  // 显示初始状态（第一个进程投入运行）
    
    // 循环调度直至所有进程完成
    while (true) {
        // 如果当前没有运行进程且就绪队列为空，则所有进程完成
        if (RUN == nullptr && READY == nullptr) break;
        
        // 执行当前运行进程一个时间片
        if (RUN != nullptr) {
            // 执行一个时间片
            RUN->cputime++;
            RUN->needtime--;
            RUN->priority--;  // 优先级减1
            
            // 检查进程是否完成
            if (RUN->needtime <= 0) {
                RUN->needtime = 0;
                insertFinish(RUN);
                RUN = nullptr;
            } else {
                // 未完成，将进程重新插入就绪队列（按优先级）
                INSERT1(RUN);
                RUN = nullptr;
            }
        }
        
        // 从就绪队列中取出下一个进程运行
        FIRSTIN();
        PRINT();  // 显示每次调度后的状态
    }
    cout << "所有进程已完成！" << endl;
}

// 时间片轮转调度算法主程序
void ROUNDSCH() {
    cout << "时间片轮转调度算法开始运行（时间片=" << TIME_SLICE << "）..." << endl;
    // 初始调度
    FIRSTIN();
    PRINT();
    
    while (true) {
        if (RUN == nullptr && READY == nullptr) break;
        
        if (RUN != nullptr) {
            // 确定实际执行的时间片（剩余时间可能不足一个时间片）
            int exec_time = (RUN->needtime < TIME_SLICE) ? RUN->needtime : TIME_SLICE;
            RUN->cputime += exec_time;
            RUN->needtime -= exec_time;
            
            // 检查进程是否完成
            if (RUN->needtime <= 0) {
                RUN->needtime = 0;
                insertFinish(RUN);
                RUN = nullptr;
            } else {
                // 未完成，插入就绪队列尾部
                INSERT2(RUN);
                RUN = nullptr;
            }
        }
        
        FIRSTIN();
        PRINT();
    }
    cout << "所有进程已完成！" << endl;
}

int main() {
    int algo;
    cout << "请选择调度算法：1-优先数算法  2-时间片轮转算法" << endl;
    cin >> algo;
    while (algo != 1 && algo != 2) {
        cout << "输入错误，请重新选择（1或2）：";
        cin >> algo;
    }
    
    int n;
    cout << "请输入进程数量：";
    cin >> n;
    
    // 创建进程
    for (int i = 0; i < n; i++) {
        string name;
        int needtime;
        cout << "请输入第" << i+1 << "个进程的名称和需要时间（needtime）：";
        cin >> name >> needtime;
        CREATE(name, needtime, algo);
    }
    
    // 根据算法运行调度
    if (algo == 1) {
        PRISCH();
    } else {
        ROUNDSCH();
    }
    
    // 释放动态分配的内存（简单实验，可省略，但养成好习惯）
    // 注意：链表节点需要逐个释放，此处略（进程结束后操作系统会回收）
    
    return 0;
}