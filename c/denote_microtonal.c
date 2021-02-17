#include "ext.h"
#include "ext_obex.h"
#include <math.h>
#include "ext_dictionary.h"

#define NUM_NOTE_RATIOS 12
#define MAX_CHORD_SIZE 8

const char notenames[12][3] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

typedef struct ratio_list {
    uint8_t len;
    double* ratio_arr;
    long active_ratio;
} ratio_list_t;

typedef struct _denote_microtonal {
	t_object p_ob;
	void *out[MAX_CHORD_SIZE];
    
    long notes[MAX_CHORD_SIZE][3];
    long note_mod_index[NUM_NOTE_RATIOS];           //array that stores the current note voice (if active), otherwise -1
    t_atom list_out[3];
    int chordElementRouting[MAX_CHORD_SIZE];
    ratio_list_t ratio_list[NUM_NOTE_RATIOS];
    bool dict_processed;
    t_dictionary* d;
} t_denote_microtonal;

#define SEMITONE    0.059463

#define MIDI_NOTE   0
#define VELOCITY    1
#define PITCHBEND   2

#define LENGTH      8

t_symbol* SYM_RATIO;
t_symbol* SYM_NOTE;
t_symbol* SYM_LOADDICT;
t_symbol** SYM_NOTES;
t_symbol* SYM_INIT;
t_symbol* SYM_PB;
t_symbol* SYM_RATIOS;
t_symbol* SYM_NAMES;



void denote_microtonal_read_dictionary(t_denote_microtonal *x, t_symbol *s, long argc, t_atom *argv);
void denote_microtonal_process_ratio_change(t_denote_microtonal* x, t_symbol *s, long argc, t_atom *argv);
void denote_microtonal_note_list(t_denote_microtonal *x, t_symbol *s, long argc, t_atom *argv);
void denote_microtonal_assist(t_denote_microtonal *x, void *b, long m, long a, char *s);
double calculate_pitchbend(long mod_note_index, double ratio);
void *denote_microtonal_new(long n);
void denote_microtonal_free(t_denote_microtonal* x);


t_class *denote_microtonal_class;		// global pointer to the object class - so max can reference the object


//--------------------------------------------------------------------------

void ext_main(void *r)
{
	t_class *c;

	c = class_new("denote_microtonal", (method)denote_microtonal_new, (method)denote_microtonal_free, sizeof(t_denote_microtonal), 0L, A_DEFLONG, 0);

    class_addmethod(c, (method)denote_microtonal_note_list,             "note",         A_GIMME, 0);
    class_addmethod(c, (method)denote_microtonal_read_dictionary,       "loaddict",     A_GIMME, 0);
    class_addmethod(c, (method)denote_microtonal_process_ratio_change,  "set_ratio",    A_GIMME, 0);
	class_addmethod(c, (method)denote_microtonal_assist,	            "assist",	    A_CANT, 0);	// (optional) assistance method needs to be declared like this

	class_register(CLASS_BOX, c);
	denote_microtonal_class = c;
    
    SYM_NOTE = gensym("note");
    SYM_RATIO = gensym("sel_ratio");
    SYM_LOADDICT = gensym("loaddict");
    SYM_INIT = gensym("init");
    SYM_PB = gensym("pb");
    SYM_RATIOS = gensym("ratios");
    SYM_NAMES = gensym("names");
    SYM_NOTES = (t_symbol**) malloc(sizeof(t_symbol*) * NUM_NOTE_RATIOS);
    
    
    for (int i = 0; i < NUM_NOTE_RATIOS; i++){
        SYM_NOTES[i] = gensym(notenames[i]);
    }
    

	post("denote_microtonal has finished loading",0);	// post any important info to the max window when our class is loaded
}


//--------------------------------------------------------------------------

void *denote_microtonal_new(long n)		// n = int argument typed into object box (A_DEFLONG) -- defaults to 0 if no args are typed
{
	t_denote_microtonal *x;				// local variable (pointer to a t_denote_microtonal data structure)

	x = (t_denote_microtonal *)object_alloc(denote_microtonal_class); // create a new instance of this object
    
    for (short i = 0; i < LENGTH; i++){
        x->out[i] = listout(x);        // create an int outlet and assign it to our outlet variable in the instance's data structure
    }
    
    memset(x->note_mod_index, -1, sizeof(long) * NUM_NOTE_RATIOS);
    memset(x->chordElementRouting, -1, sizeof(int) * MAX_CHORD_SIZE);
    x->dict_processed = false;
	return(x);					// return a reference to the object instance
}


//--------------------------------------------------------------------------

void denote_microtonal_assist(t_denote_microtonal *x, void *b, long m, long a, char *s) // 4 final arguments are always the same for the assistance method
{
	if (m == ASSIST_OUTLET)
		sprintf(s,"List of midi note, velocity and pitchbend, or list of 'pb' and pitchbend");
	else {
		switch (a) {
		case 0:
			sprintf(s,"Inlet %ld: list. See helpfile for possible messages", a);
			break;
		}
	}
}

void update_pb(t_denote_microtonal *x, long index){
    t_atom l[2];
    
    atom_setsym(l, SYM_PB);
    atom_setlong(l+1, x->notes[index][PITCHBEND]);
    
    outlet_list(x->out[index], NULL, 2, l);
}

