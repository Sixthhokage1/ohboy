
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/stat.h> /* for mkdir */


#ifdef WIZ
#include <fcntl.h>
#include <sys/mman.h>
#endif

#include <SDL/SDL.h>

#include "gnuboy.h"
#include "fb.h"
#include "input.h"
#include "rc.h"
#include "pcm.h"
#include "ubytegui/gui.h"
#include "hw.h"
#include "loader.h"
#include "gui_sdl.h"
#include "menu.h"

void ohb_loadrom(char *rom);

#define FONT_NAME "FreeUniversal-Regular.ttf"
#define FONT_SIZE 14
static font_t *font;

struct fb fb;
static int upscaler=0, frameskip=0, cpu_speed=0;
static char* romdir=0;
static int osd_persist=0;
static int dvolume=0;

rcvar_t vid_exports[] =
{
	RCV_INT("upscaler", &upscaler),
	RCV_INT("frameskip", &frameskip),
	RCV_INT("cpuspeed",&cpu_speed),
	RCV_STRING("romdir",&romdir),
	RCV_END
};

struct fb vid_fb;
static un16 vid_frame[160*144];
SDL_Surface *screen;
int framecounter = 0;

struct pcm pcm;
rcvar_t pcm_exports[] =
{
	RCV_END
};

#define PCM_BUFFER 4096
#define PCM_FRAME 512
#define PCM_SAMPLERATE 44100

#define UP_MASK 	0x83
#define DOWN_MASK 	0x38
#define LEFT_MASK 	0x0E
#define RIGHT_MASK 	0xE0

n16 *buf;
n16 pcm_buffer[PCM_BUFFER << 1];
byte pcm_frame[PCM_FRAME << 1];
volatile int pcm_buffered = 0;

n16 *pcm_head, *pcm_tail;
int pcm_volume=0x80, pcm_bufferlen;

SDL_Joystick * sdl_joy;
static const int joy_commit_range = 3276;
static char Xstatus, Ystatus;
static int use_joy=0;

#ifdef CAANOO
enum
{
     BTN_A = 0,     //       A /             1
     BTN_X = 1,     //       X /             2
     BTN_B = 2,     //       B /             3
     BTN_Y = 3,     //       Y /             4
     BTN_L = 4,     //       L /         5, L1
     BTN_R = 5,     //       R /         6, L2
     BTN_HOME = 6,  //    Home /         7, R1
     BTN_HOLD = 7,  //    Hold /         8, R2
     BTN_HELP1 = 8, //  Help I /        Select
     BTN_HELP2 = 9, // Help II /         Start
     BTN_TACT = 10, //    Tact / L Thumb Stick
     BTN_UP = 11, 
     BTN_DOWN = 12,
     BTN_LEFT = 13,
     BTN_RIGHT = 14,

};

void Caanoo_PushAnalogEvent(int btn, int pressed){
        
    SDL_Event event;
        
    event.type = (pressed)?SDL_JOYBUTTONDOWN:SDL_JOYBUTTONUP;
    event.jbutton.button = btn;
    event.jbutton.state = (pressed)?SDL_PRESSED:SDL_RELEASED;
    event.jbutton.which = 0;
        
    SDL_PushEvent(&event);      
}

void Caanoo_UpdateAnalog(void){
        
    static int buttonsPrev = 0;
    int buttonsNow = 0;
        
    SDL_JoystickUpdate();
                
    if (SDL_JoystickGetAxis( sdl_joy, 0 ) < -16384) buttonsNow |= (1 << BTN_LEFT);
    if (SDL_JoystickGetAxis( sdl_joy, 0 ) >  16384) buttonsNow |= (1 << BTN_RIGHT);
    if (SDL_JoystickGetAxis( sdl_joy, 1 ) < -16384) buttonsNow |= (1 << BTN_UP);
    if (SDL_JoystickGetAxis( sdl_joy, 1 ) >  16384) buttonsNow |= (1 << BTN_DOWN);
    
    if (buttonsNow != buttonsPrev)
    {
        if ((buttonsNow & (1 << BTN_LEFT)) != (buttonsPrev & (1 << BTN_LEFT)))
        {
                Caanoo_PushAnalogEvent(BTN_LEFT, (buttonsNow & (1 << BTN_LEFT)));
        }
        if ((buttonsNow & (1 << BTN_RIGHT)) != (buttonsPrev & (1 << BTN_RIGHT)))
        {
                Caanoo_PushAnalogEvent(BTN_RIGHT, (buttonsNow & (1 << BTN_RIGHT)));
        }
        if ((buttonsNow & (1 << BTN_UP)) != (buttonsPrev & (1 << BTN_UP)))
        {
                Caanoo_PushAnalogEvent(BTN_UP, (buttonsNow & (1 << BTN_UP)));
        }
        if ((buttonsNow & (1 << BTN_DOWN)) != (buttonsPrev & (1 << BTN_DOWN)))
        {
                Caanoo_PushAnalogEvent(BTN_DOWN, (buttonsNow & (1 << BTN_DOWN)));
        }
    }
    buttonsPrev = buttonsNow;
}
#endif

