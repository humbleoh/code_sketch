#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <ucontext.h>

#define VML_MIN_STACK_SIZE  4096u

struct vml_coro_ctx;
struct vml_coro_task;

struct vml_coro_ctx *vml_coro_ctx_new();
int vml_coro_ctx_destroy(struct vml_coro_ctx *ctx);

struct vml_coro_task *vml_coro_task_new(struct vml_coro_ctx *ctx, size_t stksize, void (*)(struct vml_coro_task *, void *arg), void *arg);
int vml_coro_task_destroy(struct vml_coro_task *task);

void vml_coro_yield(struct vml_coro_task *task);
void vml_coro_resume(struct vml_coro_task *task);

struct vml_coro_ctx {
    ucontext_t caller;
};

struct vml_coro_task {
    struct vml_coro_ctx *ctx;
    ucontext_t callee;
    void (*callback)(struct vml_coro_task *, void *);
    size_t stksize;
    uint8_t *stack;
    void *arg;
    bool done;
};

struct vml_coro_ctx *vml_coro_ctx_new()
{
    struct vml_coro_ctx *ctx = (struct vml_coro_ctx *) malloc(sizeof(struct vml_coro_ctx));
    return ctx;
}

int vml_coro_ctx_destroy(struct vml_coro_ctx *ctx)
{
    if (!ctx)
        return -1;
    free(ctx);
    return 0;
}

static void callbackwrapper(struct vml_coro_task *task);

struct vml_coro_task *vml_coro_task_new(struct vml_coro_ctx *ctx, size_t stksize, void (*callback)(struct vml_coro_task *, void *), void *arg)
{
    if (!ctx)
        return NULL;
    if (stksize < VML_MIN_STACK_SIZE)
        return NULL;
    if (!callback)
        return NULL;

    struct vml_coro_task *task = (struct vml_coro_task *) malloc(sizeof(struct vml_coro_task *));
    if (!task)
        return NULL;
    uint8_t *stack = (uint8_t *) malloc(stksize);
    if (!stack) {
        free(task);
        return NULL;
    }
    task->stack = stack;
    task->stksize = stksize;
    task->callback = callback;
    task->ctx = ctx;
    task->arg = arg;
    task->done = false;

    getcontext(&task->callee);
    task->callee.uc_stack.ss_sp = task->stack;
    task->callee.uc_stack.ss_size = task->stksize;
    task->callee.uc_stack.ss_flags = 0;
    task->callee.uc_link = &ctx->caller; 
    // On architectures where int and pointer types are the same size (e.g., x86-32, where both types are 32 bits),
    // you may be able to get away with passing pointers as arguments to makecontext() following argc. However,
    // doing this is not guaranteed to be portable, is undefined according to the standards, and won't work on
    // architectures where pointers are larger than ints. Nevertheless, starting with version 2.8, glibc makes some
    // changes to makecontext(), to permit this on some 64-bit architectures (e.g., x86-64). 
    makecontext(&task->callee, (void (*)())callbackwrapper, 1, task);
    return task;
}

int vml_coro_task_destroy(struct vml_coro_task *task)
{
    if (!task)
        return -1;
    free(task->stack);
    free(task);
    return 0;
}

void vml_coro_yield(struct vml_coro_task *task)
{
    assert(task);
    puts("yield");
    swapcontext(&task->callee, &task->ctx->caller);
}

void vml_coro_resume(struct vml_coro_task *task)
{
    assert(task);
    if (task->done)
        return;
    puts("resume");
    swapcontext(&task->ctx->caller, &task->callee);
}

bool vml_coro_done(struct vml_coro_task *task)
{
    assert(task);
    return task->done;
}

static void callbackwrapper(struct vml_coro_task *task)
{
    assert(task);
//    puts("wrapper");
    task->callback(task, task->arg);
    puts("done");
    task->done = true;
}

void print_five_times(struct vml_coro_task *task, void* arg)
{
    for (uint8_t i = 0; i < 5; ++i) {
        printf("hello world %i\n", i);
        vml_coro_yield(task);
    }
}

int main(int argc, const char* argv[])
{
    struct vml_coro_ctx *ctx = vml_coro_ctx_new();
    assert(ctx);
    struct vml_coro_task *task = vml_coro_task_new(ctx, VML_MIN_STACK_SIZE, print_five_times, NULL);
    assert(task);

    while (!vml_coro_done(task)) { 
        puts("main");
        vml_coro_resume(task);
    }

    vml_coro_task_destroy(task);
    vml_coro_ctx_destroy(ctx);
    return 0;
}

