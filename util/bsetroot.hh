#ifndef   __bsetroot2_hh
#define   __bsetroot2_hh

#include "../src/BaseDisplay.hh"
#include "../src/Image.hh"


class bsetroot : public BaseDisplay {
public:
	bsetroot(int, char **, char * = 0);
	~bsetroot(void);

	inline virtual Bool handleSignal(int) { return False; }

	void gradient(void);
	void modula(int, int);
	void solid(void);
	void usage(int = 0);
	
private:
	BImageControl **img_ctrl;
	Pixmap *pixmaps;

	char *fore, *back, *grad;
	Display *display;
	int num_screens;
protected:
	inline virtual void process_event(XEvent *) { }

};


#endif // __bsetroot2_hh
