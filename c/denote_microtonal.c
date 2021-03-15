#include "ext.h"
#include "ext_obex.h"
#include <math.h>
#include "ext_dictionary.h"

#define NUM_NOTE_RATIOS 12
#define MAX_CHORD_SIZE 15

const char notenames[12][3] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

typedef struct ratio_list {
    uint8_t len;
    double* ratio_arr;
    long active_ratio;
} ratio_list_t;

enum operation {
    SUB = 0,
    ADD
};

typedef struct _denote_microtonal {
	t_object p_ob;
    void *out[MAX_CHORD_SIZE+1];
    long notes[MAX_CHORD_SIZE][3];
    long mod_note_num_active[NUM_NOTE_RATIOS];
    long mod_note_index[NUM_NOTE_RATIOS][MAX_CHORD_SIZE];           //array that stores the current note voice (if active), otherwise -1
    long key_offset;
    t_atom list_out[3];
    int chordElementRouting[MAX_CHORD_SIZE];
    int nr_active_notes;
    ratio_list_t ratio_list[NUM_NOTE_RATIOS];
    bool dict_processed;
    t_dictionary* d;
    t_symbol* dictionary_path;
    bool mpe;
    bool dbg;
} t_denote_microtonal;

#define SEMITONE    0.059463

#define NOTE        0
#define VELOCITY    1
#define PITCHBEND   2

t_symbol* SYM_MPE;
t_symbol* SYM_NOTE;
t_symbol* SYM_LOADDICT;
t_symbol** SYM_NOTES;
t_symbol* SYM_INIT;
t_symbol* SYM_PB;
t_symbol* SYM_RATIOS;
t_symbol* SYM_NAMES;
t_symbol* SYM_DBG;

t_symbol* max_err_inv_sym;
t_symbol* max_err_none_sym;
t_symbol* max_err_generic_sym;
t_symbol* max_err_inv_ptr_sym;
t_symbol* max_err_dupl_sym;
t_symbol* max_err_mem_sym;


double calculate_pitchbend(t_denote_microtonal* x, long mod_note_index);
int find_note_index(t_denote_microtonal* x, uint8_t note);

void denote_microtonal_set_key(t_denote_microtonal* x, long key_idx);
void denote_microtonal_read_dictionary(t_denote_microtonal *x, t_symbol *s, long argc, t_atom *argv);
void denote_microtonal_process_ratio_change(t_denote_microtonal* x, t_symbol *s, long argc, t_atom *argv);
void denote_microtonal_note_list(t_denote_microtonal *x, t_symbol *s, long argc, t_atom *argv);
void denote_microtonal_assist(t_denote_microtonal *x, void *b, long m, long a, char *s);
void *denote_microtonal_new(t_symbol *s, long argc, t_atom *argv);
void denote_microtonal_free(t_denote_microtonal* x);

void denote_microtonal_check_dict_state(t_denote_microtonal* x);

long correct_interval_or_note_with_key(t_denote_microtonal* x, long mod_note, enum operation o);
long interval_to_note(t_denote_microtonal* x, long interval);
long note_to_interval(t_denote_microtonal* x, long note);



t_class *denote_microtonal_class;


//--------------------------------------------------------------------------

