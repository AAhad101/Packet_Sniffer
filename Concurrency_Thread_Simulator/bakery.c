#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

// Defining macros for bakery constraints
#define MAX_CUSTOMERS 1000
#define MAX_CAPACITY 25
#define SOFA_CAPACITY 4
#define NUM_CHEFS 4
#define NUM_OVENS 4

// Simulating time for the timestamps (not real time)
int sim_time = 0;
pthread_mutex_t time_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t time_cond = PTHREAD_COND_INITIALIZER;

// Output synchronization
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

// Shop capacity
int customers_in_shop = 0;
pthread_mutex_t shop_mutex = PTHREAD_MUTEX_INITIALIZER;

// Sofa management
int sofa_count = 0;
pthread_mutex_t sofa_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sofa_cond = PTHREAD_COND_INITIALIZER;

// Oven semaphore (4 ovens)
sem_t oven_sem;

// Cash register mutex (only 1)
pthread_mutex_t cash_register_mutex = PTHREAD_MUTEX_INITIALIZER;

// Customer structure
typedef struct Customer {
    int id;
    int enter_time;
    pthread_t thread;
    
    bool cake_requested;
    bool being_baked;
    bool cake_ready;
    bool paid;
    bool being_paid;
    bool payment_accepted;
    bool left;
    
    int sit_time;
    int pay_time;
    
    pthread_mutex_t mutex;
    pthread_cond_t cake_cond;
    pthread_cond_t payment_cond;
} Customer;

Customer* customers[MAX_CUSTOMERS];
int num_customers = 0;
pthread_mutex_t customer_mutex = PTHREAD_MUTEX_INITIALIZER;

// Chef structure
typedef struct Chef {
    int id;
    pthread_t thread;
} Chef;

Chef chefs[NUM_CHEFS];
bool simulation_done = false;

// Thread-safe print
void print_event(int timestamp, const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    pthread_mutex_lock(&print_mutex);
    printf("%d %s\n", timestamp, buffer);
    fflush(stdout);
    pthread_mutex_unlock(&print_mutex);
}

// Get current simulation time
int get_sim_time() {
    pthread_mutex_lock(&time_mutex);
    int t = sim_time;
    pthread_mutex_unlock(&time_mutex);
    return t;
}

// Wait until simulation reaches target time
void wait_until(int target_time) {
    pthread_mutex_lock(&time_mutex);
    while (sim_time < target_time) {
        pthread_cond_wait(&time_cond, &time_mutex);
    }
    pthread_mutex_unlock(&time_mutex);
}

// Advance simulation time
void advance_to(int new_time) {
    pthread_mutex_lock(&time_mutex);
    if (new_time > sim_time) {
        sim_time = new_time;
        pthread_cond_broadcast(&time_cond);
    }
    pthread_mutex_unlock(&time_mutex);
}

// Find customer needing cake (FIFO order)
Customer* find_customer_needing_cake() {
    pthread_mutex_lock(&customer_mutex);
    for (int i = 0; i < num_customers; i++) {
        Customer* c = customers[i];
        pthread_mutex_lock(&c->mutex);
        if (c->cake_requested && !c->being_baked && !c->cake_ready && !c->left) {
            c->being_baked = true;
            pthread_mutex_unlock(&c->mutex);
            pthread_mutex_unlock(&customer_mutex);
            return c;
        }
        pthread_mutex_unlock(&c->mutex);
    }
    pthread_mutex_unlock(&customer_mutex);
    return NULL;
}

// Find customer needing payment acceptance (FIFO order)
Customer* find_customer_needing_payment() {
    pthread_mutex_lock(&customer_mutex);
    for (int i = 0; i < num_customers; i++) {
        Customer* c = customers[i];
        pthread_mutex_lock(&c->mutex);
        if (c->paid && !c->being_paid && !c->payment_accepted && !c->left) {
            c->being_paid = true;
            pthread_mutex_unlock(&c->mutex);
            pthread_mutex_unlock(&customer_mutex);
            return c;
        }
        pthread_mutex_unlock(&c->mutex);
    }
    pthread_mutex_unlock(&customer_mutex);
    return NULL;
}

// Customer thread function
void* customer_function(void* arg) {
    Customer* c = (Customer*)arg;
    
    wait_until(c->enter_time);
    
    pthread_mutex_lock(&shop_mutex);
    if (customers_in_shop >= MAX_CAPACITY) {
        pthread_mutex_unlock(&shop_mutex);
        c->left = true;
        return NULL;
    }
    customers_in_shop++;
    pthread_mutex_unlock(&shop_mutex);
    print_event(c->enter_time, "Customer %d enters", c->id);
    
    // --- START OF UPDATED SITTING LOGIC ---
    int attempt_sit_time = c->enter_time + 1;
    while (1) {
        wait_until(attempt_sit_time);
        
        pthread_mutex_lock(&sofa_mutex);
        
        if (sofa_count < SOFA_CAPACITY) {
            // A spot is free
            sofa_count++;
            c->sit_time = get_sim_time();
            pthread_mutex_unlock(&sofa_mutex);
            print_event(c->sit_time, "Customer %d sits", c->id);
            break; // Exit the while(1) loop we are seated
        } else {
            // Sofa is full, wait for a signal
            pthread_cond_wait(&sofa_cond, &sofa_mutex);
            
            // We have been woken up because a spot is free
            // But we must wait until the NEXT second to try and sit
            attempt_sit_time = get_sim_time() + 1;
            pthread_mutex_unlock(&sofa_mutex);
        }
    }
    
    wait_until(c->sit_time + 1);
    
    pthread_mutex_lock(&c->mutex);
    c->cake_requested = true;
    int request_time = get_sim_time();
    pthread_mutex_unlock(&c->mutex);
    print_event(request_time, "Customer %d requests cake", c->id);
    
    int pay_time;
    pthread_mutex_lock(&c->mutex);
    while (!c->cake_ready) {
        pthread_cond_wait(&c->cake_cond, &c->mutex);
    }
    pay_time = c->pay_time;
    pthread_mutex_unlock(&c->mutex);

    wait_until(pay_time);

    pthread_mutex_lock(&c->mutex);
    c->paid = true;
    pthread_mutex_unlock(&c->mutex);
    print_event(pay_time, "Customer %d pays", c->id);
    
    pthread_mutex_lock(&c->mutex);
    while (!c->payment_accepted) {
        pthread_cond_wait(&c->payment_cond, &c->mutex);
    }
    int leave_time = get_sim_time();
    pthread_mutex_unlock(&c->mutex);
    
    c->left = true;
    print_event(leave_time, "Customer %d leaves", c->id);
    
    pthread_mutex_lock(&sofa_mutex);
    sofa_count--;
    pthread_cond_broadcast(&sofa_cond);
    pthread_mutex_unlock(&sofa_mutex);
    
    pthread_mutex_lock(&shop_mutex);
    customers_in_shop--;
    pthread_mutex_unlock(&shop_mutex);
    
    return NULL;
}

