#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <limits.h> /* For PIPE_BUF */
#include <errno.h>

#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/param.h> /* For MIN(), MAX() */
#include <sys/wait.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <ncurses.h>


int run_command( char* const* args, int* fd )  {
	const char* name = args[0];

	int pipefds[2];
	int piperes = pipe( pipefds );
	if ( piperes == -1 ) {
		perror( "pipe" );
		return -3;
	}

	pid_t pid = fork();
	if ( pid == -1 ) {
		/* error */
		close( pipefds[0] );
		close( pipefds[1] );
		return -1;
	} else if ( pid == 0 ) {
		/* child */

		/* now close all the standard streams, as we will redirect them */
		close( STDIN_FILENO );
		close( STDOUT_FILENO );
		close( STDERR_FILENO );

		/* redirect stdin to /dev/null */
		int fd_in = open( "/dev/null", O_RDWR );
		if ( fd_in != STDIN_FILENO ) {
			/* fd_in is usually 0 since it's now available... */
			dup2( fd_in, STDIN_FILENO );
			close( fd_in );
		}

		/* redirect output to pipe */
		dup2( pipefds[1], STDOUT_FILENO );
		dup2( pipefds[1], STDERR_FILENO );

		close( pipefds[0] );
		close( pipefds[1] );

		/* the following never returns, except if an error occurred */
		execvp( name, args );
		perror( "execvp" );
		exit( 1 );
	} else {
		/* parent */
		close( pipefds[1] );

		/* Do not share the file descriptor across multiple chidren processes */
		fcntl( pipefds[0], F_SETFD, FD_CLOEXEC );

		*fd = pipefds[0];

		return pid;
	}
}

char* get_command_output( char** command_args ) {
	int fd;

	int pid = run_command( command_args, &fd );

	/* Error in run_command() */
	if ( pid < 0 ) return NULL;

	char* ret = malloc( 1 );
	ret[0] = '\0';
	size_t len = 0;
	int err = 0;

	for (;;) {
		char buf[PIPE_BUF];

		ssize_t nread = read( fd, buf, PIPE_BUF );

		if ( nread == -1 ) {
			if ( errno == EINTR ) continue;
			break;
		} else if ( nread == 0 ) {
			close( fd );
			break;
		} else if ( !err ) {
			char* new = (char*) realloc( (void*) ret, len + nread + 1 );
			if ( new == NULL ) {
				/* error */
				/* we won't return anything, but we'll keep consuming the output and wait for the process to exit */
				free( ret );
				ret = NULL;
				err = 1;
				continue;
			}

			ret = new;
			memcpy( ret + len, buf, nread );
			ret[len + nread] = '\0'; /* ensure it is NUL terminated */
			len += nread;
		}
	}

	waitpid( pid, NULL, 0 );

	return ret;
}

/**
 * Add two timespec's in place
 */
void add_timespec( struct timespec* base, const struct timespec* add ) {
	base->tv_sec += add->tv_sec;
	base->tv_nsec += add->tv_nsec;

	while ( base->tv_nsec >= 1000000000 ) {
		base->tv_sec++;
		base->tv_nsec -= 1000000000;
	}
}

/**
 * Compute the positive difference of two timespec's and return the results in the given 10th power of second
 */
long int diff_timespec( const struct timespec* left, const struct timespec* right, const int expo ) {
	if ( left->tv_sec < right->tv_sec || ( left->tv_sec == right->tv_sec && left->tv_nsec <= right->tv_nsec ) ) return 0;

	long int diff_sec = left->tv_sec - right->tv_sec;
	long int diff_nsec;
	if ( left->tv_nsec < right->tv_nsec ) {
		diff_sec--;
		diff_nsec = 1000000000 + left->tv_nsec - right->tv_nsec;
	} else {
		diff_nsec = left->tv_nsec - right->tv_nsec;
	}

	if ( expo <= 0 ) {
		for ( int i = 0; i < -expo; i++ )  diff_sec /= 10;
		return diff_sec;
	} else {
		for ( int i = 0; i < expo; i++ )  diff_sec *= 10;
		for ( int i = 0; i < 9 - expo; i++ )  diff_nsec /= 10;
		return diff_sec + diff_nsec;
	}
}

/**
 * Get the left part of the title line.
 *
 * This is just the current time.
 */
char* get_title_left( char* command ) {
	char hostname[1025];
	memset( hostname, '\0', sizeof( hostname ) );
	int hn_res = gethostname( hostname, sizeof( hostname ) - 1 );

	char* ret = NULL;
	if ( hn_res == 0 ) {
		asprintf( &ret, "%s: %s", hostname, command );
	} else {
		asprintf( &ret, "%s", command );
	}

	return ret;
}

/**
 * Get the right part of the title line.
 *
 * This is just the current time.
 */