void ext_main(void *r)
{
	t_class *c;

	c = class_new("denote_microtonal", (method)denote_microtonal_new, (method)denote_microtonal_free, sizeof(t_denote_microtonal), 0L, A_GIMME, 0);

    class_addmethod(c, (method)denote_microtonal_note_list,             "note",         A_GIMME, 0);
    class_addmethod(c, (method)denote_microtonal_read_dictionary,       "loaddict",     A_GIMME, 0);
    class_addmethod(c, (method)denote_microtonal_process_ratio_change,  "set_ratio",    A_GIMME, 0);
    class_addmethod(c, (method)denote_microtonal_set_key,               "set_key",      A_LONG, 0);
	class_addmethod(c, (method)denote_microtonal_assist,	            "assist",	    A_CANT, 0);
    class_addmethod(c, (method)denote_microtonal_check_dict_state,      "bang",         A_NOTHING, 0);
    
    CLASS_ATTR_SYM(c, "Dictionary path", ATTR_SET_OPAQUE_USER, t_denote_microtonal, dictionary_path);
    CLASS_ATTR_SAVE(c, "Dictionary path", 0);

    
	class_register(CLASS_BOX, c);
	denote_microtonal_class = c;
    
    SYM_MPE = gensym("mpe");
    SYM_NOTE = gensym("note");
    SYM_LOADDICT = gensym("loaddict");
    SYM_INIT = gensym("init");
    SYM_PB = gensym("pb");
    SYM_RATIOS = gensym("ratios");
    SYM_NAMES = gensym("names");
    SYM_DBG = gensym("debug");
    
    max_err_inv_sym = gensym("Invalid error code");
    max_err_none_sym = gensym("No error");
    max_err_generic_sym = gensym("Generic error");
    max_err_inv_ptr_sym = gensym("Invalid pointer");
    max_err_dupl_sym = gensym("Is duplicate");
    max_err_mem_sym = gensym("Out of memory");
    
	post("denote_microtonal has finished loading",0);
    post("version: %s :: %s", __DATE__, __TIME__, 0);
}


t_symbol* get_err_msg(t_max_err e){
    switch(e){
        case MAX_ERR_NONE:
            return max_err_none_sym;
            break;
        case MAX_ERR_GENERIC:
            return max_err_generic_sym;
            break;
        case MAX_ERR_INVALID_PTR:
            return max_err_inv_ptr_sym;
            break;
        case MAX_ERR_DUPLICATE:
            return max_err_dupl_sym;
            break;
        case MAX_ERR_OUT_OF_MEM:
            return max_err_mem_sym;
            break;
        default:
            return max_err_inv_sym;
            break;
    }
}

void out_error(t_denote_microtonal* x, t_symbol* loc, t_symbol* err_msg){
    t_atom err[3];
    atom_setsym(err, gensym("error"));
    atom_setsym(err+1, loc);
    atom_setsym(err+2, err_msg);
    outlet_list(x->out[0], NULL, 3, err);
}


//--------------------------------------------------------------------------

void *denote_microtonal_new(t_symbol *s, long argc, t_atom *argv)
{
	t_denote_microtonal *x;
	x = (t_denote_microtonal *)object_alloc(denote_microtonal_class);
    
    
    if (argc > 1){
        for (int i = 0; i < argc; i++){
            t_symbol* recv;
            t_max_err res = atom_arg_getsym(&recv, i, argc, argv);
            if (res == MAX_ERR_NONE && recv == SYM_MPE){
                x->mpe = true;
            } else if (res == MAX_ERR_NONE && recv == SYM_DBG){
                x->dbg = true;
            }
        }
    } else {
    
        if (argc == 1 && atom_getsym(argv) == SYM_MPE){
            post("MPE enabled");
            x->mpe = true;
        } else {
            post("MPE disabled");
            x->mpe = false;
        }
        
        x->dbg = atom_getsym(argv) == SYM_DBG;
    }
    
    for (short i = 0; i < MAX_CHORD_SIZE+1; i++){
        x->out[i] = listout(x);
    }
    
    
    SYM_NOTES = (t_symbol**) sysmem_newptr(sizeof(t_symbol*) * NUM_NOTE_RATIOS);
    
    for (int i = 0; i < NUM_NOTE_RATIOS; i++){
        SYM_NOTES[i] = gensym(notenames[i]);
    }
    
    for (int i = 0; i < NUM_NOTE_RATIOS; i++){
        memset(x->mod_note_index[i], -1, sizeof(long) * MAX_CHORD_SIZE);
    }
    memset(x->chordElementRouting, -1, sizeof(int) * MAX_CHORD_SIZE);
    x->dict_processed = false;
    x->key_offset = 0;
    x->nr_active_notes = 0;
    
    x->dictionary_path = gensym("no path");
    
    
	return x;
}


