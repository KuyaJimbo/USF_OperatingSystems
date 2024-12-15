/*************************************************************************
 * Group 4:     Kimberly Jean Baptiste      NetID: jeanbaptistek@usf.edu *
 *              James Ocampo                NetID: jamesocampo@usf.edu   *
 *              Jorge Carbajal-Hernandez    NetID: jorge250@usf.edu      *
 *                                                                       *
 * Description: This program enhances the file                           *
 *              compression program by using threads to                  *
 *              improve the speed and efficiency of the                  *
 *              operation.                                               *          
 *************************************************************************/

#include <dirent.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <time.h>
#include <pthread.h>

#define BUFFER_SIZE 1048576 // 1MB
#define MAX_THREADS 8

// structure of thread arguments
struct thread_function_args {
    pthread_mutex_t *lock;
    FILE *f_out;
    int *total_in; 
    int *total_out;
    char *full_path;
    unsigned char *write_buffer_out;  
    int write_nbytes_zipped;            
}; 

int cmp(const void *a, const void *b) {
    return strcmp(*(char **) a, *(char **) b);
}

/*******************************************************************
 * thread_function_args: This function initializes the arguments   *
 * required for the worker function and passes them to the thread. * 
 * Since the pthread_create function allows only a single argument *
 * to be passed, a struct is used to encapsulate all the necessary *
 * data for the worker function (frame_compression).               *
 * *****************************************************************/
struct thread_function_args* initialize_thread_args(pthread_mutex_t *lock, FILE *f_out, int *total_in, int *total_out, char *directory, char *filename) {
    struct thread_function_args *args = malloc(sizeof(struct thread_function_args));
    assert(args != NULL);

    // intialize thread variables 
    args->lock = lock;
    args->f_out = f_out;
    args->total_in = total_in;
    args->total_out = total_out;

    // construct path to PPM frames 
    int len = strlen(directory) + strlen(filename) + 2;
    args->full_path = malloc(len * sizeof(char));
    assert(args->full_path != NULL);
    strcpy(args->full_path, directory);
    strcat(args->full_path, "/");
    strcat(args->full_path, filename);

    args->write_buffer_out = NULL;
    args->write_nbytes_zipped = 0;

    return args;
}

/********************************************************************
 * FRAME_COMPRESSION: This function acts as the worker function for *
 * the threads. It takes the thread_args struct containing the data *
 * required to perform file compression on the image frames. Since  *
 * the argument is of type void, the struct variable is typecast.   *
 * To prevent race conditions and ensure accurate incrementation,   *
 * the counters are updated using a mutex_lock.                     *
 * ******************************************************************/
void* frame_compression(void *thread_args) {
    struct thread_function_args *args = (struct thread_function_args*)thread_args;
    unsigned char buffer_in[BUFFER_SIZE];
	unsigned char buffer_out[BUFFER_SIZE];

    // load file
    FILE *f_in = fopen(args->full_path, "r");
    assert(f_in != NULL);
    int nbytes = fread(buffer_in, sizeof(unsigned char), BUFFER_SIZE, f_in);
    fclose(f_in);
    
    // zip file
    z_stream strm;
    int ret = deflateInit(&strm, 9);
    strm.avail_in = nbytes;
    strm.next_in = buffer_in;
    strm.avail_out = BUFFER_SIZE;
    strm.next_out = buffer_out;
    
    ret = deflate(&strm, Z_FINISH);
    assert(ret == Z_STREAM_END);
    
    // size of compressed data stored in buffer_out
    int nbytes_zipped = BUFFER_SIZE - strm.avail_out; 
    // store compressed data size for access in join_write function
    args->write_nbytes_zipped = nbytes_zipped;  
    args->write_buffer_out = malloc(nbytes_zipped);
    // copy buffer_out to write_buffer_out for access in join_write function
    memcpy(args->write_buffer_out, buffer_out, nbytes_zipped); 
   
    // mutex ensures safe incrementing of counters and stores 
    //compressed data sizE for access in the join_and_write function
    pthread_mutex_lock(args->lock);
    *args->total_in += nbytes;
    *args->total_out += nbytes_zipped;
    pthread_mutex_unlock(args->lock);
    
    return NULL;
}

