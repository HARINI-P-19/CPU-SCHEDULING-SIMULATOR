/*
 * ============================================================
 *   CPU SCHEDULING SIMULATOR
 *   Algorithms: FCFS | SJF (Non-Preemptive) | Round Robin
 * ============================================================
 */

#include <stdio.h>
#include <string.h>
#include <limits.h>

#define MAX_PROC 10
#define MAX_GANTT 200

/* ─── Data Structures ─── */

typedef struct {
    int  id;
    int  arrival;
    int  burst;
    int  remaining;      /* used in Round Robin */
    int  completion;
    int  waiting;
    int  turnaround;
    int  response;
    int  started;        /* flag: has process started? */
} Process;

typedef struct {
    int pid;             /* process id, -1 = idle */
    int start;
    int end;
} GanttSlot;

/* ─── Utility Functions ─── */

void printLine(char c, int n) {
    for (int i = 0; i < n; i++) putchar(c);
    putchar('\n');
}

void readProcesses(Process p[], int *n) {
    printf("\n  Enter number of processes (1-%d): ", MAX_PROC);
    scanf("%d", n);
    if (*n < 1 || *n > MAX_PROC) {
        printf("  Invalid count. Setting to 4.\n");
        *n = 4;
    }
    printf("\n");
    for (int i = 0; i < *n; i++) {
        p[i].id        = i + 1;
        p[i].remaining = 0;
        p[i].started   = 0;
        printf("  P%d  Arrival Time: ", i + 1);
        scanf("%d", &p[i].arrival);
        printf("  P%d  Burst Time  : ", i + 1);
        scanf("%d", &p[i].burst);
    }
}

void copyProcesses(Process src[], Process dst[], int n) {
    for (int i = 0; i < n; i++) dst[i] = src[i];
}

/* ─── Gantt Chart Printer ─── */

void printGantt(GanttSlot g[], int gn) {
    printf("\n  Gantt Chart:\n  ");

    /* top border */
    for (int i = 0; i < gn; i++) {
        int len = g[i].end - g[i].start;
        for (int j = 0; j < len * 3 + 1; j++) putchar('-');
    }
    printf("\n  |");

    /* process labels */
    for (int i = 0; i < gn; i++) {
        int len = g[i].end - g[i].start;
        int total = len * 3;
        char label[8];
        if (g[i].pid == -1) snprintf(label, sizeof(label), "IDLE");
        else                 snprintf(label, sizeof(label), "P%d", g[i].pid);
        int llen = strlen(label);
        int pad  = (total - llen) / 2;
        for (int j = 0; j < pad; j++) putchar(' ');
        printf("%s", label);
        for (int j = 0; j < total - pad - llen; j++) putchar(' ');
        printf("|");
    }

    /* bottom border */
    printf("\n  ");
    for (int i = 0; i < gn; i++) {
        int len = g[i].end - g[i].start;
        for (int j = 0; j < len * 3 + 1; j++) putchar('-');
    }

    /* time labels */
    printf("\n  ");
    printf("%-3d", g[0].start);
    for (int i = 0; i < gn; i++) {
        int len = g[i].end - g[i].start;
        for (int j = 0; j < len * 3 - 1; j++) putchar(' ');
        printf("%-3d", g[i].end);
    }
    printf("\n");
}

/* ─── Results Table Printer ─── */

void printTable(Process p[], int n, const char *algo) {
    printf("\n  %-28s\n", algo);
    printLine('-', 68);
    printf("  | %-5s | %-8s | %-6s | %-10s | %-9s | %-11s |\n",
           "Proc", "Arrival", "Burst", "Completion", "Waiting", "Turnaround");
    printLine('-', 68);
    for (int i = 0; i < n; i++) {
        printf("  | P%-4d | %-8d | %-6d | %-10d | %-9d | %-11d |\n",
               p[i].id, p[i].arrival, p[i].burst,
               p[i].completion, p[i].waiting, p[i].turnaround);
    }
    printLine('-', 68);
}

