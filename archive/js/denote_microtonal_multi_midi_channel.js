//ADD:
//MIDI CHANNEL
//
//
//

var chordElementRouting = [];
var notes = [[0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0]];
var length = 8;

var MIDI_NOTE = 0;
var VELOCITY = 1;
var PITCHBEND = 2;

var accidentals = [1, 1.0125, 0.987654, 0.972222, 1.028571, 1.03125, 0.969697, 1.015625, 0.984615, 1.02, 0.980392, 0.989583, 1.010526, 1.022222, 0.978261, 1.006944, 0.993103, 1.033333, 0.967742];

var p = this.patcher;
var me;
var mylocation = [];

inlets = length;
outlets = 3;

for (var i = 0; i < length; i++){
    chordElementRouting[i] = -1;
}

function findMyselfFfs(h){
    if(h.maxclass == "js"){
        me = h;
        mylocation = me.rect;
    }
}

function create_objects(){
    p.apply(findMyselfFfs);

    if (me != null){
        var ins = [];
        for (var i = 0; i < length; i++){
            ins[i] = -1;
        }

        var mf = p.getnamed("mf");

        if (mf == null){
            mf = p.newdefault(mylocation[0], mylocation[1] + 30, "midiformat", "@hires", 2);
            mf.varname = "mf";
        }



        for (var i = 0; i < length; i++){

            var lm = p.getnamed("lm" + i.toString());
            if (lm != null){
                p.remove(lm);
            }

            var x = mylocation[0] + (i * 34);
            var y = mylocation[1] - 30;
            lm = p.newdefault(x, y, "live.menu");

            lm.fontname("Microtonal Notation");
            lm.fontsize(14);
            lm.varname = "lm" + i.toString();
            lm.rect = [x, y, x + 30, y + 20];
            lm._parameter_type(2);
            lm._parameter_range([" "],["("],[")"],["+"],["*"],[","],["-"],["."],["."],["0"],["1"],["2"],["3"],["4"],["5"],["6"],["7"],["8"], ["9"]);
            lm.presentation(1);
            lm.presentation_rect(i * 34, 10, 30, 30);
            p.connect(lm, 0, me, i);
        }

        p.connect(me, 0, mf, 0);
        p.connect(me, 1, mf, 5);
        p.connect(me, 2, mf, 6);
    }
}

//give max 1s to init everything before initializing/connecting to all objects
var t = new Task(create_objects, this, 0);
t.schedule(1000);


function updateChord(i){
    outlet(2, i);
    outlet(1, notes[i][PITCHBEND]);
    outlet(0, [notes[i][MIDI_NOTE], notes[i][VELOCITY]]);
}

function msg_int(i){
    //IMPL SELECTING MIDI CHANNEL + CALCULATING PB
    notes[inlet][PITCHBEND] = ((accidentals[i] - 1) * 8192);
}


function list(l){
    if (arguments.length == 2){
        var changed = 0;
        var add = (arguments[1] > 0);

        var firstEmptySpot = -1;
        var found = false;
        var foundIndex = -1;
        for (var i = length-1; i >= 0; i--){
            if (chordElementRouting[i] == -1){
                firstEmptySpot = i;
            }

            if (chordElementRouting[i] == arguments[0]){
                found = true;
                foundIndex = i;
            }
        }

        if (found && !add){
            changed = foundIndex;
            chordElementRouting[foundIndex] = -1;
            notes[foundIndex][MIDI_NOTE] = arguments[MIDI_NOTE];
            notes[foundIndex][VELOCITY] = arguments[VELOCITY];
        } else if (!found && add && firstEmptySpot != -1){
            changed = firstEmptySpot;
            chordElementRouting[firstEmptySpot] = arguments[0];
            notes[firstEmptySpot][MIDI_NOTE] = arguments[MIDI_NOTE];
            notes[firstEmptySpot][VELOCITY] = arguments[VELOCITY];
        }

        updateChord(changed);
    }
}


function listMethods(object){
	var list = []
	list = Object.getOwnPropertyNames(object)
	    		.filter(function(property) {
	       			return typeof object[property] == 'function';
	    		}
	    		);
	post()
	post("# METHODS #")
	for(var i = 0 ; i < list.length ; i++){
		post()
		post(list[i]+"()")
	}
	post()
	post("# END #")
}


function listProps(obj){


	for(var prop in obj){
		post();
		post(prop , " : " , obj[prop])
	}

}