rcvar_t joy_exports[] =
{
	RCV_BOOL("joy", &use_joy),
	RCV_END
};

/* keymap - mappings of the form { scancode, localcode } - from sdl/keymap.c */
extern int keymap[][2];

#ifdef DINGOO_NATIVE
/*
int mkdir(const char *pathname, mode_t mode);
void mkdir(char *);
void mkdir(char *x)
{
    
}
*/
#define mkdir(x, y) fsys_mkdir(x)
#endif /* DINGOO_NATIVE */

void vid_init() {

	SDL_LockSurface(screen);
	vid_fb.w = screen->w;
	vid_fb.h = screen->h;
	vid_fb.pelsize = screen->format->BytesPerPixel;
	vid_fb.pitch = screen->pitch;
	vid_fb.indexed = fb.pelsize == 1;
	vid_fb.ptr = screen->pixels;
	vid_fb.cc[0].r = screen->format->Rloss;
	vid_fb.cc[0].l = screen->format->Rshift;
	vid_fb.cc[1].r = screen->format->Gloss;
	vid_fb.cc[1].l = screen->format->Gshift;
	vid_fb.cc[2].r = screen->format->Bloss;
	vid_fb.cc[2].l = screen->format->Bshift;
	SDL_UnlockSurface(screen);

	fb.w = 160;
	fb.h = 144;
	fb.pelsize = 2;
	fb.pitch = 320;
	fb.indexed = 0;
	fb.cc[0].r = screen->format->Rloss+2;
	fb.cc[0].l = screen->format->Rshift;
	fb.cc[1].r = screen->format->Gloss+2;
	fb.cc[1].l = screen->format->Gshift;
	fb.cc[2].r = screen->format->Bloss+2;
	fb.cc[2].l = screen->format->Bshift;
	fb.ptr = vid_frame;
	fb.enabled = 1;
	fb.dirty = 0;

}

void vid_setpal(int i, int r, int g, int b){
}

#define SCALE3X(l,r,t,b,E0,E1,E2,E3,E4,E5,E6,E7,E8)\
A = src[t+l], B = src[t], C = src[t+r];\
D = src[ l ], E = src[0], F = src[ r ];\
G = src[b+l], H = src[b], I = src[b+r];\
if (B != H && D != F) {\
	E0 += D == B ? D : E;\
	E1 += (D == B && E != C) || (B == F && E != A) ? B : E;\
	E2 += B == F ? F : E;\
	E3 += (D == B && E != G) || (D == H && E != A) ? D : E;\
	E4 += E;\
	E5 += (B == F && E != I) || (H == F && E != C) ? F : E;\
	E6 += D == H ? D : E;\
	E7 += (D == H && E != I) || (H == F && E != G) ? H : E;\
	E8 += H == F ? F : E;\
} else {\
	E0 += E;\
	E1 += E;\
	E2 += E;\
	E3 += E;\
	E4 += E;\
	E5 += E;\
	E6 += E;\
	E7 += E;\
	E8 += E;\
}

#ifdef WIZ

static un16 buffer[3][216];