void printMetrics(Process p[], int n) {
    float total_wt = 0, total_tat = 0;
    for (int i = 0; i < n; i++) {
        total_wt  += p[i].waiting;
        total_tat += p[i].turnaround;
    }
    printf("\n  Average Waiting Time    : %.2f\n", total_wt / n);
    printf("  Average Turnaround Time : %.2f\n", total_tat / n);
}

/* ─── Compute WT and TAT from completion ─── */

void computeStats(Process p[], int n) {
    for (int i = 0; i < n; i++) {
        p[i].turnaround = p[i].completion - p[i].arrival;
        p[i].waiting    = p[i].turnaround - p[i].burst;
        if (p[i].waiting < 0) p[i].waiting = 0;
    }
}

/* ══════════════════════════════════════════
   ALGORITHM 1 — FCFS
   ══════════════════════════════════════════ */

void fcfs(Process orig[], int n) {
    Process p[MAX_PROC];
    copyProcesses(orig, p, n);

    /* sort by arrival time */
    for (int i = 0; i < n - 1; i++)
        for (int j = i + 1; j < n; j++)
            if (p[j].arrival < p[i].arrival) {
                Process tmp = p[i]; p[i] = p[j]; p[j] = tmp;
            }

    GanttSlot g[MAX_GANTT];
    int gn = 0, time = 0;

    for (int i = 0; i < n; i++) {
        if (time < p[i].arrival) {
            g[gn++] = (GanttSlot){ -1, time, p[i].arrival };
            time = p[i].arrival;
        }
        g[gn++] = (GanttSlot){ p[i].id, time, time + p[i].burst };
        time += p[i].burst;
        p[i].completion = time;
    }

    computeStats(p, n);
    printGantt(g, gn);
    printTable(p, n, "FIRST COME FIRST SERVED (FCFS)");
    printMetrics(p, n);
}

/* ══════════════════════════════════════════
   ALGORITHM 2 — SJF (Non-Preemptive)
   ══════════════════════════════════════════ */

void sjf(Process orig[], int n) {
    Process p[MAX_PROC];
    copyProcesses(orig, p, n);

    GanttSlot g[MAX_GANTT];
    int gn = 0, time = 0, done = 0;
    int finished[MAX_PROC] = {0};

    while (done < n) {
        int idx = -1, min_burst = INT_MAX;

        /* find shortest available job */
        for (int i = 0; i < n; i++) {
            if (!finished[i] && p[i].arrival <= time) {
                if (p[i].burst < min_burst ||
                   (p[i].burst == min_burst && p[i].arrival < p[idx].arrival)) {
                    min_burst = p[i].burst;
                    idx = i;
                }
            }
        }

        if (idx == -1) {
            /* CPU idle — find next arriving process */
            int next_arrive = INT_MAX;
            for (int i = 0; i < n; i++)
                if (!finished[i] && p[i].arrival < next_arrive)
                    next_arrive = p[i].arrival;
            g[gn++] = (GanttSlot){ -1, time, next_arrive };
            time = next_arrive;
            continue;
        }

        g[gn++] = (GanttSlot){ p[idx].id, time, time + p[idx].burst };
        time += p[idx].burst;
        p[idx].completion = time;
        finished[idx] = 1;
        done++;
    }

    computeStats(p, n);
    printGantt(g, gn);
    printTable(p, n, "SHORTEST JOB FIRST — NON-PREEMPTIVE (SJF)");
    printMetrics(p, n);
}

/* ══════════════════════════════════════════
   ALGORITHM 3 — ROUND ROBIN
   ══════════════════════════════════════════ */

