// jamesocampo@usf.edu
// James Ocampo
// This program takes the first 50 characters of an string input from the user
// and produces each character into a shared circular buffer.
// It uses a producer-consumer thread model where the producer adds the input characters to the buffer,
// and the consumer removes and consumes the characters from the buffer.
// Since threads share the same address space, they can access the shared buffer variables.
// But to do so safely, the program uses a mutex lock to protect the shared resources
// and condition variables to signal when the buffer is not full or not empty.

// Critical Sections:
// The main critical sections in this program are when the producer adds characters to the buffer,
// when the consumer removes characters from the buffer, and when the producer signals that production is done.

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> // For threading
#include <string.h>

// Circular buffer size has 15 positions
#define CIRCULAR_BUFFER_SIZE 15
// Maximum input size is 50 characters (if input is larger, the program will truncate it)
#define MAX_INPUT 50

// Shared circular buffer
char buffer[CIRCULAR_BUFFER_SIZE];
// Shared variables to keep track of buffer state
int buffer_counter = 0; // number of elements in the buffer
int buffer_in = 0;      // next position of the buffer that the producer will write to
int buffer_out = 0;     // next position of the buffer that the consumer will read from

// Synchronization primitives
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;          // Mutex Lock to protect shared resources
pthread_cond_t buffer_not_full = PTHREAD_COND_INITIALIZER;  // Condition variable to signal that the buffer is not full
pthread_cond_t buffer_not_empty = PTHREAD_COND_INITIALIZER; // Condition variable to signal that the buffer is not empty

// Flag to indicate production is complete
int completed_production = 0;

// Producer thread function takes in a pointer to the input string
void *producer(void *arg)
{
    char *input = (char *)arg;
    int input_length = strlen(input);

    // Iterate through each character in the input string
    for (int i = 0; i < input_length; i++)
    {
        // Acquire MUTEX LOCK to protect shared resources of the circular buffer
        pthread_mutex_lock(&mutex);

        // While the buffer is full, wait until the buffer is not full
        while (buffer_counter == CIRCULAR_BUFFER_SIZE)
        {
            // This will release the mutex lock and wait for the condition (buffer is not full) to be signaled
            pthread_cond_wait(&buffer_not_full, &mutex);
        }

        // Add character to buffer
        buffer[buffer_in] = input[i];       // Get the character from the input at index i and add it to the buffer at the correct buffer_in index
        printf("Produced: %c\n", input[i]); // Output the character that was produced

        // Update buffer_in index by incrementing and modding by the buffer size
        // This way it will wrap around to the beginning of the buffer if it reaches the end
        buffer_in = (buffer_in + 1) % CIRCULAR_BUFFER_SIZE;
        // Increment the buffer_counter to keep track of the number of elements in the buffer
        buffer_counter++;

        // Signal consumer that buffer is not empty
        pthread_cond_signal(&buffer_not_empty);

        // Release mutex
        pthread_mutex_unlock(&mutex);
    }

    // Signal production is complete (CRITICAL SECTION)
    pthread_mutex_lock(&mutex);             // Acquire mutex lock
    completed_production = 1;               // Set completed_production flag to 1 to indicate production is complete
    pthread_cond_signal(&buffer_not_empty); // Signal that the buffer is not empty
    pthread_mutex_unlock(&mutex);           // Release mutex lock

    // Output that the producer is done
    printf("Producer: done\n");
    return NULL;
}

// Consumer thread function
void *consumer(void *arg)
{
    // Consumer only stops if buffer is empty and production is done
    while (1)
    {
        // Acquire MUTEX LOCK to protect shared resources of the circular buffer
        pthread_mutex_lock(&mutex);

        // Wait if buffer is empty and production is not done
        while (buffer_counter == 0 && !completed_production)
        {
            // This will release the mutex lock and wait for the condition (buffer is not empty) to be signaled
            pthread_cond_wait(&buffer_not_empty, &mutex);
        }

        // No more elements in the buffer and production is finished, unlock the mutex and break
        if (buffer_counter == 0 && completed_production)
        {
            pthread_mutex_unlock(&mutex); // Release the mutex lock on the buffer
            break;
        }

        // Remove character from buffer
        char c = buffer[buffer_out];
        // Output the character that was consumed
        printf("Consumed: %c\n", c);

        // Update buffer_out index by incrementing and modding by the buffer size
        // This way it will wrap around to the beginning of the buffer if it reaches the end
        buffer_out = (buffer_out + 1) % CIRCULAR_BUFFER_SIZE;
        // Decrement the buffer_counter since we consumed an element
        buffer_counter--;

        // Signal producer that buffer is not full
        pthread_cond_signal(&buffer_not_full);

        // Release mutex
        pthread_mutex_unlock(&mutex);
    }

    // Output that the consumer is done
    printf("Consumer: done\n");
    return NULL;
}

int main()
{
    char input[MAX_INPUT + 1];

    while (1)
    {
        // Reset variables for next iteration
        buffer_counter = 0;
        buffer_in = 0;            // used to keep track of the next position of the buffer that the producer will write to
        buffer_out = 0;           // used to keep track of the next position of the buffer that the consumer will read from
        completed_production = 0; // used to indicate that the producer has finished producing

        // Program asks for user input
        printf("Enter input (type 'exit' to quit): ");

        // Reads input from user, if input is NULL, break
        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        // Remove newline character if present
        input[strcspn(input, "\n")] = '\0';

        // Clear input buffer if input is too long
        if (strlen(input) == MAX_INPUT && input[MAX_INPUT - 1] != '\n')
        {
            int c;
            while ((c = getchar()) != '\n' && c != EOF)
                ; // flush out the buffer by reading characters with getchar until '\n' or EOF
        }

        // If the user enters "exit", the parent process will indicate it is done and break
        if (strcmp(input, "exit") == 0)
        {
            printf("Parent: done\n"); // Print that the parent process is done
            break;
        }

        // DISPLAY OUTPUT
        int input_length = strlen(input);               // Get the length of the input
        printf("Input: %s\n", input);                   // Print the input
        printf("Count: %d characters\n", input_length); // Print the length of the input

        // Create producer and consumer threads
        pthread_t prod_thread, cons_thread;

        // Initialize threads
        pthread_create(&prod_thread, NULL, producer, input);
        pthread_create(&cons_thread, NULL, consumer, NULL);

        // Wait for threads to complete
        pthread_join(prod_thread, NULL);
        pthread_join(cons_thread, NULL);
    }

    // Destroy mutex and condition variables
    pthread_mutex_destroy(&mutex);           // Destroy the mutex used for synchronization
    pthread_cond_destroy(&buffer_not_full);  // Destroy the condition variable used for synchronization
    pthread_cond_destroy(&buffer_not_empty); // Destroy the condition variable used for synchronization

    return 0;
}