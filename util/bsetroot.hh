#ifndef BSETROOT_HH
#define BSETROOT_HH

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
	void setRootAtoms(Pixmap pixmap, int screen);

private:
	BImageControl **img_ctrl;
	Pixmap *pixmaps;

	char *fore, *back, *grad;
	Display *display;
	int num_screens;
protected:
	inline virtual void process_event(XEvent *) { }

};


#endif // BSETROOT_HH
