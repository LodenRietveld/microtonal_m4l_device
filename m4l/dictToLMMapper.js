inlets = 1;
outlets = 1;

var p = this.patcher;
var rng = jsarguments[1];

function dict(v){
    var d = new Dict(rng + "ratio_values");
    var init = d.get("init");
    var keys = init.getkeys();
	
    for (var i = 0; i < keys.length; i++){
        var lm = p.getnamed(rng + "lm_" + keys[i]);
		
        var note_object = init.get(keys[i]);
        var num_ratios = note_object.get("ratios");
		
        if (num_ratios == 1){
            lm.ignoreclick = 1;
            //lm._parameter_range(["None"])
			lm._parameter_invisible(1);
            lm.assign(0);
			
        } else {
	
			lm._parameter_invisible(0);
        	lm._parameter_range(note_object.get("names"));
			lm.ignoreclick = 0;
		}

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
		if (list[i][0] == "_"){
			post()
			post(list[i]+"()")
			}
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