void ohb_scale3x(){

	un16 *dst = buffer[0];
	un16 *src = (un16*)fb.ptr+159;
	un16 *base = (un16*)vid_fb.ptr + 9612;
	int x,y;

	un16 A,B,C,D,E,F,G,H,I;

	memset(buffer,0,432*3);

	// Top-right
	SCALE3X(-1,0,0,160,dst[216],dst[0],dst[0],dst[216],dst[0],dst[0],dst[217],dst[1],dst[1]);
	dst++, src+=160;
	// right
	for(y=1; y<143; y+=2){
		SCALE3X(-1,0,-160,160,dst[216],dst[0],dst[0],dst[217],dst[1],dst[1],dst[217],dst[1],dst[1]);
		dst+=2, src+=160;
		SCALE3X(-1,0,-160,160,dst[216],dst[0],dst[0],dst[216],dst[0],dst[0],dst[217],dst[1],dst[1]);
		dst++, src+=160;
	}
	// bottom-Right
	SCALE3X(-1,0,-160,0,dst[216],dst[0],dst[0],dst[217],dst[1],dst[1],dst[217],dst[1],dst[1]);
	dst+=2, src+=160;


	for(x=158; x>1; x-=2){
		src = (un16*)fb.ptr + x;
		// top
		SCALE3X(-1,1,0,160,dst[216],dst[216],dst[0],dst[216],dst[216],dst[0],dst[217],dst[217],dst[1]);
		dst++, src+=160;
		// middle
		for(y=1; y<143; y+=2){
			SCALE3X(-1,1,-160,160,dst[216],dst[216],dst[0],dst[217],dst[217],dst[1],dst[217],dst[217],dst[1]);
			dst+=2, src+=160;
			SCALE3X(-1,1,-160,160,dst[216],dst[216],dst[0],dst[216],dst[216],dst[0],dst[217],dst[217],dst[1]);
			dst++, src+=160;
		}
		// bottom
		SCALE3X(-1,1,-160,0,dst[216],dst[216],dst[0],dst[217],dst[217],dst[1],dst[217],dst[217],dst[1]);
		dst+=2, src+=160;

		memcpy(base,buffer[0],432);
		memcpy(base+240,buffer[1],432);
		memcpy(base+480,buffer[2],432);
		dst=buffer[0];
		base += 720;

		memset(buffer,0,432*3);

		src = (un16*)fb.ptr + x-1;
		// top
		SCALE3X(-1,1,0,160,dst[216],dst[0],dst[0],dst[216],dst[0],dst[0],dst[217],dst[1],dst[1]);
		dst++, src+=160;
		// middle
		for(y=1; y<143; y+=2){
			SCALE3X(-1,1,-160,160,dst[216],dst[0],dst[0],dst[217],dst[1],dst[1],dst[217],dst[1],dst[1]);
			dst+=2, src+=160;
			SCALE3X(-1,1,-160,160,dst[216],dst[0],dst[0],dst[216],dst[0],dst[0],dst[217],dst[1],dst[1]);
			dst++, src+=160;
		}
		// bottom
		SCALE3X(-1,1,-160,0,dst[216],dst[0],dst[0],dst[217],dst[1],dst[1],dst[217],dst[1],dst[1]);
		dst+=2, src+=160;
	}

	src = fb.ptr;
	// top-left
	SCALE3X(0,1,0,160,dst[216],dst[216],dst[0],dst[216],dst[216],dst[0],dst[217],dst[217],dst[1]);
	dst++, src+=160;
	// left
	for(y=1; y<143; y+=2){
		SCALE3X(0,1,-160,160,dst[216],dst[216],dst[0],dst[217],dst[217],dst[1],dst[217],dst[217],dst[1]);
		dst+=2, src+=160;
		SCALE3X(0,1,-160,160,dst[216],dst[216],dst[0],dst[216],dst[216],dst[0],dst[217],dst[217],dst[1]);
		dst++, src+=160;
	}
	// bottom-right
	SCALE3X(0,1,-160,0,dst[216],dst[216],dst[0],dst[217],dst[217],dst[1],dst[217],dst[217],dst[1]);
	dst+=2, src+=160;

	memcpy(base,buffer[0],432);
	memcpy(base+240,buffer[1],432);
	memcpy(base+480,buffer[2],432);
}

void ohb_render(){

	un16 *mix, *buf;

	un16 *src = fb.ptr;
	un16 *dst = (un16*)vid_fb.ptr + 9612;
	int x,y;

	for(x=159; x>0; x-=2){
		mix = buffer[0];
		src = (un16*)fb.ptr+x;
		for(y=0; y<144; y+=2){
			*(dst++) = (*(mix++) = *src<<1)<<1;
			*mix = *src; src+=160;
			*(dst++) = (*(mix++) += *src)<<1;
			*(dst++) = (*(mix++) = *src<<1)<<1;
			 src+=160;
		}
		dst += 24;
		mix = buffer[0];
		buf = buffer[1];
		src = (un16*)fb.ptr+x-1;
		for(y=0; y<144; y+=2){
			*(dst++) = *(mix++) + ((*(buf++)  = *src<<2)>>1);
			*buf = *src<<1; src+=160;
			*(dst++) = *(mix++) + ((*(buf++) += *src<<1)>>1);
			*(dst++) = *(mix++) + ((*(buf++)  = *src<<2)>>1);
			 src+=160;
		}
		dst += 24;
		memcpy(dst, buffer[1], 432);
		dst += 240;
	}
}
#else

static un16 buffer[3][240];

