/* Micro Tetris, based on an obfuscated tetris, 1989 IOCCC Best Game
 *
 * Copyright (c) 1989  John Tromp <john.tromp@gmail.com>
 * Copyright (c) 2009-2021  Joachim Wiberg <troglobit@gmail.com>
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

#include <paths.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define clrscr()       puts ("\033[2J\033[1;1H")
#define hidecursor()   puts ("\033[?25l")
#define showcursor()   puts ("\033[?25h")
#define bgcolor(c,s)   printf("\033[%dm" s, c ? c + 40 : 0)

#define SIGNAL(signo, cb)			\
	sigemptyset(&sa.sa_mask);		\
	sigaddset(&sa.sa_mask, signo);		\
	sa.sa_flags = 0;			\
	sa.sa_handler = cb;			\
	sigaction(signo, &sa, NULL)

/* the board */
#define B_COLS 12
#define B_ROWS 23
#define B_SIZE (B_ROWS * B_COLS)

/* min size */
#define MIN_COLS 40
#define MIN_ROWS 21

#define TL     -B_COLS-1	/* top left */
#define TC     -B_COLS		/* top center */
#define TR     -B_COLS+1	/* top right */
#define ML     -1		/* middle left */
#define MR     1		/* middle right */
#define BL     B_COLS-1		/* bottom left */
#define BC     B_COLS		/* bottom center */
#define BR     B_COLS+1		/* bottom right */

/* These can be overridden by the user. */
#define DEFAULT_KEYS "jkl pq"
#define KEY_LEFT   0
#define KEY_RIGHT  2
#define KEY_ROTATE 1
#define KEY_DROP   3
#define KEY_PAUSE  4
#define KEY_QUIT   5

static volatile sig_atomic_t running = 1;
static volatile sig_atomic_t resized = 0;

static struct termios savemodes;
static int havemodes = 0;

static char *keys = DEFAULT_KEYS;
static int level = 1;
static int points = 0;
static int lines_cleared = 0;
static int board[B_SIZE], shadow[B_SIZE];

static int ttcols;		/* probed terminal width */
static int ttrows;		/* probed terminal height */

static int *peek_shape;		/* peek preview of next shape */
static int  pcolor;
static int *shape;
static int  color;

static int shapes[] = {
	 7, TL, TC, MR, 2,	/* ""__   */
	 8, TR, TC, ML, 3,	/* __""   */
	 9, ML, MR, BC, 1,	/* "|"    */
	 3, TL, TC, ML, 4,	/* square */
	12, ML, BL, MR, 5,	/* |"""   */
	15, ML, BR, MR, 6,	/* """|   */
	18, ML, MR,  2, 7,	/* ---- sticks out */
	 0, TC, ML, BL, 2,	/* /    */
	 1, TC, MR, BR, 3,	/* \    */
	10, TC, MR, BC, 1,	/* |-   */
	11, TC, ML, MR, 1,	/* _|_  */
	 2, TC, ML, BC, 1,	/* -|   */
	13, TC, BC, BR, 5,	/* |_   */
	14, TR, ML, MR, 5,	/* ___| */
	 4, TL, TC, BC, 5,	/* "|   */
	16, TR, TC, BC, 6,	/* |"   */
	17, TL, MR, ML, 6,	/* |___ */
	 5, TC, BC, BL, 6,	/* _| */
	 6, TC, BC,  2 * B_COLS, 7, /* | sticks out */
};

static void gotoxy(int x, int y)
{
	int xpos = (ttcols - MIN_COLS) / 2;
	int ypos = (ttrows - MIN_ROWS) / 2;

	printf("\033[%d;%dH", ypos + y, xpos + x);
}


static void draw(int x, int y, int c)
{
	gotoxy(x, y);
	bgcolor(c, "  ");
}

static int update(void)
{
	int x, y;

#ifdef ENABLE_PREVIEW
	static int shadow_preview[B_COLS * 10] = { 0 };
	int preview[B_COLS * 10] = { 0 };
	const int start = 5;

	preview[2 * B_COLS + 1] = pcolor;
	preview[2 * B_COLS + 1 + peek_shape[1]] = pcolor;
	preview[2 * B_COLS + 1 + peek_shape[2]] = pcolor;
	preview[2 * B_COLS + 1 + peek_shape[3]] = pcolor;

	for (y = 0; y < 4; y++) {
		for (x = 0; x < B_COLS; x++) {
			if (preview[y * B_COLS + x] - shadow_preview[y * B_COLS + x]) {
				int c = preview[y * B_COLS + x]; /* color */

				shadow_preview[y * B_COLS + x] = c;
				draw(x * 2 + 26, start + y, c);
			}
		}
	}
#endif

	/* Display board. */
	for (y = 1; y < B_ROWS - 1; y++) {
		for (x = 0; x < B_COLS; x++) {
			if (board[y * B_COLS + x] - shadow[y * B_COLS + x]) {
				int c = board[y * B_COLS + x]; /* color */

				shadow[y * B_COLS + x] = c;
				draw(x * 2, y, c);
			}
		}
	}

	/* Update points and level */
	while (lines_cleared >= 10) {
		lines_cleared -= 10;
		level++;
	}

#ifdef ENABLE_SCORE
	/* Display current level and points */
	gotoxy(26, 2);
	printf("\033[0mLevel  : %d", level);
	gotoxy(26, 3);
	printf("Points : %d", points);
#endif
#ifdef ENABLE_PREVIEW
	gotoxy(26, 5);
	printf("Preview:");
#endif
	gotoxy(26, 10);
	printf("Keys:");
	fflush(stdout);

	return getchar();
}

