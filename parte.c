#include "config.h"
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xft/Xft.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#define NORMAL_EXIT 1
#define ACTION_EXIT 0
#define ERROR_EXIT 2


struct Xorg {
	Display *dpy;
	XftDraw *draw;
	Window root;
	int screen;
	int depth;
	XRRScreenResources *screen_res;
	Window win;
	Visual *visual;
	Colormap colormap;
	XftFont *font;
	int monitor_index;
};

struct Coords {
	int x;
	int y;
	int w;
	int h;
};

struct Colors {
	XftColor background;
	XftColor border;
	XftColor font;
};

struct Parte {
	struct Xorg x;
	struct Coords root;
	struct Coords coords;
	struct Colors c;
	char *text;
	uint16_t flags;
};

volatile int end = 1;

char *background_color_g;
char *border_color_g;
char *font_color_g;
char *font_g;
ssize_t border_size_g = -1;
int corner_g = CORNER;
int focus_g = FOCUS_MONITOR;
ssize_t time_g = -1;


static void
die(const char *msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(ERROR_EXIT);
}

void
init_Xorg(struct Xorg *x)
{
	x->dpy = XOpenDisplay(NULL);
	if(!x->dpy) {
		fprintf(stderr, "Fail at XOpenDisplay(NULL)\n");
		die("init_Xorg()");
	}

	x->screen = DefaultScreen(x->dpy);
	x->root = RootWindow(x->dpy, x->screen);

	XVisualInfo vinfo;
	if(XMatchVisualInfo(x->dpy, x->screen, 32, TrueColor, &vinfo)){
		x->visual = vinfo.visual;
		x->depth = vinfo.depth;
		x->colormap = XCreateColormap(x->dpy, x->root, x->visual, AllocNone);
	}else{
		x->visual = DefaultVisual(x->dpy, x->screen);
		x->depth = DefaultDepth(x->dpy, x->screen);
		x->colormap = DefaultColormap(x->dpy, x->screen);
	}

	x->screen_res = XRRGetScreenResources(x->dpy, x->root);
	if(!x->screen_res){
		fprintf(stderr, "Fail at XRRGetScreenResources(dpy, root)\n");
		die("init_Xorg()");
	}
	
	x->font = XftFontOpenName(x->dpy, x->screen, FONT);
	x->monitor_index = -1;
}

void
close_Parte(struct Parte *p)
{
	XftColorFree(p->x.dpy, p->x.visual, p->x.colormap, &p->c.background);
	XftColorFree(p->x.dpy, p->x.visual, p->x.colormap, &p->c.border);
	XftColorFree(p->x.dpy, p->x.visual, p->x.colormap, &p->c.font);
	XftDrawDestroy(p->x.draw);
	XftFontClose(p->x.dpy, p->x.font);
	XRRFreeScreenResources(p->x.screen_res);
	XDestroyWindow(p->x.dpy, p->x.win);
	XFreeColormap(p->x.dpy, p->x.colormap);
	XCloseDisplay(p->x.dpy);
	free(p->text);
}


void
check_mouse_monitor(struct Parte *p)
{
	Window child;
	int x, y, win_x, win_y;
	unsigned int mask;
	if(!XQueryPointer(p->x.dpy, p->x.root, &p->x.root, &child, &x, &y, &win_x, &win_y, &mask)){
		fprintf(stderr, "Fail at XQueryPointer()\n");
		die("check_mouse_monitor()");
	}

	for(int i = 0; i < p->x.screen_res->ncrtc; i++){
		XRRCrtcInfo *crtc = XRRGetCrtcInfo(p->x.dpy, p->x.screen_res, p->x.screen_res->crtcs[i]);
		if(!crtc) continue;
		if(crtc->mode == None){
			XRRFreeCrtcInfo(crtc);
			continue;
		}
		int mx = crtc->x;
		int my = crtc->y;
		int mw = crtc->width;
		int mh = crtc->height;

		if(x >= mx && x < mx + mw && y >= my && y < my + mh){
			p->x.monitor_index = i;
			p->root = (struct Coords){mx, my, mw, mh};
			XRRFreeCrtcInfo(crtc);
			break;
		}

		XRRFreeCrtcInfo(crtc);
	}
	if(p->x.monitor_index == -1){
		fprintf(stderr, "Unable to find the monitor at (%d, %d)\n", x, y);
		die("check_mouse_monitor()");
	}
}