void ohb_scale3x(){

	un16 *dst = buffer[0];
	un16 *src = fb.ptr;
	un16 *base = (un16*)vid_fb.ptr + 3880;
	int x,y;

	un16 A,B,C,D,E,F,G,H,I;

	memset(buffer,0,480*3);

	// Top-left
	SCALE3X(0,1,0,160,dst[0],dst[0],dst[1],dst[0],dst[0],dst[1],dst[240],dst[240],dst[241]);
	dst++, src++;
	// Top
	for(x=1; x<159; x+=2){
		SCALE3X(-1,1,0,160,dst[0],dst[1],dst[1],dst[0],dst[1],dst[1],dst[240],dst[241],dst[241]);
		dst+=2, src++;
		SCALE3X(-1,1,0,160,dst[0],dst[0],dst[1],dst[0],dst[0],dst[1],dst[240],dst[240],dst[241]);
		dst++, src++;
	}
	// Top-Right
	SCALE3X(-1,0,0,160,dst[0],dst[1],dst[1],dst[0],dst[1],dst[1],dst[240],dst[241],dst[241]);
	dst+=2, src++;

	for(y=1; y<142; y+=2){
		// left
		SCALE3X(0,1,-160,160,dst[0],dst[0],dst[1],dst[240],dst[240],dst[241],dst[240],dst[240],dst[241]);
		dst++, src++;
		// middle
		for(x=1; x<159; x+=2){
			SCALE3X(-1,1,-160,160,dst[0],dst[1],dst[1],dst[240],dst[241],dst[241],dst[240],dst[241],dst[241]);
			dst+=2, src++;
			SCALE3X(-1,1,-160,160,dst[0],dst[0],dst[1],dst[240],dst[240],dst[241],dst[240],dst[240],dst[241]);
			dst++, src++;
		}
		// right
		SCALE3X(-1,0,-160,160,dst[0],dst[1],dst[1],dst[240],dst[241],dst[241],dst[240],dst[241],dst[241]);
		dst+=2, src++;

		memcpy(base,buffer[0],480);
		memcpy(base+320,buffer[1],480);
		memcpy(base+640,buffer[2],480);
		dst=buffer;
		base += 960;

		memset(buffer,0,480*3);
		// left
		SCALE3X(0,1,-160,160,dst[0],dst[0],dst[1],dst[0],dst[0],dst[1],dst[240],dst[240],dst[241]);
		dst++, src++;
		// middle
		for(x=1; x<159; x+=2){
			SCALE3X(-1,1,-160,160,dst[0],dst[1],dst[1],dst[0],dst[1],dst[1],dst[240],dst[241],dst[241]);
			dst+=2, src++;
			SCALE3X(-1,1,-160,160,dst[0],dst[0],dst[1],dst[0],dst[0],dst[1],dst[240],dst[240],dst[241]);
			dst++, src++;
		}
		// right
		SCALE3X(-1,0,-160,160,dst[0],dst[1],dst[1],dst[0],dst[1],dst[1],dst[240],dst[241],dst[241]);
		dst+=2, src++;
	}

	// left
	SCALE3X(0,1,-160,0,dst[0],dst[0],dst[1],dst[240],dst[240],dst[241],dst[240],dst[240],dst[241]);
	dst++, src++;
	// middle
	for(x=1; x<159; x+=2){
		SCALE3X(-1,1,-160,0,dst[0],dst[1],dst[1],dst[240],dst[241],dst[241],dst[240],dst[241],dst[241]);
		dst+=2, src++;
		SCALE3X(-1,1,-160,0,dst[0],dst[0],dst[1],dst[240],dst[240],dst[241],dst[240],dst[240],dst[241]);
		dst++, src++;
	}
	// right
	SCALE3X(-1,0,-160,0,dst[0],dst[1],dst[1],dst[240],dst[241],dst[241],dst[240],dst[241],dst[241]);
	dst+=2, src++;

	memcpy(base,buffer[0],480);
	memcpy(base+320,buffer[1],480);
	memcpy(base+640,buffer[2],480);
}

void ohb_render(){

	un16 *mix, *buf;

	un16 *src = fb.ptr;
	un16 *dst = (un16*)vid_fb.ptr + 3880;
	int x,y;

	for(y=0; y<144; y+=2){
		mix = buffer[0];
		for(x=0; x<160; x+=2){
			*(dst++) = (*(mix++) = (*src<<1))<<1;
			*mix = *(src++);
			*(dst++) = (*(mix++) += *src)<<1;
			*(dst++) = (*(mix++) = (*(src++)<<1))<<1;
		}
		dst += 80;
		mix = buffer[0];
		buf = buffer[1];
		for(x=0; x<160; x+=2){
			*(dst++) = *(mix++) + ((*(buf++)  = (*src<<2))>>1);
			*buf = *(src++)<<1;
			*(dst++) = *(mix++) + ((*(buf++) += (*src<<1))>>1);
			*(dst++) = *(mix++) + ((*(buf++)  = (*(src++)<<2))>>1);
		}
		dst += 80;
		memcpy(dst, buffer[1], 480);
		dst += 320;
	}
}
#endif

void vid_preinit(){
}

void vid_close(){
}

