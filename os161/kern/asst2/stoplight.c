/* 
 * stoplight.c
 *
 * You can use any synchronization primitives available to solve
 * the stoplight problem in this file.
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
#include <synch.h>

//      NW:0    NE:3
//      SW:1    SE:2
static struct semaphore* approach_mutex[4];
static struct semaphore* region_mutex[4];
static struct semaphore* atleasttwoR;

/*
 * Number of cars created.
 */

#define NCARS 20


/*
 *
 * Function Definitions
 *
 */

static const char *directions[] = { "N", "W", "S", "E" };

static const char *msgs[] = {
        "approaching:",
        "region1:    ",
        "region2:    ",
        "region3:    ",
        "leaving:    "
};

/* use these constants for the first parameter of message */
enum { APPROACHING, REGION1, REGION2, REGION3, LEAVING };

static void
message(int msg_nr, int carnumber, int cardirection, int destdirection)
{
        kprintf("%s car = %2d, direction = %s, destination = %s\n",
                msgs[msg_nr], carnumber,
                directions[cardirection], directions[destdirection]);
}

/*
 * initalize 4 approach mutex, 4 region mutex, 1 semaphore
*/
static void initglobals(void){
        char approc[32];
        char region[32];
        int i;
        for (i = 0; i < 4; i++)
        {
                snprintf(approc, sizeof(approc), "approch%d", i);
                snprintf(region, sizeof(region), "region%d", i);
                if (approach_mutex[i]==NULL) {
                        approach_mutex[i] = sem_create(approc, 1);
                        if (approach_mutex[i] == NULL) {
                                panic("stoplight: %s failed\n", approc);
                        }
                }
                if (region_mutex[i]==NULL) {
                        region_mutex[i] = sem_create(region, 1);
                        if (region_mutex[i] == NULL) {
                                panic("stoplight: %s failed\n", region);
                        }
                }
        }
        if (atleasttwoR==NULL) {
                atleasttwoR = sem_create("atleasttwoR", 3);
                if (atleasttwoR == NULL) {
                        panic("stoplight: atleasttwoR failed\n");
                }
        }
        
}


/*
 * gostraight()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement passing straight through the
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
gostraight(unsigned long cardirection,
           unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */
        P(atleasttwoR);

        int destdirection;
        destdirection = (cardirection+2) % 4;
        
        int pos1, pos2;
        pos1 = cardirection;
        pos2 = (cardirection+1)%4;

        
        P(approach_mutex[pos1]);

                message(0, carnumber, cardirection, destdirection);


        P(region_mutex[pos1]);    

                        message(1, carnumber, cardirection, destdirection);


        V(approach_mutex[pos1]);
        P(region_mutex[pos2]);

                        message(2, carnumber, cardirection, destdirection);


        V(region_mutex[pos1]);
        V(atleasttwoR);

                message(4, carnumber, cardirection, destdirection);

        
        V(region_mutex[pos2]);
}


/*
 * turnleft()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a left turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnleft(unsigned long cardirection,
         unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */
        P(atleasttwoR);

        int destdirection;
        destdirection = (cardirection+3) % 4;
        
        int pos1, pos2, pos3;
        pos1 = cardirection;
        pos2 = (cardirection+1)%4;
        pos3 = (cardirection+2)%4;


        P(approach_mutex[pos1]);

                message(0, carnumber, cardirection, destdirection);


        P(region_mutex[pos1]);  

                        message(1, carnumber, cardirection, destdirection);



        V(approach_mutex[pos1]);
        P(region_mutex[pos2]);

                        message(2, carnumber, cardirection, destdirection);


        V(region_mutex[pos1]);
        P(region_mutex[pos3]);
        

                        message(3, carnumber, cardirection, destdirection);


        V(region_mutex[pos2]);
        V(atleasttwoR);

                        message(4, carnumber, cardirection, destdirection);


        V(region_mutex[pos3]);                 

}


/*
 * turnright()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a right turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnright(unsigned long cardirection,
          unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        int destdirection;
        destdirection = (cardirection+1) % 4;

        int pos1;
        pos1 = cardirection;

        P(approach_mutex[pos1]);                     

                message(0, carnumber, cardirection, destdirection);


        P(region_mutex[pos1]);

                        message(1, carnumber, cardirection, destdirection);
        

        V(approach_mutex[pos1]);

                message(4, carnumber, cardirection, destdirection);


        V(region_mutex[pos1]);               
}


/*
 * approachintersection()
 *
 * Arguments: 
 *      void * unusedpointer: currently unused.
 *      unsigned long carnumber: holds car id number.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Change this function as necessary to implement your solution. These
 *      threads are created by createcars().  Each one must choose a direction
 *      randomly, approach the intersection, choose a turn randomly, and then
 *      complete that turn.  The code to choose a direction randomly is
 *      provided, the rest is left to you to implement.  Making a turn
 *      or going straight should be done by calling one of the functions
 *      above.
 */
 
static
void
approachintersection(void * unusedpointer,
                     unsigned long carnumber)
{
        int cardirection;
        int carturn;
        /*
         * Avoid unused variable and function warnings.
         */

        (void) unusedpointer;

        /*
         * cardirection/carturn is set randomly.
         * cardirection N:0 W:1 S:2 E:3 
         * carturn straight: 0 left: 1 right: 2
         */

        cardirection = random() % 4;
        carturn = random() % 3;


        switch (carturn)
        {
        case 0:
                gostraight(cardirection, carnumber);
                break;
        case 1:
                turnleft(cardirection, carnumber);
                break;
        case 2:
                turnright(cardirection, carnumber);
                break;
        }
}


/*
 * createcars()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up the approachintersection() threads.  You are
 *      free to modiy this code as necessary for your solution.
 */

int
createcars(int nargs,
           char ** args)
{
        int index, error;
        
        // initalize all mutex, steps_counter and deadlock cv
        initglobals();

        /*
         * Start NCARS approachintersection() threads.
         */

        for (index = 0; index < NCARS; index++) {
                error = thread_fork("approachintersection thread",
                                    NULL, index, approachintersection, NULL);

                /*
                * panic() on error.
                */

                if (error) {         
                        panic("approachintersection: thread_fork failed: %s\n",
                              strerror(error));
                }
        }
        
        /*
         * wait until all other threads finish
         */

        while (thread_count() > 1)
                thread_yield();

	(void)message;
        (void)nargs;
        (void)args;
        kprintf("stoplight test done\n");
        return 0;
}

