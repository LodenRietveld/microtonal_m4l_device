inlets = 1;
outlets = 0;

var p = this.patcher;


function bang(){
    var d = new Dict("ratio_values");

    var init = d.get("init");
    var keys = init.getkeys();

    for (var i = 0; i < keys.length; i++){
        var lm = p.getnamed("lm_" + keys[i]);
        var note_object = init.get(keys[i]);
        var num_ratios = note_object.get("ratios");

        if (num_ratios == 1){
            lm.ignoreclick = 1;
            lm._parameter_range(["None"], [" "])
            lm.assign(0);
        }

        lm._parameter_range(note_object.get("names"));

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