void vid_settitle(char *title){
}
#ifdef WIZ
int ohb_updatecpu(int mhz){

	static int current_mhz = 0, def_v=0;
	volatile unsigned int *memregl;
	int mdiv, pdiv, sdiv = 0;
	int memdev, v=0;

	if(mhz==current_mhz) return 1;

	if(mhz){
		pdiv = 9;
		mdiv = (mhz * pdiv) / 27;
		if (mdiv & ~0x3ff) return 0;
		v = (pdiv<<18) | (mdiv<<8) | sdiv;
	}

	memdev = open("/dev/mem", O_RDWR);
	memregl	= mmap(0, 0x20000, PROT_READ|PROT_WRITE, MAP_SHARED, memdev, 0xc0000000);

	if(!def_v){
		def_v = memregl[0xf004>>2];
	} else if(!mhz){
		v = def_v;
	}

	memregl[0xf004>>2] = v;
	memregl[0xf07c>>2] |= 0x8000;

	munmap((void *)memregl, 0x20000);
	close(memdev);

	current_mhz = mhz;

	return v;
}
#else
int ohb_updatecpu(int mhz){
	return 1;
}
#endif

void vid_begin(){
	ohb_updatecpu(cpu_speed);
	if(frameskip>=0)
		vid_fb.enabled = framecounter==0;
	fb.enabled = vid_fb.enabled;
	fb.ptr = vid_frame;
}

void osd_volume(){
    int rounded=0; /* NOTE TODO FIXME unused */
	static const char *volstr = "Volume";
	static int x = 0;
	int w;
	if(!x) x = font_textwidth(font,volstr)+4;

	w = (pcm_volume * (screen->w-x-4)) >> 8;

	osd_cls(0,screen->h-font->height-4,screen->w,font->height+4);
	osd_drawtext(font,volstr,2,screen->h+font->descent-2,gui_maprgb(0xFF,0xFF,0xFF));
	osd_drawrect(2+x,screen->h-font->height-2,w,font->height,gui_maprgb(0xFF,0xFF,0xFF), rounded);
}

void vid_end() {
	if(fb.enabled){
		vid_fb.ptr = screen->pixels;
		SDL_LockSurface(screen);

		if(vid_fb.dirty){
			memset(vid_fb.ptr,0,vid_fb.pitch*vid_fb.h);
			vid_fb.dirty = 0;
		}

#define USE_OHBOY_SCALE
#ifdef USE_OHBOY_SCALE
		if(upscaler)
			ohb_scale3x();
		else
			ohb_render();
#endif /* USE_OHBOY_SCALE */

		if(osd_persist || dvolume){
			osd_volume();
			vid_fb.dirty=1;
			if(osd_persist) osd_persist--;
		}
		SDL_UnlockSurface(screen);
		SDL_Flip(screen);
	}
	framecounter++;
	if(framecounter>frameskip) framecounter = 0;

}

static void audio_callback(void *d, byte *stream, int len) {
	if(pcm_buffered==0) return;
	memcpy(stream, pcm_tail, len);
	pcm_tail += pcm.len;
	if(pcm_tail == pcm_buffer+pcm_bufferlen) pcm_tail = pcm_buffer;
	pcm_buffered -= pcm.len;
}

void pcm_init(){
	SDL_AudioSpec as;
	SDL_InitSubSystem(SDL_INIT_AUDIO);

	as.freq = PCM_SAMPLERATE;
	as.format = AUDIO_S16;
	as.channels = 2;
	as.samples = PCM_FRAME;
	as.callback = audio_callback;
	as.userdata = 0;

	if (SDL_OpenAudio(&as, 0))
		return;

	pcm.hz = as.freq;
	pcm.stereo = as.channels - 1;
	pcm.len = PCM_FRAME<<pcm.stereo;
	pcm.buf = pcm_frame;
	pcm.pos = 0;
	memset(pcm.buf, 0, pcm.len);

	pcm_head = pcm_buffer;
	pcm_tail = pcm_buffer;
	pcm_bufferlen = PCM_BUFFER << pcm.stereo;
	pcm_buffered = 0;

	SDL_PauseAudio(0);
}

int pcm_submit(){
	static int lastbuffer = 0;
	byte *src;
	n16 sample;
#define DEBUG_DISABLE_SOUND
#undef DEBUG_DISABLE_SOUND
#ifdef DEBUG_DISABLE_SOUND
return 0; /* no sound */
#endif /* DEBUG_DISABLE_SOUND */
    
	if(pcm.pos<pcm.len)
		return 1;

	if (pcm_buffered>=1) while(pcm_buffered == pcm_bufferlen);

	src = pcm.buf;

	pcm_volume += dvolume;
	if(pcm_volume < 0)
		pcm_volume = 0;
	else if(pcm_volume >256)
		pcm_volume = 256;

	while(pcm.pos){
		sample = (*src++) - 128;
		sample = sample * pcm_volume;
		*(pcm_head++) = sample;
		pcm.pos--;
	}
	if(pcm_head == pcm_buffer + pcm_bufferlen)
		pcm_head = pcm_buffer;

	pcm_buffered += pcm.len;
	if(frameskip==-1){
		if(pcm_buffered<lastbuffer)
			vid_fb.enabled = 0;
		else if(pcm_buffered==pcm_bufferlen || pcm_buffered==0)
			vid_fb.enabled = 1;
	}
	lastbuffer = pcm_buffered;

	return 1;
}

