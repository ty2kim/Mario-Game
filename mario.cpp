#include <iostream>
#include <list>
#include <cstdlib>
#include <sys/time.h>
#include <math.h>
#include <stdio.h>
#include <vector>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

using namespace std;
 
const int Border = 10;
const int BufferSize = 10;
const int FPS = 130;

const int GC_White = 0;
const int GC_Black = 1;
const int GC_Red = 2;
const int GC_Orange =3;

const int default_window_width = 800;
const int default_window_height = 600;

const int block_width = 80;
const int land_height_list[10] = {1,1,2,3,1,1,2,1,0,1};

int mario_move = 20;
int x_jump = 40;
int y_jump = 80;

enum Mario_state {FACELEFT, FACERIGHT, JUMPINGLEFT, JUMPINGRIGHT};
enum Mario_command {LEFT, RIGHT, JUMP, LAND, PAUSED, UNPAUSED, NONE};

struct XInfo {
	Display	 *display;
	int		 screen;
	Window	 window;
	GC		 gc[4];
	Pixmap pixmap;
	int		width;	
	int		height;
	Mario_command   command;
};


/*
 * Function to put out a message on error exits.
 */
void error( string str ) {
  cerr << str << endl;
  exit(0);
}


/*
 * An abstract class representing displayable things. 
 */
class Displayable {
	public:
		virtual void paint(XInfo &xinfo) = 0;
};       



class Sun : public Displayable {
	public:
		virtual void paint(XInfo &xinfo) {
			XFillArc(xinfo.display, xinfo.window, xinfo.gc[gc_idx], x, y, diameter, diameter, 0, 360*64);
		}
		
		void move(XInfo &xinfo) {
			x = x + direction;
			if (x < 0 || x > xinfo.width - diameter) {
				direction = -direction;
			}
		}
		
		int getX() {
			return x;
		}
		
		int getY() {
			return y;
		}
		
		Sun(int x, int y, int diameter, int gc_idx): x(x), y(y), diameter(diameter), gc_idx(gc_idx) {
			direction = 2;
		}
	
	private:
		int x;
		int y;
		int diameter;
		int direction;
		int gc_idx;
};

class Column  {
	public:
		int getColumnX() {
			return column_x;
		}
		int getColumnY() {
			return column_y;
		}
		int getColumnWidth(){
			return column_width;
		}
		int getColumnHeight(){
			return column_height;
		}
		int getNumBox(){
			return num_box;
		}
		Column(int x, int y, int w, int h, int n): column_x(x), column_y(y), column_width(w), column_height(h), num_box(n) {}

	private:
		int column_x;
		int column_y;
		int column_width;
		int column_height;
		int num_box;
};

class Mario : public Displayable{
	public:
		virtual void paint(XInfo &xinfo){
			XFillArc(xinfo.display, xinfo.window, xinfo.gc[gc_idx], x, y, diameter, diameter, 0, 360*64);
		}
	
	void move(XInfo &xinfo){
		switch(xinfo.command){
			case LEFT:
				if(state == FACERIGHT) state = FACELEFT;
				else if(x - mario_move < 0) x = 0;
				else if(x == 320 || x == 560) x = x;
				else if(x == 160 || x == 240 || x == 480 || x == 720) y = y - 80;
				else x = x - mario_move;
			
				xinfo.command = NONE;
				break;
			case RIGHT:
				if(state == FACELEFT) state = FACERIGHT;
				else if(x + diameter == 160 || x + diameter == 240 || x + diameter == 480) x = x;
				else if(x + diameter == 320) y = y - 160;
				else if(x + diameter == 560) y = y - 80;
				else if(x + diameter == 640) y = default_window_height;
				else x = x + mario_move	;			
				xinfo.command = NONE;
				break;
			case JUMP:
				if(state == FACELEFT) {
					if(!(x - (x_jump * 2) < 0)){
						x = x - x_jump;
						y = y + y_jump;
						state = JUMPINGLEFT;
					}
				}
				else if(state == FACERIGHT){
					x = x + x_jump;
					y = y + y_jump;
					state = JUMPINGRIGHT;
				}	
				break;
			case LAND:
				if(state == JUMPINGLEFT){
					x = x - x_jump;
					y = y - y_jump;
				}
				else if(state == JUMPINGRIGHT){
					x = x + x_jump;
					y = y - y_jump;
				}
				break;
			case NONE:
				break;
		}	
	}			

	Mario(int x, int y, int diameter, int gc_idx): x(x), y(y), diameter(diameter), gc_idx(gc_idx){
		state = FACERIGHT;
	}
	Mario_state get_state(){ return state; }
	private:
		int x;
		int y;
		int diameter;
		int gc_idx;
		Mario_state state;
};

