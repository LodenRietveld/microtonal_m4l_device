#include "ext.h"
#include "ext_obex.h"

typedef struct _denote_microtonal {
	t_object p_ob;
    long inlet_val[8];
	void *out[8];
    long notes[8][3];
    t_atom list_out[3];
    int chordElementRouting[8];
    void* proxy;
    long proxy_inletnum;
} t_denote_microtonal;

#define SEMITONE    0.059463

#define MIDI_NOTE   0
#define VELOCITY    1
#define PITCHBEND   2

#define LENGTH      8

const double accidentals[19] = {1, 1.0125, 0.987654, 0.972222, 1.028571, 1.03125, 0.969697, 1.015625, 0.984615, 1.02, 0.980392, 0.989583, 1.010526, 1.022222, 0.978261, 1.006944, 0.993103, 1.033333, 0.967742};


void denote_microtonal_list0(t_denote_microtonal *x, t_symbol *s, long argc, t_atom *argv);
void denote_microtonal_in0(t_denote_microtonal *x, long n);
void denote_microtonal_in1(t_denote_microtonal *x, long n);
void denote_microtonal_in2(t_denote_microtonal *x, long n);
void denote_microtonal_in3(t_denote_microtonal *x, long n);
void denote_microtonal_in4(t_denote_microtonal *x, long n);
void denote_microtonal_in5(t_denote_microtonal *x, long n);
void denote_microtonal_in6(t_denote_microtonal *x, long n);
void denote_microtonal_in7(t_denote_microtonal *x, long n);
void denote_microtonal_assist(t_denote_microtonal *x, void *b, long m, long a, char *s);
void *denote_microtonal_new(long n);


t_class *denote_microtonal_class;		// global pointer to the object class - so max can reference the object


//--------------------------------------------------------------------------

void ext_main(void *r)
{
	t_class *c;

	c = class_new("denote_microtonal", (method)denote_microtonal_new, (method)NULL, sizeof(t_denote_microtonal), 0L, A_DEFLONG, 0);
	// class_new() loads our external's class into Max's memory so it can be used in a patch
	// denote_microtonal_new = object creation method defined below

    class_addmethod(c, (method)denote_microtonal_list0,    "list",     A_GIMME, 0);
    class_addmethod(c, (method)denote_microtonal_in0,      "int",      A_LONG, 0);
    class_addmethod(c, (method)denote_microtonal_in1,      "in1",      A_LONG, 0); // the method for an int in the right inlet (inlet 1)
    class_addmethod(c, (method)denote_microtonal_in2,      "in2",      A_LONG, 0); // the method for an int in the right inlet (inlet 1)
    class_addmethod(c, (method)denote_microtonal_in3,      "in3",      A_LONG, 0); // the method for an int in the right inlet (inlet 1)
    class_addmethod(c, (method)denote_microtonal_in4,      "in4",      A_LONG, 0); // the method for an int in the right inlet (inlet 1)
    class_addmethod(c, (method)denote_microtonal_in5,      "in5",      A_LONG, 0); // the method for an int in the right inlet (inlet 1)
    class_addmethod(c, (method)denote_microtonal_in6,      "in6",      A_LONG, 0); // the method for an int in the right inlet (inlet 1)
    class_addmethod(c, (method)denote_microtonal_in7,      "in7",      A_LONG, 0); // the method for an int in the right inlet (inlet 1)
    
	class_addmethod(c, (method)denote_microtonal_assist,	"assist",	A_CANT, 0);	// (optional) assistance method needs to be declared like this

	class_register(CLASS_BOX, c);
	denote_microtonal_class = c;

	post("denote_microtonal has finished loading",0);	// post any important info to the max window when our class is loaded
}


//--------------------------------------------------------------------------

void *denote_microtonal_new(long n)		// n = int argument typed into object box (A_DEFLONG) -- defaults to 0 if no args are typed
{
	t_denote_microtonal *x;				// local variable (pointer to a t_denote_microtonal data structure)

	x = (t_denote_microtonal *)object_alloc(denote_microtonal_class); // create a new instance of this object
    
    for (short i = 0; i < LENGTH; i++){
        if (i > 0){
            intin(x, i);
        }
        x->out[i] = listout(x);        // create an int outlet and assign it to our outlet variable in the instance's data structure
    }
    
	

    memset(x->inlet_val, 0, sizeof(long) * 8);
    memset(x->chordElementRouting, -1, sizeof(int) * 8);

	return(x);					// return a reference to the object instance
}