void roundRobin(Process orig[], int n, int quantum) {
    Process p[MAX_PROC];
    copyProcesses(orig, p, n);
    for (int i = 0; i < n; i++) {
        p[i].remaining = p[i].burst;
        p[i].started   = 0;
        p[i].response  = -1;
    }

    GanttSlot g[MAX_GANTT];
    int gn = 0, time = 0, done = 0;

    /* ready queue */
    int queue[MAX_PROC * 100];
    int front = 0, rear = 0;
    int in_queue[MAX_PROC] = {0};

    /* enqueue processes arriving at time 0 */
    for (int i = 0; i < n; i++)
        if (p[i].arrival == 0) {
            queue[rear++] = i;
            in_queue[i]   = 1;
        }

    while (done < n) {
        if (front == rear) {
            /* idle — advance to next arrival */
            int next = INT_MAX;
            for (int i = 0; i < n; i++)
                if (p[i].remaining > 0 && p[i].arrival < next)
                    next = p[i].arrival;
            g[gn++] = (GanttSlot){ -1, time, next };
            time = next;
            for (int i = 0; i < n; i++)
                if (!in_queue[i] && p[i].remaining > 0 && p[i].arrival <= time) {
                    queue[rear++] = i;
                    in_queue[i]   = 1;
                }
            continue;
        }

        int idx = queue[front++];

        /* record response time on first execution */
        if (p[idx].response == -1)
            p[idx].response = time - p[idx].arrival;

        int run = (p[idx].remaining < quantum) ? p[idx].remaining : quantum;
        g[gn++] = (GanttSlot){ p[idx].id, time, time + run };
        time            += run;
        p[idx].remaining -= run;

        /* enqueue newly arrived processes */
        for (int i = 0; i < n; i++)
            if (!in_queue[i] && p[i].remaining > 0 && p[i].arrival <= time) {
                queue[rear++] = i;
                in_queue[i]   = 1;
            }

        if (p[idx].remaining == 0) {
            p[idx].completion = time;
            done++;
        } else {
            queue[rear++] = idx;  /* re-enqueue */
        }
    }

    computeStats(p, n);
    printGantt(g, gn);
    printTable(p, n, "ROUND ROBIN (RR)");
    printMetrics(p, n);
    printf("  Time Quantum Used       : %d\n", quantum);
}

/* ─── Main ─── */

int main() {
    Process procs[MAX_PROC];
    int n, choice, quantum;

    printf("\n");
    printLine('=', 50);
    printf("       CPU SCHEDULING SIMULATOR\n");
    printf("       FCFS | SJF | Round Robin\n");
    printLine('=', 50);

    readProcesses(procs, &n);

    do {
        printf("\n");
        printLine('-', 40);
        printf("  SELECT ALGORITHM\n");
        printLine('-', 40);
        printf("  1. First Come First Served (FCFS)\n");
        printf("  2. Shortest Job First — Non-Preemptive (SJF)\n");
        printf("  3. Round Robin (RR)\n");
        printf("  4. Run ALL Algorithms\n");
        printf("  5. Enter New Processes\n");
        printf("  0. Exit\n");
        printLine('-', 40);
        printf("  Choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                printf("\n"); printLine('=', 50);
                fcfs(procs, n);
                break;

            case 2:
                printf("\n"); printLine('=', 50);
                sjf(procs, n);
                break;

            case 3:
                printf("\n  Enter Time Quantum: ");
                scanf("%d", &quantum);
                if (quantum < 1) quantum = 1;
                printf("\n"); printLine('=', 50);
                roundRobin(procs, n, quantum);
                break;

            case 4:
                printf("\n  Enter Time Quantum for Round Robin: ");
                scanf("%d", &quantum);
                if (quantum < 1) quantum = 1;

                printf("\n\n"); printLine('=', 50);
                printf("  [1/3] FCFS\n");
                printLine('=', 50);
                fcfs(procs, n);

                printf("\n\n"); printLine('=', 50);
                printf("  [2/3] SJF\n");
                printLine('=', 50);
                sjf(procs, n);

                printf("\n\n"); printLine('=', 50);
                printf("  [3/3] ROUND ROBIN (q=%d)\n", quantum);
                printLine('=', 50);
                roundRobin(procs, n, quantum);
                break;

            case 5:
                readProcesses(procs, &n);
                break;

            case 0:
                printf("\n  Goodbye!\n\n");
                break;

            default:
                printf("  Invalid choice.\n");
        }
    } while (choice != 0);

    return 0;
}
