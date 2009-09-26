/* Micro Tetris, based on an obfuscated tetris, 1989 IOCCC Best Game
 * 
 * Copyright (c) 1989, John Tromp <john.tromp@gmail.com>
 * Copyright (c) 2009, Joachim Nilsson <joachim.nilsson@vmlinux.org>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * See the following URLs for more information, first John Tromp's page about
 * the game http://homepages.cwi.nl/~tromp/tetris.html then there's the entry
 * page at IOCCC http://www.ioccc.org/1989/tromp.hint
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>

#include "tetris.h"

static struct termios savemodes;
static int havemodes = 0;

#define      TL     -B_COLS-1       /* top left */
#define      TC     -B_COLS         /* top center */
#define      TR     -B_COLS+1       /* top right */
#define      ML     -1              /* middle left */
#define      MR     1               /* middle right */
#define      BL     B_COLS-1        /* bottom left */
#define      BC     B_COLS          /* bottom center */
#define      BR     B_COLS+1        /* bottom right */

int level = 0;
int board[B_SIZE], shadow[B_SIZE];

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

void alarm_handler (int signal __attribute__ ((unused)))
{
   static long h[4];

   if (!signal)
   {
      /* On init from main() */
      h[3] = 1000000 / (level = 2);
   }

   h[3] -= h[3] / 3000;
   setitimer (0, (struct itimerval *)h, 0);
} 

int update (void)
{
   int i, I = 0, k;
   static int K = 0;

   for (i = 11; ++i < 264;)
   {
      if ((k = board[i]) - shadow[i])
      {
         shadow[i] = k;
         if (i - ++I || i % 12 < 1)
            printf ("\e[%d;%dH", (I = i) / 12, i % 12 * 2 + 28);
         printf ("\e[%dm  " + (K - k ? 0 : 5), k);
         K = k;
      }
   }
   shadow[263] = k = getchar ();

   return k;
}

int fits_in (int *shape, int pos)
{
   if (board[pos] || board[pos + shape[1]] || 
       board[pos + shape[2]]  || board[pos + shape[3]])
   {
      return 0;
   }

   return 1;
}

void place (int *shape, int pos, int b)
{
   board[pos] = b;
   board[pos + shape[1]] = b;
   board[pos + shape[2]] = b;
   board[pos + shape[3]] = b;
}

/* Code stolen from http://c-faq.com/osdep/cbreak.html */
int tty_break (void)
{
   struct termios modmodes;

   if (tcgetattr(fileno(stdin), &savemodes) < 0)
   {
      return -1;
   }
   havemodes = 1;

   /* "stty cbreak -echo stop p" */
   modmodes = savemodes;
   modmodes.c_lflag &= ~ICANON;
   modmodes.c_lflag &= ~ECHO;
   modmodes.c_cc[VMIN] = 1;
   modmodes.c_cc[VTIME] = 0;
   modmodes.c_cc[VSTOP] = 'p'; /* Pause button */

   return tcsetattr(fileno(stdin), TCSANOW, &modmodes);
}

int tty_fix (void)
{
   if (!havemodes)
   {
      return 0;
   }

   /* "stty sane" */
   return tcsetattr(fileno(stdin), TCSANOW, &savemodes);
}

int main (int argc __attribute__ ((unused)), char *argv[] __attribute__ ((unused)))
{
   int c = 0, i, j, *ptr;
   int pos = 17;
   int score = 0;
   int *backup, *shape;
   char *keys = "jkl pq";
   sigset_t set;
   struct sigaction action;

   /* Initialize board */
   ptr = board;
   for (i = B_SIZE; i; i--)
   {
      *ptr++ = i < 25 || i % 12 < 2 ? 7 : 0;
   }

   srand ((unsigned int)time (NULL));
   if (tty_break () == -1)
   {
      return 1;
   }

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

   puts ("\e[H\e[J");
   shape = &shapes[rand () % 7 * 4];
   while (1)
   {
      if (c < 0)
      {
         if (fits_in (shape, pos + 12))
         {
            pos += 12;
         }
         else
         {
            place (shape, pos, 7);
            ++score;
            for (j = 0; j < 252; j = 12 * (j / 12 + 1))
            {
               for (; board[++j];)
               {
                  if (j % 12 == 10)
                  {
                     for (; j % 12; board[j--] = 0);
                     c = update ();
                     for (; --j; board[j + 12] = board[j]);
                     c = update ();
                  }
               }
            }
            shape = &shapes[rand () % 7 * 4];
            if (!fits_in (shape, pos = 17))
               c = keys[5];
         }
      }
      if (c == keys[0])         /* j - "left" */
      {
         if (!fits_in (shape, --pos))
            ++pos;
      }
      if (c == keys[1])         /* k - "rotate" */
      {
         backup = shape;
         shape = &shapes[4 * *shape]; /* Rotate */
         /* Check if it fits, if not restore shape from backup */
         if (!fits_in (shape, pos))
            shape = backup;
      }

      if (c == keys[2])         /* l - "right" */
      {
         if (!fits_in (shape, ++pos))
            --pos;
      }
      if (c == keys[3])         /* Space - "drop" */
      {
         for (; fits_in (shape, pos + 12); ++score)
         {
            pos += 12;
         }
      }
      if (c == keys[4] || c == keys[5])
      {
         sigprocmask (SIG_BLOCK, &set, NULL);
         printf ("\e[H\e[J\e[0mLevel: %d, Score: %d\n", level, score);
         if (c == keys[5])
            break;

         for (j = 264; j--; shadow[j] = 0)
            ;

         while (getchar () - keys[4])
            ;

         puts ("\e[H\e[J\e[7m");
         sigprocmask (SIG_UNBLOCK, &set, NULL);
      }

      place (shape, pos, 7);
      c = update ();
      place (shape, pos, 0);
   }

   if (tty_fix () == -1)
   {
      return 1;
   }

   return 0;
}

/**
 * Local Variables:
 *  version-control: t
 *  c-file-style: "ellemtel"
 * End:
 */