list<Displayable*> dList;           // list of Displayables
Sun sun(50, 50, 120, GC_Red);
Mario mario(0, 480, 40, GC_Orange);
vector<Column> cList;		    // list of columns on land

/*
 * Initialize X and create a window
 */
void initX(int argc, char *argv[], XInfo &xInfo) {
	XSizeHints hints;
	unsigned long white, black;

	xInfo.display = XOpenDisplay( "" );
	if ( !xInfo.display )	{
		error( "Can't open display." );
	}
	
	xInfo.screen = DefaultScreen( xInfo.display );

	white = XWhitePixel( xInfo.display, xInfo.screen );
	black = XBlackPixel( xInfo.display, xInfo.screen );

	hints.x = 100;
	hints.y = 100;
	hints.width = default_window_width;
	hints.height = default_window_height;
	hints.flags = PPosition | PSize;

	xInfo.window = XCreateSimpleWindow( 
		xInfo.display,				// display where window appears
		DefaultRootWindow( xInfo.display ), // window's parent in window tree
		hints.x, hints.y,			// upper left corner location
		hints.width, hints.height,	// size of the window
		Border,						// width of window's border
		black,						// window border colour
		white );					// window background colour
		
	XSetStandardProperties(
		xInfo.display,		// display containing the window
		xInfo.window,		// window whose properties are set
		"Mario",		// window's title
		"MRO",			// icon's title
		None,				// pixmap for the icon
		argv, argc,			// applications command line args
		&hints );			// size hints for the window


	XColor color;
	Colormap colorMap;
	colorMap = DefaultColormap(xInfo.display, 0);

	/* 
	 * Create Graphics Contexts
	 */
	int i = GC_White;
	xInfo.gc[i] = XCreateGC(xInfo.display, xInfo.window, 0, 0);
	XSetForeground(xInfo.display, xInfo.gc[i], WhitePixel(xInfo.display, xInfo.screen));
	XSetBackground(xInfo.display, xInfo.gc[i], BlackPixel(xInfo.display, xInfo.screen));
	XSetFillStyle(xInfo.display, xInfo.gc[i], FillSolid);
	XSetLineAttributes(xInfo.display, xInfo.gc[i], 7, LineSolid, CapRound, JoinMiter);

	i = GC_Black;
	xInfo.gc[i] = XCreateGC(xInfo.display, xInfo.window, 0, 0);
	XSetForeground(xInfo.display, xInfo.gc[i], BlackPixel(xInfo.display, xInfo.screen));
	XSetBackground(xInfo.display, xInfo.gc[i], WhitePixel(xInfo.display, xInfo.screen));
	XSetFillStyle(xInfo.display, xInfo.gc[i], FillSolid);
	XSetLineAttributes(xInfo.display, xInfo.gc[i], 7, LineSolid, CapRound, JoinMiter);

	i = GC_Red;
	XParseColor(xInfo.display, colorMap, "Red", &color );
	XAllocColor(xInfo.display, colorMap, &color);
	xInfo.gc[i] = XCreateGC(xInfo.display, xInfo.window, 0, 0);
	XSetForeground(xInfo.display, xInfo.gc[i], color.pixel);
	XSetBackground(xInfo.display, xInfo.gc[i],color.pixel);
	XSetFillStyle(xInfo.display, xInfo.gc[i], FillStippled);
	XSetLineAttributes(xInfo.display, xInfo.gc[i],1, LineSolid, CapButt, JoinRound);

	i = GC_Orange;
	XParseColor(xInfo.display, colorMap, "Orange", &color );
	XAllocColor(xInfo.display, colorMap, &color);
	xInfo.gc[i] = XCreateGC(xInfo.display, xInfo.window, 0, 0);
	XSetForeground(xInfo.display, xInfo.gc[i], color.pixel);
	XSetBackground(xInfo.display, xInfo.gc[i],color.pixel);
	XSetFillStyle(xInfo.display, xInfo.gc[i], FillStippled);
	XSetLineAttributes(xInfo.display, xInfo.gc[i],1, LineSolid, CapButt, JoinRound);

	xInfo.command = NONE;

	int depth = DefaultDepth(xInfo.display, DefaultScreen(xInfo.display));
	xInfo.pixmap = XCreatePixmap(xInfo.display, xInfo.window, hints.width, hints.height, depth);
	xInfo.width = hints.width;
	xInfo.height = hints.height;

	XSelectInput(xInfo.display, xInfo.window, 
		ButtonPressMask | KeyPressMask |
		StructureNotifyMask);  // for resize events

	/*
	 * Put the window on the screen.
	 */
	XMapRaised( xInfo.display, xInfo.window );
	
	XFlush(xInfo.display);
	sleep(2);	// let server get set up before sending drawing commands
}