void pcm_close() {
	SDL_CloseAudio();
}

void *sys_timer()
{
	Uint32 *tv;

	tv = malloc(sizeof *tv);
	*tv = SDL_GetTicks() * 1000;
	return tv;
}

int sys_elapsed(void *in_ptr)
{
	Uint32 *cl;
	Uint32 now;
	Uint32 usecs;

	cl = (Uint32 *) in_ptr;
	now = SDL_GetTicks() * 1000;
	usecs = now - *cl;
	*cl = now;
	return (int) usecs;
}


void sys_sleep(int us)
{
	if(us>0) SDL_Delay(us/1000);
}

void sys_sanitize(char *s)
{
#ifndef DINGOO_NATIVE
	int i;
	for (i = 0; s[i]; i++)
		if (s[i] == '\\') s[i] = '/';
#endif /* DINGOO_NATIVE */
}

void sys_initpath(char *exe)
{
	char *buf, *home, *p;

	home = strdup(exe);
	sys_sanitize(home);
	p = strrchr(home, '/');
	if (p) *p = 0;
	else
	{
		buf = ".";
		rc_setvar("rcpath", 1, &buf);
		rc_setvar("savedir", 1, &buf);
		return;
	}
	buf = malloc(strlen(home) + 8);
	sprintf(buf, ".;%s/", home);
	rc_setvar("rcpath", 1, &buf);
	sprintf(buf, ".", home);
	rc_setvar("savedir", 1, &buf);
	free(buf);
}

void sys_checkdir(char *path, int wr) {
}

static int mapscancode(SDLKey sym)
{
	/* this could be faster:  */
	/*  build keymap as int keymap[256], then ``return keymap[sym]'' */

	int i;
	for (i = 0; keymap[i][0]; i++)
		if (keymap[i][0] == sym)
			return keymap[i][1];
	if (sym >= '0' && sym <= '9')
		return sym;
	if (sym >= 'a' && sym <= 'z')
		return sym;
	return 0;
}

