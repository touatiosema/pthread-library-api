#include "thread.h"

#define ERROR_JOINED 1
#define EBUSY 1
#define UNUSED(x) (void)x
#define CLOCKID CLOCK_PROCESS_CPUTIME_ID
#define SIG SIGRTMIN
#define errExit(msg)        \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

// the thread currently running.
thread_perso_t *current_thread = NULL;

// the list of all threads.
TAILQ_HEAD(head_s, thread_perso)
waiting_queue;

int initialized = 0;
int next_id = 0;

// variables used for preemption and signal handling.
timer_t timerid;
struct sigevent sev;
struct itimerspec its;
sigset_t mask;
struct sigaction sa;

volatile sig_atomic_t signal_pending;
volatile sig_atomic_t defer_signal;

// singal handler for preemption.
void timer_handler()
{
  if (defer_signal)
    signal_pending = SIG;
  else{
    signal_pending = 0;
    thread_yield();
  }
}

/*function that initilizes signals and seghandlers and timers needed for preemption*/
static void makeTimer()
{
    /* Establish handler for timer signal */
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timer_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIG, &sa, NULL) == -1)
        errExit("sigaction");

    /* Block timer signal temporarily */

    sigemptyset(&mask);
    sigaddset(&mask, SIG);
    if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
        errExit("sigprocmask");

    /* Create the timer */

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIG;
    sev.sigev_value.sival_ptr = &timerid;
    if (timer_create(CLOCKID, &sev, &timerid) == -1)
        errExit("timer_create");

    /* no need for time intervals */
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = 0;

    /* unblocking signal */
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
        errExit("sigprocmask");
}

// this is used to set timers each time we switch thread according to thread priority.
void set_waiting_time(unsigned char priority)
{
    switch (priority)
    {
    case MIN_PRIORITY:
        //expires after 10ms
        its.it_value.tv_nsec = 10000000;
        break;
    case MAX_PRIORITY:
        //expires after 50ms
        its.it_value.tv_nsec = 50000000;
        break;
    case NORM_PRIORITY:
        // expires after 20ms
        its.it_value.tv_nsec = 20000000;
        break;
    }
    // starting the vitual timer
    if (timer_settime(timerid, 0, &its, NULL) == -1)
        errExit("timer_settime");
}

//making valgrind by freeing the first thread.
void clearing_first_thread()
{
    //if (NULL != t && t->join_thread == NULL && ((t->my_context.uc_stack).ss_sp != NULL)){
    if (current_thread->stack_id != -1)
    {
        VALGRIND_STACK_DEREGISTER(current_thread->stack_id);
        free((current_thread->my_context.uc_stack).ss_sp);
    }
    free(current_thread);
    timer_delete(timerid);
}

//function to call first when using the library
void init()
{

    if (!initialized)
    {
        //initializing preemption signal handlers:
        makeTimer();

        //initializing thread queue
        TAILQ_INIT(&waiting_queue);
        ;
        //make copy of main thread
        current_thread = malloc(sizeof(thread_perso_t));

        getcontext(&current_thread->my_context);
        current_thread->stack_id = -1;
        current_thread->my_context.uc_link = NULL;
        current_thread->id = next_id++;
        current_thread->is_finished = 0;
        current_thread->join_thread = NULL;
        current_thread->priority = NORM_PRIORITY;
        next_id++;
        initialized = 1;

        //this line is to make sure that the first thread is cleaning its heap.
        atexit(clearing_first_thread);
        set_waiting_time(NORM_PRIORITY); // before scheduling it to be called.
    }
}

// setting waiting time according to priority.
int set_priority(thread_t *thread, unsigned char priority)
{
    init();
    switch (priority)
    {
    case MIN_PRIORITY:
        ((thread_perso_t *)thread)->priority = MIN_PRIORITY;
        break;
    case MAX_PRIORITY:
        ((thread_perso_t *)thread)->priority = MAX_PRIORITY;
        break;
    case NORM_PRIORITY:
        ((thread_perso_t *)thread)->priority = NORM_PRIORITY;
        break;
    default:
        return 0;
    }
    return 1;
}

int unblock_signal(){
  defer_signal--;
  if (signal_pending != 0){
    raise(signal_pending);
    return 1;
  }
  return 0;
}

// put the current thread in the waiting queue and execute the first thread on head of waiting queue
int thread_yield(void)
{
    init();
    thread_perso_t *old_thread = current_thread;
    TAILQ_INSERT_TAIL(&waiting_queue, current_thread, threads);
    current_thread = TAILQ_FIRST(&waiting_queue);
    set_waiting_time(current_thread->priority);
    if (current_thread != NULL)
    {
        TAILQ_REMOVE(&waiting_queue, current_thread, threads);
        swapcontext(&old_thread->my_context, &current_thread->my_context);
    }
    else
    {
        current_thread = old_thread;
    }
    return 0;
}

void thread_func(void *(*func)(void *), void *funcarg)
{
    void *res = func(funcarg);
    thread_exit(res);
}

thread_t thread_self(void)
{
    init();
    return current_thread;
}