/* Check if shape fits in the current position */
static int fits_in(int *s, int pos)
{
	if (board[pos] || board[pos + s[1]] || board[pos + s[2]] || board[pos + s[3]])
		return 0;

	return 1;
}

/* place shape at pos with color */
static void place(int *s, int pos, int c)
{
	board[pos] = c;
	board[pos + s[1]] = c;
	board[pos + s[2]] = c;
	board[pos + s[3]] = c;
}

static int *next_shape(void)
{
	int  pos  = rand() % 7 * 5;
	int *next = peek_shape;

	peek_shape = &shapes[pos];
	pcolor = peek_shape[4];
	if (!next)
		return next_shape();
	color = next[4];

	return next;
}

/* Try /var/games first, fallback to $HOME or even /tmp if not available */
static void show_high_score(void)
{
#ifdef ENABLE_HIGH_SCORE
	const char *score_name = "tetris.scores";
	char score_file[256];
	char buf[512];
	FILE *fp;

	snprintf(score_file, sizeof(score_file), "/var/games/%s", score_name);
	fp = fopen(score_file, "a");
	if (!fp) {
		const char *home = getenv("HOME");

		if (home) {
			snprintf(score_file, sizeof(score_file), "%s/.%s", home, score_name);
			fp = fopen(score_file, "a");
		}
	}

	if (!fp) {
		snprintf(score_file, sizeof(score_file), "%s%s", _PATH_TMP, score_name);
		fp = fopen(score_file, "a");
	}

	if (fp) {
		char temp_file[] = "/tmp/tetris-XXXXXX";
		const char *name = getenv("LOGNAME");
		int fd;

		if (!name) {
			name = getenv("USER");
			if (!name)
				name = "anonymous";
		}

		fprintf(fp, "%7d\t %5d\t  %3d\t%s\n", points * level, points, level, name);
		fclose(fp);

		fd = mkstemp(temp_file);
		if (fd != -1) {
			pid_t pid = fork();

			if (pid == 0) {
				dup2(fd, STDOUT_FILENO);
				close(fd);

				snprintf(buf, sizeof(buf), "cat %s |sort -rn |head -10", score_file);
				system(buf);
				exit(0);
			} else if (pid > 0) {
				int status;

				close(fd);
				waitpid(pid, &status, 0);

				if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
					snprintf(buf, sizeof(buf), "mv %s %s", temp_file, score_file);
					system(buf);
				}
			} else {
				close(fd);
			}

			remove(temp_file);
		}
	}

	fp = fopen(score_file, "r");
	if (fp) {
//		puts("\nHit RETURN to see high scores, ^C to skip.");
		fprintf(stderr, "  Score\tPoints\tLevel\tName\n");

		while (fgets(buf, sizeof(buf), fp))
			fputs(buf, stderr);

		fclose(fp);
	}
#endif /* ENABLE_HIGH_SCORE */
}

static void show_online_help(void)
{
	const int start = 11;

	gotoxy(26, start);
	puts("\033[0mj     - left");
	gotoxy(26, start + 1);
	puts("k     - rotate");
	gotoxy(26, start + 2);
	puts("l     - right");
	gotoxy(26, start + 3);
	puts("space - drop");
	gotoxy(26, start + 4);
	puts("p     - pause");
	gotoxy(26, start + 5);
	puts("q     - quit");
}


static int tty_size(void)
{
	struct pollfd fd = { STDIN_FILENO, POLLIN, 0 };
	struct winsize ws = { 0 };
	const char *cols, *lins;

	cols = getenv("COLUMNS");
	lins = getenv("LINES");
	if (cols && lins) {
		ttcols = atoi(cols);
		ttrows = atoi(lins);
	}

	if (!ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)) {
		ttcols = ws.ws_col;
		ttrows = ws.ws_row;
	} else {
		fprintf(stderr, "\e7\e[r\e[999;999H\e[6n");

		if (poll(&fd, 1, 300) > 0) {
			int row, col;

			if (scanf("\e[%d;%dR", &row, &col) == 2) {
				ttcols = col;
				ttrows = row;
			}
		}

		fprintf(stderr, "\e8");
	}

	if (ttcols < MIN_COLS || ttrows < MIN_ROWS) {
		fprintf(stderr, "Terminal too small (min: %dx%d)\n", MIN_COLS, MIN_ROWS);
		return -1;
	}

	return 0;
}

