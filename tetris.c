/* Micro Tetris, based on an obfuscated tetris, 1989 IOCCC Best Game
 *
 * Copyright (c) 1989  John Tromp <john.tromp@gmail.com>
 * Copyright (c) 2009, 2010 Joachim Nilsson <joachim.nilsson@vmlinux.org>
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
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>

#include "conio.h"
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

/* These can be overridden by the user. */
#define DEFAULT_KEYS "jkl pq"
#define KEY_LEFT   0
#define KEY_RIGHT  2
#define KEY_ROTATE 1
#define KEY_DROP   3
#define KEY_PAUSE  4
#define KEY_QUIT   5

char *keys = DEFAULT_KEYS;
int level = 1;
int score = 0;
int board[B_SIZE];

int *peek_shape;                /* peek preview of next shape */
int *shape;
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
      h[3] = 500000;
   }

   h[3] -= h[3] / 3000;
   setitimer (0, (struct itimerval *)h, 0);
}

int update (void)
{
   int x, y;
#ifdef ENABLE_PREVIEW
   int preview[10 * 5];

   /* Display piece preview. */
   memset (preview, 0, sizeof(preview));
   preview[1 * 10 + 1] = 7;
   preview[1 * 10 + 1 + peek_shape[1]] = 7;
   preview[1 * 10 + 1 + peek_shape[2]] = 7;
   preview[1 * 10 + 1 + peek_shape[3]] = 7;

   for (y = 0; y < 5; y++)
   {
      for (x = 0; x < 10; x++)
      {
         gotoxy (x * 2 + 26 + 28, y + 5);
         printf ("\e[%dm  ", preview[y * 10 + x]);
      }
   }
#endif

   /* Display board. */
   for (y = 0; y < B_ROWS; y++)
   {
      for (x = 0; x < B_COLS; x++)
      {
         gotoxy (x * 2 + 28, y);
         printf ("\e[%dm  ", board[y * B_COLS + x]);
      }
   }

#ifdef ENABLE_SCORE
   /* Display current level and score */
   textattr(RESETATTR);
   gotoxy (26 + 28, 2);
   printf ("Level  : %d", level);
   gotoxy (26 + 28, 3);
   printf ("Score  : %d", score);
   gotoxy (26 + 28, 4);
   printf ("Preview:");
#endif

   return getchar ();
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

int *next_shape (void)
{
   int *next = peek_shape;

   peek_shape = &shapes[rand () % 7 * 4];
   if (!next)
   {
      return next_shape ();
   }

   return next;
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

   hidecursor();

   /* "stty cbreak -echo stop p" */
   modmodes = savemodes;
   modmodes.c_lflag &= ~ICANON;
   modmodes.c_lflag &= ~ECHO;
   modmodes.c_cc[VMIN] = 1;
   modmodes.c_cc[VTIME] = 0;
   modmodes.c_cc[VSTOP] = keys[KEY_PAUSE];

   return tcsetattr(fileno(stdin), TCSANOW, &modmodes);
}

int tty_fix (void)
{
   if (!havemodes)
   {
      return 0;
   }

   showcursor();

   /* "stty sane" */
   return tcsetattr(fileno(stdin), TCSANOW, &savemodes);
}

int main (int argc __attribute__ ((unused)), char *argv[] __attribute__ ((unused)))
{
   int c = 0, i, j, *ptr;
   int pos = 17;
   int *backup;
   sigset_t set;
   struct sigaction action;

   /* Initialize board */
   ptr = board;
   for (i = B_SIZE; i; i--)
   {
      *ptr++ = i < 25 || i % B_COLS < 2 ? 7 : 0;
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

   clrscr ();
   shape = next_shape ();
   while (1)
   {
      if (c < 0)
      {
         if (fits_in (shape, pos + B_COLS))
         {
            pos += B_COLS;
         }
         else
         {
            place (shape, pos, 7);
            ++score;
            for (j = 0; j < 252; j = B_COLS * (j / B_COLS + 1))
            {
               for (; board[++j];)
               {
                  if (j % B_COLS == 10)
                  {
                     for (; j % B_COLS; board[j--] = 0);
                     c = update ();
                     for (; --j; board[j + B_COLS] = board[j]);
                     c = update ();
                  }
               }
            }
            shape = next_shape ();
            if (!fits_in (shape, pos = 17))
               c = keys[KEY_QUIT];
         }
      }
      if (c == keys[KEY_LEFT])
      {
         if (!fits_in (shape, --pos))
            ++pos;
      }
      if (c == keys[KEY_ROTATE])
      {
         backup = shape;
         shape = &shapes[4 * *shape]; /* Rotate */
         /* Check if it fits, if not restore shape from backup */
         if (!fits_in (shape, pos))
            shape = backup;
      }

      if (c == keys[KEY_RIGHT])
      {
         if (!fits_in (shape, ++pos))
            --pos;
      }
      if (c == keys[KEY_DROP])
      {
         for (; fits_in (shape, pos + B_COLS); ++score)
         {
            pos += B_COLS;
         }
      }
      if (c == keys[KEY_PAUSE] || c == keys[KEY_QUIT])
      {
         sigprocmask (SIG_BLOCK, &set, NULL);
         clrscr();
         gotoxy(0,0);
         textattr(RESETATTR);
         printf ("Level: %d, Score: %d\n", level, score);
         if (c == keys[KEY_QUIT])
            break;

         while (getchar () - keys[KEY_PAUSE])
            ;

//         puts ("\e[H\e[J\e[7m");
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