int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg)
{
    init();

    //Block the signal temporarily
    defer_signal++;
    
    // printf("====>thread_create called\n");
    thread_perso_t *t = malloc(sizeof(thread_perso_t));

    //we call getcontext to init some variables
    getcontext(&t->my_context);
    (t->my_context.uc_stack).ss_size = 64 * 1024;
    (t->my_context.uc_stack).ss_sp = malloc((t->my_context.uc_stack).ss_size);

    int valgrind_stackid = VALGRIND_STACK_REGISTER(t->my_context.uc_stack.ss_sp,
                                                   t->my_context.uc_stack.ss_sp + t->my_context.uc_stack.ss_size);
    t->stack_id = valgrind_stackid;

    t->my_context.uc_link = NULL;
    //create new context for this thread
    makecontext(&t->my_context, (void (*)(void))thread_func, 2, (void (*)(void))func, funcarg);
    t->id = next_id;
    next_id++;
    t->join_thread = NULL;
    t->is_finished = 0;
    t->priority = NORM_PRIORITY;
    TAILQ_INSERT_TAIL(&waiting_queue, t, threads);
    *newthread = t;

    //Unblock the signal and check wether a signal has been received or not
    unblock_signal();
    
    return 0;
}

/* terminer le thread courant en renvoyant la valeur de retour retval.
 * cette fonction ne retourne jamais.
 *
 * L'attribut noreturn aide le compilateur à optimiser le code de
 * l'application (élimination de code mort). Attention à ne pas mettre
 * cet attribut dans votre interface tant que votre thread_exit()
 * n'est pas correctement implémenté (il ne doit jamais retourner).
 */
void thread_exit(void *retval)
{
    init();
    if (current_thread->join_thread != NULL)
        TAILQ_INSERT_TAIL(&waiting_queue, current_thread->join_thread, threads);
    current_thread->is_finished = 1;
    current_thread->join_thread = NULL;
    current_thread->retVal = retval;
    thread_perso_t *old_thread = current_thread;

    current_thread = TAILQ_FIRST(&waiting_queue);
    if (current_thread != NULL)
    {
        TAILQ_REMOVE(&waiting_queue, current_thread, threads);
        swapcontext(&old_thread->my_context, &current_thread->my_context);
    }
    else
    {
        current_thread = old_thread;
        exit(0);
    }
    while (1)
        ;
}

/* attendre la fin d'exécution d'un thread.
 * la valeur renvoyée par le thread est placée dans *retval.
 * si retval est NULL, la valeur de retour est ignorée.
 */
int thread_join(thread_t thread, void **retval)
{
    init();

    thread_perso_t *old_thread = current_thread;
    thread_perso_t *t = (thread_perso_t *)thread;
    if (t->join_thread != NULL)
        return ERROR_JOINED;

    if (!t->is_finished)
    {
        t->join_thread = current_thread;
        current_thread = TAILQ_FIRST(&waiting_queue);
        if (current_thread != NULL)
        {
            TAILQ_REMOVE(&waiting_queue, current_thread, threads);
            swapcontext(&old_thread->my_context, &current_thread->my_context);
        }
    }

    if (retval != NULL)
        *retval = t->retVal;
    
    if (t->stack_id != -1)
    {
        VALGRIND_STACK_DEREGISTER(t->stack_id);
        free((t->my_context.uc_stack).ss_sp);
    }
    free(t);

    return 0;
}

/* Mutex functions need to be safe against timer signal */

int thread_mutex_destroy(thread_mutex_t *mutex)
{
    //Block the signal temporarily
    defer_signal++;
  
    if (TAILQ_EMPTY(&mutex->waiting_queue))
    {
        mutex->is_locked = 0;
    }

     //Unblock the signal and check wether a signal has been received or not
    unblock_signal();
    
    return EBUSY;
}

int thread_mutex_init(thread_mutex_t *mutex)
{
    //Block the signal temporarily
    defer_signal++;
  
    TAILQ_INIT(&mutex->waiting_queue);
    mutex->is_locked = 0;

    //Unblock the signal and check wether a signal has been received or not
    unblock_signal();
    
    return 0;
}

int thread_mutex_unlock(thread_mutex_t *mutex)
{
  //Block the signal temporarily
  defer_signal++;

  if (TAILQ_EMPTY(&mutex->waiting_queue))
    {
        mutex->is_locked = 0;
        return 0;
    }
    thread_perso_t *t = TAILQ_FIRST(&mutex->waiting_queue);
    TAILQ_REMOVE(&mutex->waiting_queue, t, threads);
    TAILQ_INSERT_TAIL(&waiting_queue, t, threads);

    //Unblock the signal and check wether a signal has been received or not
    unblock_signal();
    
    return 0;
}

int thread_mutex_lock(thread_mutex_t *mutex)
{
    //Block the signal temporarily
    defer_signal++;
  
    if (!mutex->is_locked)
    {
        mutex->is_locked = 1;
        return 0;
    }
    TAILQ_INSERT_TAIL(&mutex->waiting_queue, current_thread, threads);
    thread_perso_t *old_thread = current_thread;
    current_thread = TAILQ_FIRST(&waiting_queue);
    if (current_thread != NULL)
    {
        TAILQ_REMOVE(&waiting_queue, current_thread, threads);
        swapcontext(&old_thread->my_context, &current_thread->my_context);
    }

    //Unblock the signal and check wether a signal has been received or not
    unblock_signal();
    
    return 0;
}