//--------------------------------------------------------------------------

void denote_microtonal_assist(t_denote_microtonal *x, void *b, long m, long a, char *s) // 4 final arguments are always the same for the assistance method
{
	if (m == ASSIST_OUTLET)
		sprintf(s,"List of midi note, velocity and pitchbend");
	else {
		switch (a) {
		case 0:
			sprintf(s,"Inlet %ld: list or int. List registers notes and triggers output, int changes pitchbend", a);
			break;
        //fallthrough
		case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
			sprintf(s,"Inlet %ld: int. Changes pitchbend for that note (in order) in the chord", a);
			break;
		}
	}
}

void update_pb(t_denote_microtonal *x, long index){
    x->notes[index][PITCHBEND] = (((accidentals[x->inlet_val[index]] - 1) / SEMITONE) * 8192);
    
    t_atom l[2];
    
    atom_setsym(l, gensym("pb"));
    atom_setlong(l+1, x->notes[index][PITCHBEND]);
    
    outlet_list(x->out[index], NULL, 2, l);
}

void update_chord(t_denote_microtonal *x, long changed){
    atom_setlong(x->list_out+MIDI_NOTE, x->notes[changed][MIDI_NOTE]);
    atom_setlong(x->list_out+VELOCITY, x->notes[changed][VELOCITY]);
    atom_setlong(x->list_out+PITCHBEND, x->notes[changed][PITCHBEND]);
    outlet_list(x->out[changed], NULL, 3, x->list_out);
}

void denote_microtonal_list0(t_denote_microtonal *x, t_symbol *s, long argc, t_atom *argv){
    if (argc == 2){
        long note = atom_getlong(argv);
        long vel = atom_getlong(argv+1);
        uint8_t changed = 0;
        bool add = (vel > 0);
        bool found = false;
        
        int index = -1;
        for (int i = LENGTH-1; i >= 0; i--){
            if (x->chordElementRouting[i] == -1 && index == -1){
                index = i;
            }
            
            if (x->chordElementRouting[i] == note){
                found = true;
                index = i;
            }
        }
        
        if (found && !add){
            changed = index;
            x->chordElementRouting[index] = -1;
            x->notes[index][MIDI_NOTE] = note;
            x->notes[index][VELOCITY] = vel;
        } else if (!found && add && index != -1){
            changed = index;
            x->chordElementRouting[index] = note;
            x->notes[index][MIDI_NOTE] = note;
            x->notes[index][VELOCITY] = vel;
        }
        
        update_chord(x, changed);
    }
}

void denote_microtonal_in0(t_denote_microtonal *x, long n)	// x = the instance of the object, n = the int received in the inlet
{
    x->inlet_val[7] = n;
    update_pb(x, 7);
}


void denote_microtonal_in1(t_denote_microtonal *x, long n)
{
    x->inlet_val[6] = n;
    update_pb(x, 6);
}

void denote_microtonal_in2(t_denote_microtonal *x, long n)
{
    x->inlet_val[5] = n;
    update_pb(x, 5);
}


void denote_microtonal_in3(t_denote_microtonal *x, long n)
{
    x->inlet_val[4] = n;
    update_pb(x, 4);
}


void denote_microtonal_in4(t_denote_microtonal *x, long n)
{
    x->inlet_val[3] = n;
    update_pb(x, 3);
}


void denote_microtonal_in5(t_denote_microtonal *x, long n)
{
    x->inlet_val[2] = n;
    update_pb(x, 2);
}


void denote_microtonal_in6(t_denote_microtonal *x, long n)
{
    x->inlet_val[1] = n;
    update_pb(x, 1);
}


void denote_microtonal_in7(t_denote_microtonal *x, long n)
{
    x->inlet_val[0] = n;
    update_pb(x, 0);
}