//--------------------------------------------------------------------------

void denote_microtonal_assist(t_denote_microtonal *x, void *b, long m, long a, char *s)
{
    if (m == ASSIST_OUTLET && a < MAX_CHORD_SIZE){
		sprintf(s,"List of midi note, velocity and pitchbend, or list of 'pb' and pitchbend for midi channel %ld", a);
    } else if (a == MAX_CHORD_SIZE){
        sprintf(s, "Error outlet. Puts out any errors encountered in a list");
    }
	else {
		switch (a) {
		case 0:
			sprintf(s,"Inlet %ld: list. See helpfile for possible messages", a);
			break;
		}
	}
}

void send_pb(t_denote_microtonal *x, long index){
    t_atom l[2];
    
    atom_setsym(l, SYM_PB);
    atom_setlong(l+1, x->notes[index][PITCHBEND]);

    outlet_list(x->out[index+1], NULL, 2, l);
}

void send_chord(t_denote_microtonal *x, long changed){
    atom_setlong(x->list_out+NOTE, x->notes[changed][NOTE]);
    atom_setlong(x->list_out+VELOCITY, x->notes[changed][VELOCITY]);
    atom_setlong(x->list_out+PITCHBEND, x->notes[changed][PITCHBEND]);
    
    outlet_list(x->out[changed+1], NULL, 3, x->list_out);
}

void dbg_out(t_denote_microtonal* x, char* msg){
    t_atom l[2];
    atom_setsym(l, SYM_DBG);
    t_symbol* s = gensym(msg);
    atom_setsym(l+1, s);
    
    outlet_list(x->out[0], NULL, 2, l);
}

void denote_microtonal_note_list(t_denote_microtonal *x, t_symbol *s, long argc, t_atom *argv){
    if (argc == 2){
        long note = atom_getlong(argv);
        uint8_t mod_note = note % 12;
        long vel = atom_getlong(argv+1);
        uint8_t changed = 0;
        bool add = (vel > 0);
        bool found = false;
        
        int index = -1;
        for (int i = MAX_CHORD_SIZE-1; i >= 0; i--){
            if (x->chordElementRouting[i] == -1 && index == -1){
                index = i;
            } else if (x->chordElementRouting[i] == note){
                found = true;
                index = i;
                break;
            }
        }
        
        //early exit if we didn't find anything to do
        if (index == -1){
            return;
        }
        
        //if the note already exists and the velocity is 0 (or lower), remove it from all arrays tracking it
        if (found && !add){
            changed = index;
            x->chordElementRouting[index] = -1;
            x->mod_note_index[mod_note][x->mod_note_num_active[mod_note]] = -1;
            
            if (x->mod_note_num_active[mod_note] > 0){
                x->mod_note_num_active[mod_note]--;
            }
            
            x->notes[index][NOTE] = note;
            x->notes[index][VELOCITY] = vel;
            x->nr_active_notes--;
        //if it does not yet exist, and there's still space, add the note to all lists
        } else if (!found && add && index != -1){
            changed = index;
            x->chordElementRouting[index] = note;
            x->mod_note_index[mod_note][x->mod_note_num_active[mod_note]] = index;
            
            if (x->mod_note_num_active[mod_note] < MAX_CHORD_SIZE){
                x->mod_note_num_active[mod_note]++;
            }
            
            x->notes[index][NOTE] = note;
            x->notes[index][VELOCITY] = vel;
            x->nr_active_notes++;
            
            //if the dictionary has been successfully read
            if (x->dict_processed){
                //get the correct ratio list
                ratio_list_t* rl = &x->ratio_list[note_to_interval(x, mod_note)];
                
                //if it's not null, calculate the pitchbend. Otherwise set pitchbend to 0 and log an error
                if (rl->ratio_arr != NULL){
                    x->notes[index][PITCHBEND] = calculate_pitchbend(x, mod_note);
                } else {
                    x->notes[index][PITCHBEND] = 0;
                    out_error(x, gensym("send_chord pitchbend calc: "), gensym("ratio_array is null"));
                }
            } else {
                x->notes[index][PITCHBEND] = 0;
            }
        //if it already exists and velocity > 0, just change the velocity
        } else if (found && add){
            changed = index;
            x->notes[index][VELOCITY] = vel;
        }
        
        send_chord(x, changed);
    }
}