void ev_poll()
{
	event_t ev;
	SDL_Event event;
	int axisval;

#ifdef GP2X
	while (GP2X_PollEvent(&event))
#else
	while (SDL_PollEvent(&event))
#endif
	{
		switch(event.type)
		{
		case SDL_ACTIVEEVENT:
			if (event.active.state == SDL_APPACTIVE)
				fb.enabled = event.active.gain;
			break;
		case SDL_KEYDOWN:
			if(event.key.keysym.sym==SDLK_EQUALS){
				dvolume = 1;
			} else if(event.key.keysym.sym==SDLK_MINUS){
				dvolume = - 1;
			} else if(event.key.keysym.sym==SDLK_ESCAPE){
				dvolume = 0;
				osd_persist = 0;
				hw.pad = 0;
				menu();
			}
			ev.type = EV_PRESS;
			ev.code = mapscancode(event.key.keysym.sym);
			ev_postevent(&ev);
			break;
		case SDL_KEYUP:
			if(event.key.keysym.sym==SDLK_EQUALS){
				dvolume = 0;
				osd_persist = 60;
			} else if(event.key.keysym.sym==SDLK_MINUS){
				dvolume = 0;
				osd_persist = 60;
			}
			ev.type = EV_RELEASE;
			ev.code = mapscancode(event.key.keysym.sym);
			ev_postevent(&ev);
			break;
		case SDL_JOYAXISMOTION:
			switch (event.jaxis.axis)
			{
			case 0: /* X axis */
				axisval = event.jaxis.value;
				if (axisval > joy_commit_range)
				{
					if (Xstatus==2) break;

					if (Xstatus==0)
					{
						ev.type = EV_RELEASE;
						ev.code = K_JOYLEFT;
        			  		ev_postevent(&ev);
					}

					ev.type = EV_PRESS;
					ev.code = K_JOYRIGHT;
					ev_postevent(&ev);
					Xstatus=2;
					break;
				}

				if (axisval < -(joy_commit_range))
				{
					if (Xstatus==0) break;

					if (Xstatus==2)
					{
						ev.type = EV_RELEASE;
						ev.code = K_JOYRIGHT;
						ev_postevent(&ev);
					}

					ev.type = EV_PRESS;
					ev.code = K_JOYLEFT;
					ev_postevent(&ev);
					Xstatus=0;
					break;
				}

				/* if control reaches here, the axis is centered,
				 * so just send a release signal if necisary */

				if (Xstatus==2)
				{
					ev.type = EV_RELEASE;
					ev.code = K_JOYRIGHT;
					ev_postevent(&ev);
				}

				if (Xstatus==0)
				{
					ev.type = EV_RELEASE;
					ev.code = K_JOYLEFT;
					ev_postevent(&ev);
				}
				Xstatus=1;
				break;

			case 1: /* Y axis*/
				axisval = event.jaxis.value;
				if (axisval > joy_commit_range)
				{
					if (Ystatus==2) break;

					if (Ystatus==0)
					{
						ev.type = EV_RELEASE;
						ev.code = K_JOYUP;
						ev_postevent(&ev);
					}

					ev.type = EV_PRESS;
					ev.code = K_JOYDOWN;
					ev_postevent(&ev);
					Ystatus=2;
					break;
				}

				if (axisval < -joy_commit_range)
				{
					if (Ystatus==0) break;

					if (Ystatus==2)
					{
						ev.type = EV_RELEASE;
						ev.code = K_JOYDOWN;
						ev_postevent(&ev);
					}

					ev.type = EV_PRESS;
					ev.code = K_JOYUP;
					ev_postevent(&ev);
					Ystatus=0;
					break;
				}

				/* if control reaches here, the axis is centered,
				 * so just send a release signal if necisary */

				if (Ystatus==2)
				{
					ev.type = EV_RELEASE;
					ev.code = K_JOYDOWN;
					ev_postevent(&ev);
				}

				if (Ystatus==0)
				{
					ev.type = EV_RELEASE;
					ev.code = K_JOYUP;
					ev_postevent(&ev);
				}
				Ystatus=1;
				break;
			}
			break;
		case SDL_JOYBUTTONUP:
			if(event.jbutton.button==16){
				dvolume = 0;
				osd_persist = 60;
			} else if(event.jbutton.button==17){
				dvolume = 0;
				osd_persist = 60;
			}
			if (event.jbutton.button>15) break;
			ev.type = EV_RELEASE;
			ev.code = K_JOY0 + event.jbutton.button;
			ev_postevent(&ev);
			break;
		case SDL_JOYBUTTONDOWN:
			if(event.jbutton.button==16){
				dvolume = 1;
			} else if(event.jbutton.button==17){
				dvolume = - 1;
			} else if(event.jbutton.button==6){
				dvolume = 0;
				osd_persist = 0;
				hw.pad = 0;
				menu();
			}
			if (event.jbutton.button>15) break;
			ev.type = EV_PRESS;
			ev.code = K_JOY0+event.jbutton.button;
			ev_postevent(&ev);
			break;
		case SDL_QUIT:
			exit(1);
			break;
		default:
			break;
		}
	}
}

static int bad_signals[] =
{
	/* These are all standard, so no need to #ifdef them... */
	SIGINT, SIGSEGV, SIGTERM, SIGFPE, SIGABRT, SIGILL,
#ifdef SIGQUIT
	SIGQUIT,
#endif
#ifdef SIGPIPE
	SIGPIPE,
#endif
	0
};

static void fatalsignal(int s)
{
	die("Signal %d\n", s);
}

static void catch_signals()
{
	int i;
	for (i = 0; bad_signals[i]; i++)
		signal(bad_signals[i], fatalsignal);
}

static void shutdown()
{
	ohb_updatecpu(0);
	dialog_close();
	vid_close();
	pcm_close();
	SDL_Quit();
	fb.enabled = 0;
}

void ohb_loadrom(char *rom){
	char *base, *save, *ext=0, *tok;
	sys_sanitize(rom);

	save = strdup(rom);
	base = strtok(save,"/");
	while(tok = strtok(NULL,"/"))
		base = tok;
	tok = base;
	while(*tok){
		if(*tok == '.') ext = tok;
		tok++;
	}
	if(ext)
		*ext = 0;

	rc_setvar("savename",1,&base);
	free(save);

	loader_init(rom);
	emu_reset();
}

int main(int argc, char *argv[]){
	FILE *config;
	char *rom;
	int x, y;
	pixmap_t *pix;
	char *cpu;

	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK);
#ifdef WIZ
	screen = WIZ_SetVideoMode(320, 240, 16, SDL_SWSURFACE);
#else
	screen = SDL_SetVideoMode(320, 240, 16, SDL_SWSURFACE);