// Chef thread function
void* chef_function(void* arg) {
    Chef* chef = (Chef*)arg;
    
    while (!simulation_done) {
        Customer* payment_customer = find_customer_needing_payment();
        if (payment_customer) {
            pthread_mutex_lock(&cash_register_mutex);
            
            int current_time = get_sim_time();
            int start_time;

            if (current_time == payment_customer->pay_time) {
                // Customer just paid; chef needs to wait 1s to react
                start_time = current_time + 1;
            } else {
                // Customer has been waiting; chef can start immediately
                start_time = current_time;
            }
            
            wait_until(start_time);
            
            print_event(start_time, "Chef %d accepts payment for Customer %d", chef->id, payment_customer->id);
            
            int end_time = start_time + 2;
            wait_until(end_time);
            
            pthread_mutex_lock(&payment_customer->mutex);
            payment_customer->payment_accepted = true;
            pthread_cond_signal(&payment_customer->payment_cond);
            pthread_mutex_unlock(&payment_customer->mutex);
            
            pthread_mutex_unlock(&cash_register_mutex);
            continue;
        }
        
        Customer* cake_customer = find_customer_needing_cake();
        if (cake_customer) {
            sem_wait(&oven_sem);
            
            int start_time = get_sim_time() + 1;
            wait_until(start_time);
            
            print_event(start_time, "Chef %d bakes for Customer %d", chef->id, cake_customer->id);
            
            int end_time = start_time + 2;
            wait_until(end_time);
            
            pthread_mutex_lock(&cake_customer->mutex);
            cake_customer->cake_ready = true;
            cake_customer->pay_time = end_time;
            pthread_cond_signal(&cake_customer->cake_cond);
            pthread_mutex_unlock(&cake_customer->mutex);
            
            sem_post(&oven_sem);
            continue;
        }
        
        usleep(1000); // Prevent busy-waiting when idle
    }
    
    return NULL;
}

// Time advancement thread
void* time_thread_function(void* arg) {
    int* max_time = (int*)arg;
    
    for (int t = 0; t <= *max_time + 50; t++) {
        advance_to(t);
        usleep(5000);
    }
    
    return NULL;
}

int main() {
    sem_init(&oven_sem, 0, NUM_OVENS);
    
    char line[256];
    int max_enter_time = 0;
    
    while (fgets(line, sizeof(line), stdin) != NULL) {
        if (feof(stdin) || strcmp(line, "<EOF>\n") == 0) {
            break;
        }
        int enter_time, id;
        if (sscanf(line, "%d Customer %d", &enter_time, &id) == 2) {
            Customer* c = malloc(sizeof(Customer));
            *c = (Customer){.id = id, .enter_time = enter_time};
            pthread_mutex_init(&c->mutex, NULL);
            pthread_cond_init(&c->cake_cond, NULL);
            pthread_cond_init(&c->payment_cond, NULL);
            customers[num_customers++] = c;
            if (enter_time > max_enter_time) {
                max_enter_time = enter_time;
            }
        }
    }
    
    if (num_customers == 0) return 0;
    
    pthread_t time_thread;
    pthread_create(&time_thread, NULL, time_thread_function, &max_enter_time);
    
    for (int i = 0; i < NUM_CHEFS; i++) {
        chefs[i].id = i + 1;
        pthread_create(&chefs[i].thread, NULL, chef_function, &chefs[i]);
    }
    
    for (int i = 0; i < num_customers; i++) {
        pthread_create(&customers[i]->thread, NULL, customer_function, customers[i]);
    }
    
    for (int i = 0; i < num_customers; i++) {
        pthread_join(customers[i]->thread, NULL);
    }
    
    wait_until(get_sim_time() + 5); 
    simulation_done = true;
    
    for (int i = 0; i < NUM_CHEFS; i++) {
        pthread_join(chefs[i].thread, NULL);
    }
    pthread_join(time_thread, NULL);
    
    for (int i = 0; i < num_customers; i++) {
        pthread_mutex_destroy(&customers[i]->mutex);
        pthread_cond_destroy(&customers[i]->cake_cond);
        pthread_cond_destroy(&customers[i]->payment_cond);
        free(customers[i]);
    }
    
    sem_destroy(&oven_sem);
    
    return 0;
}
