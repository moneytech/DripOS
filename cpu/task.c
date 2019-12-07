#include "task.h"
#include "../libc/mem.h"
#include "../drivers/screen.h"
#include "../libc/string.h"
#include "../kernel/kernel.h"
#include "../kernel/terminal.h"
#include "../cpu/isr.h"
#include "../drivers/serial.h"

Task *runningTask;
static Task mainTask;
static Task otherTask;
static Task temp;
Task *next_temp;
uint32_t pid_max = 0;
registers_t *global_regs;

static void otherMain() {
    while (1) {
        //kprint("\nHello multitasking world!");
        //yeild();
    }
}

static void otherMain2() {
    while (1) {
        //kprint("\nHello!");
        //yield();
    }
}

void initTasking() {
    // Get EFLAGS and CR3
    asm volatile("movl %%cr3, %%eax; movl %%eax, %0;":"=m"(temp.regs.cr3)::"%eax"); // No paging yet
    asm volatile("pushfl; movl (%%esp), %%eax; movl %%eax, %0; popfl;":"=m"(temp.regs.eflags)::"%eax");
 
    createTask(&otherTask, otherMain);
    createTask(&mainTask, otherMain2);
    mainTask.next = &otherTask; // Head of task list
    otherTask.next = &mainTask; // Tail of task list
 
    runningTask = &otherTask;
    execute_command("bgtask");
    loaded = 1;
    otherMain();
}
 
uint32_t createTask(Task *task, void (*main)()) {//, uint32_t *pagedir) { // No paging yet
    asm volatile("movl %%cr3, %%eax; movl %%eax, %0;":"=m"(temp.regs.cr3)::"%eax"); // No paging yet
    asm volatile("cli; pushfl; movl (%%esp), %%eax; movl %%eax, %0; popfl; sti;":"=m"(temp.regs.eflags)::"%eax");
    task->regs.eax = 0;
    task->regs.ebx = 0;
    task->regs.ecx = 0;
    task->regs.edx = 0;
    task->regs.esi = 0;
    task->regs.edi = 0;
    task->regs.eflags = temp.regs.eflags;
    task->regs.eip = (uint32_t) main;
    task->regs.cr3 = (uint32_t)temp.regs.cr3; // No paging yet
    task->regs.esp = (uint32_t) kmalloc(0x4000) + 0x4000; // Allocate 16KB for the process stack
    task->regs.ebp = 0;
    next_temp = mainTask.next;
    task->next = next_temp;
    mainTask.next = task;
    task->pid = pid_max;
    task->ticks_cpu_time = 0;
    task->state = RUNNING;
    task->priority = NORMAL;
    pid_max++;
    return task->pid;
}
 
void yield() {
    //Task *last = runningTask;
    runningTask = runningTask->next;
    switchTask(&runningTask->regs);
    //kprint("\nswitchTask call didn't work, execution returned");
}

int32_t kill_task(uint32_t pid) {
    if (pid == 0 || pid == 1) {
        return 2; // Permission denied
    }
    Task *temp_kill = &mainTask;
    Task *to_kill = 0;
    Task *prev_task;
    uint32_t loop = 0;
    //kprint("\nPID to find: ");
    //kprint_uint(pid);
    //kprint("\nMax PID: ");
    //kprint_uint(pid_max);
    while (1) {
        //kprint("\nLoop: ");
        //kprint_uint(loop);
        if (loop > pid_max) {
            break;
        }
        //kprint("\nCurrent pid: ");
        //kprint_uint(temp_kill->next->pid);
        if (temp_kill->next->pid == pid) {
            to_kill = temp_kill->next; // Process to kill
            prev_task = temp_kill; // Process before
            temp_kill = temp_kill->next->next; // Process after
            break;
        }
        if (temp_kill->next->pid == 0) {
            return 1; // Task not found, looped back to 0
        }
        loop++;
        temp_kill = temp_kill->next;
    }
    if ((uint32_t)to_kill == 0) {
        return 1; // This should never be reached but eh, safety first
    }
    prev_task->next = temp_kill; // Remove killed task from the chain
    return 0; // Worked... hopefully lol
}

void print_tasks() {
    Task *temp_list = &mainTask;
    uint32_t oof = 1;
    while (1) {
        if (temp_list == &mainTask && oof != 1) {
            break;
        }
        kprint("\n");
        kprint_uint(temp_list->pid);
        kprint("  ");
        kprint_uint(temp_list->ticks_cpu_time);
        temp_list = temp_list->next;
        oof = 0;
    }
}

void timer_switch_task(registers_t *from, Task *to) {
    //free(test, 0x1000);
    Registers *regs = &runningTask->regs;
    regs->eflags = from->eflags;
    regs->eax = from->eax;
    regs->ebx = from->ebx;
    regs->ecx = from->ecx;
    regs->edx = from->edx;
    regs->edi = from->edi;
    regs->esi = from->esi;
    regs->eip = from->eip;
    regs->esp = from->esp;
    regs->ebp = from->ebp;
    runningTask = to;
    switchTask(&to->regs);
}

void schedule(registers_t *from) {
    Registers *regs = &runningTask->regs; // Get registers
    /* Set old registers */
    regs->eflags = from->eflags;
    regs->eax = from->eax;
    regs->ebx = from->ebx;
    regs->ecx = from->ecx;
    regs->edx = from->edx;
    regs->edi = from->edi;
    regs->esi = from->esi;
    regs->eip = from->eip;
    regs->esp = from->esp;
    regs->ebp = from->ebp;
    // Select new running task
    runningTask = runningTask->next;
    while (runningTask->state != RUNNING) {
        runningTask = runningTask->next;
    }
    // Switch
    //sprint_uint(3);
    switchTask(&runningTask->regs);
}
void irq_schedule() {
    //sprint_uint(2);
    schedule(global_regs);
}
void store_global(registers_t *ok) {
    sprint("\n");
    sprint_uint(ok->eip);
    sprint("\n");
    sprint_uint(ok->esp);
    global_regs = ok;
}