void read_dictionary(t_denote_microtonal* x, char* name, long size){
    short path;
    t_fourcc outtype;
    char* filename_path = (char*) malloc(sizeof(char) * size);
    
    memcpy(filename_path, name, size);
    
    x->dict_processed = false;
    
    if (locatefile_extended(name, &path, &outtype, NULL, 0)) { // non-zero: not found
        object_error((t_object*) x, "Dictionary at path %s not found", filename_path);
        return;
    }
    
    
    if (dictionary_read(name, path, &x->d) == MAX_ERR_NONE){
        //init the init dict
        t_dictionary* init_d;
        t_max_err e;
        
        //get the initialization dict from the main dict
        if ((e = dictionary_getdictionary(x->d, SYM_INIT, (t_object**) &init_d)) == MAX_ERR_NONE){
            
            x->dict_processed = true;
            for (int i = 0; i < NUM_NOTE_RATIOS; i++){
                long num_ratios;
                
                //init the note init dict
                t_dictionary* note_init_d;
                
                //get number of ratios for this note from the init dictionary embedded in the init dict
                if ((e = dictionary_getdictionary(init_d, SYM_NOTES[i], (t_object**) &note_init_d)) == MAX_ERR_NONE){
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
                    
                } else {
                    x->dict_processed = false;
                    out_error(x, gensym("Note dict loading: "), get_err_msg(e));
                }
            }
            post("Tuning file %s loaded.", name);
            
            t_atom out[2];
            
            atom_setsym(out, gensym("loaded"));
            atom_setsym(out+1, gensym("bang"));
            outlet_list(x->out[0], NULL, 2, out);
            
            x->dictionary_path = gensym(filename_path);
        } else {
            out_error(x, gensym("init dictionary loading: "), gensym("loading INIT dictionary"));
        }
        
        object_free((void*) x->d);
    }
    
    free(filename_path);
}



void denote_microtonal_read_dictionary(t_denote_microtonal *x, t_symbol *s, long argc, t_atom *argv){
    char** filename;
    long size = 100;
    
    filename = (char**) malloc(sizeof(char*));
    *filename = (char*) malloc(sizeof(char) * size);
    
    atom_gettext(argc, argv, &size, filename, 0);
    
    read_dictionary(x, *filename, size);
    free(*filename);
    free(filename);
}


void denote_microtonal_process_ratio_change(t_denote_microtonal* x, t_symbol *s, long argc, t_atom *argv){
    //if the dictionary was succesfully read
    if (x->dict_processed){
        t_atom_long interval = -1;
        t_atom_long ratio_index = -1;
        
        if (x->dbg){
            dbg_out(x, "ratio_change: dict processed");
        }
        
        if (argc == 2){
            //get the values from the argument list
            atom_arg_getlong(&interval, 0, 2, argv);
            atom_arg_getlong(&ratio_index, 1, 2, argv);
            
            long key_corrected_note = interval_to_note(x, interval);
            
            if (x->dbg){
                char buf[80];
                sprintf(buf, "ratio change: interval: %"PRId64", note: %ld, ratio_index %"PRId64, interval, key_corrected_note, ratio_index);
                dbg_out(x, buf);
            }
            
            //if we were able to load both values and they're valid
            if (interval > -1 && ratio_index > -1){
                x->ratio_list[interval].active_ratio = ratio_index;
                
                if (x->dbg){
                    char buf[80];
                    sprintf(buf, "note and ratio valid, number active notes: %ld", x->mod_note_num_active[key_corrected_note]);
                    dbg_out(x, buf);
                }
                
                //if the note we're changing is active, calculate the new pitchbend and send it out
                if (x->mod_note_num_active[key_corrected_note] > 0){
                    double pitchbend = 0;
                    
                    if (x->dbg){
                        dbg_out(x, "more than one note active");
                    }
                    
                    //but double check array sanity
                    if (x->ratio_list[key_corrected_note].ratio_arr != NULL){
                        pitchbend = calculate_pitchbend(x, key_corrected_note);
                        
                        if (x->dbg){
                            char buf[40];
                            sprintf(buf, "ratio change: pitchbend: %f", pitchbend);
                            dbg_out(x, buf);
                        }
                    } else {
                        out_error(x, gensym("Process ratio change: "), gensym("ratio_arr is null"));
                    }
                    
                    //set all octaved versions of this note to the same pitchbend
                    for (int i = 0; i < x->mod_note_num_active[key_corrected_note]; i++){
                        uint8_t note_idx = x->mod_note_index[key_corrected_note][i];
                        x->notes[note_idx][PITCHBEND] = pitchbend;
                        send_pb(x, note_idx);
                    }
                }
            }
        }
    }
}