/*
 * Function to repaint a display list
 */
void repaint( XInfo &xinfo) {
	list<Displayable *>::const_iterator begin = dList.begin();
	list<Displayable *>::const_iterator end = dList.end();

	// big black rectangle to clear background
	//XFillRectangle(xinfo.display, xinfo.window, xinfo.gc[1], 0, 0, xinfo.width, xinfo.height);
	
	XCopyArea(xinfo.display, xinfo.pixmap, xinfo.window, xinfo.gc[GC_White], 0, 0, default_window_width, default_window_height, 0, 0);	
	// draw display list
	while( begin != end ) {
		Displayable *d = *begin;
		d->paint(xinfo);
		begin++;
	}
	XFlush( xinfo.display );
}


void handleButtonPress(XInfo &xinfo, XEvent &event) {
	printf("Got button press!\n");
	// dList.push_front(new Text(event.xbutton.x, event.xbutton.y, "Urrp!"));
	// repaint( dList, xinfo );
	
}

void handleKeyPress(XInfo &xinfo, XEvent &event) {
	KeySym key;
	char text[BufferSize];

	int i = XLookupString( 
		(XKeyEvent *)&event, 	// the keyboard event
		text, 					// buffer when text will be written
		BufferSize, 			// size of the text buffer
		&key, 					// workstation-independent key symbol
		NULL );					// pointer to a composeStatus structure (unused)
	if ( i == 1 || i == 0) {
		printf("Got key press -- %c\n", text[0]);
		if (text[0] == 'q') {
			error("Terminating normally.");
		}
		else if(key == 65361){
			xinfo.command = LEFT;
		}
		else if(key == 65363){
			xinfo.command = RIGHT;
		}
		else if(text[0] == 'j'){
			xinfo.command = JUMP;
		}
		else if(key == 32){
			if(xinfo.command != PAUSED) xinfo.command = PAUSED;
			else xinfo.command = UNPAUSED;
		}
		else{
			xinfo.command = NONE;
		}	
			
	}
}

// update width and height when window is resized
void handleResize(XInfo &xinfo, XEvent &event) {
	XConfigureEvent xce = event.xconfigure;
	fprintf(stderr, "Handling resize  w=%d  h=%d\n", xce.width, xce.height);
	if (xce.width != xinfo.width || xce.height != xinfo.height) {
		xinfo.width = xce.width;
		xinfo.height = xce.height;
	}
}


// get microseconds
unsigned long now() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}

void handleAnimation(XInfo &xinfo) {
	if(xinfo.command != PAUSED) {
		if(mario.get_state() == JUMPINGLEFT || mario.get_state() == JUMPINGRIGHT) xinfo.command == LAND;
		sun.move(xinfo);
		mario.move(xinfo);
		repaint(xinfo);
	}
}


void eventLoop(XInfo &xinfo) {
	dList.push_front(&sun);
	dList.push_front(&mario);
	XEvent event;
	unsigned long lastRepaint = 0;

	while( true ) {
		
		if (XPending(xinfo.display) > 0) {
			XNextEvent( xinfo.display, &event );
			switch( event.type ) {
				case ButtonPress:
					handleButtonPress(xinfo, event);
					break;
				case KeyPress:
					handleKeyPress(xinfo, event);
					break;
				case ConfigureNotify:
					handleResize(xinfo, event);
					break;	
			}
		} 
		unsigned long end = now();

		if(end - lastRepaint > 100000){
			handleAnimation(xinfo);
			lastRepaint = now();
		}
		else if(XPending(xinfo.display) == 0){
			usleep(100000 - (end - lastRepaint));
		}
	}
}

void drawMap(XInfo &xinfo){
	XFillRectangle(xinfo.display, xinfo.pixmap, xinfo.gc[GC_Black], 0, 0, default_window_width, default_window_height);
	
	int cur_x = 0;
	int cur_y = default_window_height;
	
	int cur_col_height = 0;
	
	
	for(int i=0; i<10; i++){
		for(int j=0; j < land_height_list[i]; j++){
			cur_y = cur_y - block_width;
			XDrawRectangle(xinfo.display, xinfo.pixmap, xinfo.gc[GC_White], cur_x, cur_y, block_width, block_width);
			
		}
		cur_x = cur_x + block_width;
		Column col(cur_x, cur_y, block_width, land_height_list[i]*80, land_height_list[i]);
		cList.push_back(col);
		cur_y = default_window_height;
	}
}


int main ( int argc, char *argv[] ) {
	XInfo xInfo;
	initX(argc, argv, xInfo);
	drawMap(xInfo);
	eventLoop(xInfo);
	XCloseDisplay(xInfo.display);
}