/* Code stolen from http://c-faq.com/osdep/cbreak.html */
static int tty_init(void)
{
	struct termios modmodes;

	if (tty_size() < 0)
		return -1;

	if (tcgetattr(fileno(stdin), &savemodes) < 0)
		return -1;

	havemodes = 1;
	hidecursor();

	/* "stty cbreak -echo" */
	modmodes = savemodes;
	modmodes.c_lflag &= ~ICANON;
	modmodes.c_lflag &= ~ECHO;
	modmodes.c_cc[VMIN] = 1;
	modmodes.c_cc[VTIME] = 0;

	return tcsetattr(fileno(stdin), TCSANOW, &modmodes);
}

static int tty_exit(void)
{
	if (!havemodes)
		return 0;

	showcursor();

	/* "stty sane" */
	return tcsetattr(fileno(stdin), TCSANOW, &savemodes);
}

static void freeze(int enable)
{
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, SIGALRM);

	sigprocmask(enable ? SIG_BLOCK : SIG_UNBLOCK, &set, NULL);
}

static void alarm_handler(int signo)
{
	static long h[4];

	(void)signo;

	/* On init from main() */
	if (!signo)
		h[3] = 500000;

	h[3] -= h[3] / (3000 - 10 * level);
	setitimer(0, (struct itimerval *)h, 0);
}

static void exit_handler(int signo)
{
	(void)signo;
	running = 0;
}

static void resize_handler(int signo)
{
	(void)signo;
	resized = 1;
}

static void sig_init(void)
{
	struct sigaction sa;

	SIGNAL(SIGINT, exit_handler);
	SIGNAL(SIGTERM, exit_handler);
	SIGNAL(SIGALRM, alarm_handler);
	SIGNAL(SIGWINCH, resize_handler);

	/* Start update timer. */
	alarm_handler(0);
}

int main(void)
{
	int c = 0, i, j, *ptr;
	int pos = 17;
	int *backup;

	/* Initialize board, grey border, used to be white(7) */
	ptr = board;
	for (i = B_SIZE; i; i--)
		*ptr++ = i < 25 || i % B_COLS < 2 ? 60 : 0;

	srand((unsigned int)time(NULL));

	/* Probe screen size, save to global variables ttcols and ttrows */
	if (tty_init() == -1)
		return 1;

	/* Set up signals */
	sig_init();

	clrscr();
	show_online_help();

	shape = next_shape();
	while (running) {
		if (resized) {
			resized = 0;

			/*
			 * Ignore too small error at runtime, the game
			 * will be garbled, forcing the user resize it
			 * again to see, better than exiting mid game.
			 */
			tty_size();

			/* Force full redraw by clearing shadow */
			clrscr();
			for (j = B_SIZE; j--; shadow[j] = 0)
			   ;
			show_online_help();

			/* Prevent unwanted drops or speedups */
			c = 0;
		}

		if (c < 0) {
			if (fits_in(shape, pos + B_COLS)) {
				pos += B_COLS;
			} else {
				place(shape, pos, color);
				++points;
				for (j = 0; j < 252; j = B_COLS * (j / B_COLS + 1)) {
					for (; board[++j];) {
						if (j % B_COLS == 10) {
							lines_cleared++;

							for (; j % B_COLS; board[j--] = 0)
							   ;
							update();

							for (; --j; board[j + B_COLS] = board[j])
							   ;
							update();
						}
					}
				}
				shape = next_shape();
				if (!fits_in(shape, pos = 17))
					c = keys[KEY_QUIT];
			}
		}

		if (c == keys[KEY_LEFT]) {
			if (!fits_in(shape, --pos))
				++pos;
		}

		if (c == keys[KEY_ROTATE]) {
			backup = shape;
			shape = &shapes[5 * *shape];	/* Rotate */
			/* Check if it fits, if not restore shape from backup */
			if (!fits_in(shape, pos))
				shape = backup;
		}

		if (c == keys[KEY_RIGHT]) {
			if (!fits_in(shape, ++pos))
				--pos;
		}

		if (c == keys[KEY_DROP]) {
			for (; fits_in(shape, pos + B_COLS); ++points)
				pos += B_COLS;
		}

		if (c == keys[KEY_PAUSE] || c == keys[KEY_QUIT]) {
			freeze(1);

			if (c == keys[KEY_QUIT])
				break;

			for (j = B_SIZE; j--; shadow[j] = 0)
			   ;

			while (getchar() - keys[KEY_PAUSE])
			   ;

//			puts("\033[H\033[J\033[7m");
			freeze(0);
		}

		place(shape, pos, color);
		c = update();
		place(shape, pos, 0);
	}

	clrscr();
	printf("\033[0mYour score: %d points x level %d = %d\n\n", points, level, points * level);
	show_high_score();

	if (tty_exit() == -1)
		return 1;

	return 0;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