double calculate_pitchbend(t_denote_microtonal* x, long mod_note_index){
    //calculate the "regular" tempered interval
    long key_corrected_interval = note_to_interval(x, mod_note_index);
    double tempered_interval = pow(2, key_corrected_interval / 12.);
    
    double ratio;
    if (x->ratio_list[key_corrected_interval].ratio_arr != NULL){
        ratio = x->ratio_list[key_corrected_interval].ratio_arr[x->ratio_list[key_corrected_interval].active_ratio];
    } else {
        return 0;
    }
    
    //calculate the difference between the just ratio and the tempered ratio by dividing them
    //98304 is 8192 * 12, to scale the difference to + and - 1 semitone
    return 98304 * log2(ratio / tempered_interval);
}


void denote_microtonal_set_key(t_denote_microtonal* x, long key_idx){
    if (key_idx < 0){
        key_idx = 0;
    } else if (key_idx > 11){
        key_idx = 11;
    }
    
    x->key_offset = key_idx;
    
    if (x->nr_active_notes > 0){
        for (int i = 0; i < MAX_CHORD_SIZE; i++){
            if (x->chordElementRouting[i] != -1 && x->dict_processed){
                x->notes[i][PITCHBEND] = calculate_pitchbend(x, x->notes[i][NOTE] % 12);
                send_pb(x, i);
            }
        }
    }
}

long interval_to_note(t_denote_microtonal* x, long interval){
    return correct_interval_or_note_with_key(x, interval, ADD);
}

long note_to_interval(t_denote_microtonal* x, long note){
    return correct_interval_or_note_with_key(x, note, SUB);
}

long correct_interval_or_note_with_key(t_denote_microtonal* x, long input, enum operation o){
    
    long key_corrected_value;
    
    switch(o){
        case SUB: {
            key_corrected_value = input - x->key_offset;
            if (key_corrected_value < 0){
                key_corrected_value += 12;
            }
            break;
        }
        case ADD: {
            key_corrected_value = input + x->key_offset;
            key_corrected_value = key_corrected_value % 12;
        }
    }
    
    
    return key_corrected_value;
}



void denote_microtonal_check_dict_state(t_denote_microtonal* x){
    if (x->dictionary_path != gensym("no path")){
        read_dictionary(x, x->dictionary_path->s_name, strlen(x->dictionary_path->s_name));
    }
}
    

//function that frees all allocated memory
void denote_microtonal_free(t_denote_microtonal* x){
    for (int i = 0; i < NUM_NOTE_RATIOS; i++){
        if (x->ratio_list[i].ratio_arr != NULL){
            free(x->ratio_list[i].ratio_arr);
        }
    }
    
    if (x->d != NULL){
        sysmem_freeptr((void*) x->d);
    }
    
    sysmem_freeptr((void**) SYM_NOTES);
}



