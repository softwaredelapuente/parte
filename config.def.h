#include <stdint.h>
#include <stddef.h>

size_t PADDING_X = 25;
size_t PADDING_Y = 25;
size_t PADDING_TEXT_X = 10;
size_t PADDING_TEXT_Y = 10;
char *FONT = "monospace:size=12";


enum corners { TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT};
#define CORNER TOP_RIGHT
enum focus_monitor { FOCUS_MOUSE, FOCUS_KEYBOARD };
#define FOCUS_MONITOR FOCUS_MOUSE

/* options for normal normal urgency */
char *NORMAL_FONT_COLOR = "#ABB2BF";
char *NORMAL_BACKGROUND = "#282C34";
char *NORMAL_BORDER = "#61AFEF";
size_t NORMAL_BORDER_SIZE = 3;
size_t NORMAL_TIME = 5;



/* options for normal low urgency */
char *LOW_FONT_COLOR = "#FFFFFF";
char *LOW_BACKGROUND = "grey20";
char *LOW_BORDER = "grey";
size_t LOW_BORDER_SIZE = 3;
size_t LOW_TIME = 2;



/* options for normal urgent urgency */
char *URGENT_FONT_COLOR = "#FFFFFF";
char *URGENT_BACKGROUND = "#FF5555";
char *URGENT_BORDER = "#FF8800";
size_t URGENT_BORDER_SIZE = 3;
size_t URGENT_TIME = 10;
