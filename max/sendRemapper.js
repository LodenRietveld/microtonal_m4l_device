inlets = 1;
outlets = 0;

var p = this.patcher;
var r = p.getnamed("sendRouter");

function msg_int(val){
    for (var i = 0; i < 8; i++){
        var s = p.getnamed("s" + i.toString());
        p.remove(s);

        s = p.newdefault(178 + (i * 63), 676, "send", "dm_" + val.toString() + "_ch" + i.toString());
        s.varname = "s" + i.toString();
        var x = 178 + (i * 63);
        var y = 676;
        s.rect = [x, y, x + 61, y + 49];
        p.connect(r, i, s, 0);
    }
}