#endif
	SDL_ShowCursor(0);

	sdl_joy = SDL_JoystickOpen(0);
	SDL_JoystickEventState(SDL_ENABLE);

	font = font_load("etc/"FONT_NAME, 0, FONT_SIZE);
	if(!dialog_init(font,gui_maprgb(255,255,255)))
		die("GUI: Could not initialise GUI (maybe missing font file)\n");

	init_exports();

	/* Start: gnuboy default settings */
	rc_command("bind esc quit");
	rc_command("bind up +up");
	rc_command("bind down +down");
	rc_command("bind left +left");
	rc_command("bind right +right");
	rc_command("bind d +a");
	rc_command("bind s +b");
	rc_command("bind enter +start");
	rc_command("bind space +select");
	rc_command("bind tab +select");
	rc_command("bind joyup +up");
	rc_command("bind joydown +down");
	rc_command("bind joyleft +left");
	rc_command("bind joyright +right");
	rc_command("bind joy0 +b");
	rc_command("bind joy1 +a");
	rc_command("bind joy2 +select");
	rc_command("bind joy3 +start");
	rc_command("bind 1 \"set saveslot 1\"");
	rc_command("bind 2 \"set saveslot 2\"");
	rc_command("bind 3 \"set saveslot 3\"");
	rc_command("bind 4 \"set saveslot 4\"");
	rc_command("bind 5 \"set saveslot 5\"");
	rc_command("bind 6 \"set saveslot 6\"");
	rc_command("bind 7 \"set saveslot 7\"");
	rc_command("bind 8 \"set saveslot 8\"");
	rc_command("bind 9 \"set saveslot 9\"");
	rc_command("bind 0 \"set saveslot 0\"");
	rc_command("bind ins savestate");
	rc_command("bind del loadstate");
	/* End: gnuboy default settings */
    
	rc_command("set savedir saves");
	rc_command("set stereo true");

#ifdef CAANOO
	rc_command("bind joyup +up");
	rc_command("bind joydown +down");
	rc_command("bind joyleft +left");
	rc_command("bind joyright +right");
	rc_command("bind joy0 +b");
	rc_command("bind joy3 +a");
	rc_command("bind joy8 +start");
	rc_command("bind joy9 +select");
#endif

#ifdef GP2X_ONLY
	rc_command("bind joyup +up");
	rc_command("bind joydown +down");
	rc_command("bind joyleft +left");
	rc_command("bind joyright +right");
	rc_command("bind joy13 +a");
	rc_command("bind joy14 +b");
	rc_command("bind joy8 +start");
	rc_command("bind joy9 +select");
#endif

	rc_command("set upscaler 0");
	rc_command("set frameskip 0");
	rc_command("set clockspeed 0");
	rc_command("set romdir \"./roms\"");
	rc_sourcefile("ohboy.rc");

	mkdir("./saves", 0777);

	pix = pixmap_loadpng("etc/launch.png");
	if(pix){
		SDL_LockSurface(screen);
		x = (screen->w - pix->width)/2;
		y = (screen->h - pix->height)/2;
		osd_drawpixmap(pix,x,y,0);
		pixmap_free(pix);
		SDL_UnlockSurface(screen);
	}

	do{
		rom = launcher();
	}while(!rom);

	memset(screen->pixels,0,screen->pitch*screen->h);

	vid_preinit();
	atexit(shutdown);
	catch_signals();
	vid_init();
	pcm_init();
	ohb_loadrom(rom);
	emu_run();
	return 0;
}

int GP2X_PollEvent(SDL_Event *ev){

	static unsigned int joy_status = 0;
	static signed int joy_x=0, joy_y=0;
	SDL_Event event;
	SDL_Event jevent;
	unsigned int status;

	status = joy_status;
		
#ifdef CAANOO
	Caanoo_UpdateAnalog();
#endif

	if (SDL_PollEvent(&event)) switch(event.type){
		case SDL_JOYBUTTONUP:
			if(event.jbutton.which==0 && event.jbutton.button<8){
				status &= ~(1<<event.jbutton.button);
				break;
			}
			*ev = event;
			return 1;
		case SDL_JOYBUTTONDOWN:
			if(event.jbutton.which==0 && event.jbutton.button<8){
				status |= 1<<event.jbutton.button;
				break;
			}
			*ev = event;
			return 1;
		default:
			*ev = event;
			return 1;
	} else {
		return 0;
	}

	if(status==joy_status) return 0;

#ifdef GP2X_ONLY
	jevent.type = SDL_JOYAXISMOTION;
	jevent.jaxis.which = 0;

	jevent.jaxis.axis = 1;
	jevent.jaxis.value = 0;
	jevent.jaxis.value += status & UP_MASK ? -32768 : 0;
	jevent.jaxis.value += status & DOWN_MASK ? 32767 : 0;
	if(joy_y != jevent.jaxis.value) SDL_PushEvent(&jevent);
	joy_y = jevent.jaxis.value;

	jevent.jaxis.axis = 0;
	jevent.jaxis.value = 0;
	jevent.jaxis.value += status & LEFT_MASK ? -32768 : 0;
	jevent.jaxis.value += status & RIGHT_MASK ? 32767 : 0;
	if(joy_x != jevent.jaxis.value) SDL_PushEvent(&jevent);
	joy_x = jevent.jaxis.value;
#endif
	joy_status = status;

	*ev = event;

	return 1;
}

