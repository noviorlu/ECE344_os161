/*
 * catlock.c
 *
 * Please use LOCKS/CV'S to solve the cat syncronization problem in 
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

enum spec{CAT, MOUSE};

enum spec species;
int catInQueue;
int mouseInQueue;
struct bowl{
        int count;
        int bowl[NFOODBOWLS];
}Bowl;

struct lock* lock_beforeCatEat;
struct lock* lock_afterCatEat;
struct lock* lock_beforMouseEat;
struct lock* lock_afterMouseEat;
struct cv* cv_cat;
struct cv* cv_mouse;

/*
 * empty position is -1
 * returns bowl index Occupyed by current species,
 * and add 1 to occupycounter
 * if no empty position found returns -1
 */
static int bowl_occupyPos(int specIdx){
        int i;
        for(i = 0; i < NFOODBOWLS; i++){
                if(Bowl.bowl[i] == -1){
                       Bowl.bowl[i] = specIdx;
                       Bowl.count += 1;
                       return i;
                }
        }
        return -1;
}


/*
 * before the cat eat cv wait under condition
 * returns the occpuyed space index
*/
int beforeCatEat(unsigned long catnumber){
        lock_acquire(lock_beforeCatEat);

        catInQueue += 1;

        int idx;
        while(species == MOUSE || (idx = bowl_occupyPos(catnumber)) == -1){
                cv_wait(cv_cat, lock_beforeCatEat);
        }

        catInQueue -= 1;
        
        lock_release(lock_beforeCatEat);
        
        return idx;
}


/*
 * cat starvation problem solution
*/
void afterCatEat(int bowlIdx){
        assert(species == CAT);
        lock_acquire(lock_afterCatEat);

        Bowl.bowl[bowlIdx] = -1;
        Bowl.count -= 1;

        if(mouseInQueue > catInQueue){
                if(Bowl.count == 0){
                        species = MOUSE;
                        lock_release(lock_afterCatEat);
                        cv_broadcast(cv_mouse, lock_beforMouseEat); 
                }
                lock_release(lock_afterCatEat);

        }else{
                lock_release(lock_afterCatEat);
                cv_signal(cv_cat, lock_beforeCatEat); 
        }
}

/*
 * before the Mouse eat cv wait under condition
 * returns the occpuyed space index
*/
int beforeMouseEat(unsigned long mousenumber){
        lock_acquire(lock_beforMouseEat);

        mouseInQueue += 1;

        int idx;
        while(species == CAT || (idx = bowl_occupyPos(mousenumber)) == -1){
                cv_wait(cv_mouse, lock_beforMouseEat);
        }

        mouseInQueue -= 1;
        
        lock_release(lock_beforMouseEat);
        
        return idx;
}

/*
 * mosue starvation problem solution
*/
void afterMouseEat(int bowlIdx){
        assert(species == MOUSE);
        lock_acquire(lock_afterMouseEat);

        Bowl.bowl[bowlIdx] = -1;
        Bowl.count -= 1;

        if(mouseInQueue < catInQueue){
                if(Bowl.count == 0){
                        species = CAT;
                        lock_release(lock_afterMouseEat);
                        cv_broadcast(cv_cat, lock_beforeCatEat); 
                }
                lock_release(lock_afterMouseEat);

        }else{
                lock_release(lock_afterMouseEat);
                cv_signal(cv_mouse, lock_beforMouseEat); 
        }
}


void initglobals(void){
        if (lock_beforeCatEat==NULL) {
		lock_beforeCatEat = lock_create("lock_beforeCatEat");
		if (lock_beforeCatEat == NULL) {
			panic("catmouse: lock_beforeCatEat failed\n");
		}
	}if (lock_afterCatEat==NULL) {
		lock_afterCatEat = lock_create("lock_afterCatEat");
		if (lock_afterCatEat == NULL) {
			panic("catmouse: lock_afterCatEat failed\n");
		}
	}if (lock_beforMouseEat==NULL) {
		lock_beforMouseEat = lock_create("lock_beforMouseEat");
		if (lock_beforMouseEat == NULL) {
			panic("catmouse: lock_beforMouseEat failed\n");
		}
	}if (lock_afterMouseEat==NULL) {
		lock_afterMouseEat = lock_create("lock_afterMouseEat");
		if (lock_afterMouseEat == NULL) {
			panic("catmouse: lock_afterMouseEat failed\n");
		}
	}if (cv_cat==NULL) {
		cv_cat = cv_create("cv_cat");
		if (cv_cat == NULL) {
			panic("catmouse: cv_cat failed\n");
		}
	}if (cv_mouse==NULL) {
		cv_mouse = cv_create("cv_mouse");
		if (cv_mouse == NULL) {
			panic("catmouse: cv_mouse failed\n");
		}
	}
        int i;
        for(i = 0; i < NFOODBOWLS; i++){
                Bowl.bowl[i] = -1;
        }
        Bowl.count = 0;
        species = CAT;
        catInQueue = 0;
        mouseInQueue = 0;
}


/*
 * catlock()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS -
 *      1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using locks/cv's.
 *
 */

static
void
catlock(void * unusedpointer, 
        unsigned long catnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) unusedpointer;
        
        int bowl;
        int i;
        for(i = 0; i < NMEALS; i++){
                bowl = beforeCatEat(catnumber);
                catmouse_eat("cat", catnumber, bowl + 1, i);
                afterCatEat(bowl);
        }
}
	

/*
 * mouselock()
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
 *      Write and comment this function using locks/cv's.
 *
 */

static
void
mouselock(void * unusedpointer,
          unsigned long mousenumber)
{
        /*
         * Avoid unused variable warnings.
         */
        
        (void) unusedpointer;
        int bowl;
        int i;
        for(i = 0; i < NMEALS; i++){
                bowl = beforeMouseEat(mousenumber);
                catmouse_eat("mouse", mousenumber, bowl + 1, i);
                afterMouseEat(mousenumber);
        }
}


/*
 * catmouselock()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catlock() and mouselock() threads.  Change
 *      this code as necessary for your solution.
 */

int
catmouselock(int nargs,
             char ** args)
{
        int index, error;
        initglobals();
        /*
         * Start NCATS catlock() threads.
         */

        for (index = 0; index < NCATS; index++) {
           
                error = thread_fork("catlock thread", 
                                    NULL, 
                                    index, 
                                    catlock, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
                 
                        panic("catlock: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

        /*
         * Start NMICE mouselock() threads.
         */

        for (index = 0; index < NMICE; index++) {
   
                error = thread_fork("mouselock thread", 
                                    NULL, 
                                    index, 
                                    mouselock, 
                                    NULL
                                    );
      
                /*
                 * panic() on error.
                 */

                if (error) {
         
                        panic("mouselock: thread_fork failed: %s\n", 
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
        kprintf("catlock test done\n");

        return 0;
}