/********************************************************************
 * CREATE_THREADS: This function prepares and initializes the       *
 * thread data before creating the threads. The loop runs a total   *
 * of MAX_THREAD times and nfiles - i times.The frame index tracks  *
 * the next group of files to be processed, ensuring proper         *
 * management of the 100 total files. By processing only MAX_THREAD *
 * files at a time, the function limits the number of threads       *
 * running simultaneously.                                          *
 ********************************************************************/
void create_threads(pthread_t *threads, struct thread_function_args *thread_args, int num_threads, pthread_mutex_t *lock, FILE *f_out, int *total_in, int *total_out,  char *directory, char **files, int frame_index){
    for (int j = 0; j < num_threads; j++) {
        //initiliaze thread arguments
        thread_args[j] = *initialize_thread_args(lock, f_out, total_in, total_out, directory, files[frame_index + j]);
        // Create thread
        pthread_create(&threads[j], NULL, frame_compression, &thread_args[j]);
    }
}

/*******************************************************************
 * JOIN_AND_WRITE: This function is responsible for joining the    *
 * threads. Once the threads have been joined, it writes the data  *
 *  to the video.zip output file. After joining the threads, the   *
 * data is written to the video.zip output file.                   *
 * *****************************************************************/
void join_and_write(pthread_t *threads, struct thread_function_args *thread_args,  int num_threads, FILE *f_out) {
    for (int j = 0; j < num_threads; j++) {
        pthread_join(threads[j], NULL);

        // write data to video.zip file
        fwrite(&thread_args[j].write_nbytes_zipped, sizeof(int), 1, f_out);
        fwrite(thread_args[j].write_buffer_out, sizeof(unsigned char), thread_args[j].write_nbytes_zipped, f_out);

    }
}

int main(int argc, char **argv) {
    // time computation header
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);
    // end of time computation header

	// do not modify the main function before this point!

	assert(argc == 2);
    
    DIR *d;
    struct dirent *dir;
    char **files = NULL;
    int nfiles = 0;
    
    d = opendir(argv[1]);
    if (d == NULL) {
        printf("An error has occurred\n");
        return 0;
    }

    // create sorted list of PPM files
    while ((dir = readdir(d)) != NULL) {
        files = realloc(files, (nfiles + 1) * sizeof(char *));
        assert(files != NULL);

        int len = strlen(dir->d_name);
        if (len > 4 && dir->d_name[len - 4] == '.' && dir->d_name[len - 3] == 'p' && dir->d_name[len - 2] == 'p' && dir->d_name[len - 1] == 'm') {
            files[nfiles] = strdup(dir->d_name);
            assert(files[nfiles] != NULL);
            
            nfiles++;
        }
    }
    closedir(d);
    qsort(files, nfiles, sizeof(char*), cmp);

    // initialize variables for file compression
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; 
    int total_in = 0, total_out = 0;
    FILE *f_out = fopen("video.vzip", "w");
    assert(f_out != NULL);

    // store thread IDs and arguments
    pthread_t threads[MAX_THREADS];
    struct thread_function_args *thread_args = NULL;
    
    // process all PPM files
    for (int i = 0; i < nfiles; i += MAX_THREADS) {
        int num_threads; 

        // calculate the number of threads needed to process the current 
        // group of PPM files files are processed in groups of MAX_THREADS (8)
        if (i + MAX_THREADS <= nfiles) {
            num_threads = MAX_THREADS;
        } else {
            num_threads = nfiles - i;
        }
        
        // allocate memory for thread arguments
        thread_args = malloc(num_threads * sizeof(struct thread_function_args));
        assert(thread_args != NULL);

        // function call for thread creation
        create_threads(threads, thread_args, num_threads, &lock, f_out, &total_in, &total_out, argv[1], files, i);
        // function call to join threads and write to output file
        join_and_write(threads, thread_args, num_threads, f_out);
     
        free(thread_args);
       
    }
   
    fclose(f_out);
    printf("Compression rate: %.2lf%%\n", 100.0 * ((total_in - total_out)) / total_in);

    // release list of files
    for (int i = 0; i < nfiles; i++) {
        free(files[i]);
    }
    free(files);

    // time computation footer
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Time: %.2f seconds\n", ((double)end.tv_sec + 1.0e-9 * end.tv_nsec)-((double)start.tv_sec + 1.0e-9 * start.tv_nsec));
     // end of time computation footer

    return 0;
}
