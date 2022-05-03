/*
 * catsem.c
 *
 * Please use SEMAPHORES to solve the cat syncronization problem in 
 * this file.
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include "catmouse.h"
#include <synch.h>



// static struct semaphore* bowl_count;
// static struct semaphore* mice_mutex;
// static struct semaphore* mutex;
// static int specWait;
// static int bowl_switch[NFOODBOWLS];

// static int assignBowl(void)
// {
//         P(mutex);
//         int i;
//         for(i=0; i<NFOODBOWLS; i++){
//                 if(bowl_switch[i] == 0){
//                         bowl_switch[i] = 1;
//                         V(mutex);
//                         return i;
//                 }
//         }
//         V(mutex);
//         return -1;
// }

/*
 * inital semaphores and iteration count that are used
 */
// static void initglobals(void)
// {
//         if (bowl_count==NULL) {
//                 bowl_count = sem_create("bowl_count", NFOODBOWLS);
//                 if (bowl_count == NULL) {
//                         panic("catsem: bowl_count failed\n");
//                 }
//         }
//         if (mice_mutex==NULL) {
//                 mice_mutex = sem_create("mice_mutex", 1);
//                 if (mice_mutex == NULL) {
//                         panic("catsem: mice_mutex failed\n");
//                 }
//         }if (mutex==NULL) {
//                 mutex = sem_create("mutex", 1);
//                 if (mutex == NULL) {
//                         panic("catsem: mutex failed\n");
//                 }
//         }
//         specWait = 0;
//         int i;
//         for(i=0; i<NFOODBOWLS; i++){
//                 bowl_switch[i] = 0;
//         }
// }


/*
 * catsem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using semaphores.
 *
 */
static
void
catsem(void * unusedpointer, 
       unsigned long catnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) unusedpointer;
        (void) catnumber;

        // int idx;
        // for (idx = 0; idx < NMEALS; idx++){

        //         P(mutex);
                
        //         specWait += 1;
        //         if(specWait == 1){
        //                 P(mice_mutex);
        //         }

        //         V(mutex);

        //         P(bowl_count);
        //         int bowl = assignBowl();
        //         assert(bowl != -1);

        //         catmouse_eat("cat", catnumber, bowl+1, idx);

        //         bowl_switch[bowl] = 0;
        //         V(bowl_count);

        //         P(mutex);
        //         specWait -= 1;
        //         if(specWait == 0){
        //                 V(mice_mutex);
        //         }
        //         V(mutex);

        // }
        
}
        

/*
 * mousesem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds the mouse identifier from 0 to 
 *              NMICE - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using semaphores.
 *
 */

static
void
mousesem(void * unusedpointer, 
         unsigned long mousenumber)
{
        
        
        /*
         * Avoid unused variable warnings.
         */
        
        (void) unusedpointer;
        (void) mousenumber;

        // int idx;
        // for (idx = 0; idx < NMEALS; idx++){

        //         P(mice_mutex);
                
        //         P(bowl_count);
        //         int bowl = assignBowl();
        //         assert(bowl != -1);

        //         catmouse_eat("mouse", mousenumber, bowl+1, idx);
                
        //         bowl_switch[bowl] = 0;
        //         V(bowl_count);

        //         V(mice_mutex);
        
        // }
}


/*
 * catmousesem()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catsem() and mousesem() threads.  Change this 
 *      code as necessary for your solution.
 */

int
catmousesem(int nargs,
            char ** args)
{
        int index, error;
        
        //initglobals();
        /*
         * Start NCATS catsem() threads.
         */
        for (index = 0; index < NCATS; index++) {
           
                error = thread_fork("catsem Thread", 
                                    NULL, 
                                    index, 
                                    catsem, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
                 
                        panic("catsem: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }
        
        /*
         * Start NMICE mousesem() threads.
         */

        for (index = 0; index < NMICE; index++) {
   
                error = thread_fork("mousesem Thread", 
                                    NULL, 
                                    index, 
                                    mousesem, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
         
                        panic("mousesem: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

        /*
         * wait until all other threads finish
         */

        while (thread_count() > 1)
                thread_yield();

        (void)nargs;
        (void)args;
        kprintf("catsem test done\n");

        return 0;
}