void
check_keyboard_monitor(struct Parte *p)
{
	Window focused_win, child;
	int revert_to;
	int x, y;

	XGetInputFocus(p->x.dpy, &focused_win, &revert_to);

	if(focused_win == None || focused_win == PointerRoot) {
		fprintf(stderr, "No hay ventana con foco\n");
		die("check_keyboard_monitor");
	}
	if(!XTranslateCoordinates(p->x.dpy, focused_win, p->x.root, 0, 0, &x, &y, &child)){
		fprintf(stderr, "Error en XTranslateCoordinates\n");
		die("check_keyboard_monitor");
	}

	for(int i = 0; i < p->x.screen_res->noutput; i++){
		XRROutputInfo *output = XRRGetOutputInfo(p->x.dpy, p->x.screen_res, p->x.screen_res->outputs[i]);
		if(!output) continue;

		if(output->connection == RR_Connected && output->crtc != None){
			XRRCrtcInfo *crtc = XRRGetCrtcInfo(p->x.dpy, p->x.screen_res, output->crtc);
			if(crtc){
				int mx = crtc->x;
				int my = crtc->y;
				int mw = crtc->width;
				int mh = crtc->height;

				if(x >= mx && x < mx + mw && y >= my && y < my + mh){
					p->x.monitor_index = i;
					p->root = (struct Coords){mx, my, mw, mh};
					XRRFreeCrtcInfo(crtc);
					XRRFreeOutputInfo(output);
					return;
				}
				XRRFreeCrtcInfo(crtc);
			}
		}
		XRRFreeOutputInfo(output);
	}
}


static void
get_text_metrics(struct Parte *p, size_t *w, size_t *h)
{
	char *copy = strdup(p->text);
	char *line = copy;
	char *next;

	XGlyphInfo info;
	size_t max_width = 0;
	size_t total_height = 0;
	while(line) {
		next = strchr(line, '\n');
		if(next) *next = '\0';

		XftTextExtentsUtf8(p->x.dpy, p->x.font, (XftChar8*)line, strlen(line), &info);

		if(info.xOff > max_width) max_width = info.xOff;
		total_height += p->x.font->height;

		if(!next) break;
		line = next + 1;
	}

	free(copy);
	*w = max_width;
	*h = total_height;
}

void
get_coords(struct Parte *p)
{
	size_t text_w, text_h;
	int relative_x, relative_y;

	get_text_metrics(p, &text_w, &text_h);
	
	if(corner_g == TOP_RIGHT || corner_g == BOTTOM_RIGHT){
		relative_x = p->root.w - text_w - ((border_size_g + PADDING_TEXT_X) * 2) - PADDING_X;
	}else{
		relative_x = PADDING_X;
	}
	if(corner_g == BOTTOM_LEFT || corner_g == BOTTOM_RIGHT){
		relative_y = p->root.h - text_h - ((border_size_g + PADDING_TEXT_Y) * 2) - PADDING_Y;
	}else{
		relative_y = PADDING_Y;
	}
	p->coords.w = text_w + (PADDING_TEXT_X * 2);
	p->coords.h = text_h + (PADDING_TEXT_Y * 2);
	p->coords.x = p->root.x + relative_x;
	p->coords.y = p->root.y + relative_y;

}

void
create_window(struct Parte *p)
{
	XSetWindowAttributes attrs;

	XftColorAllocName(p->x.dpy, p->x.visual, p->x.colormap, background_color_g, &p->c.background);
	attrs.background_pixel = p->c.background.pixel;

	XftColorAllocName(p->x.dpy, p->x.visual, p->x.colormap, border_color_g, &p->c.border);
	attrs.border_pixel = p->c.border.pixel;

	attrs.override_redirect = True;
	attrs.colormap = p->x.colormap;

	p->x.win = XCreateWindow(p->x.dpy, p->x.root,
		p->coords.x, p->coords.y, p->coords.w, p->coords.h,
		border_size_g,
		p->x.depth,
		InputOutput,
		p->x.visual,
		CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWColormap,
		&attrs
	);

	p->x.draw = XftDrawCreate(p->x.dpy, p->x.win, p->x.visual, p->x.colormap);
	XSelectInput(p->x.dpy, p->x.win, ExposureMask | ButtonPress);

	XftColorAllocName(p->x.dpy, p->x.visual, p->x.colormap, font_color_g, &p->c.font);
}

void
draw_text(struct Parte *p, int x, int y)
{
	char *copy = strdup(p->text);
	char *line = copy;
	char *next;
	int ascent = p->x.font->ascent;
	int descent = p->x.font->descent;
	int line_h = ascent + descent;
	int current_y = y + ascent;

	while(line) {
		next = strchr(line, '\n');
		if(next) *next = '\0';

		XftDrawStringUtf8(p->x.draw, &p->c.font, p->x.font, x, current_y, (XftChar8*)line, strlen(line));

		if(!next) break;
		current_y += line_h;
		line = next + 1;
	}

	free(copy);
}