void update_chord(t_denote_microtonal *x, long changed){
    atom_setlong(x->list_out+MIDI_NOTE, x->notes[changed][MIDI_NOTE]);
    atom_setlong(x->list_out+VELOCITY, x->notes[changed][VELOCITY]);
    atom_setlong(x->list_out+PITCHBEND, x->notes[changed][PITCHBEND]);
    outlet_list(x->out[changed], NULL, 3, x->list_out);
}

void denote_microtonal_note_list(t_denote_microtonal *x, t_symbol *s, long argc, t_atom *argv){
    if (argc - 1 == 2){
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
            x->note_mod_index[note % 12] = -1;
            x->notes[index][MIDI_NOTE] = note;
            x->notes[index][VELOCITY] = vel;
        } else if (!found && add && index != -1){
            changed = index;
            x->chordElementRouting[index] = note;
            x->note_mod_index[note % 12] = index;
            x->notes[index][MIDI_NOTE] = note;
            x->notes[index][VELOCITY] = vel;
            
            ratio_list_t* rl = &x->ratio_list[index % 12];
            
            x->notes[index][PITCHBEND] = rl->ratio_arr[rl->active_ratio];
        }
        
        update_chord(x, changed);
    }
}



void denote_microtonal_read_dictionary(t_denote_microtonal *x, t_symbol *s, long argc, t_atom *argv){
    t_fourcc outtype;
    char** filename;
    long size = 100;
    short path;
    
    filename = (char**) malloc(sizeof(char*));
    *filename = (char*) malloc(sizeof(char) * size);
    atom_gettext(argc, argv, &size, filename, 0);
    
    x->dict_processed = false;
    
    if (locatefile_extended(*filename, &path, &outtype, NULL, 0)) { // non-zero: not found
        object_error((t_object*) x, "%s: not found", s->s_name);
        return;
    }
    
    
    if (dictionary_read(*filename, path, &x->d) == MAX_ERR_NONE){
        //init the init dict
        t_dictionary* init_d = (t_dictionary*) malloc(sizeof(t_dictionary));
        
        //get the initialization dict from the main dict
        dictionary_getdictionary(x->d, SYM_INIT, (t_object**) &init_d);
        
        for (int i = 0; i < NUM_NOTE_RATIOS; i++){
            long num_ratios;
            
            //init the note init dict
            t_dictionary* note_init_d = (t_dictionary*) malloc(sizeof(t_dictionary));
            
            //get number of ratios for this note from the init dictionary embedded in the init dict
            dictionary_getdictionary(init_d, SYM_NOTES[i], (t_object**) &note_init_d);
            
            //get the number of
            dictionary_getlong(note_init_d, SYM_RATIOS, (t_atom_long*) &num_ratios);

            //set the number of ratios and init the array of ratios
            x->ratio_list[i].len = num_ratios;
            x->ratio_list[i].ratio_arr = (double*) malloc(sizeof(double) * num_ratios);
            
            t_atom* ratio_doubles = NULL;
            //get the ratios into an atom array
            dictionary_copyatoms(x->d, SYM_NOTES[i], (long*) &num_ratios, &ratio_doubles);
            
            //get the doubles from the t_atom*
            atom_getdouble_array(num_ratios, ratio_doubles, num_ratios, x->ratio_list[i].ratio_arr);
            x->ratio_list[i].active_ratio = 0;
            
            //free the t_atom*
            sysmem_freeptr(ratio_doubles);
        }
        
        x->dict_processed = true;
    }
}


void denote_microtonal_process_ratio_change(t_denote_microtonal* x, t_symbol *s, long argc, t_atom *argv){
    if (x->dict_processed){
        t_atom_long note = -1;
        t_atom_long ratio_index = -1;
        
        if (argc == 2){
            //get the values from the argument list
            atom_arg_getlong(&note, 0, 2, argv);
            atom_arg_getlong(&ratio_index, 1, 2, argv);
            
            //if we were able to load both values and they're valid
            if (note > -1 && ratio_index > -1){
                //if the note we're changing is active, calculate the new pitchbend and send it out
                if (x->note_mod_index[note] != -1){
                    x->notes[x->note_mod_index[note]][PITCHBEND] = calculate_pitchbend(note, x->ratio_list[note].ratio_arr[ratio_index]);
                    update_pb(x, x->note_mod_index[note]);
                }
            }
        }
    }
}

double calculate_pitchbend(long mod_note_index, double ratio){
    //calculate the "regular" tempered interval
    double tempered_interval = pow(2, mod_note_index / 12.);
    
    //calculate the difference between the just ratio and the tempered ratio by dividing them
    //98304 is 8192 * 12, to scale the difference to + and - 1 semitone
    return 98304 * log2(ratio / tempered_interval);
}
    

void denote_microtonal_free(t_denote_microtonal* x){
    for (int i = 0; i < NUM_NOTE_RATIOS; i++){
        free(x->ratio_list[i].ratio_arr);
    }
    
    sysmem_freeptr((void*) x->d);
    sysmem_freeptr((void*) SYM_NOTES);
    sysmem_freeptr(x->out);
}