char* get_title_right() {
	time_t t = time( NULL );

	struct tm loct;
	if ( localtime_r( &t, &loct ) == NULL ) {
		return NULL;
	}

	char* ret = calloc( 256, 1 );
	if ( ret == NULL ) {
		return NULL;
	}

	int res = strftime( ret, 255, "%c", &loct );
	if ( res == 0 ) {
		free( ret );
		return NULL;
	} else {
		return ret;
	}
}

int main( int argc, char** argv ) {
	int help = 0;
	int interval = 1;
	int shell = 0;

	/* Parse command line */
	/* ------------------ */

	while ( 1 ) {
		int opt = getopt( argc, argv, "++hns" );
		if ( opt < 0 ) break;
		if ( opt == '?' ) exit( 2 );
		if ( opt == 'h' ) help++;
		if ( opt == 'n' ) interval++;
		if ( opt == 's' ) shell++;
	}

	if ( help || argc - optind < 1 ) {
#define HELP_MESSAGE \
	"Usage: %s [OPTION...] [--] <command> [arg...]\n" \
	"\n" \
	"Program options:\n" \
	"  -h --help         Display this help message\n" \
	"  -n --interval=N   Refresh the command every N seconds\n" \
	"  -s --shell        Use a shell to execute the command\n"

		fprintf( stderr, HELP_MESSAGE, argv[0] );

		if ( help ) exit( EXIT_SUCCESS );
		else exit( 2 );
	}

	/* Prepare the command to execute */
	/* ------------------------------ */

	char** command_args;

	if ( shell ) {
		size_t shell_len = argc - optind; /* (n-1) space in between the arguments, plus the final NULL byte */
		for ( int argi = optind; argi < argc; argi++ ) shell_len += strlen( argv[ argi ] );

		char* shell_command = malloc( shell_len );
		if ( shell_command == NULL ) {
			perror( "malloc" );
			exit( EXIT_FAILURE );
		}

		size_t start = 0;
		for ( int argi = optind; argi < argc; argi++ ) {
			if ( argi > optind ) {
				shell_command[ start ] = ' ';
				start++;
				shell_command[ start ] = '\0';
			}

			strncpy( shell_command + start, argv[ argi ], shell_len - start );
			start += strlen( argv[ argi ] );
		}

		command_args = malloc( sizeof( char* ) * 4 );
		if ( command_args == NULL ) {
			perror( "malloc" );
			exit( EXIT_FAILURE );
		}

		command_args[0] = "/bin/sh";
		command_args[1] = "-c";
		command_args[2] = shell_command;
		command_args[3] = NULL;
	} else {
		size_t argn = argc - optind;
		command_args = malloc( sizeof( char* ) * ( argn + 1 ) );
		if ( command_args == NULL ) {
			perror( "malloc" );
			exit( EXIT_FAILURE );
		}

		for ( int argi = 0; argi < argn; argi++ ) {
			command_args[argi] = argv[optind + argi];
		}
		command_args[argn] = NULL;
	}

	WINDOW* win = initscr();
	noecho();
	curs_set( 0 );
	keypad( win, 1 );

	int cont = 1;
	int refresh = 2;
	struct timespec next_timer = { 0, 0 };
	struct timespec inter_timer = { interval, 0 };

	char* title_left = NULL;
	char* title_right = NULL;
	char* display_string = NULL;
	int res_max_height = 0;
	int res_max_width = 0;
	size_t lines_alloc = 0;
	char** lines = NULL;
	int* lines_len = NULL;

	int v_offset = 0;
	int h_offset = 0;
	int v_end = 0;
	int v_diff = 0;
	int h_diff = 0;
	int past = 0;

	while( cont ) {
		if ( refresh ) {
			if ( refresh == 2 ) {
				int res = clock_gettime( CLOCK_MONOTONIC, &next_timer );
				if ( res < 0 ) {
					perror( "clock_gettime" );
					exit( EXIT_FAILURE );
				}
			}
			add_timespec( &next_timer, &inter_timer );
			refresh = 0;

			free( title_left );
			free( title_right );
			free( display_string );
			res_max_height = 0;
			res_max_width = 0;

			title_left = get_title_left( argv[optind] );
			title_right = get_title_right();
			display_string = get_command_output( command_args );

			/* Split the result into lines */
			if ( display_string != NULL ) {
				char* line_start = display_string;
				char* cur_pos = display_string;

				while( 1 ) {
					char c = *cur_pos;
					if ( c == '\n' || c == '\0' ) {
						const size_t line_len = cur_pos - line_start;

						if ( line_len > res_max_width ) {
							res_max_width = line_len;
						}

						*cur_pos = '\0';

						if ( c == '\n' || line_len > 0 ) {
							if ( res_max_height + 1 > lines_alloc ) {
								lines = realloc( lines, sizeof( char* ) * res_max_height + 1 );
								lines_len = realloc( lines_len, sizeof( int* ) * res_max_height + 1 );
								lines_alloc = res_max_height + 1;
							}

							lines[res_max_height] = line_start;
							lines_len[res_max_height] = line_len;
							res_max_height++;
						}

						if ( c == '\0' ) {
							break;
						} else {
							line_start = cur_pos + 1;
						}
					}
					cur_pos++;
				}
			}
		}

		werase( win );
		int screen_height = getmaxy( win );
		int screen_width = getmaxx( win );

		const int title_left_len = ( title_left == NULL ? 0 : strlen( title_left ) );
		const int title_right_len = ( title_right == NULL ? 0 : strlen( title_right ) );
		const int title_height = 1;

		const int right_start = screen_width - title_right_len;

		wattron( win, A_REVERSE );

		if ( title_left != NULL ) {
			wmove( win, 0, 0 );
			if ( right_start > title_left_len ) {
				waddnstr( win, title_left, title_left_len );
			} else if ( right_start > 4 ) {
				waddnstr( win, title_left, right_start - 4 );
				waddnstr( win, "...", 3 );
			}
		}

		if ( title_right != NULL ) {
			if ( right_start >= 0 ) {
				wmove( win, 0, right_start );
				waddnstr( win, title_right, title_right_len );
			} else {
				wmove( win, 0, 0 );
				waddnstr( win, title_right - right_start, screen_width );
			}
		}

		wattroff( win, A_REVERSE );

		/* Size of the zone where the output of the command will be display */
		/* One line less because of the title */
		int display_height = screen_height - title_height;
		int display_width = screen_width;

		if ( v_end ) {
			v_offset = res_max_height > display_height ? res_max_height - display_height : 0;
		} else if ( v_diff != 0 && past ) {
			v_offset += v_diff;
		} else if ( v_diff > 0 ) {
			v_offset = MAX( v_offset, MIN( v_offset + v_diff, MAX( res_max_height - display_height, 0 ) ) );
		} else if ( v_diff < 0 ) {
			v_offset = MIN( v_offset, MAX( v_offset + v_diff, 0 ) );
		}

		if ( h_diff != 0 && past ) {
			h_offset += h_diff;
		} else if ( h_diff > 0 ) {
			h_offset = MAX( h_offset, MIN( h_offset + h_diff, MAX( res_max_width - display_width, 0 ) ) );
		} else if ( h_diff < 0 ) {
			h_offset = MIN( h_offset, MAX( h_offset + h_diff, 0 ) );
		}

		if ( display_string != NULL && v_offset > -display_height && v_offset < res_max_height && h_offset > -display_width && h_offset < res_max_width ) {
			int v_disp_off = MAX( -v_offset, 0 );
			int v_start = MAX( v_offset, 0 );
			int h_disp_off = MAX( -h_offset, 0 );
			int h_start = MAX( h_offset, 0 );

			const int v_end = MIN( v_offset + display_height, res_max_height ) - v_start;
			for ( int v = 0; v < v_end; v++ ) {
				const int line_len = lines_len[v + v_start];
				if ( line_len <= h_offset ) {
					continue;
				}
				const int h_end = MIN( line_len, h_offset + display_width ) - h_start;
				wmove( win, v_disp_off + v + title_height, h_disp_off );
				waddnstr( win, lines[v + v_start] + h_start, h_end );
			}
		} else {
			perror( NULL );
		}

		wrefresh( win );

		struct timespec cur_timer = { 0, 0 };
		int res = clock_gettime( CLOCK_MONOTONIC, &cur_timer );
		if ( res < 0 ) {
			perror( "clock_gettime" );
			exit( EXIT_FAILURE );
		}

		wtimeout( win, diff_timespec( &next_timer, &cur_timer, 3 ) );

		/* Reset these ones */
		v_diff = 0;
		h_diff = 0;
		past = 0;

		switch ( wgetch( win ) ) {
		case 'q':
			cont = 0;
			break;
		case 'r':
		case 'R':
			refresh = 2;
			break;
		case KEY_LEFT:
			h_diff = -1;
			break;
		case KEY_RIGHT:
			h_diff = 1;
			break;
		case KEY_UP:
		case 'k':
		case 'y':
			v_end = 0;
			v_diff = -1;
			break;
		case 'K':
		case 'Y':
			v_end = 0;
			past = 1;
			v_diff = -1;
			break;
		case KEY_DOWN:
		case 'e':
		case 'j':
			v_diff = 1;
			break;
		case 'E':
		case 'J':
			v_end = 0;
			past = 1;
			v_diff = 1;
			break;
		case ' ':
		case 'f':
			v_end = 0;
			v_diff = display_height;
			break;
		case 'b':
			v_end = 0;
			v_diff = -display_height;
			break;
		case 'd':
			v_end = 0;
			v_diff = display_height / 2;
			break;
		case 'u':
			v_end = 0;
			v_diff = -display_height / 2;
			break;
		case 'g':
			v_end = 0;
			v_offset = 0;
			break;
		case 'G':
			v_end = 0;
			v_offset = 0;
			v_diff = res_max_height;
			break;
		case 'F':
			v_end = 1;
			break;
		case ERR:
			refresh = 1;
			break;
		default:
			break;
		}
	}

	keypad( win, 0 );
	echo();
	endwin();

	return EXIT_SUCCESS;
}
