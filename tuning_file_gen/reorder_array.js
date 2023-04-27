inlets = 1;
outlets = 0;

var d = new Dict("new");

function order(s){
	var d_init_names = d.get("init::" + s);
	post(d_init_names.get("names"));
	post();
}	