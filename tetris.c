/* http://homepages.cwi.nl/~tromp/tetris.html */

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <signal.h>

#include "tetris.h"

long h[4];

void alarm_handler (int signal __attribute__ ((unused)))
{
   h[3] -= h[3] / 3000;
   setitimer (0, (struct itimerval *)h, 0);
} 

int c, level, score, s, I, K = 0, j, k, board[B_SIZE], Q[B_SIZE], *n = board, *m;

#define      TL     -B_COLS-1       /* top left */
#define      TC     -B_COLS         /* top center */
#define      TR     -B_COLS+1       /* top right */
#define      ML     -1              /* middle left */
#define      MR     1               /* middle right */
#define      BL     B_COLS-1        /* bottom left */
#define      BC     B_COLS          /* bottom center */
#define      BR     B_COLS+1        /* bottom right */

int shapes[] = {
    7,  TL,  TC,  MR, 
    8,  TR,  TC,  ML, 
    9,  ML,  MR,  BC, 
    3,  TL,  TC,  ML,
   12,  ML,  BL,  MR,
   15,  ML,  BR,  MR,
   18,  ML,  MR,   2,           /* sticks out */
    0,  TC,  ML,  BL,
    1,  TC,  MR,  BR,
   10,  TC,  MR,  BC,
   11,  TC,  ML,  MR,
    2,  TC,  ML,  BC,
   13,  TC,  BC,  BR,
   14,  TR,  ML,  MR, 
    4,  TL,  TC,  BC,
   16,  TR,  TC,  BC,
   17,  TL,  MR,  ML, 
    5,  TC,  BC,  BL,
    6,  TC,  BC,  2 * B_COLS,   /* sticks out */
};

void update ()
{
   int i;

   for (i = 11; ++i < 264;)
   {
      if ((k = board[i]) - Q[i])
      {
         Q[i] = k;
         if (i - ++I || i % 12 < 1) 
         {
            printf ("\033[%d;%dH", (I = i) / 12, i % 12 * 2 + 28);
         }
         printf ("\033[%dm  " + (K - k ? 0 : 5), k);
         K = k;
      }
   }
   Q[263] = c = getchar ();
}

int fits_in (int b)
{
   int i;

   for (i = 4; i; i--)
   {
      if (board[i ? b + n[i] : b])
         return 0;
   }

   return 1;
}

void place (int pos, int onoff)
{
   int i;

   for (i = 4; i; i--)
   {
      board[i ? pos + n[i] : pos] = onoff;
   }
}

int main (int argc, char *argv[])
{
   int i;
   int pos = 17;                /* start position */
   char *keys = "jkl pq";
   sigset_t set;
   struct sigaction action;

   h[3] = 1000000 / (level = 2);

   for (i = B_SIZE; i; i--)
      *n++ = i < 25 || i % 12 < 2 ? 7 : 0;

   srand (getpid ());
   system ("stty cbreak -echo stop u");

   /* Set up signal set with just SIGALRM. */
   sigemptyset(&set);
   sigaddset(&set, SIGALRM);

   /* Trap SIGALRM. */
   sigemptyset(&action.sa_mask);
   sigaddset(&action.sa_mask, SIGALRM);
   action.sa_flags = 0;
   action.sa_handler = alarm_handler;
   sigaction(SIGALRM, &action, NULL);

   /* Call it once to start the timer. */
   alarm_handler (0);

   puts ("\033[H\033[J");
   n = shapes + rand () % 7 * 4;
   while (1)
   {
      if (c < 0)
      {
         if (fits_in (pos + 12))
         {
            pos += 12;
         }
         else
         {
            place (pos, 7);
            ++score;
            for (j = 0; j < 252; j = 12 * (j / 12 + 1))
            {
               for (; board[++j];)
               {
                  if (j % 12 == 10)
                  {
                     for (; j % 12; board[j--] = 0);
                     update ();
                     for (; --j; board[j + 12] = board[j]);
                     update ();
                  }
               }
            }
            n = shapes + rand () % 7 * 4;
            fits_in (pos = 17) || (c = keys[5]);
         }
      }
      if (c == keys[0])         /* j - "left" */
         fits_in (--pos) || ++pos;

      if (c == keys[1])         /* k - "rotate" */
         n = f + 4 ** (m = n), fits_in (pos) || (n = m);

      if (c == keys[2])         /* l - "right" */
         fits_in (++pos) || --pos;

      if (c == keys[3])         /* Space - "drop" */
      {
         for (; fits_in (pos + 12); ++score)
         {
            pos += 12;
         }
      }
      if (c == keys[4] || c == keys[5])
      {
         sigprocmask (SIG_BLOCK, &set, NULL);
         printf ("\033[H\033[J\033[0mLevel: %d, Score: %d\n", level, score);
         if (c == keys[5])
            break;

         for (j = 264; j--; Q[j] = 0)
            ;

         while (getchar () - keys[4])
            ;

         puts ("\033[H\033[J\033[7m");
         sigprocmask (SIG_UNBLOCK, &set, NULL);
      }

      place (pos, 7);
      update ();
      place (pos, 0);
   }

   system ("stty sane");

   return 0;
}

/**
 * Local Variables:
 *  version-control: t
 *  c-file-style: "ellemtel"
 * End:
 */
