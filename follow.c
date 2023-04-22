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

	int v_bottom = 0;
	int v_offset = 0;
	int h_offset = 0;

	while( cont ) {
		char* title_left = get_title_left( argv[optind] );
		char* title_right = get_title_right();
		char* display_string = get_command_output( command_args );

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

			free( title_left );
		}

		if ( title_right != NULL ) {
			if ( right_start >= 0 ) {
				wmove( win, 0, right_start );
				waddnstr( win, title_right, title_right_len );
			} else {
				wmove( win, 0, 0 );
				waddnstr( win, title_right - right_start, screen_width );
			}

			free( title_right );
		}

		wattroff( win, A_REVERSE );

		/* Size of the zone where the output of the command will be display */
		/* One line less because of the title */
		int display_height = screen_height - title_height;
		int display_width = screen_width;

		/* Size of the result in both directions */
		int res_max_height = 0;
		int res_max_width = 0;

		if ( display_string != NULL ) {
			char* line_start = display_string;
			char* cur_pos = display_string;
			char** lines = NULL;
			int* lines_len = NULL;
			while( 1 ) {
				char c = *cur_pos;
				if ( c == '\n' || c == '\0' ) {
					const size_t line_len = cur_pos - line_start;

					if ( line_len > res_max_width ) {
						res_max_width = line_len;
					}

					*cur_pos = '\0';

					if ( c == '\n' || line_len > 0 ) {
						lines = realloc( lines, sizeof( char* ) * res_max_height + 1 );
						lines_len = realloc( lines_len, sizeof( int* ) * res_max_height + 1 );
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

			if ( display_height >= res_max_height ) {
				v_offset = 0;
			} else if ( v_offset + display_height > res_max_height || v_bottom ) {
				v_offset = res_max_height - display_height;
			} else if ( v_offset < 0 ) {
				v_offset = 0;
			}

			if ( display_width >= res_max_width ) {
				h_offset = 0;
			} else if ( h_offset + display_width > res_max_width ) {
				h_offset = res_max_width - display_width;
			} else if ( h_offset < 0 ) {
				h_offset = 0;
			}

			const int y_max = res_max_height < display_height ? res_max_height : display_height;
			for ( int y = 0; y < y_max; y++ ) {
				const int line_len = lines_len[y + v_offset];
				if ( line_len <= h_offset ) {
					continue;
				}
				const int x_max = line_len - h_offset < display_width ? line_len - h_offset : display_width;
				wmove( win, y + title_height, 0 );
				waddnstr( win, lines[y + v_offset] + h_offset, x_max );
			}

			free( lines );
			free( lines_len );
			free( display_string );
		} else {
			perror( NULL );
		}

		wrefresh( win );

		int begin = 1;
		while ( 1 ) {
			if ( begin ) {
				begin = 0;
				halfdelay( 10 * interval );
				nodelay( win, 0 );
			} else {
				nodelay( win, 1 );
				nocbreak();
			}

			int c = wgetch( win );
			int done = 0;

			switch ( c ) {
			case 'q':
				cont = 0;
				break;
			case KEY_LEFT:
				h_offset--;
				break;
			case KEY_RIGHT:
				h_offset++;
				break;
			case KEY_UP:
			case 'k':
			case 'y':
				v_bottom = 0;
				v_offset--;
				break;
			case KEY_DOWN:
			case 'e':
			case 'j':
				v_offset++;
				break;
			case ' ':
			case 'f':
				v_bottom = 0;
				v_offset += display_height;
				break;
			case 'b':
				v_bottom = 0;
				v_offset -= display_height;
				break;
			case 'd':
				v_bottom = 0;
				v_offset += display_height / 2;
				break;
			case 'u':
				v_bottom = 0;
				v_offset += display_height / 2;
				break;
			case 'g':
				v_bottom = 0;
				v_offset = 0;
				break;
			case 'G':
				v_bottom = 0;
				v_offset = res_max_height;
				break;
			case 'F':
				v_bottom = 1;
				break;
			default:
				done = 1;
				break;
			}

			if ( done ) {
				break;
			}
		}
	}

	keypad( win, 0 );
	echo();
	endwin();

	return EXIT_SUCCESS;
}