static size_t
expand_escapes(char *out, const char *in)
{
	size_t written = 0;
	while(*in){
		if(*in == '\\' && in[1]){
			switch(in[1]) {
				case 'n': out[written++] = '\n'; in += 2; continue;
				case 't': out[written++] = '\t'; in += 2; continue;
				case '\\': out[written++] = '\\'; in += 2; continue;
				case '"': out[written++] = '"'; in += 2; continue;
				case '\'': out[written++] = '\''; in += 2; continue;
				default:
					out[written++] = '\\';
					out[written++] = in[1];
					in += 2;
					continue;
			}
		}
		out[written++] = *in++;
	}
	return written;
}

static void
get_stdin(char **ptr, size_t *nbytes)
{
	if(isatty(STDIN_FILENO)){
		*ptr = NULL;
		*nbytes = 0;
		return;
	}

	char *buffer = NULL;
	size_t bufsize = 0;
	ssize_t len;

	len = getdelim(&buffer, &bufsize, '\0', stdin);
	if(len == -1){
		*ptr = NULL;
		*nbytes = 0;
		free(buffer);
		return;
	}

	if(len > 0 && buffer[len - 1] == '\n') len--;

	char *tmp = realloc(buffer, len + 1);
	if(tmp) buffer = tmp;
	buffer[len] = '\0';

	*ptr = buffer;
	*nbytes = len;
}

void
get_text(struct Parte *p, int argc, char **argv) {
	char *stdin_buf = NULL;
	size_t stdin_len = 0;
	get_stdin(&stdin_buf, &stdin_len);

	size_t nbytes = stdin_len;
	if(stdin_len && argc > 1) nbytes++;

	for(int i = 0; i < argc; i++){
		const char *s = argv[i];
		while(*s){
			if(*s == '\\' && s[1]){
				switch (s[1]) {
				case 'n': case 't': case '\\':
				case '"': case '\'':
					nbytes += 1;
					s += 2;
					continue;
				}
				nbytes += 2;
				s += 2;
				continue;
			}
			nbytes++;
			s++;
		}
		if(i < argc - 1) nbytes++;
	}

	p->text = malloc(nbytes + 1);
	if(!p->text) exit(1);
	char *ptr = p->text;

	if(stdin_len){
		memcpy(ptr, stdin_buf, stdin_len);
		ptr += stdin_len;
		if(argc > 1) *ptr++ = '\n';
	}
	free(stdin_buf);

	for(int i = 0; i < argc; i++){
		ptr += expand_escapes(ptr, argv[i]);
		if(i < argc - 1) *ptr++ = '\n';
	}
	*ptr = '\0';
}

void
exit_signal(int sig)
{
	end = 0;
}

void
show_help()
{
	printf("parte [options] [text ...]\n");
	printf(
	"	-h, -?, --help\n"
	"		Show help and exit.\n"
	"\n"
	"	-u mode\n"
	"		Set the notification mode. Valid modes are low, normal and urgent.\n"
	"		Defaults to normal.  \n"
	"		Note: `-u` overrides previous visual settings, so use it before other flags.\n"
	"\n"
	"	-t seconds\n"
	"		Display the notification for seconds seconds.\n"
	"\n"
	"	--border-size pixels\n"
	"		Set the size of the border in pixels.\n"
	"\n"
	"	-fg \"#RRGGBB\"\n"
	"		Set the foreground (font) color.\n"
	"\n"
	"	-bg \"#RRGGBB\"\n"
	"		Set the background color.\n"
	"\n"
	"	--border-color \"#RRGGBB\"\n"
	"		Set the border color.\n"
	"\n"
	"	-font pattern\n"
	"		Set the font using a Fontconfig pattern.\n"
	"\n"
	"	-focus-keyboard\n"
	"		Place the notification on the monitor with keyboard focus.\n"
	"\n"
	"	-focus-mouse\n"
	"		Place the notification on the monitor with mouse focus.\n"
	"\n"
	"	-top-left\n"
	"	-top-right\n"
	"	-bottom-left\n"
	"	-bottom-right\n"
	"		Position the notification on the specified screen corner.\n"
	);
	exit(NORMAL_EXIT);
}

int
main(int argc, char *argv[])
{
	struct Parte p = {0};
	size_t flag_iter = 1;


	while (flag_iter < argc && argv[flag_iter][0] == '-') {

		if(!strcmp(argv[flag_iter], "--help") ||
			!strcmp(argv[flag_iter], "-h") ||
			!strcmp(argv[flag_iter], "-?")) {
			show_help();
		}

		else if(!strcmp(argv[flag_iter], "-u")) {

			if(flag_iter + 1 >= argc)
				die("missing argument for -u");

			char *mode = argv[++flag_iter];

			if(!strcmp(mode, "low")) {
				font_color_g = LOW_FONT_COLOR;
				background_color_g = LOW_BACKGROUND;
				border_color_g = LOW_BORDER;
				border_size_g = LOW_BORDER_SIZE;
				time_g = LOW_TIME;
			}
			else if(!strcmp(mode, "urgent")) {
				font_color_g = URGENT_FONT_COLOR;
				background_color_g = URGENT_BACKGROUND;
				border_color_g = URGENT_BORDER;
				border_size_g = URGENT_BORDER_SIZE;
				time_g = URGENT_TIME;
			}
			else if(!strcmp(mode, "normal")) {
				/* default */
			}
			else {
				fprintf(stderr, "invalid argument for -u flag: <%s>\n", mode);
				die("expected <low>, <normal> or <urgent>");
			}
		}

		else if(!strcmp(argv[flag_iter], "-t")) {
			if(++flag_iter >= argc) die("missing argument for -t");
			time_g = atol(argv[flag_iter]);
		}

		else if(!strcmp(argv[flag_iter], "-border-size")) {
			if(++flag_iter >= argc) die("missing argument for -border-size");
			border_size_g = atol(argv[flag_iter]);
		}

		else if(!strcmp(argv[flag_iter], "-fg")) {
			if(++flag_iter >= argc) die("missing argument for -fg");
			font_color_g = argv[flag_iter];
		}

		else if(!strcmp(argv[flag_iter], "-bg")) {
			if(++flag_iter >= argc) die("missing argument for -bg");
			background_color_g = argv[flag_iter];
		}

		else if(!strcmp(argv[flag_iter], "-border-color")) {
			if(++flag_iter >= argc) die("missing argument for -border-color");
			border_color_g = argv[flag_iter];
		}

		else if(!strcmp(argv[flag_iter], "-font")) {
			if(++flag_iter >= argc) die("missing argument for -font");
			font_g = argv[flag_iter];
		}

		else if(!strcmp(argv[flag_iter], "-focus-keyboard")) {
			focus_g = FOCUS_KEYBOARD;
		}

		else if(!strcmp(argv[flag_iter], "-focus-mouse")) {
			focus_g = FOCUS_MOUSE;
		}

		else if(!strcmp(argv[flag_iter], "-top-left")) {
			corner_g = TOP_LEFT;
		}

		else if(!strcmp(argv[flag_iter], "-top-right")) {
			corner_g = TOP_RIGHT;
		}

		else if(!strcmp(argv[flag_iter], "-bottom-left")) {
			corner_g = BOTTOM_LEFT;
		}

		else if(!strcmp(argv[flag_iter], "-bottom-right")) {
			corner_g = BOTTOM_RIGHT;
		}

		flag_iter++;
	}

	if(!background_color_g) background_color_g = NORMAL_BACKGROUND;
	if(!border_color_g)	 border_color_g	 = NORMAL_BORDER;
	if(!font_color_g)	   font_color_g	   = NORMAL_FONT_COLOR;
	if(border_size_g < 0) border_size_g = NORMAL_BORDER_SIZE;
	if(time_g < 0) time_g = NORMAL_TIME;


	get_text(&p, argc - flag_iter, argv + flag_iter);

	init_Xorg(&p.x);

	if(focus_g == FOCUS_MOUSE){
		check_mouse_monitor(&p);
	}else{
		check_keyboard_monitor(&p);
	}

	get_coords(&p);

	create_window(&p);
	XMapWindow(p.x.dpy, p.x.win);

	signal(SIGALRM, exit_signal);
	alarm(time_g);


	while(end){
		if(XPending(p.x.dpy) > 0){
			XEvent event;
			XNextEvent(p.x.dpy, &event);

			if(event.type == Expose){
				XClearWindow(p.x.dpy, p.x.win);
				draw_text(&p, PADDING_TEXT_X, PADDING_TEXT_Y);
			}
			else if(event.type == ButtonPress){
				if(event.xbutton.button == Button1){
					close_Parte(&p);
					return NORMAL_EXIT;
				}
				else if(event.xbutton.button == Button3){
					close_Parte(&p);
					return ACTION_EXIT;
				}
			}
		}
	}

	close_Parte(&p);
	return NORMAL_EXIT;
}

